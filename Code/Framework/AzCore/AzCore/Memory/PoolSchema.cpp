/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/PoolSchema.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/intrusive_slist.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/thread.h>

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/std/parallel/containers/lock_free_intrusive_stamped_stack.h>

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Pool Allocation algorithm
    /**
    * Pool Allocation algorithm implementation. Used in both PoolAllocator and ThreadPoolAllocator.
    */
    template<class Allocator>
    class PoolAllocation
    {
    public:
        AZ_CLASS_ALLOCATOR(PoolAllocation<Allocator>, SystemAllocator, 0)

        using PageType = typename Allocator::Page;
        using BucketType = typename Allocator::Bucket;

        PoolAllocation(Allocator* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize);
        virtual ~PoolAllocation();

        void*   Allocate(size_t byteSize, size_t alignment);
        void    DeAllocate(void* ptr);
        size_t  AllocationSize(void* ptr);
        // if isForceFreeAllPages is true we will free all pages even if they have allocations in them.
        void    GarbageCollect(bool isForceFreeAllPages = false);

        Allocator*              m_allocator;
        size_t                  m_pageSize;
        size_t                  m_minAllocationShift;
        size_t                  m_minAllocationSize;
        size_t                  m_maxAllocationSize;
        size_t                  m_numBuckets;
        BucketType*             m_buckets;
        size_t                  m_numBytesAllocated;
    };

    /**
     * PoolSchema Implementation... to keep the header clean.
     */
    class PoolSchemaImpl
    {
    public:
        AZ_CLASS_ALLOCATOR(PoolSchemaImpl, SystemAllocator, 0)

        PoolSchemaImpl(const PoolSchema::Descriptor& desc);
        ~PoolSchemaImpl();

        PoolSchema::pointer_type    Allocate(PoolSchema::size_type byteSize, PoolSchema::size_type alignment, int flags = 0);
        void                        DeAllocate(PoolSchema::pointer_type ptr);
        PoolSchema::size_type       AllocationSize(PoolSchema::pointer_type ptr);

        /**
        * We allocate memory for pools in pages. Page is a information struct
        * located at the end of the allocated page. When it's in the at the end
        * we can usually hide it's size in the free bytes left from the pagesize/poolsize.
        * \note IMPORTANT pages are aligned on the page size, this way can find quickly which
        * pool the pointer belongs to.
        */
        struct Page
            : public AZStd::intrusive_list_node<Page>
        {
            struct FakeNode
                : public AZStd::intrusive_slist_node<FakeNode>
            {};

            void SetupFreeList(size_t elementSize, size_t pageDataBlockSize);

            /// We just use a free list of nodes which we cast to the pool type.
            using FreeListType = AZStd::intrusive_slist<FakeNode, AZStd::slist_base_hook<FakeNode>>;

            FreeListType m_freeList;
            u32 m_bin;
            Debug::Magic32 m_magic;
            u32 m_elementSize;
            u32 m_maxNumElements;
        };

        /**
        * A bucket has a list of pages used with the specific pool size.
        */
        struct Bucket
        {
            using PageListType = AZStd::intrusive_list<Page, AZStd::list_base_hook<Page>>;
            PageListType            m_pages;
        };

        // Functions used by PoolAllocation template
        AZ_INLINE Page* PopFreePage();
        AZ_INLINE void  PushFreePage(Page* page);
        void            GarbageCollect();
        inline bool     IsInStaticBlock(Page* page)
        {
            const char* staticBlockStart = reinterpret_cast<const char*>(m_staticDataBlock);
            const char* staticBlockEnd = staticBlockStart + m_numStaticPages*m_pageSize;
            const char* pageAddress = reinterpret_cast<const char*>(page);
            // all pages are the same size so we either in or out, no need to check the pageAddressEnd
            if (pageAddress >= staticBlockStart && pageAddress < staticBlockEnd)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        inline Page*    ConstructPage(size_t elementSize)
        {
            AZ_Assert(m_isDynamic, "We run out of static pages (%d) and this is a static allocator!", m_numStaticPages);
            // We store the page struct at the end of the block
            char* memBlock;
            memBlock = reinterpret_cast<char*>(m_pageAllocator->Allocate(m_pageSize, m_pageSize, 0, "AZSystem::PoolSchemaImpl::ConstructPage", __FILE__, __LINE__));
            size_t pageDataSize = m_pageSize - sizeof(Page);
            Page* page = new(memBlock+pageDataSize)Page();
            page->SetupFreeList(elementSize, pageDataSize);
            page->m_elementSize = static_cast<u32>(elementSize);
            page->m_maxNumElements = static_cast<u32>(pageDataSize / elementSize);
            return page;
        }

        inline void     FreePage(Page* page)
        {
            // TODO: It's optional if we want to check the guard value for corruption, since we are not going
            // to use this memory. Yet it might be useful to catch bugs.
            // We store the page struct at the end of the block
            char* memBlock = reinterpret_cast<char*>(page) - m_pageSize + sizeof(Page);
            page->~Page(); // destroy the page
            m_pageAllocator->DeAllocate(memBlock);
        }

        inline Page*    PageFromAddress(void* address)
        {
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~(m_pageSize-1));
            memBlock += m_pageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->m_magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        using AllocatorType = PoolAllocation<PoolSchemaImpl>;
        IAllocatorAllocate*                 m_pageAllocator;
        AllocatorType                       m_allocator;
        void*                               m_staticDataBlock;
        unsigned int                        m_numStaticPages;
        bool                                m_isDynamic;
        size_t                              m_pageSize;
        Bucket::PageListType                m_freePages;
    };

    /**
         * Thread safe pool allocator.
         */
    class ThreadPoolSchemaImpl
    {
    public:
        AZ_CLASS_ALLOCATOR(ThreadPoolSchemaImpl, SystemAllocator, 0)

        /**
         * Specialized \ref PoolAllocator::Page page for lock free allocator.
         */
        struct Page
            : public AZStd::intrusive_list_node<Page>
        {
            Page(ThreadPoolData* threadData)
                : m_threadData(threadData) {}

            struct FakeNode
                : public AZStd::intrusive_slist_node<FakeNode>
            {};
            // Fake Lock Free node used when we delete an element from another thread.
            struct FakeNodeLF
                : public AZStd::lock_free_intrusive_stack_node<FakeNodeLF>{};

            void SetupFreeList(size_t elementSize, size_t pageDataBlockSize);

            /// We just use a free list of nodes which we cast to the pool type.
            using FreeListType = AZStd::intrusive_slist<FakeNode, AZStd::slist_base_hook<FakeNode>>;

            FreeListType m_freeList;
            AZStd::lock_free_intrusive_stack_node<Page> m_lfStack;          ///< Lock Free stack node
            struct ThreadPoolData* m_threadData;                        ///< The thread data that own's the page.
            u32 m_bin;
            Debug::Magic32 m_magic;
            u32 m_elementSize;
            u32 m_maxNumElements;
        };

        /**
        * A bucket has a list of pages used with the specific pool size.
        */
        struct Bucket
        {
            using PageListType = AZStd::intrusive_list<Page, AZStd::list_base_hook<Page>>;
            PageListType            m_pages;
        };

        ThreadPoolSchemaImpl(const ThreadPoolSchema::Descriptor& desc, ThreadPoolSchema::GetThreadPoolData threadPoolGetter, ThreadPoolSchema::SetThreadPoolData threadPoolSetter);
        ~ThreadPoolSchemaImpl();

        ThreadPoolSchema::pointer_type  Allocate(ThreadPoolSchema::size_type byteSize, ThreadPoolSchema::size_type alignment, int flags = 0);
        void                            DeAllocate(ThreadPoolSchema::pointer_type ptr);
        ThreadPoolSchema::size_type     AllocationSize(ThreadPoolSchema::pointer_type ptr);
        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void            GarbageCollect();
        //////////////////////////////////////////////////////////////////////////

        // Functions used by PoolAllocation template
        AZ_INLINE Page* PopFreePage();
        AZ_INLINE void  PushFreePage(Page* page);
        inline bool     IsInStaticBlock(Page* page)
        {
            const char* staticBlockStart = reinterpret_cast<const char*>(m_staticDataBlock);
            const char* staticBlockEnd = staticBlockStart + m_numStaticPages*m_pageSize;
            const char* pageAddress = reinterpret_cast<const char*>(page);
            // all pages are the same size so we either in or out, no need to check the pageAddressEnd
            if (pageAddress > staticBlockStart && pageAddress < staticBlockEnd)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        inline Page*    ConstructPage(size_t elementSize)
        {
            AZ_Assert(m_isDynamic, "We run out of static pages (%d) and this is a static allocator!", m_numStaticPages);
            // We store the page struct at the end of the block
            char* memBlock;
            memBlock = reinterpret_cast<char*>(m_pageAllocator->Allocate(m_pageSize, m_pageSize, 0, "AZSystem::ThreadPoolSchema::ConstructPage", __FILE__, __LINE__));
            size_t pageDataSize = m_pageSize - sizeof(Page);
            Page* page = new(memBlock+pageDataSize)Page(m_threadPoolGetter());
            page->SetupFreeList(elementSize, pageDataSize);
            page->m_elementSize = static_cast<u32>(elementSize);
            page->m_maxNumElements = static_cast<u32>(pageDataSize / elementSize);
            return page;
        }

        inline void     FreePage(Page* page)
        {
            // TODO: It's optional if we want to check the guard value for corruption, since we are not going
            // to use this memory. Yet it might be useful to catch bugs.
            // We store the page struct at the end of the block
            char* memBlock = reinterpret_cast<char*>(page) - m_pageSize + sizeof(Page);
            page->~Page();     // destroy the page
            m_pageAllocator->DeAllocate(memBlock);
        }

        inline Page*    PageFromAddress(void* address)
        {
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~static_cast<size_t>(m_pageSize-1));
            memBlock += m_pageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->m_magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        // NOTE we allow only one instance of a allocator, you need to subclass it for different versions.
        // so for now it's safe to use static. We use TLS on a static because on some platforms set thread key is available
        // only for pthread lib and we don't use it. I can't find other way to it, otherwise please switch this to
        // use TlsAlloc/TlsFree etc.
        ThreadPoolSchema::GetThreadPoolData m_threadPoolGetter;
        ThreadPoolSchema::SetThreadPoolData m_threadPoolSetter;

        // Fox X64 we push/pop pages using the m_mutex to sync. Pages are
        using FreePagesType = Bucket::PageListType;
        FreePagesType               m_freePages;
        AZStd::vector<ThreadPoolData*> m_threads;           ///< Array with all separate thread data. Used to traverse end free elements.

        IAllocatorAllocate*         m_pageAllocator;
        void*                       m_staticDataBlock;
        size_t                      m_numStaticPages;
        size_t                      m_pageSize;
        size_t                      m_minAllocationSize;
        size_t                      m_maxAllocationSize;
        bool                        m_isDynamic;
        // TODO rbbaklov Changed to recursive_mutex from mutex for Linux support.
        AZStd::recursive_mutex      m_mutex;
    };

    struct ThreadPoolData
    {
        AZ_CLASS_ALLOCATOR(ThreadPoolData, SystemAllocator, 0)

        ThreadPoolData(ThreadPoolSchemaImpl* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize);
        ~ThreadPoolData();

        using AllocatorType = PoolAllocation<ThreadPoolSchemaImpl>;
        /**
        * Stack with freed elements from other threads. We don't need stamped stack since the ABA problem can not
        * happen here. We push from many threads and pop from only one (we don't push from it).
        */
        using FreedElementsStack = AZStd::lock_free_intrusive_stack<ThreadPoolSchemaImpl::Page::FakeNodeLF, AZStd::lock_free_intrusive_stack_base_hook<ThreadPoolSchemaImpl::Page::FakeNodeLF>>;

        AllocatorType           m_allocator;
        FreedElementsStack      m_freedElements;
    };
}

using namespace AZ;

//=========================================================================
// PoolAllocation
// [9/09/2009]
//=========================================================================
template<class Allocator>
PoolAllocation<Allocator>::PoolAllocation(Allocator* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize)
    : m_allocator(alloc)
    , m_pageSize(pageSize)
    , m_numBytesAllocated(0)
{
    AZ_Assert(alloc->m_pageAllocator, "We need the page allocator setup!");
    AZ_Assert(pageSize >= maxAllocationSize * 4, "We need to fit at least 4 objects in a pool! Increase your page size! Page %d MaxAllocationSize %d",pageSize,maxAllocationSize);
    AZ_Assert(minAllocationSize == maxAllocationSize || ((minAllocationSize)&(minAllocationSize - 1)) == 0, "Min allocation should be either equal to max allocation size or power of two");

    m_minAllocationSize = AZ::GetMax(minAllocationSize, size_t(8));
    m_maxAllocationSize = AZ::GetMax(maxAllocationSize, minAllocationSize);

    m_minAllocationShift = 0;
    for (size_t i = 1; i < sizeof(unsigned int)*8; i++)
    {
        if (m_minAllocationSize >> i == 0)
        {
            m_minAllocationShift = i-1;
            break;
        }
    }

    AZ_Assert(m_maxAllocationSize % m_minAllocationSize == 0, "You need to be able to divide m_maxAllocationSize (%d) / m_minAllocationSize (%d) without fraction!", m_maxAllocationSize, m_minAllocationSize);
    m_numBuckets = m_maxAllocationSize / m_minAllocationSize;
    AZ_Assert(m_numBuckets <= 0xffff, "You can't have more than 65535 number of buckets! We need to increase the index size!");
    m_buckets = reinterpret_cast<BucketType*>(alloc->m_pageAllocator->Allocate(sizeof(BucketType)*m_numBuckets, AZStd::alignment_of<BucketType>::value));
    for (size_t i = 0; i < m_numBuckets; ++i)
    {
        new(m_buckets + i)BucketType();
    }
}

//=========================================================================
// ~PoolAllocation
// [9/09/2009]
//=========================================================================
template<class Allocator>
PoolAllocation<Allocator>::~PoolAllocation()
{
    GarbageCollect(true);

    for (size_t i = 0; i < m_numBuckets; ++i)
    {
        m_buckets[i].~BucketType();
    }
    m_allocator->m_pageAllocator->DeAllocate(m_buckets, sizeof(BucketType) * m_numBuckets);
}

//=========================================================================
// Allocate
// [9/09/2009]
//=========================================================================
template<class Allocator>
AZ_INLINE void*
PoolAllocation<Allocator>::Allocate(size_t byteSize, size_t alignment)
{
    AZ_Assert(byteSize>0, "You can not allocate 0 bytes!");
    AZ_Assert(alignment>0&&(alignment&(alignment-1))==0, "Alignment must be >0 and power of 2!");

    // pad the size to the min allocation size.
    byteSize = AZ::SizeAlignUp(byteSize, m_minAllocationSize);
    byteSize = AZ::SizeAlignUp(byteSize, alignment);

    if (byteSize > m_maxAllocationSize)
    {
        AZ_Assert(false, "Allocation size (%d) is too big (max: %d) for pools!", byteSize, m_maxAllocationSize);
        return nullptr;
    }

    u32 bucketIndex = static_cast<u32>((byteSize >> m_minAllocationShift)-1);
    BucketType& bucket = m_buckets[bucketIndex];
    PageType* page = nullptr;
    if (!bucket.m_pages.empty())
    {
        page = &bucket.m_pages.front();

        // check if we have free slot in the page
        if (page->m_freeList.empty())
        {
            page = nullptr;
        }
        else if (page->m_freeList.size()==1)
        {
            // if we have only 1 free slot this allocation will
            // fill the page, so put in on the back
            bucket.m_pages.pop_front();
            bucket.m_pages.push_back(*page);
        }
    }
    if (!page)
    {
        page = m_allocator->PopFreePage();
        if (page)
        {
            // We have any pages available on free page stack.
            if (page->m_bin != bucketIndex)  // if this page was used the same bucket we are ready to roll.
            {
                size_t elementSize = byteSize;
                size_t pageDataSize = m_pageSize - sizeof(PageType);
                page->SetupFreeList(elementSize, pageDataSize);
                page->m_bin = bucketIndex;
                page->m_elementSize = static_cast<u32>(elementSize);
                page->m_maxNumElements = static_cast<u32>(pageDataSize / elementSize);
            }
        }
        else
        {
            // We need to align each page on it's size, this way we can quickly find which page the pointer belongs to.
            page = m_allocator->ConstructPage(byteSize);
            page->m_bin = bucketIndex;
        }
        bucket.m_pages.push_front(*page);
    }

    // The data address and the fake node address are shared.
    void* address = &page->m_freeList.front();
    page->m_freeList.pop_front();

    m_numBytesAllocated += byteSize;

    return address;
}

//=========================================================================
// DeAllocate
// [9/09/2009]
//=========================================================================
template<class Allocator>
AZ_INLINE void
PoolAllocation<Allocator>::DeAllocate(void* ptr)
{
    PageType* page = m_allocator->PageFromAddress(ptr);
    if (page==nullptr)
    {
        AZ_Error("Memory", false, "Address 0x%08x is not in the ThreadPool!", ptr);
        return;
    }

    // (pageSize - info struct at the end) / (element size)
    size_t maxElementsPerBucket = page->m_maxNumElements;

    size_t numFreeNodes = page->m_freeList.size();
    typename PageType::FakeNode* node = new(ptr) typename PageType::FakeNode();
    page->m_freeList.push_front(*node);

    if (numFreeNodes==0)
    {
        // if the page was full before sort at the front
        BucketType& bucket = m_buckets[page->m_bin];
        bucket.m_pages.erase(*page);
        bucket.m_pages.push_front(*page);
    }
    else if (numFreeNodes == maxElementsPerBucket-1)
    {
        // push to the list of free pages
        BucketType& bucket = m_buckets[page->m_bin];
        PageType* frontPage = &bucket.m_pages.front();
        if (frontPage != page)
        {
            bucket.m_pages.erase(*page);
            // check if the front page is full if so push the free page to the front otherwise push
            // push it on the free pages list so it can be reused by other bins.
            if (frontPage->m_freeList.empty())
            {
                bucket.m_pages.push_front(*page);
            }
            else
            {
                m_allocator->PushFreePage(page);
            }
        }
        else if (frontPage->m_next != nullptr)
        {
            // if the next page has free slots free the current page
            if (frontPage->m_next->m_freeList.size() < maxElementsPerBucket)
            {
                bucket.m_pages.erase(*page);
                m_allocator->PushFreePage(page);
            }
        }
    }

    m_numBytesAllocated -= page->m_elementSize;
}

//=========================================================================
// AllocationSize
// [11/22/2010]
//=========================================================================
template<class Allocator>
AZ_INLINE size_t
PoolAllocation<Allocator>::AllocationSize(void* ptr)
{
    PageType* page = m_allocator->PageFromAddress(ptr);
    size_t elementSize;
    if (page)
    {
        elementSize = page->m_elementSize;
    }
    else
    {
        elementSize = 0;
    }

    return elementSize;
}

//=========================================================================
// GarbageCollect
// [3/1/2012]
//=========================================================================
template<class Allocator>
AZ_INLINE void
PoolAllocation<Allocator>::GarbageCollect(bool isForceFreeAllPages)
{
    // Free empty pages in the buckets (or better be empty)
    for (unsigned int i = 0; i < (unsigned int)m_numBuckets; ++i)
    {
        // (pageSize - info struct at the end) / (element size)
        size_t maxElementsPerBucket = (m_pageSize - sizeof(PageType)) / ((i+1) << m_minAllocationShift);

        typename BucketType::PageListType& pages = m_buckets[i].m_pages;
        while (!pages.empty())
        {
            PageType& page = pages.front();
            pages.pop_front();
            if (page.m_freeList.size()==maxElementsPerBucket || isForceFreeAllPages)
            {
                if (!m_allocator->IsInStaticBlock(&page))
                {
                    m_allocator->FreePage(&page);
                }
                else
                {
                    m_allocator->PushFreePage(&page);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// PollAllocator
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// PoolSchema
// [9/15/2009]
//=========================================================================
PoolSchema::PoolSchema(const Descriptor& desc)
    : m_impl(nullptr)
{
    (void)desc;  // ignored here, applied in Create()
}

//=========================================================================
// ~PoolSchema
// [9/15/2009]
//=========================================================================
PoolSchema::~PoolSchema()
{
    AZ_Assert(m_impl==nullptr, "You did not destroy the pool schema!");
    delete m_impl;
}

//=========================================================================
// Create
// [9/15/2009]
//=========================================================================
bool PoolSchema::Create(const Descriptor& desc)
{
    AZ_Assert(m_impl==nullptr, "PoolSchema already created!");
    if (m_impl == nullptr)
    {
        m_impl = aznew PoolSchemaImpl(desc);
    }
    return (m_impl!=nullptr);
}

//=========================================================================
// ~Destroy
// [9/15/2009]
//=========================================================================
bool PoolSchema::Destroy()
{
    delete m_impl;
    m_impl = nullptr;
    return true;
}

//=========================================================================
// Allocate
// [9/15/2009]
//=========================================================================
PoolSchema::pointer_type
PoolSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    (void)flags;
    (void)name;
    (void)fileName;
    (void)lineNum;
    (void)suppressStackRecord;
    return m_impl->Allocate(byteSize, alignment);
}

//=========================================================================
// DeAllocate
// [9/15/2009]
//=========================================================================
void
PoolSchema::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    (void)byteSize;
    (void)alignment;
    m_impl->DeAllocate(ptr);
}

//=========================================================================
// Resize
// [10/14/2018]
//=========================================================================
PoolSchema::size_type
PoolSchema::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;
    return 0;  // unsupported
}

//=========================================================================
// ReAllocate
// [10/14/2018]
//=========================================================================
PoolSchema::pointer_type 
PoolSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    (void)ptr;
    (void)newSize;
    (void)newAlignment;
    AZ_Assert(false, "unsupported");

    return ptr;
}

//=========================================================================
// AllocationSize
// [11/22/2010]
//=========================================================================
PoolSchema::size_type
PoolSchema::AllocationSize(pointer_type ptr)
{
    return m_impl->AllocationSize(ptr);
}

//=========================================================================
// DeAllocate
// [9/15/2009]
//=========================================================================
void
PoolSchema::GarbageCollect()
{
    // External requests for garbage collection may come from any thread, and the
    // garbage collection operation isn't threadsafe, which can lead to crashes.
    //
    // Due to the low memory consumption of this allocator in practice on Dragonfly
    // (~3kb) it makes sense to not bother with garbage collection and leave it to
    // occur exclusively in the destruction of the allocator.
    //
    // TODO: A better solution needs to be found for integrating back into mainline 
    // Open 3D Engine.
    //m_impl->GarbageCollect();
}

auto PoolSchema::GetMaxContiguousAllocationSize() const -> size_type
{
    return m_impl->m_allocator.m_maxAllocationSize;
}

//=========================================================================
// NumAllocatedBytes
// [11/1/2010]
//=========================================================================
PoolSchema::size_type
PoolSchema::NumAllocatedBytes() const
{
    return m_impl->m_allocator.m_numBytesAllocated;
}

//=========================================================================
// Capacity
// [11/1/2010]
//=========================================================================
PoolSchema::size_type
PoolSchema::Capacity() const
{
    return m_impl->m_numStaticPages * m_impl->m_pageSize;
}

//=========================================================================
// GetPageAllocator
// [11/17/2010]
//=========================================================================
IAllocatorAllocate*
PoolSchema::GetSubAllocator()
{
    return m_impl->m_pageAllocator;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// PollAllocator Implementation
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// PoolSchemaImpl
// [9/15/2009]
//=========================================================================
PoolSchemaImpl::PoolSchemaImpl(const PoolSchema::Descriptor& desc)
    : m_pageAllocator(desc.m_pageAllocator ? desc.m_pageAllocator : &AllocatorInstance<SystemAllocator>::Get())
    , m_allocator(this, desc.m_pageSize, desc.m_minAllocationSize, desc.m_maxAllocationSize)
    , m_staticDataBlock(nullptr)
    , m_numStaticPages(desc.m_numStaticPages)
    , m_isDynamic(desc.m_isDynamic)
    , m_pageSize(desc.m_pageSize)
{
    if (m_numStaticPages)
    {
        // We store the page struct at the end of the block
        char* memBlock = reinterpret_cast<char*>(m_pageAllocator->Allocate(m_pageSize*m_numStaticPages, m_pageSize, 0, "AZSystem::PoolAllocation::Page static array", __FILE__, __LINE__));
        m_staticDataBlock = memBlock;
        size_t pageDataSize = m_pageSize - sizeof(Page);
        for (unsigned int i = 0; i < m_numStaticPages; ++i)
        {
            Page* page = new(memBlock+pageDataSize)Page();
            page->m_bin = 0xffffffff;
            page->m_elementSize = 0;
            page->m_maxNumElements = 0;
            PushFreePage(page);
            memBlock += m_pageSize;
        }
    }
}

//=========================================================================
// ~PoolSchemaImpl
// [9/15/2009]
//=========================================================================
PoolSchemaImpl::~PoolSchemaImpl()
{
    // Force free all pages
    m_allocator.GarbageCollect(true);

    // Free all unused memory
    GarbageCollect();

    if (m_staticDataBlock)
    {
        while (!m_freePages.empty())
        {
            Page* page = &m_freePages.front();
            (void)page;
            m_freePages.pop_front();
            AZ_Assert(IsInStaticBlock(page), "All dynamic pages should be deleted by now!");
        }
        ;

        char* memBlock = reinterpret_cast<char*>(m_staticDataBlock);
        size_t pageDataSize = m_pageSize - sizeof(Page);
        for (unsigned int i = 0; i < m_numStaticPages; ++i)
        {
            Page* page = reinterpret_cast<Page*>(memBlock+pageDataSize);
            page->~Page();
            memBlock += m_pageSize;
        }
        m_pageAllocator->DeAllocate(m_staticDataBlock);
    }
}

//=========================================================================
// Allocate
// [9/15/2009]
//=========================================================================
PoolSchema::pointer_type
PoolSchemaImpl::Allocate(PoolSchema::size_type byteSize, PoolSchema::size_type alignment, int flags)
{
    //AZ_Warning("Memory",m_ownerThread==AZStd::this_thread::get_id(),"You can't allocation from a different context/thread, use ThreadPoolAllocator!");
    (void)flags;
    void* address = m_allocator.Allocate(byteSize, alignment);
    return address;
}

//=========================================================================
// DeAllocate
// [9/15/2009]
//=========================================================================
void
PoolSchemaImpl::DeAllocate(PoolSchema::pointer_type ptr)
{
    //AZ_Warning("Memory",m_ownerThread==AZStd::this_thread::get_id(),"You can't deallocate from a different context/thread, use ThreadPoolAllocator!");
    m_allocator.DeAllocate(ptr);
}

//=========================================================================
// AllocationSize
// [11/22/2010]
//=========================================================================
PoolSchema::size_type
PoolSchemaImpl::AllocationSize(PoolSchema::pointer_type ptr)
{
    //AZ_Warning("Memory",m_ownerThread==AZStd::this_thread::get_id(),"You can't use PoolAllocator from a different context/thread, use ThreadPoolAllocator!");
    return m_allocator.AllocationSize(ptr);
}

//=========================================================================
// Pop
// [9/15/2009]
//=========================================================================
AZ_FORCE_INLINE PoolSchemaImpl::Page*
PoolSchemaImpl::PopFreePage()
{
    Page* page = nullptr;
    if (!m_freePages.empty())
    {
        page = &m_freePages.front();
        m_freePages.pop_front();
    }
    return page;
}

//=========================================================================
// Push
// [9/15/2009]
//=========================================================================
AZ_INLINE void
PoolSchemaImpl::PushFreePage(Page* page)
{
    m_freePages.push_front(*page);
}

//=========================================================================
// PurgePages
// [9/11/2009]
//=========================================================================
void
PoolSchemaImpl::GarbageCollect()
{
    //if( m_ownerThread == AZStd::this_thread::get_id() )
    {
        if (m_isDynamic)
        {
            m_allocator.GarbageCollect();

            Bucket::PageListType staticPages;
            while (!m_freePages.empty())
            {
                Page* page = &m_freePages.front();
                m_freePages.pop_front();
                if (IsInStaticBlock(page))
                {
                    staticPages.push_front(*page);
                }
                else
                {
                    FreePage(page);
                }
            }

            while (!staticPages.empty())
            {
                Page* page = &staticPages.front();
                staticPages.pop_front();
                m_freePages.push_front(*page);
            }
        }
    }
}

//=========================================================================
// SetupFreeList
// [9/09/2009]
//=========================================================================
AZ_FORCE_INLINE void
PoolSchemaImpl::Page::SetupFreeList(size_t elementSize, size_t pageDataBlockSize)
{
    char* pageData = reinterpret_cast<char*>(this) - pageDataBlockSize;
    m_freeList.clear();
    // setup free list
    size_t numElements = pageDataBlockSize / elementSize;
    for (unsigned int i = 0; i < numElements; ++i)
    {
        char* address = pageData+i*elementSize;
        Page::FakeNode* node = new(address) Page::FakeNode();
        m_freeList.push_back(*node);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ThreadPoolSchema
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// ThreadPoolSchema
// [9/15/2009]
//=========================================================================
ThreadPoolSchema::ThreadPoolSchema(GetThreadPoolData getThreadPoolData, SetThreadPoolData setThreadPoolData)
    : m_impl(nullptr)
    , m_threadPoolGetter(getThreadPoolData)
    , m_threadPoolSetter(setThreadPoolData)
{
}

//=========================================================================
// ~ThreadPoolSchema
// [9/15/2009]
//=========================================================================
ThreadPoolSchema::~ThreadPoolSchema()
{
    AZ_Assert(m_impl==nullptr, "You did not destroy the thread pool schema!");
    delete m_impl;
}

//=========================================================================
// Create
// [9/15/2009]
//=========================================================================
bool ThreadPoolSchema::Create(const Descriptor& desc)
{
    AZ_Assert(m_impl==nullptr, "PoolSchema already created!");
    if (m_impl == nullptr)
    {
        m_impl = aznew ThreadPoolSchemaImpl(desc, m_threadPoolGetter, m_threadPoolSetter);
    }
    return (m_impl!=nullptr);
}

//=========================================================================
// Destroy
// [9/15/2009]
//=========================================================================
bool ThreadPoolSchema::Destroy()
{
    delete m_impl;
    m_impl = nullptr;
    return true;
}
//=========================================================================
// Allocate
// [9/15/2009]
//=========================================================================
ThreadPoolSchema::pointer_type
ThreadPoolSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    (void)flags;
    (void)name;
    (void)fileName;
    (void)lineNum;
    (void)suppressStackRecord;
    return m_impl->Allocate(byteSize, alignment);
}

//=========================================================================
// DeAllocate
// [9/15/2009]
//=========================================================================
void
ThreadPoolSchema::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    (void)byteSize;
    (void)alignment;
    m_impl->DeAllocate(ptr);
}

//=========================================================================
// Resize
// [10/14/2018]
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchema::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;
    return 0;  // unsupported
}

//=========================================================================
// ReAllocate
// [10/14/2018]
//=========================================================================
ThreadPoolSchema::pointer_type
ThreadPoolSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    (void)ptr;
    (void)newSize;
    (void)newAlignment;
    AZ_Assert(false, "unsupported");
    
    return ptr;
}

//=========================================================================
// AllocationSize
// [11/22/2010]
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchema::AllocationSize(pointer_type ptr)
{
    return m_impl->AllocationSize(ptr);
}

//=========================================================================
// DeAllocate
// [9/15/2009]
//=========================================================================
void
ThreadPoolSchema::GarbageCollect()
{
    m_impl->GarbageCollect();
}

auto ThreadPoolSchema::GetMaxContiguousAllocationSize() const -> size_type
{
    return m_impl->m_maxAllocationSize;
}

//=========================================================================
// NumAllocatedBytes
// [11/1/2010]
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchema::NumAllocatedBytes() const
{
    size_type bytesAllocated = 0;
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_impl->m_mutex);
        for (size_t i = 0; i < m_impl->m_threads.size(); ++i)
        {
            bytesAllocated += m_impl->m_threads[i]->m_allocator.m_numBytesAllocated;
        }
    }
    return bytesAllocated;
}

//=========================================================================
// Capacity
// [11/1/2010]
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchema::Capacity() const
{
    return m_impl->m_numStaticPages * m_impl->m_pageSize;
}

//=========================================================================
// GetPageAllocator
// [11/17/2010]
//=========================================================================
IAllocatorAllocate*
ThreadPoolSchema::GetSubAllocator()
{
    return m_impl->m_pageAllocator;
}


//=========================================================================
// ThreadPoolSchemaImpl
// [9/15/2009]
//=========================================================================
ThreadPoolSchemaImpl::ThreadPoolSchemaImpl(const ThreadPoolSchema::Descriptor& desc, ThreadPoolSchema::GetThreadPoolData threadPoolGetter, ThreadPoolSchema::SetThreadPoolData threadPoolSetter)
    : m_threadPoolGetter(threadPoolGetter)
    , m_threadPoolSetter(threadPoolSetter)
    , m_pageAllocator(desc.m_pageAllocator)
    , m_staticDataBlock(nullptr)
    , m_numStaticPages(desc.m_numStaticPages)
    , m_pageSize(desc.m_pageSize)
    , m_minAllocationSize(desc.m_minAllocationSize)
    , m_maxAllocationSize(desc.m_maxAllocationSize)
    , m_isDynamic(desc.m_isDynamic)
{
#   if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
    // In memory allocation case (usually tools) we might have high contention,
    // using spin lock will improve performance.
    SetCriticalSectionSpinCount(m_mutex.native_handle(), 4000);
#   endif

    if (m_pageAllocator == nullptr)
    {
        m_pageAllocator = &AllocatorInstance<SystemAllocator>::Get();  // use the SystemAllocator if no page allocator is provided
    }
    if (m_numStaticPages)
    {
        // We store the page struct at the end of the block
        char* memBlock = reinterpret_cast<char*>(m_pageAllocator->Allocate(m_pageSize*m_numStaticPages, m_pageSize, 0, "AZSystem::ThreadPoolSchemaImpl::Page static array", __FILE__, __LINE__));
        m_staticDataBlock = memBlock;
        size_t pageDataSize = m_pageSize - sizeof(Page);
        for (unsigned int i = 0; i < m_numStaticPages; ++i)
        {
            Page* page = new(memBlock+pageDataSize)Page(m_threadPoolGetter());
            page->m_bin = 0xffffffff;
            PushFreePage(page);
            memBlock += m_pageSize;
        }
    }
}

//=========================================================================
// ~ThreadPoolSchemaImpl
// [9/15/2009]
//=========================================================================
ThreadPoolSchemaImpl::~ThreadPoolSchemaImpl()
{
    // clean up all the thread data.
    // IMPORTANT: We assume/rely that all threads (except the calling one) are or will
    // destroyed before you create another instance of the pool allocation.
    // This should generally be ok since the all allocators are singletons.
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        if (!m_threads.empty())
        {
            for (size_t i = 0; i < m_threads.size(); ++i)
            {
                if (m_threads[i])
                {
                    // Force free all pages
                    delete m_threads[i];
                }
            }

            /// reset the variable for the owner thread.
            m_threadPoolSetter(nullptr);
        }
    }

    GarbageCollect();

    if (m_staticDataBlock)
    {
        Page* page;
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            while (!m_freePages.empty())
            {
                page = &m_freePages.front();
                m_freePages.pop_front();
                AZ_Assert(IsInStaticBlock(page), "All dynamic pages should be free by now!");
            }
        }

        char* memBlock = reinterpret_cast<char*>(m_staticDataBlock);
        size_t pageDataSize = m_pageSize - sizeof(Page);
        for (unsigned int i = 0; i < m_numStaticPages; ++i)
        {
            page = reinterpret_cast<Page*>(memBlock+pageDataSize);
            page->~Page();
            memBlock += m_pageSize;
        }
        m_pageAllocator->DeAllocate(m_staticDataBlock);
    }
}

//=========================================================================
// Allocate
// [9/15/2009]
//=========================================================================
ThreadPoolSchema::pointer_type
ThreadPoolSchemaImpl::Allocate(ThreadPoolSchema::size_type byteSize, ThreadPoolSchema::size_type alignment, int flags)
{
    (void)flags;

    ThreadPoolData* threadData = m_threadPoolGetter();

    if (threadData == nullptr)
    {
        threadData = aznew ThreadPoolData(this, m_pageSize, m_minAllocationSize, m_maxAllocationSize);
        m_threadPoolSetter(threadData);
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            m_threads.push_back(threadData);
        }
    }
    else
    {
        // deallocate elements if they were freed from other threads
        Page::FakeNodeLF* fakeLFNode;
        while ((fakeLFNode = threadData->m_freedElements.pop())!=nullptr)
        {
            threadData->m_allocator.DeAllocate(fakeLFNode);
        }
    }

    return threadData->m_allocator.Allocate(byteSize, alignment);
}

//=========================================================================
// DeAllocate
// [9/15/2009]
//=========================================================================
void
ThreadPoolSchemaImpl::DeAllocate(ThreadPoolSchema::pointer_type ptr)
{
    Page* page = PageFromAddress(ptr);
    if (page==nullptr)
    {
        AZ_Error("Memory", false, "Address 0x%08x is not in the ThreadPool!", ptr);
        return;
    }
    AZ_Assert(page->m_threadData!=nullptr, ("We must have valid page thread data for the page!"));
    ThreadPoolData* threadData = m_threadPoolGetter();
    if (threadData == page->m_threadData)
    {
        // we can free here
        threadData->m_allocator.DeAllocate(ptr);
    }
    else
    {
        // push this element to be deleted from it's own thread!
        // cast the pointer to a fake lock free node
        Page::FakeNodeLF*   fakeLFNode = reinterpret_cast<Page::FakeNodeLF*>(ptr);
#ifdef AZ_DEBUG_BUILD
        // we need to reset the fakeLFNode because we share the memory.
        // otherwise we will assert the node is in the list
        fakeLFNode->m_next = 0;
#endif
        page->m_threadData->m_freedElements.push(*fakeLFNode);
    }
}

//=========================================================================
// AllocationSize
// [11/22/2010]
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchemaImpl::AllocationSize(ThreadPoolSchema::pointer_type ptr)
{
    Page* page = PageFromAddress(ptr);
    if (page==nullptr)
    {
        return 0;
    }
    AZ_Assert(page->m_threadData!=nullptr, ("We must have valid page thread data for the page!"));
    return page->m_threadData->m_allocator.AllocationSize(ptr);
}

//=========================================================================
// PopFreePage
// [9/15/2009]
//=========================================================================
AZ_INLINE ThreadPoolSchemaImpl::Page*
ThreadPoolSchemaImpl::PopFreePage()
{
    Page* page;
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        if (m_freePages.empty())
        {
            page = nullptr;
        }
        else
        {
            page = &m_freePages.front();
            m_freePages.pop_front();
        }
    }
    if (page)
    {
#   ifdef AZ_DEBUG_BUILD
        AZ_Assert(page->m_threadData == 0, "If we stored the free page properly we should have null here!");
#   endif
        // store the current thread data, used when we free elements
        page->m_threadData = m_threadPoolGetter();
    }
    return page;
}

//=========================================================================
// PushFreePage
// [9/15/2009]
//=========================================================================
AZ_INLINE void
ThreadPoolSchemaImpl::PushFreePage(Page* page)
{
#ifdef AZ_DEBUG_BUILD
    page->m_threadData = 0;
#endif
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        m_freePages.push_front(*page);
    }
}

//=========================================================================
// GarbageCollect
// [9/15/2009]
//=========================================================================
void
ThreadPoolSchemaImpl::GarbageCollect()
{
    if (!m_isDynamic)
    {
        return;                // we have the memory statically allocated, can't collect garbage.
    }

    FreePagesType staticPages;
    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
    while (!m_freePages.empty())
    {
        Page* page = &m_freePages.front();
        m_freePages.pop_front();
        if (IsInStaticBlock(page))
        {
            staticPages.push_front(*page);
        }
        else
        {
            FreePage(page);
        }
    }
    while (!staticPages.empty())
    {
        Page* page = &staticPages.front();
        staticPages.pop_front();
        m_freePages.push_front(*page);
    }
}

//=========================================================================
// SetupFreeList
// [9/15/2009]
//=========================================================================
inline void
ThreadPoolSchemaImpl::Page::SetupFreeList(size_t elementSize, size_t pageDataBlockSize)
{
    char* pageData = reinterpret_cast<char*>(this) - pageDataBlockSize;
    m_freeList.clear();
    // setup free list
    size_t numElements = pageDataBlockSize / elementSize;
    for (size_t i = 0; i < numElements; ++i)
    {
        char* address = pageData+i*elementSize;
        Page::FakeNode* node = new(address) Page::FakeNode();
        m_freeList.push_back(*node);
    }
}

//=========================================================================
// ThreadPoolData::ThreadPoolData
// [9/15/2009]
//=========================================================================
ThreadPoolData::ThreadPoolData(ThreadPoolSchemaImpl* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize)
    : m_allocator(alloc, pageSize, minAllocationSize, maxAllocationSize)
{}

//=========================================================================
// ThreadPoolData::~ThreadPoolData
// [9/15/2009]
//=========================================================================
ThreadPoolData::~ThreadPoolData()
{
    // deallocate elements if they were freed from other threads
    ThreadPoolSchemaImpl::Page::FakeNodeLF* fakeLFNode;
    while ((fakeLFNode = m_freedElements.pop())!=nullptr)
    {
        m_allocator.DeAllocate(fakeLFNode);
    }
}
