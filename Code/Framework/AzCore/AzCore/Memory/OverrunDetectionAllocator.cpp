/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/OverrunDetectionAllocator.h>
#include <AzCore/Memory/OSAllocator.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzCore/Memory/OverrunDetectionAllocator_Platform.h>

namespace AZ
{
    namespace Internal
    {
        class SmallPageAllocator;
        class SmallAllocationGroup;

        static const int ODS_DEFAULT_ALIGNMENT = sizeof(void*) * 2;  // Default malloc alignment

        static_assert(1 << ODS_PAGE_SHIFT == ODS_PAGE_SIZE, "Mismatched ODS_PAGE_SIZE and ODS_PAGE_SHIFT");
        static_assert(1 << ODS_PAGES_PER_ALLOCATION_SHIFT == ODS_PAGES_PER_ALLOCATION, "Mismatched ODS_PAGES_PER_ALLOCATION and ODS_PAGES_PER_ALLOCATION_SHIFT");

        //---------------------------------------------------------------------
        // PageListNode - An intrusively linked list node representing a set of
        // available, consecutive pages within an allocation.
        //
        // E.g. on Windows, an allocation of 64kb will contain 16x 4kb pages.
        // For this platform, the Nth PageListNode for that allocation 
        // represents:
        // * where in the 64kb allocation you can find N consecutive available 
        //   pages
        // * if the Nth page has been been allocated, how many consecutive 
        //   pages are allocated from there (i.e. total sub-allocation size)
        //
        // We use indices instead of pointers for the linked list both for 
        // compactness and so we can store all PageListNodes in a single vector
        // without invalidating the links whenever the vector grows.
        //---------------------------------------------------------------------
        class PageListNode
        {
        public:
            PageListNode();

            void Init(int index);
            SmallAllocationGroup* GetOwningAllocationGroup() const;
            uint32_t GetOwnerIndex() const;
            
            void SetAvailablePage(int availablePage);
            uint32_t GetAvailablePage() const;
            
            void SetAllocatedPageCount(int allocatedPageCount);
            uint32_t GetAllocatedPageCount() const;
            
            bool IsLinked();
            void Unlink();
            void Link(PageListNode& other);

        private:
            static const uint32_t UNINITIALIZED = 0xFFFFFFFF;

            uint32_t m_next = 0;
            uint32_t m_prev = 0;
            uint8_t m_index = 0;
            uint8_t m_availablePage = 0;
            uint8_t m_allocatedPageCount = 0;

            friend class SmallAllocationGroup;
            friend class SmallPageAllocator;
        };


        //---------------------------------------------------------------------
        // SmallAllocationGroup - Represents a grouping of pages used for 
        // small allocations.
        // 
        // This exists because the minimum allocation size on some platforms
        // (e.g. 64k on Windows) greatly exceeds the page size (4k on Windows).
        //
        // So for every large allocation, a SmallAllocationGroup is used to 
        // track the small sub-allocations within it.
        //
        // These are stored on the system heap with the OS-provided allocator.
        //---------------------------------------------------------------------
        class SmallAllocationGroup
        {
        public:
            SmallAllocationGroup(void* data, uint32_t index) : m_data(data), m_index(index)
            {
                for (int i = 0; i < ODS_PAGES_PER_ALLOCATION; i++)
                {
                    m_freeListNodes[i].Init(i);
                }
            }

            /// Returns a pointer to the beginning of vector of SmallAllocationGroups.
            SmallAllocationGroup* ArrayHead()
            {
                return this - m_index;
            }

        private:
            void* m_data;
            uint32_t m_index;
            PageListNode m_freeListNodes[ODS_PAGES_PER_ALLOCATION];
            uint16_t m_allocationMap = 0;

            friend class PageListNode;
            friend class SmallPageAllocator;
        };


        //---------------------------------------------------------------------
        // AllocationRecord - A header placed before each allocation with
        // tracking information used for deallocation.
        //
        // (Note: for underrun detection, this would need to be placed at the
        // end of the allocation block, probably with a conspicuous magic 
        // number header so that it could be found from the original pointer.)
        //---------------------------------------------------------------------
        struct AllocationRecord
        {
            static const uint32_t MARKER = 0x10FACADE;

            /// Used for large allocations (>= ODS_ALLOCATION_SIZE)
            AllocationRecord(void* p, size_t size) :
                m_data((size_t)p),
                m_size((uint32_t)(size & 0xFFFFFFFF))
            {
                AZ_Assert(size < 0x100000000ull, "Maximum 4 GB allocations");
            }

            /// Used for small allocations (< ODS_ALLOCATION_SIZE)
            AllocationRecord(uint32_t index, uint32_t page_index, size_t size) :
                m_data(index | ((uint64_t)page_index << 32) | 0x8000000000000000ull),
                m_size((uint32_t)(size & 0xFFFFFFFF))
            {
                AZ_Assert(size < 0x100000000ull, "Maximum 4 GB allocations");
            }

            size_t GetSize() const
            {
                return m_size;
            }

            /// Returns a pointer to the base allocation (large allocations only)
            void* GetPointer() const
            {
                return (void *)(m_data & 0x7FFFFFFFFFFFFFFFull);
            }

            bool IsSmallAllocation() const
            {
                return (m_data & 0x8000000000000000ull) != 0;
            }

            bool IsValidAllocationRecord() const
            {
                return m_marker == MARKER;
            }

            /// Returns the index of the SmallAllocationGroup containing this allocation
            uint32_t GetSmallAllocationIndex() const
            {
                return (uint32_t)(m_data & 0xFFFFFFFF);
            }

            /// Returns the page within the SmallAllocationGroup containing this allocation
            uint32_t GetPageIndex() const
            {
                return (uint32_t)((m_data >> 32) & 0x7FFFFFFF);
            }

            uint64_t m_data = 0;
            uint32_t m_marker = MARKER;
            uint32_t m_size = 0;
        };

        //---------------------------------------------------------------------
        // SmallPageAllocator - The logic behind small page allocations.
        //---------------------------------------------------------------------
        class SmallPageAllocator
        {
        public:
            struct AllocationResult
            {
                void* m_data = nullptr;
                uint32_t m_index = 0;
                uint32_t m_pageIndex = 0;
            };

            void Init(OverrunDetectionSchema::PlatformAllocator* platformAllocator, bool underrunMode);
            AllocationResult AllocateConsecutivePages(uint32_t pageCount);
            void Deallocate(void* p, uint32_t smallAllocationIndex, uint32_t pageIndex);

        private:
            void UpdateAvailability(SmallAllocationGroup* allocationGroup);

            OverrunDetectionSchema::PlatformAllocator* m_platformAllocator = nullptr;
            AZStd::vector<SmallAllocationGroup, AZ::OSStdAllocator> m_smallAllocationGroups;
            bool m_underrunMode = false;
        };
    }


    //---------------------------------------------------------------------
    // OverrunDetectionSchemaImpl - Private implementation class for 
    // OverrunDetectionScheam.
    //---------------------------------------------------------------------
    class OverrunDetectionSchemaImpl
    {
    public:
        using pointer_type = void *;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        OverrunDetectionSchemaImpl(const OverrunDetectionSchema::Descriptor& desc);
        ~OverrunDetectionSchemaImpl();

        pointer_type Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord);
        void DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment);
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment);
        size_type Resize(pointer_type ptr, size_type newSize);
        size_type AllocationSize(pointer_type ptr);

        size_type NumAllocatedBytes() const;
        size_type Capacity() const;
        size_type GetMaxAllocationSize() const;
        size_type GetMaxContiguousAllocationSize() const;
        IAllocatorAllocate* GetSubAllocator();
        void GarbageCollect();

    private:
        void* RepositionAllocationToPageBoundary(void* p, size_t requestedByteSize, size_t requestedAlignment, size_t allocatedSpace) const;
        Internal::AllocationRecord* GetAllocationRecord(void* p) const;
        Internal::AllocationRecord* CreateAllocationRecord(void* p, size_t size) const;

    private:
        using mutex_type = AZStd::mutex;
        using lock_type = AZStd::lock_guard<mutex_type>;

        AZStd::unique_ptr<OverrunDetectionSchema::PlatformAllocator> m_platformAllocator;
        mutex_type m_mutex;
        Internal::SmallPageAllocator m_smallPageAllocator;

        AZStd::atomic<size_t> m_used;
        bool m_underrunMode = false;

        friend class Internal::SmallPageAllocator;
    };
}


//---------------------------------------------------------------------
// PageListNode methods
//---------------------------------------------------------------------

AZ::Internal::PageListNode::PageListNode() : 
    m_next(UNINITIALIZED), 
    m_prev(UNINITIALIZED)
{
}

void AZ::Internal::PageListNode::Init(int index)
{
    m_index = (uint8_t)index;
    m_next = GetOwnerIndex();
    m_prev = m_next;
}

AZ::Internal::SmallAllocationGroup* AZ::Internal::PageListNode::GetOwningAllocationGroup() const
{
    return (SmallAllocationGroup*)((char*)this - offsetof(SmallAllocationGroup, m_freeListNodes) - sizeof(PageListNode) * m_index);
}

uint32_t AZ::Internal::PageListNode::GetOwnerIndex() const
{
    return GetOwningAllocationGroup()->m_index;
}

void AZ::Internal::PageListNode::SetAvailablePage(int availablePage)
{
    m_availablePage = (uint8_t)availablePage;
}

uint32_t AZ::Internal::PageListNode::GetAvailablePage() const
{
    return m_availablePage;
}

void AZ::Internal::PageListNode::SetAllocatedPageCount(int allocatedPageCount)
{
    m_allocatedPageCount = (uint8_t)allocatedPageCount;
}

uint32_t AZ::Internal::PageListNode::GetAllocatedPageCount() const
{
    return m_allocatedPageCount;
}

bool AZ::Internal::PageListNode::IsLinked()
{
    return m_next != GetOwningAllocationGroup()->m_index;
}

void AZ::Internal::PageListNode::Unlink()
{
    SmallAllocationGroup* head = GetOwningAllocationGroup()->ArrayHead();
    PageListNode* next = (head + m_next)->m_freeListNodes + m_index;
    PageListNode* prev = (head + m_prev)->m_freeListNodes + m_index;
    next->m_prev = m_prev;
    prev->m_next = m_next;
    m_next = GetOwnerIndex();
    m_prev = m_next;
}

void AZ::Internal::PageListNode::Link(PageListNode& other)
{
    m_prev = other.m_prev;
    m_next = other.GetOwnerIndex();
    other.m_prev = GetOwnerIndex();

    SmallAllocationGroup* head = GetOwningAllocationGroup()->ArrayHead();
    PageListNode* prev = (head + m_prev)->m_freeListNodes + m_index;
    prev->m_next = other.m_prev;
}


//---------------------------------------------------------------------
// SmallPageAllocator methods
//---------------------------------------------------------------------

void AZ::Internal::SmallPageAllocator::Init(OverrunDetectionSchema::PlatformAllocator* platformAllocator, bool underrunMode)
{
    m_platformAllocator = platformAllocator;
    m_underrunMode = underrunMode;
    m_smallAllocationGroups.reserve(1024);
    m_smallAllocationGroups.emplace_back(SmallAllocationGroup(nullptr, 0));  // Our sentinel object for linked lists
}

AZ::Internal::SmallPageAllocator::AllocationResult AZ::Internal::SmallPageAllocator::AllocateConsecutivePages(uint32_t pageCount)
{
    AZ_Assert(pageCount > 1, "PageCount %d must be 1 page, plus 1 for the overrun trap", pageCount);  // Minimum 1 page, plus 1 for the overrun trap
    AZ_Assert(pageCount <= ODS_PAGES_PER_ALLOCATION, "Cannot perform a small allocation with %d pages (more than %d pages per allocation)", pageCount, ODS_PAGES_PER_ALLOCATION);

    AllocationResult result;
    PageListNode& listHead = m_smallAllocationGroups[0].m_freeListNodes[pageCount - 1];
    SmallAllocationGroup* allocationGroup = nullptr;

    if (listHead.IsLinked())
    {
        // There's currently a SmallAllocationGroup that has pageCount consecutive available pages
        allocationGroup = &m_smallAllocationGroups[listHead.m_prev];  // Take the first element of the linked list, before the sentinel object
    }
    else
    {
        // There's not currently a SmallAllocationGroup with pageCount consecutive pages available, so we need to allocate memory for a new one
        void* data = m_platformAllocator->ReserveBytes(ODS_ALLOCATION_SIZE);
        size_t index = m_smallAllocationGroups.size();
        m_smallAllocationGroups.emplace_back(SmallAllocationGroup(data, (uint32_t)index));
        allocationGroup = &m_smallAllocationGroups.back();
    }

    uint32_t pageIndex = allocationGroup->m_freeListNodes[pageCount - 1].GetAvailablePage();  // The SmallAllocationGroup tells us which page is the start of the requested free pages
    uint32_t requestMask = (1 << pageCount) - 1;
    result.m_index = allocationGroup->m_index;
    result.m_pageIndex = pageIndex;

    // Mark these pages as allocated
    AZ_Assert((allocationGroup->m_allocationMap & (requestMask << pageIndex)) == 0, "Allocating to space that is already marked as allocated");
    AZ_Assert(allocationGroup->m_freeListNodes[pageIndex].GetAllocatedPageCount() == 0, "Allocated pages already exist");
    allocationGroup->m_allocationMap |= requestMask << pageIndex;

    // Record the number of pages we are allocating for this slot
    allocationGroup->m_freeListNodes[pageIndex].SetAllocatedPageCount(pageCount);

#if 0
    // Used for debugging the allocator only, make all pages in the allocated range reflect the requested pageCount, instead of just the first
    for (auto i = pageIndex; i < pageIndex + pageCount; i++)
    {
        allocationGroup->m_freeListNodes[i].SetAllocatedPageCount(pageCount);
    }
#endif

    // Update the slots and freelists for every permissible pageCount request
    UpdateAvailability(allocationGroup);

    // Commit the memory
    void* baseAddress = (char*)allocationGroup->m_data + (pageIndex << ODS_PAGE_SHIFT);

    if (m_underrunMode)
    {
        baseAddress = (char*)baseAddress + ODS_PAGE_SIZE;  // Trap goes before the data
    }

    result.m_data = m_platformAllocator->CommitBytes((char*)baseAddress, (pageCount - 1) << ODS_PAGE_SHIFT);  // Don't allocate the trap page

    AZ_Assert(baseAddress == result.m_data, "CommitBytes returned a different base address (%p) than the calculated page address (%p)", result.m_data, baseAddress);

    return result;
}

void AZ::Internal::SmallPageAllocator::Deallocate(void* p, uint32_t smallAllocationIndex, uint32_t pageIndex)
{
    SmallAllocationGroup* allocationGroup = &m_smallAllocationGroups[smallAllocationIndex];
    AZ_UNUSED(p);
    AZ_Assert((uint32_t)(((size_t)p - sizeof(AllocationRecord) - (size_t)allocationGroup->m_data) >> ODS_PAGE_SHIFT) == pageIndex, "WTF");
    uint32_t pageCount = allocationGroup->m_freeListNodes[pageIndex].GetAllocatedPageCount();
    uint32_t releaseMask = (1 << pageCount) - 1;

    AZ_Assert(pageCount, "Invalid page count");

    // Mark these pages as freed
    allocationGroup->m_allocationMap &= ~(releaseMask << pageIndex);
    allocationGroup->m_freeListNodes[pageIndex].SetAllocatedPageCount(0);

#if 0
    // Used for debugging the allocator only, reversing action contained in Allocate step
    for (auto i = pageIndex; i < pageIndex + pageCount; i++)
    {
        allocationGroup->m_freeListNodes[i].SetAllocatedPageCount(0);
    }
#endif

    // Update the slots and freelists for every permissible pageCount request
    UpdateAvailability(allocationGroup);

    // Decommit the memory
    void* baseAddress = (char*)allocationGroup->m_data + (pageIndex << ODS_PAGE_SHIFT);

    if (m_underrunMode)
    {
        m_platformAllocator->DecommitBytes((char*)baseAddress + ODS_PAGE_SIZE, (pageCount - 1) << ODS_PAGE_SHIFT);  // Trap page wasn't allocated
    }
    else
    {
        m_platformAllocator->DecommitBytes(baseAddress, (pageCount - 1) << ODS_PAGE_SHIFT);  // Trap page wasn't allocated
    }
}

void AZ::Internal::SmallPageAllocator::UpdateAvailability(SmallAllocationGroup* allocationGroup)
{
    uint32_t startIndex = 0;
    uint32_t endIndex = startIndex;
    uint32_t maxAvailability = 0;

    // Iterate over the allocation map, finding consecutive runs of empty pages
    while (startIndex < ODS_PAGES_PER_ALLOCATION && endIndex < ODS_PAGES_PER_ALLOCATION)
    {
        while (endIndex < ODS_PAGES_PER_ALLOCATION)
        {
            if (allocationGroup->m_allocationMap & (1 << endIndex))
            {
                // This page is allocated, so end the run of consecutive available spaces and begin at the next spot
                startIndex = endIndex++;
                break;
            }

            // This page is unallocated, so continue the run of consecutive available pages
            uint32_t segmentLength = endIndex - startIndex + 1;

            if (segmentLength > maxAvailability)
            {
                // We are hitting this segment length for the first time. Record availability for this many consecutive spaces.
                auto& nd = allocationGroup->m_freeListNodes[segmentLength - 1];

                nd.SetAvailablePage(startIndex);  // Record which slot we can find the available pages in
                maxAvailability = segmentLength;

                if (!nd.IsLinked())
                {
                    nd.Link(m_smallAllocationGroups[0].m_freeListNodes[segmentLength - 1]);  // Add this group to the free list for this segment length
                }
            }

            endIndex++;
        }

        startIndex++;
    }

    // Any consecutive runs longer than maxAvailability are no longer available. Remove ourseleves from any freelists that we may still be connected to.
    while (maxAvailability < ODS_PAGES_PER_ALLOCATION)
    {
        auto& nd = allocationGroup->m_freeListNodes[maxAvailability++];

        if (nd.IsLinked())
        {
            nd.Unlink();
        }
        else
        {
            break;
        }
    }
}


//---------------------------------------------------------------------
// OverrunDetectionSchemaImpl methods
//---------------------------------------------------------------------

AZ::OverrunDetectionSchemaImpl::OverrunDetectionSchemaImpl(const OverrunDetectionSchema::Descriptor& desc) :
    m_underrunMode(desc.m_underrunDetection),
    m_used(0)
{
    m_platformAllocator.reset(new Internal::PlatformOverrunDetectionSchema);

    [[maybe_unused]] auto info = m_platformAllocator->GetSystemInformation();
    AZ_Assert(info.m_pageSize == Internal::ODS_PAGE_SIZE, "System page size %d does not equal expected page size %d", info.m_pageSize, Internal::ODS_PAGE_SIZE);
    AZ_Assert(info.m_minimumAllocationSize == Internal::ODS_ALLOCATION_SIZE, "System minimum allocation size %d does not equal expected size %d", info.m_minimumAllocationSize, Internal::ODS_ALLOCATION_SIZE);

    m_smallPageAllocator.Init(m_platformAllocator.get(), m_underrunMode);
}

AZ::OverrunDetectionSchemaImpl::~OverrunDetectionSchemaImpl()
{
}

AZ::OverrunDetectionSchema::pointer_type AZ::OverrunDetectionSchemaImpl::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    (void)flags;
    (void)name;
    (void)fileName;
    (void)lineNum;
    (void)suppressStackRecord;

    using namespace Internal;

    if (alignment == 0)
    {
        alignment = ODS_DEFAULT_ALIGNMENT;
    }

    pointer_type result = nullptr;
    size_t requiredAlignment = AZStd::max(alignment, AZStd::alignment_of<AllocationRecord>::value);
    size_t requiredSpace = SizeAlignUp(SizeAlignUp(byteSize, requiredAlignment) + sizeof(AllocationRecord), ODS_PAGE_SIZE);  // Add space for the allocation record and size up to page boundaries
    size_t requiredPageSpace = requiredSpace + ODS_PAGE_SIZE;  // Add the trap page

    if (requiredPageSpace < ODS_ALLOCATION_SIZE && alignment < ODS_PAGE_SIZE)
    {
        // We can do a small allocation
        lock_type lock(m_mutex);
        uint32_t numberOfConsecutivePages = uint32_t(requiredPageSpace >> ODS_PAGE_SHIFT);
        auto allocationResult = m_smallPageAllocator.AllocateConsecutivePages(numberOfConsecutivePages);
        result = m_underrunMode ? allocationResult.m_data : RepositionAllocationToPageBoundary(allocationResult.m_data, byteSize, alignment, requiredSpace);

        // Create allocation record
        AllocationRecord* record = CreateAllocationRecord(result, byteSize);
        new (record) AllocationRecord(allocationResult.m_index, allocationResult.m_pageIndex, byteSize);
    }
    else
    {
        // Must do a large allocation
        if (!m_underrunMode && alignment >= ODS_PAGE_SIZE)
        {
            requiredPageSpace += alignment;  // We need extra space to prepend the allocation record if we're aligning to page boundaries
        }

        void* reservedBytes = m_platformAllocator->ReserveBytes(requiredPageSpace);
        void* committedBytes;
        
        if (m_underrunMode)
        {
            committedBytes = m_platformAllocator->CommitBytes((char*)reservedBytes + ODS_PAGE_SIZE, requiredPageSpace - ODS_PAGE_SIZE);  // Do not commit the trap page
            result = committedBytes;
        }
        else
        {
            committedBytes = m_platformAllocator->CommitBytes(reservedBytes, requiredPageSpace - ODS_PAGE_SIZE);  // Do not commit the trap page
            result = RepositionAllocationToPageBoundary(committedBytes, byteSize, alignment, requiredSpace);
        }

        if (!m_underrunMode && alignment >= ODS_PAGE_SIZE)
        {
            result = (char*)result + alignment;  // If we're aligning to page boundaries then make space at the beginning for the allocation record
        }

        // Create allocation record
        AllocationRecord* record = CreateAllocationRecord(result, byteSize);
        new (record) AllocationRecord(reservedBytes, byteSize);
    }

    m_used += byteSize;

    return result;
}

void AZ::OverrunDetectionSchemaImpl::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    (void)byteSize;
    (void)alignment;

    using namespace Internal;

    if (!ptr)
    {
        return;
    }

    AllocationRecord* record = GetAllocationRecord(ptr);
    AZ_Assert(!byteSize || record->GetSize() == byteSize, "Deallocating a pointer with different allocation size on record than submitted");

    m_used -= record->GetSize();

    if (record->IsSmallAllocation())
    {
        lock_type lock(m_mutex);
        m_smallPageAllocator.Deallocate(ptr, record->GetSmallAllocationIndex(), record->GetPageIndex());
    }
    else
    {
        void* p = record->GetPointer();
        m_platformAllocator->ReleaseReservedBytes(p);
    }
}

AZ::OverrunDetectionSchema::pointer_type AZ::OverrunDetectionSchemaImpl::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    using namespace Internal;

    // CryEngine uses realloc(nullptr) and realloc(ptr, 0) as aliases for malloc and free, so we must handle these
    void* newPointer = newSize ? Allocate(newSize, newAlignment, 0, nullptr, nullptr, 0, 0) : nullptr;

    if (ptr)
    {
        AllocationRecord* allocationRecord = GetAllocationRecord(ptr);

        if (newPointer)
        {
            memcpy(newPointer, ptr, AZStd::min(allocationRecord->GetSize(), newSize));
        }

        DeAllocate(ptr, 0, 0);
    }

    return newPointer;
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchemaImpl::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;

    return 0;  // Unsupported
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchemaImpl::AllocationSize(pointer_type ptr)
{
    using namespace Internal;

    AllocationRecord* allocationRecord = GetAllocationRecord(ptr);

    return allocationRecord->GetSize();
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchemaImpl::NumAllocatedBytes() const
{ 
    return m_used; 
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchemaImpl::Capacity() const
{
    return 0;
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchemaImpl::GetMaxAllocationSize() const
{
    return 0;
}

auto AZ::OverrunDetectionSchemaImpl::GetMaxContiguousAllocationSize() const -> size_type
{
    return 0;
}

AZ::IAllocatorAllocate* AZ::OverrunDetectionSchemaImpl::GetSubAllocator()
{
    return nullptr;
}

void AZ::OverrunDetectionSchemaImpl::GarbageCollect()
{
}

void* AZ::OverrunDetectionSchemaImpl::RepositionAllocationToPageBoundary(void* p, size_t requestedByteSize, size_t requestedAlignment, size_t allocatedSpace) const
{
    using namespace Internal;

    if (m_underrunMode)
    {
        AZ_Assert(false, "WTF");
    }
    else
    {
        // Want the end of the requested block to line up with the end of the allocated space so that overwrites will fall into the trap
        p = PointerAlignDown((char*)p + allocatedSpace - requestedByteSize, requestedAlignment);
    }

    return p;
}

AZ::Internal::AllocationRecord* AZ::OverrunDetectionSchemaImpl::GetAllocationRecord(void* p) const
{
    using namespace Internal;

    AllocationRecord* result;

    if (m_underrunMode)
    {
        // Need to search upwards for the allocation record
        result = PointerAlignUp((AllocationRecord*)p, ODS_PAGE_SIZE) - 1;

        while (!result->IsValidAllocationRecord())
        {
            result = (AllocationRecord*)((char *)result + ODS_PAGE_SIZE);
        }
    }
    else
    {
        // Allocation record comes in the space just before the allocated memory
        result = (AllocationRecord*)((char*)PointerAlignDown(p, AZStd::alignment_of<AllocationRecord>::value) - sizeof(AllocationRecord));
    }

    return result;
}

AZ::Internal::AllocationRecord* AZ::OverrunDetectionSchemaImpl::CreateAllocationRecord(void* p, size_t size) const
{
    using namespace Internal;

    AllocationRecord* result;

    if (m_underrunMode)
    {
        // Allocation record comes at the page boundary after the allocation
        result = PointerAlignUp((AllocationRecord*)((char*)p + size + sizeof(AllocationRecord)), ODS_PAGE_SIZE) - 1;
    }
    else
    {
        // Allocation record comes in the space just before the allocated memory
        result = (AllocationRecord*)((char*)PointerAlignDown(p, AZStd::alignment_of<AllocationRecord>::value) - sizeof(AllocationRecord));
    }

    return result;
}


//---------------------------------------------------------------------
// OverrunDetectionSchema methods
//---------------------------------------------------------------------

AZ::OverrunDetectionSchema::OverrunDetectionSchema(const Descriptor& desc) : m_impl(new OverrunDetectionSchemaImpl(desc))
{
}

AZ::OverrunDetectionSchema::~OverrunDetectionSchema()
{
    delete m_impl;
}

AZ::OverrunDetectionSchema::pointer_type AZ::OverrunDetectionSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    return m_impl->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
}

void AZ::OverrunDetectionSchema::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    m_impl->DeAllocate(ptr, byteSize, alignment);
}

AZ::OverrunDetectionSchema::pointer_type AZ::OverrunDetectionSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    return m_impl->ReAllocate(ptr, newSize, newAlignment);
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchema::Resize(pointer_type ptr, size_type newSize)
{
    return m_impl->Resize(ptr, newSize);
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchema::AllocationSize(pointer_type ptr)
{
    return m_impl->AllocationSize(ptr);
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchema::NumAllocatedBytes() const
{
    return m_impl->NumAllocatedBytes();
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchema::Capacity() const
{
    return m_impl->Capacity();
}

AZ::OverrunDetectionSchema::size_type AZ::OverrunDetectionSchema::GetMaxAllocationSize() const
{
    return m_impl->GetMaxAllocationSize();
}

auto AZ::OverrunDetectionSchema::GetMaxContiguousAllocationSize() const -> size_type
{
    return m_impl->GetMaxContiguousAllocationSize();
}

AZ::IAllocatorAllocate* AZ::OverrunDetectionSchema::GetSubAllocator()
{
    return m_impl->GetSubAllocator();
}

void AZ::OverrunDetectionSchema::GarbageCollect()
{
    m_impl->GarbageCollect();
}
