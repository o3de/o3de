/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cstdint>

#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/AllocatorInstance.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/std/allocator_stateless.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/intrusive_slist.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/typetraits/alignment_of.h>

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/std/parallel/containers/lock_free_intrusive_stamped_stack.h>

#define POOL_ALLOCATION_PAGE_SIZE (size_t{4} * size_t{1024})
#define POOL_ALLOCATION_MIN_ALLOCATION_SIZE size_t{8}
#define POOL_ALLOCATION_MAX_ALLOCATION_SIZE size_t{512}

namespace AZ
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(PoolSchema, "PoolSchema", "{3BFAC20A-DBE9-4C94-AC20-8417FD9C9CB2}");
}

namespace AZ::Internal
{
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_IMPL(PoolAllocatorHelper, "PoolAllocatorHelper", PoolAllocatorHelperTemplateId, AZ_TYPE_INFO_CLASS);
    AZ_RTTI_NO_TYPE_INFO_IMPL((PoolAllocatorHelper, AZ_TYPE_INFO_CLASS), Base);
    template class PoolAllocatorHelper<PoolSchema>;

    // Also instantiate the PoolAllocatorHelper for the Thread Pool Allocator
    template class PoolAllocatorHelper<ThreadPoolSchemaHelper<ThreadPoolAllocator>>;
}

namespace AZ
{
    // Instantiate the PoolAllocatorHelper<PoolSchema> AZ::AzTypeInfo template
    template struct AzTypeInfo<Internal::PoolAllocatorHelper<PoolSchema>>;
}

namespace
{
    struct Magic32
    {
        static const AZ::u32 m_defValue = 0xfeedf00d;
        AZ_FORCE_INLINE Magic32()
        {
            m_value = (m_defValue ^ AZ::u32(reinterpret_cast<uintptr_t>(this)));
        }
        AZ_FORCE_INLINE ~Magic32()
        {
            m_value = 0;
        }
        AZ_FORCE_INLINE bool Validate() const
        {
            return m_value == (m_defValue ^ AZ::u32(reinterpret_cast<uintptr_t>(this)));
        }

    private:
        AZ::u32 m_value;
    };
}

namespace AZ
{
    // Definining the PoolAllocator::GetDebugConfig
    // method in the cpp file to prevent a lengthy recompile
    // when changing the O3DE_STACK_CAPTURE_DEPTH define
    AllocatorDebugConfig PoolAllocator::GetDebugConfig()
    {
        return AllocatorDebugConfig()
            .ExcludeFromDebugging(false)
            .StackRecordLevels(O3DE_STACK_CAPTURE_DEPTH)
            .MarksUnallocatedMemory(false)
            .UsesMemoryGuards(false);
    }

    AllocatorDebugConfig ThreadPoolAllocator::GetDebugConfig()
    {
        return AllocatorDebugConfig()
            .ExcludeFromDebugging(false)
            .StackRecordLevels(O3DE_STACK_CAPTURE_DEPTH)
            .MarksUnallocatedMemory(false)
            .UsesMemoryGuards(false);
    }
}

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
        AZ_CLASS_ALLOCATOR(PoolAllocation<Allocator>, SystemAllocator);

        using PageType = typename Allocator::Page;
        using BucketType = typename Allocator::Bucket;

        PoolAllocation(Allocator* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize);
        virtual ~PoolAllocation();

        AllocateAddress Allocate(size_t byteSize, size_t alignment);
        size_t DeAllocate(void* ptr);
        size_t get_allocated_size(void* ptr) const;
        void GarbageCollect();

        Allocator* m_allocator;
        size_t m_pageSize;
        size_t m_minAllocationShift;
        size_t m_minAllocationSize;
        size_t m_maxAllocationSize;
        size_t m_numBuckets;
        BucketType* m_buckets;
        size_t m_numBytesAllocated;
    };

    /**
     * PoolSchema Implementation... to keep the header clean.
     */
    class PoolSchemaImpl
    {
    public:
        PoolSchemaImpl(PoolSchema::size_type pageSize, PoolSchema::size_type minAllocationSize, PoolSchema::size_type maxAllocationSize);
        PoolSchemaImpl();
        ~PoolSchemaImpl();

        AllocateAddress Allocate(PoolSchema::size_type byteSize, PoolSchema::size_type alignment);
        PoolSchema::size_type DeAllocate(PoolSchema::pointer ptr);
        PoolSchema::size_type get_allocated_size(PoolSchema::pointer ptr, PoolSchema::align_type alignment) const;

        /**
         * We allocate memory for pools in pages. Page is a information struct
         * located at the end of the allocated page. When it's in the at the end
         * we can usually hide it's size in the free bytes left from the pagesize/poolsize.
         * \note IMPORTANT pages are aligned on the page size, this way can find quickly which
         * pool the pointer belongs to.
         */
        struct Page : public AZStd::intrusive_list_node<Page>
        {
            struct FakeNode : public AZStd::intrusive_slist_node<FakeNode>
            {
            };

            void SetupFreeList(size_t elementSize, size_t pageDataBlockSize);

            /// We just use a free list of nodes which we cast to the pool type.
            using FreeListType = AZStd::intrusive_slist<FakeNode, AZStd::slist_base_hook<FakeNode>>;

            FreeListType m_freeList;
            u32 m_bin;
            Magic32 m_magic;
            u32 m_elementSize;
            u32 m_maxNumElements;
        };

        /**
         * A bucket has a list of pages used with the specific pool size.
         */
        struct Bucket
        {
            using PageListType = AZStd::intrusive_list<Page, AZStd::list_base_hook<Page>>;
            PageListType m_pages;
        };

        // Functions used by PoolAllocation template
        AZ_INLINE Page* PopFreePage();
        AZ_INLINE void PushFreePage(Page* page);
        void GarbageCollect();
        inline Page* ConstructPage(size_t elementSize)
        {
            // We store the page struct at the end of the block
            char* memBlock;
            memBlock = reinterpret_cast<char*>(
                m_pageAllocator->Allocate(m_pageSize, m_pageSize));
            size_t pageDataSize = m_pageSize - sizeof(Page);
            Page* page = new (memBlock + pageDataSize) Page();
            page->SetupFreeList(elementSize, pageDataSize);
            page->m_elementSize = static_cast<u32>(elementSize);
            page->m_maxNumElements = static_cast<u32>(pageDataSize / elementSize);
            return page;
        }

        inline void FreePage(Page* page)
        {
            // TODO: It's optional if we want to check the guard value for corruption, since we are not going
            // to use this memory. Yet it might be useful to catch bugs.
            // We store the page struct at the end of the block
            char* memBlock = reinterpret_cast<char*>(page) - m_pageSize + sizeof(Page);
            page->~Page(); // destroy the page
            m_pageAllocator->DeAllocate(memBlock);
        }

        inline Page* PageFromAddress(void* address)
        {
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~(m_pageSize - 1));
            memBlock += m_pageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->m_magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        inline Bucket* AllocateBuckets(size_t numBuckets)
        {
            Bucket* buckets = reinterpret_cast<Bucket*>(static_cast<void*>(m_pageAllocator->allocate(sizeof(Bucket) * numBuckets, alignof(Bucket))));
            for (size_t i = 0; i < numBuckets; ++i)
            {
                new (buckets + i) Bucket();
            }
            return buckets;
        }

        inline void DeAllocateBuckets(Bucket* buckets, size_t numBuckets)
        {
            for (size_t i = 0; i < numBuckets; ++i)
            {
                buckets[i].~Bucket();
            }
            m_pageAllocator->deallocate(buckets, sizeof(Bucket) * numBuckets);
        }

        using AllocatorType = PoolAllocation<PoolSchemaImpl>;
        IAllocator*                   m_pageAllocator;
        AllocatorType m_allocator;
        size_t m_pageSize;
        Bucket::PageListType m_freePages;
    };

    /**
     * Thread safe pool allocator.
     */
    class ThreadPoolSchemaImpl
    {
    public:
        /**
         * Specialized \ref PoolAllocator::Page page for lock free allocator.
         */
        struct Page : public AZStd::intrusive_list_node<Page>
        {
            Page(ThreadPoolData* threadData)
                : m_threadData(threadData)
            {
            }

            struct FakeNode : public AZStd::intrusive_slist_node<FakeNode>
            {
            };
            // Fake Lock Free node used when we delete an element from another thread.
            struct FakeNodeLF : public AZStd::lock_free_intrusive_stack_node<FakeNodeLF>
            {
            };

            void SetupFreeList(size_t elementSize, size_t pageDataBlockSize);

            /// We just use a free list of nodes which we cast to the pool type.
            using FreeListType = AZStd::intrusive_slist<FakeNode, AZStd::slist_base_hook<FakeNode>>;

            FreeListType m_freeList;
            AZStd::lock_free_intrusive_stack_node<Page> m_lfStack; ///< Lock Free stack node
            struct ThreadPoolData* m_threadData; ///< The thread data that own's the page.
            u32 m_bin;
            Magic32 m_magic;
            u32 m_elementSize;
            u32 m_maxNumElements;
        };

        /**
         * A bucket has a list of pages used with the specific pool size.
         */
        struct Bucket
        {
            using PageListType = AZStd::intrusive_list<Page, AZStd::list_base_hook<Page>>;
            PageListType m_pages;
        };

        ThreadPoolSchemaImpl(
            ThreadPoolSchema::GetThreadPoolData threadPoolGetter,
            ThreadPoolSchema::SetThreadPoolData threadPoolSetter);

        ThreadPoolSchemaImpl(
            ThreadPoolSchema::GetThreadPoolData threadPoolGetter,
            ThreadPoolSchema::SetThreadPoolData threadPoolSetter,
            ThreadPoolSchema::size_type pageSize,
            ThreadPoolSchema::size_type minAllocationSize,
            ThreadPoolSchema::size_type maxAllocationSize);

        ~ThreadPoolSchemaImpl();

        AllocateAddress Allocate(ThreadPoolSchema::size_type byteSize, ThreadPoolSchema::size_type alignment);
        ThreadPoolSchema::size_type DeAllocate(ThreadPoolSchema::pointer ptr);
        ThreadPoolSchema::size_type get_allocated_size(ThreadPoolSchema::pointer ptr, ThreadPoolSchema::align_type alignment) const;
        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void GarbageCollect();
        //////////////////////////////////////////////////////////////////////////

        // Functions used by PoolAllocation template
        AZ_INLINE Page* PopFreePage();
        AZ_INLINE void PushFreePage(Page* page);
        inline Page* ConstructPage(size_t elementSize)
        {
            // We store the page struct at the end of the block
            char* memBlock;
            memBlock = reinterpret_cast<char*>(static_cast<void*>(
                m_pageAllocator->allocate(m_pageSize, m_pageSize)));
            size_t pageDataSize = m_pageSize - sizeof(Page);
            Page* page = new (memBlock + pageDataSize) Page(m_threadPoolGetter());
            page->SetupFreeList(elementSize, pageDataSize);
            page->m_elementSize = static_cast<u32>(elementSize);
            page->m_maxNumElements = static_cast<u32>(pageDataSize / elementSize);
            return page;
        }

        inline void FreePage(Page* page)
        {
            // TODO: It's optional if we want to check the guard value for corruption, since we are not going
            // to use this memory. Yet it might be useful to catch bugs.
            // We store the page struct at the end of the block
            char* memBlock = reinterpret_cast<char*>(page) - m_pageSize + sizeof(Page);
            page->~Page(); // destroy the page
            m_pageAllocator->deallocate(memBlock);
        }

        inline Page* PageFromAddress(void* address) const
        {
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~static_cast<size_t>(m_pageSize - 1));
            memBlock += m_pageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->m_magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        inline Bucket* AllocateBuckets(size_t numBuckets)
        {
            Bucket* buckets = static_cast<Bucket*>(static_cast<void*>(m_pageAllocator->allocate(sizeof(Bucket) * numBuckets, alignof(Bucket))));
            for (size_t i = 0; i < numBuckets; ++i)
            {
                new (buckets + i) Bucket();
            }
            return buckets;
        }

        inline void DeAllocateBuckets(Bucket* buckets, size_t numBuckets)
        {
            for (size_t i = 0; i < numBuckets; ++i)
            {
                buckets[i].~Bucket();
            }
            m_pageAllocator->deallocate(buckets, sizeof(Bucket) * numBuckets);
        }

        // NOTE we allow only one instance of a allocator, you need to subclass it for different versions.
        // so for now it's safe to use static. We use TLS on a static because on some platforms set thread key is available
        // only for pthread lib and we don't use it. I can't find other way to it, otherwise please switch this to
        // use TlsAlloc/TlsFree etc.
        ThreadPoolSchema::GetThreadPoolData m_threadPoolGetter;
        ThreadPoolSchema::SetThreadPoolData m_threadPoolSetter;

        // Fox X64 we push/pop pages using the m_mutex to sync. Pages are
        using FreePagesType = Bucket::PageListType;
        FreePagesType m_freePages;
        AZStd::vector<ThreadPoolData*, AZStd::stateless_allocator> m_threads; ///< Array with all separate thread data. Used to traverse end free elements.

        IAllocator*           m_pageAllocator;
        size_t m_pageSize;
        size_t m_minAllocationSize;
        size_t m_maxAllocationSize;
        // TODO rbbaklov Changed to recursive_mutex from mutex for Linux support.
        AZStd::recursive_mutex m_mutex;
    };

    struct ThreadPoolData
    {
        ThreadPoolData(ThreadPoolSchemaImpl* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize);
        ~ThreadPoolData();

        using AllocatorType = PoolAllocation<ThreadPoolSchemaImpl>;
        /**
         * Stack with freed elements from other threads. We don't need stamped stack since the ABA problem can not
         * happen here. We push from many threads and pop from only one (we don't push from it).
         */
        using FreedElementsStack = AZStd::lock_free_intrusive_stack<
            ThreadPoolSchemaImpl::Page::FakeNodeLF,
            AZStd::lock_free_intrusive_stack_base_hook<ThreadPoolSchemaImpl::Page::FakeNodeLF>>;

        AllocatorType m_allocator;
        FreedElementsStack m_freedElements;
    };
} // namespace AZ

namespace AZ
{
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
        AZ_Assert(
            pageSize >= maxAllocationSize * 4,
            "We need to fit at least 4 objects in a pool! Increase your page size! Page %d MaxAllocationSize %d", pageSize,
            maxAllocationSize);
        AZ_Assert(
            minAllocationSize == maxAllocationSize || ((minAllocationSize) & (minAllocationSize - 1)) == 0,
            "Min allocation should be either equal to max allocation size or power of two");

        m_minAllocationSize = AZ::GetMax(minAllocationSize, size_t(8));
        m_maxAllocationSize = AZ::GetMax(maxAllocationSize, minAllocationSize);

        m_minAllocationShift = 0;
        for (size_t i = 1; i < sizeof(unsigned int) * 8; i++)
        {
            if (m_minAllocationSize >> i == 0)
            {
                m_minAllocationShift = i - 1;
                break;
            }
        }

        AZ_Assert(
            m_maxAllocationSize % m_minAllocationSize == 0,
            "You need to be able to divide m_maxAllocationSize (%d) / m_minAllocationSize (%d) without fraction!", m_maxAllocationSize,
            m_minAllocationSize);
        m_numBuckets = m_maxAllocationSize / m_minAllocationSize;
        AZ_Assert(m_numBuckets <= 0xffff, "You can't have more than 65535 number of buckets! We need to increase the index size!");
        m_buckets = m_allocator->AllocateBuckets(m_numBuckets);
    }

    //=========================================================================
    // ~PoolAllocation
    // [9/09/2009]
    //=========================================================================
    template<class Allocator>
    PoolAllocation<Allocator>::~PoolAllocation()
    {
#if defined(AZ_ENABLE_TRACING)
        // Check all pages in buckets are empty
        if (m_buckets)
        {
            for (const auto& bucket : AZStd::span(m_buckets, m_buckets + m_numBuckets))
            {
                AZ_Assert(bucket.m_pages.empty(), "Found page for bucket %p", &bucket);
            }
        }
#endif
        GarbageCollect();
    }

    //=========================================================================
    // Allocate
    // [9/09/2009]
    //=========================================================================
    template<class Allocator>
    inline AllocateAddress PoolAllocation<Allocator>::Allocate(size_t byteSize, size_t alignment)
    {
        AZ_Assert(byteSize > 0, "You can not allocate 0 bytes!");
        AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "Alignment must be >0 and power of 2!");

        // pad the size to the min allocation size.
        byteSize = AZ::SizeAlignUp(byteSize, m_minAllocationSize);
        byteSize = AZ::SizeAlignUp(byteSize, alignment);

        if (byteSize > m_maxAllocationSize)
        {
            AZ_Assert(false, "Allocation size (%d) is too big (max: %d) for pools!", byteSize, m_maxAllocationSize);
            return AllocateAddress{};
        }

        if (!m_buckets)
        {
            m_buckets = m_allocator->AllocateBuckets(m_numBuckets);
        }

        u32 bucketIndex = static_cast<u32>((byteSize >> m_minAllocationShift) - 1);
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
            else if (page->m_freeList.size() == 1)
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
                if (page->m_bin != bucketIndex) // if this page was used the same bucket we are ready to roll.
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

        return AllocateAddress{ address, byteSize };
    }

    //=========================================================================
    // DeAllocate
    // [9/09/2009]
    //=========================================================================
    template<class Allocator>
    inline size_t PoolAllocation<Allocator>::DeAllocate(void* ptr)
    {
        PageType* page = m_allocator->PageFromAddress(ptr);
        if (page == nullptr)
        {
            AZ_Error("Memory", false, "Address 0x%08x is not in the ThreadPool!", ptr);
            return 0;
        }

        // (pageSize - info struct at the end) / (element size)
        size_t maxElementsPerBucket = page->m_maxNumElements;

        size_t numFreeNodes = page->m_freeList.size();
        typename PageType::FakeNode* node = new (ptr) typename PageType::FakeNode();
        page->m_freeList.push_front(*node);

        if (numFreeNodes == 0)
        {
            // if the page was full before sort at the front
            BucketType& bucket = m_buckets[page->m_bin];
            bucket.m_pages.erase(*page);
            bucket.m_pages.push_front(*page);
        }
        else if (numFreeNodes == maxElementsPerBucket - 1)
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

        size_t bytesDeallocated = page->m_elementSize;
        m_numBytesAllocated -= bytesDeallocated;
        return bytesDeallocated;
    }

    //=========================================================================
    // AllocationSize
    // [11/22/2010]
    //=========================================================================
    template<class Allocator>
    AZ_INLINE size_t PoolAllocation<Allocator>::get_allocated_size(void* ptr) const
    {
        const PageType* page = m_allocator->PageFromAddress(ptr);
        return page ? page->m_elementSize : 0;
    }

    //=========================================================================
    // GarbageCollect
    // [3/1/2012]
    //=========================================================================
    template<class Allocator>
    AZ_INLINE void PoolAllocation<Allocator>::GarbageCollect()
    {
        if (m_buckets)
        {
            // Free empty pages in the buckets
            for (auto& bucket : AZStd::span(m_buckets, m_buckets + m_numBuckets))
            {
                auto& pages = bucket.m_pages;
                pages.remove_if([this](PageType& page)
                {
                    if (page.m_freeList.size() == page.m_maxNumElements)
                    {
                        m_allocator->FreePage(&page);
                        return true;
                    };
                    return false;
                });
            }

            const bool allBucketsEmpty = AZStd::all_of(m_buckets, m_buckets + m_numBuckets, [](const BucketType& bucket)
            {
                return bucket.m_pages.empty();
            });
            if (allBucketsEmpty)
            {
                m_allocator->DeAllocateBuckets(m_buckets, m_numBuckets);
                m_buckets = nullptr;
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
    PoolSchema::PoolSchema()
        : m_impl(nullptr)
    {
    }

    //=========================================================================
    // ~PoolSchema
    // [9/15/2009]
    //=========================================================================
    PoolSchema::~PoolSchema()
    {
        m_impl->~PoolSchemaImpl();
        AZStd::stateless_allocator().deallocate(m_impl, sizeof(PoolSchemaImpl));
        m_impl = nullptr;
    }

    //=========================================================================
    // Create
    // [9/15/2009]
    //=========================================================================
    bool PoolSchema::Create()
    {
        return Create(POOL_ALLOCATION_PAGE_SIZE, POOL_ALLOCATION_MIN_ALLOCATION_SIZE, POOL_ALLOCATION_MAX_ALLOCATION_SIZE);
    }

    bool PoolSchema::Create(PoolSchema::size_type pageSize, PoolSchema::size_type minAllocationSize, PoolSchema::size_type maxAllocationSize)
    {
        m_impl = new (AZStd::stateless_allocator().allocate(sizeof(PoolSchemaImpl), AZStd::alignment_of<PoolSchemaImpl>::value))
            PoolSchemaImpl(pageSize, minAllocationSize, maxAllocationSize);
        return true;
    }

    //=========================================================================
    // Allocate
    // [9/15/2009]
    //=========================================================================
    AllocateAddress PoolSchema::allocate(size_type byteSize, size_type alignment)
    {
        return m_impl->Allocate(byteSize, alignment);
    }

    //=========================================================================
    // DeAllocate
    // [9/15/2009]
    //=========================================================================
    auto PoolSchema::deallocate(pointer ptr, size_type byteSize, size_type alignment) -> size_type
    {
        (void)byteSize;
        (void)alignment;
        return m_impl->DeAllocate(ptr);
    }

    //=========================================================================
    // ReAllocate
    // [10/14/2018]
    //=========================================================================
    AllocateAddress PoolSchema::reallocate(pointer ptr, size_type newSize, size_type newAlignment)
    {
        (void)ptr;
        (void)newSize;
        (void)newAlignment;
        AZ_Assert(false, "unsupported");

        return AllocateAddress{ ptr, get_allocated_size(ptr, newAlignment) };
    }

    //=========================================================================
    // AllocationSize
    // [11/22/2010]
    //=========================================================================
    auto PoolSchema::get_allocated_size(pointer ptr, align_type alignment) const -> size_type
    {
        return m_impl->get_allocated_size(ptr, alignment);
    }

    //=========================================================================
    // DeAllocate
    // [9/15/2009]
    //=========================================================================
    void PoolSchema::GarbageCollect()
    {
        // External requests for garbage collection may come from any thread, and the
        // garbage collection operation isn't threadsafe, which can lead to crashes.

        m_impl->GarbageCollect();
    }

    //=========================================================================
    // NumAllocatedBytes
    // [11/1/2010]
    //=========================================================================
    auto PoolSchema::NumAllocatedBytes() const -> size_type
    {
        return m_impl->m_allocator.m_numBytesAllocated;
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
    PoolSchemaImpl::PoolSchemaImpl()
        : PoolSchemaImpl(POOL_ALLOCATION_PAGE_SIZE, POOL_ALLOCATION_MIN_ALLOCATION_SIZE, POOL_ALLOCATION_MAX_ALLOCATION_SIZE)
    {
    }

    PoolSchemaImpl::PoolSchemaImpl(
        PoolSchema::size_type pageSize, PoolSchema::size_type minAllocationSize, PoolSchema::size_type maxAllocationSize)
        : m_pageAllocator(&AllocatorInstance<SystemAllocator>::Get())
        , m_allocator(this, pageSize, minAllocationSize, maxAllocationSize)
        , m_pageSize(pageSize)
    {
    }

    //=========================================================================
    // ~PoolSchemaImpl
    // [9/15/2009]
    //=========================================================================
    PoolSchemaImpl::~PoolSchemaImpl()
    {
        // Free all unused memory
        GarbageCollect();
    }

    //=========================================================================
    // Allocate
    // [9/15/2009]
    //=========================================================================
    AllocateAddress PoolSchemaImpl::Allocate(PoolSchema::size_type byteSize, PoolSchema::size_type alignment)
    {
        // AZ_Warning("Memory",m_ownerThread==AZStd::this_thread::get_id(),"You can't allocation from a different context/thread, use
        // ThreadPoolAllocator!");
        return m_allocator.Allocate(byteSize, alignment);
    }

    //=========================================================================
    // DeAllocate
    // [9/15/2009]
    //=========================================================================
    PoolSchema::size_type PoolSchemaImpl::DeAllocate(PoolSchema::pointer ptr)
    {
        // AZ_Warning("Memory",m_ownerThread==AZStd::this_thread::get_id(),"You can't deallocate from a different context/thread, use
        // ThreadPoolAllocator!");
        return m_allocator.DeAllocate(ptr);
    }

    //=========================================================================
    // AllocationSize
    // [11/22/2010]
    //=========================================================================
    PoolSchema::size_type PoolSchemaImpl::get_allocated_size(PoolSchema::pointer ptr, [[maybe_unused]] PoolSchema::align_type alignment) const
    {
        // AZ_Warning("Memory",m_ownerThread==AZStd::this_thread::get_id(),"You can't use PoolAllocator from a different context/thread, use
        // ThreadPoolAllocator!");
        return m_allocator.get_allocated_size(ptr);
    }

    //=========================================================================
    // Pop
    // [9/15/2009]
    //=========================================================================
    AZ_FORCE_INLINE PoolSchemaImpl::Page* PoolSchemaImpl::PopFreePage()
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
    AZ_INLINE void PoolSchemaImpl::PushFreePage(Page* page)
    {
        m_freePages.push_front(*page);
    }

    //=========================================================================
    // PurgePages
    // [9/11/2009]
    //=========================================================================
    void PoolSchemaImpl::GarbageCollect()
    {
        while (!m_freePages.empty())
        {
            Page* page = &m_freePages.front();
            m_freePages.pop_front();
            FreePage(page);
        }
        m_allocator.GarbageCollect();
    }

    //=========================================================================
    // SetupFreeList
    // [9/09/2009]
    //=========================================================================
    AZ_FORCE_INLINE void PoolSchemaImpl::Page::SetupFreeList(size_t elementSize, size_t pageDataBlockSize)
    {
        char* pageData = reinterpret_cast<char*>(this) - pageDataBlockSize;
        m_freeList.clear();
        // setup free list
        size_t numElements = pageDataBlockSize / elementSize;
        for (unsigned int i = 0; i < numElements; ++i)
        {
            char* address = pageData + i * elementSize;
            Page::FakeNode* node = new (address) Page::FakeNode();
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
        : m_threadPoolGetter(getThreadPoolData)
        , m_threadPoolSetter(setThreadPoolData)
        , m_impl(nullptr)
    {
    }

    //=========================================================================
    // ~ThreadPoolSchema
    // [9/15/2009]
    //=========================================================================
    ThreadPoolSchema::~ThreadPoolSchema()
    {
        m_impl->~ThreadPoolSchemaImpl();
        AZStd::stateless_allocator().deallocate(m_impl, sizeof(ThreadPoolSchemaImpl));
        m_impl = nullptr;
    }

    //=========================================================================
    // Create
    // [9/15/2009]
    //=========================================================================
    bool ThreadPoolSchema::Create()
    {
        return Create(POOL_ALLOCATION_PAGE_SIZE, POOL_ALLOCATION_MIN_ALLOCATION_SIZE, POOL_ALLOCATION_MAX_ALLOCATION_SIZE);
    }

    bool ThreadPoolSchema::Create(
        PoolSchema::size_type pageSize, PoolSchema::size_type minAllocationSize, PoolSchema::size_type maxAllocationSize)
    {
        m_impl = new (AZStd::stateless_allocator().allocate(sizeof(ThreadPoolSchemaImpl), AZStd::alignment_of<ThreadPoolSchemaImpl>::value))
                     ThreadPoolSchemaImpl(m_threadPoolGetter, m_threadPoolSetter, pageSize, minAllocationSize, maxAllocationSize);
        return true;
    }

    //=========================================================================
    // Allocate
    // [9/15/2009]
    //=========================================================================
    AllocateAddress ThreadPoolSchema::allocate(size_type byteSize, size_type alignment)
    {
        return m_impl->Allocate(byteSize, alignment);
    }

    //=========================================================================
    // DeAllocate
    // [9/15/2009]
    //=========================================================================
    auto ThreadPoolSchema::deallocate(pointer ptr, size_type byteSize, size_type alignment) -> size_type
    {
        (void)byteSize;
        (void)alignment;
        return m_impl->DeAllocate(ptr);
    }

    //=========================================================================
    // ReAllocate
    // [10/14/2018]
    //=========================================================================
    AllocateAddress ThreadPoolSchema::reallocate(pointer ptr, size_type newSize, size_type newAlignment)
    {
        (void)ptr;
        (void)newSize;
        (void)newAlignment;
        AZ_Assert(false, "unsupported");

        return AllocateAddress{ ptr, get_allocated_size(ptr, newAlignment) };
    }

    //=========================================================================
    // get_allocated_size
    // [11/22/2010]
    //=========================================================================
    ThreadPoolSchema::size_type ThreadPoolSchema::get_allocated_size(pointer ptr, align_type alignment) const
    {
        return m_impl->get_allocated_size(ptr, alignment);
    }

    //=========================================================================
    // DeAllocate
    // [9/15/2009]
    //=========================================================================
    void ThreadPoolSchema::GarbageCollect()
    {
        m_impl->GarbageCollect();
    }

    //=========================================================================
    // NumAllocatedBytes
    // [11/1/2010]
    //=========================================================================
    ThreadPoolSchema::size_type ThreadPoolSchema::NumAllocatedBytes() const
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
    // ThreadPoolSchemaImpl
    // [9/15/2009]
    //=========================================================================
    ThreadPoolSchemaImpl::ThreadPoolSchemaImpl(
        ThreadPoolSchema::GetThreadPoolData threadPoolGetter, ThreadPoolSchema::SetThreadPoolData threadPoolSetter)
        : ThreadPoolSchemaImpl(
              threadPoolGetter,
              threadPoolSetter,
              POOL_ALLOCATION_PAGE_SIZE,
              POOL_ALLOCATION_MIN_ALLOCATION_SIZE,
              POOL_ALLOCATION_MAX_ALLOCATION_SIZE)
    {
    }

    ThreadPoolSchemaImpl::ThreadPoolSchemaImpl(
        ThreadPoolSchema::GetThreadPoolData threadPoolGetter,
        ThreadPoolSchema::SetThreadPoolData threadPoolSetter,
        ThreadPoolSchema::size_type pageSize,
        ThreadPoolSchema::size_type minAllocationSize,
        ThreadPoolSchema::size_type maxAllocationSize)
        : m_threadPoolGetter(threadPoolGetter)
        , m_threadPoolSetter(threadPoolSetter)
        , m_pageAllocator(&AllocatorInstance<SystemAllocator>::Get())
        , m_pageSize(pageSize)
        , m_minAllocationSize(minAllocationSize)
        , m_maxAllocationSize(maxAllocationSize)
    {
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
        // In memory allocation case (usually tools) we might have high contention,
        // using spin lock will improve performance.
        SetCriticalSectionSpinCount(m_mutex.native_handle(), 4000);
#endif
    }

    //=========================================================================
    // ~ThreadPoolSchemaImpl
    // [9/15/2009]
    //=========================================================================
    ThreadPoolSchemaImpl::~ThreadPoolSchemaImpl()
    {
        GarbageCollect();

        // clean up all the thread data.
        // IMPORTANT: We assume/rely that all threads (except the calling one) are or will
        // destroyed before you create another instance of the pool allocation.
        // This should generally be ok since the all allocators are singletons.
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        if (!m_threads.empty())
        {
            for (ThreadPoolData*& threadData : m_threads)
            {
                if (threadData)
                {
                    // Force free all pages
                    threadData->~ThreadPoolData();
                    AZStd::stateless_allocator().deallocate(
                        threadData, sizeof(ThreadPoolData), AZStd::alignment_of<ThreadPoolData>::value);
                    threadData = nullptr;
                }
            }

            /// reset the variable for the owner thread.
            m_threadPoolSetter(nullptr);
        }
    }

    //=========================================================================
    // Allocate
    // [9/15/2009]
    //=========================================================================
    AllocateAddress ThreadPoolSchemaImpl::Allocate(
        ThreadPoolSchema::size_type byteSize, ThreadPoolSchema::size_type alignment)
    {
        ThreadPoolData* threadData = m_threadPoolGetter();

        if (threadData == nullptr)
        {
            void* threadPoolData = AZStd::stateless_allocator().allocate(sizeof(ThreadPoolData), AZStd::alignment_of<ThreadPoolData>::value);
            threadData = new (threadPoolData) ThreadPoolData(this, m_pageSize, m_minAllocationSize, m_maxAllocationSize);
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
            while ((fakeLFNode = threadData->m_freedElements.pop()) != nullptr)
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
    ThreadPoolSchema::size_type ThreadPoolSchemaImpl::DeAllocate(ThreadPoolSchema::pointer ptr)
    {
        Page* page = PageFromAddress(ptr);
        if (page == nullptr)
        {
            AZ_Error("Memory", false, "Address 0x%08x is not in the ThreadPool!", ptr);
            return 0;
        }
        AZ_Assert(page->m_threadData != nullptr, ("We must have valid page thread data for the page!"));
        ThreadPoolData* threadData = m_threadPoolGetter();
        if (threadData == page->m_threadData)
        {
            // we can free here
            return threadData->m_allocator.DeAllocate(ptr);
        }
        else
        {
            // push this element to be deleted from it's own thread!
            // cast the pointer to a fake lock free node
            Page::FakeNodeLF* fakeLFNode = reinterpret_cast<Page::FakeNodeLF*>(ptr);
#ifdef AZ_DEBUG_BUILD
            // we need to reset the fakeLFNode because we share the memory.
            // otherwise we will assert the node is in the list
            fakeLFNode->m_next = 0;
#endif
            // Query the allocated size of the ptr before pushing it on the freed element stack
            const size_t allocatedSize = page->m_threadData->m_allocator.get_allocated_size(ptr);
            page->m_threadData->m_freedElements.push(*fakeLFNode);

            return allocatedSize;
        }
    }

    //=========================================================================
    // AllocationSize
    // [11/22/2010]
    //=========================================================================
    ThreadPoolSchema::size_type ThreadPoolSchemaImpl::get_allocated_size(ThreadPoolSchema::pointer ptr, [[maybe_unused]] ThreadPoolSchema::align_type alignment) const
    {
        const Page* page = PageFromAddress(ptr);
        if (page == nullptr)
        {
            return 0;
        }
        AZ_Assert(page->m_threadData != nullptr, ("We must have valid page thread data for the page!"));
        return page->m_threadData->m_allocator.get_allocated_size(ptr);
    }

    //=========================================================================
    // PopFreePage
    // [9/15/2009]
    //=========================================================================
    AZ_INLINE ThreadPoolSchemaImpl::Page* ThreadPoolSchemaImpl::PopFreePage()
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
#ifdef AZ_DEBUG_BUILD
            AZ_Assert(page->m_threadData == 0, "If we stored the free page properly we should have null here!");
#endif
            // store the current thread data, used when we free elements
            page->m_threadData = m_threadPoolGetter();
        }
        return page;
    }

    //=========================================================================
    // PushFreePage
    // [9/15/2009]
    //=========================================================================
    AZ_INLINE void ThreadPoolSchemaImpl::PushFreePage(Page* page)
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
    void ThreadPoolSchemaImpl::GarbageCollect()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        for (ThreadPoolData* threadData : m_threads)
        {
            if (threadData)
            {
                for (auto fakeLFNode = threadData->m_freedElements.pop(); fakeLFNode; fakeLFNode = threadData->m_freedElements.pop())
                {
                    threadData->m_allocator.DeAllocate(fakeLFNode);
                }
                threadData->m_allocator.GarbageCollect();
            }
        }
        while (!m_freePages.empty())
        {
            Page* page = &m_freePages.front();
            m_freePages.pop_front();
            FreePage(page);
        }
    }

    //=========================================================================
    // SetupFreeList
    // [9/15/2009]
    //=========================================================================
    inline void ThreadPoolSchemaImpl::Page::SetupFreeList(size_t elementSize, size_t pageDataBlockSize)
    {
        char* pageData = reinterpret_cast<char*>(this) - pageDataBlockSize;
        m_freeList.clear();
        // setup free list
        size_t numElements = pageDataBlockSize / elementSize;
        for (size_t i = 0; i < numElements; ++i)
        {
            char* address = pageData + i * elementSize;
            Page::FakeNode* node = new (address) Page::FakeNode();
            m_freeList.push_back(*node);
        }
    }

    //=========================================================================
    // ThreadPoolData::ThreadPoolData
    // [9/15/2009]
    //=========================================================================
    ThreadPoolData::ThreadPoolData(ThreadPoolSchemaImpl* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize)
        : m_allocator(alloc, pageSize, minAllocationSize, maxAllocationSize)
    {
    }

    //=========================================================================
    // ThreadPoolData::~ThreadPoolData
    // [9/15/2009]
    //=========================================================================
    ThreadPoolData::~ThreadPoolData()
    {
        // deallocate elements if they were freed from other threads
        for (ThreadPoolSchemaImpl::Page::FakeNodeLF* fakeLFNode = m_freedElements.pop(); fakeLFNode; fakeLFNode = m_freedElements.pop())
        {
            m_allocator.DeAllocate(fakeLFNode);
        }
    }

} // namespace AZ
