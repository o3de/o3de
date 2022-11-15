/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/RTTI/RTTI.h>

#define AZ_ALLOCATOR_TRAITS(VALUETYPE) \
    using value_type = VALUETYPE; \
    using pointer = VALUETYPE*; \
    using size_type = AZStd::size_t; \
    using difference_type = AZStd::ptrdiff_t; \
    using align_type = AZStd::size_t; \
    using propagate_on_container_copy_assignment = AZStd::true_type; \
    using propagate_on_container_move_assignment = AZStd::true_type;

#define AZ_ALLOCATOR_DEFAULT_TRAITS AZ_ALLOCATOR_TRAITS(void)

namespace AZ
{
    namespace Debug
    {
        class AllocationRecords;
    }

    template<class Allocator>
    class AllocatorWrapper;

    class AllocatorManager;

    /**
    * Standardized debug configuration for an allocator.
    */
    struct AllocatorDebugConfig
    {
        /// Sets the number of entries to omit from the top of the callstack when recording stack traces.
        AllocatorDebugConfig& StackRecordLevels(int levels) { m_stackRecordLevels = levels; return *this; }

        /// Set to true if this allocator should not have its records recorded and analyzed.
        AllocatorDebugConfig& ExcludeFromDebugging(bool exclude = true) { m_excludeFromDebugging = exclude; return *this; }

        /// Set to true if this allocator expands allocations with guard sections to detect overruns.
        AllocatorDebugConfig& UsesMemoryGuards(bool use = true) { m_usesMemoryGuards = use; return *this; }

        /// Set to true if this allocator writes into unallocated memory so it can be detected at runtime.
        AllocatorDebugConfig& MarksUnallocatedMemory(bool marks = true) { m_marksUnallocatedMemory = marks; return *this; }

        unsigned int m_stackRecordLevels = 0;
        bool m_excludeFromDebugging = false;
        bool m_usesMemoryGuards = false;
        bool m_marksUnallocatedMemory = false;
    };

    /**
     * Allocator interface base class
     */
    class IAllocator
    {
    public:
        AZ_RTTI(IAllocator, "{0A3C59AE-169C-45F6-9423-3B8C89245E2E}");

        AZ_ALLOCATOR_DEFAULT_TRAITS

        IAllocator() = default;
        IAllocator(const IAllocator&) = delete;
        IAllocator(IAllocator&&) = delete;
        IAllocator& operator=(const IAllocator&) = delete;
        IAllocator& operator=(IAllocator&&) = delete;
        virtual ~IAllocator() = default;

        virtual pointer allocate(size_type byteSize, align_type alignment = 1) = 0;
        virtual void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) = 0;

        // Reallocates a bigger block. Allocators will do best effort to maintain the same pointer,
        // but they can return a new pointer if is not possible or the allocator doesnt support it.
        // Alignment cannot change, it will be the same before/after, but needs to be specified if different
        // than 1.
        // Memory pointed by ptr is copied to the returned pointer if the returned pointer is different
        // than the passed ptr
        // Memory pointed by ptr is freed if the returned pointer is different than the passed ptr
        virtual pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = 1) = 0;

        virtual size_type max_size() const
        {
            // default, max the OS can allocate
            return AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE;
        }

        virtual size_type get_allocated_size(pointer ptr, align_type alignment = 1) const = 0;

        // Frees (if possible) unused memory from the allocator. Unused memory is memory that was allocated
        // from the OS point of view but was not assigned to be used by a pointer.
        // NOTE: this function should be thread safe, allocations can happen at the same time this function
        // is called. Allocators can optionally implement it and should do best effort (i.e. they dont need
        // to completely free all unused memory).
        virtual void GarbageCollect()
        {
        }

        const char* GetName() const
        {
            return RTTI_GetTypeName();
        }

        // Kept for backwards-compatibility reasons
        /////////////////////////////////////////////
        //AZ_DEPRECATED_MESSAGE("Use allocate instead, which matches the STD interface")
        pointer Allocate(
            size_type byteSize,
            size_type alignment = 1,
            [[maybe_unused]] int flags = 0,
            [[maybe_unused]] const char* name = nullptr,
            [[maybe_unused]] const char* fileName = nullptr,
            [[maybe_unused]] int lineNum = 0,
            [[maybe_unused]] unsigned int suppressStackRecord = 0)
        {
            return allocate(byteSize, static_cast<align_type>(alignment));
        }

        //AZ_DEPRECATED_MESSAGE("Use deallocate instead, which matches the STD interface")
        void DeAllocate(pointer ptr, size_type byteSize = 0, [[maybe_unused]] size_type alignment = 0)
        {
            deallocate(ptr, byteSize);
        }

        /// Resize an allocated memory block. Returns the new adjusted size (as close as possible or equal to the requested one) or 0 (if
        /// you don't support resize at all).
        AZ_DEPRECATED_MESSAGE("Resize no longer supported, use reallocate instead, note that the pointer address could change, "
            "Allocators should do best effort to keep the ptr at the same address")
        size_type Resize([[maybe_unused]] pointer ptr, [[maybe_unused]] size_type newSize)
        {
            return 0;
        }

        /// Realloc an allocate memory memory block. Similar to Resize except it will move the memory block if needed. Return NULL if
        /// realloc is not supported or run out of memory.
        //AZ_DEPRECATED_MESSAGE("Use reallocate instead, which matches the STD interface")
        pointer ReAllocate(pointer ptr, size_type newSize, size_type newAlignment)
        {
            return reallocate(ptr, newSize, newAlignment);
        }

        ///
        /// Returns allocation size for given address. 0 if the address doesn't belong to the allocator.
        //AZ_DEPRECATED_MESSAGE("Use get_allocated_size instead, which matches the STD interface")
        size_type AllocationSize([[maybe_unused]] pointer ptr)
        {
            return get_allocated_size(ptr);
        }

        virtual size_type NumAllocatedBytes() const
        {
            return 0;
        }

        /// Returns the capacity of the Allocator in bytes. If the return value is 0 the Capacity is undefined (usually depends on another
        /// allocator)
        //AZ_DEPRECATED_MESSAGE("Use max_size instead, which matches the STD interface")
        size_type Capacity() const
        {
            return max_size();
        }

        /// Returns max allocation size if possible. If not returned value is 0
        AZ_DEPRECATED_MESSAGE("Unused and not really useful")
        size_type GetMaxAllocationSize() const
        {
            return 0;
        }

        /// Returns the maximum contiguous allocation size of a single allocation
        //AZ_DEPRECATED_MESSAGE("Unused and not really useful")
        size_type GetMaxContiguousAllocationSize() const
        {
            return 0;
        }

        /**
         * Returns memory allocated by the allocator and available to the user for allocations.
         * IMPORTANT: this is not the overhead memory this is just the memory that is allocated, but not used. Example: the pool allocators
         * allocate in chunks. So we might be using one elements in that chunk and the rest is free/unallocated. This is the memory
         * that will be reported.
         */
        //AZ_DEPRECATED_MESSAGE("Use GetFragmentedSize instead, this method was problematic because not all overhead memory is available for allocations. "
        //    "GetFragmentedSize will give the difference between what was allocated by the allocator and what was requested, meaning, it will give the "
        //    "overhead produced by the allocator in trying to optimize the memory usage.")
        size_type GetUnAllocatedMemory([[maybe_unused]] bool isPrint = false) const
        {
            return 0;
        }

        /// Returns the debug configuration for this allocator.
        virtual AllocatorDebugConfig GetDebugConfig() { return {}; }

        /// Returns a pointer to the allocation records. They might be available or not depending on the build type. \ref Debug::AllocationRecords
        virtual Debug::AllocationRecords* GetRecords() { return nullptr; }

        /// Sets the allocation records.
        virtual void SetRecords([[maybe_unused]] Debug::AllocationRecords* records) {}

        /// Returns true if this allocator is ready to use.
        virtual bool IsReady() const { return true; }

    private:
        /// Sets whether profiling calls should be made.
        /// This is primarily a performance compromise, as the profiling calls go out on an EBus that may not exist if not activated, and will
        /// result in an expensive hash lookup if that is the case.
        virtual void SetProfilingActive([[maybe_unused]] bool active) {}

        /// Returns true if profiling calls will be made.
        virtual bool IsProfilingActive() const { return false; }

        /// All conforming allocators must call PostCreate() after their custom Create() method in order to be properly registered.
        virtual void PostCreate() {}

        /// All conforming allocators must call PreDestroy() before their custom Destroy() method in order to be properly deregistered.
        virtual void PreDestroy() {}

        /// All allocators must provide their deinitialization routine here.
        virtual void Destroy() {}

    protected:
        friend class AllocatorManager;

        template<class Allocator>
        friend class AllocatorWrapper;
    };

} // namespace AZ
