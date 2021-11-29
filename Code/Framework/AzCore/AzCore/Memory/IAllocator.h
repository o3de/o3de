/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

namespace AZ
{
    namespace Debug
    {
        class AllocationRecords;
        class MemoryDriller;
    }

    namespace AllocatorStorage
    {
        template<class Allocator>
        class StoragePolicyBase;
    }

    template<class Allocator>
    class AllocatorWrapper;

    class AllocatorManager;

    /**
     * Allocator alloc/free basic interface. It is separate because it can be used
     * for user provided allocators overrides
     */
    class IAllocatorAllocate
    {
    public:
        typedef void*           pointer_type;
        typedef size_t          size_type;
        typedef ptrdiff_t       difference_type;

        virtual ~IAllocatorAllocate() {}

        virtual pointer_type            Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) = 0;
        virtual void                    DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) = 0;
        /// Resize an allocated memory block. Returns the new adjusted size (as close as possible or equal to the requested one) or 0 (if you don't support resize at all).
        virtual size_type               Resize(pointer_type ptr, size_type newSize) = 0;
        /// Realloc an allocate memory memory block. Similar to Resize except it will move the memory block if needed. Return NULL if realloc is not supported or run out of memory.
        virtual pointer_type            ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) = 0;
        ///
        /// Returns allocation size for given address. 0 if the address doesn't belong to the allocator.
        virtual size_type               AllocationSize(pointer_type ptr) = 0;
        /**
         * Call from the system when we want allocators to free unused memory.
         * IMPORTANT: This function is/should be thread safe. We can call it from any context to free memory.
         * Freeing the actual memory is optional (if you can), thread safety is a must.
         */
        virtual void                    GarbageCollect() {}

        virtual size_type               NumAllocatedBytes() const = 0;
        /// Returns the capacity of the Allocator in bytes. If the return value is 0 the Capacity is undefined (usually depends on another allocator)
        virtual size_type               Capacity() const = 0;
        /// Returns max allocation size if possible. If not returned value is 0
        virtual size_type               GetMaxAllocationSize() const { return 0; }
        /// Returns the maximum contiguous allocation size of a single allocation
        virtual size_type               GetMaxContiguousAllocationSize() const { return 0; }
        /**
         * Returns memory allocated by the allocator and available to the user for allocations.
         * IMPORTANT: this is not the overhead memory this is just the memory that is allocated, but not used. Example: the pool allocators
         * allocate in chunks. So we might be using one elements in that chunk and the rest is free/unallocated. This is the memory
         * that will be reported.
         */
        virtual size_type               GetUnAllocatedMemory(bool isPrint = false) const { (void)isPrint; return 0; }
        /// Returns a pointer to a sub-allocator or NULL.
        virtual IAllocatorAllocate*     GetSubAllocator() = 0;
    };

    /**
    * Standardized debug configuration for an allocator.
    */
    struct AllocatorDebugConfig
    {
        /// Sets the number of entries to omit from the top of the callstack when recording stack traces.
        AllocatorDebugConfig& StackRecordLevels(int levels) { m_stackRecordLevels = levels; return *this; }

        /// Set to true if this allocator should not have its records recorded and analyzed by systems like the MemoryDriller.
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
     * Interface class for all allocators.
     */
    class IAllocator
    {
    public:
        IAllocator(IAllocatorAllocate* allocationSource);
        virtual ~IAllocator();

        // @{ Every system allocator is required to provide name this is how
        // it will be registered with the allocator manager.
        virtual const char* GetName() const = 0;
        virtual const char* GetDescription() const = 0;
        // @}

        //---------------------------------------------------------------------
        // Code releating to the allocation source is made concrete within this 
        // interface as a performance optimization.
        //---------------------------------------------------------------------

        /// Returns the current allocation source, which may be used to perform memory allocations.
        AZ_FORCE_INLINE IAllocatorAllocate* GetAllocationSource() const
        {
            return m_allocationSource;
        }

        /// Returns the original allocation source. Generally only used for debugging purposes.
        AZ_FORCE_INLINE IAllocatorAllocate* GetOriginalAllocationSource() const
        {
            return m_originalAllocationSource;
        }

        /// Returns true if the allocation source has changed from its original value.
        AZ_FORCE_INLINE bool IsAllocationSourceChanged() const
        {
            return m_allocationSource != m_originalAllocationSource;
        }

        /// Sets the allocation source, effectively overriding the allocator.
        /// Be very careful doing this, as existing allocations will be deallocated through the new source,
        /// typically leading to unwanted effects (such as crashes).
        void SetAllocationSource(IAllocatorAllocate* allocationSource);

        /// Restores the allocation source to its original value.
        /// Be very careful doing this, as allocations that came from the new source will now be deallocated
        /// through the original source, typically leading to unwanted effects (such as crashes).
        void ResetAllocationSource();

        //---------------------------------------------------------------------

        /// Returns the schema, if the allocator uses one. Returns nullptr if the allocator does not use a schema.
        /// This is mainly used when debugging to determine if allocators alias each other under the hood.
        virtual IAllocatorAllocate* GetSchema() = 0;

        /// Returns the debug configuration for this allocator.
        virtual AllocatorDebugConfig GetDebugConfig() = 0;

        /// Returns a pointer to the allocation records. They might be available or not depending on the build type. \ref Debug::AllocationRecords
        virtual Debug::AllocationRecords* GetRecords() = 0;

        /// Sets the allocation records.
        virtual void SetRecords(Debug::AllocationRecords* records) = 0;

        /// Returns true if this allocator is ready to use.
        virtual bool IsReady() const = 0;

        /// Returns true if this allocator can be overridden with a different source.
        /// Almost all allocators should return true. There are very few minor exceptions, such as the OS Allocator, that are required for direct
        /// interfacing with the kernel and must never be overridden under any circumstances.
        virtual bool CanBeOverridden() const = 0;

        /// Returns true if the allocator was lazily created. Exposed primarily for testing systems that need to verify the state of allocators.
        virtual bool IsLazilyCreated() const = 0;

    private:
        /// Determines whether the allocator was lazily created, possibly at static initialization.
        /// This is primarily meant to support older allocation systems, such as those used by CryEngine.
        /// Newer systems should create and destroy their allocators deliberately at program start-up and shut-down.
        virtual void SetLazilyCreated(bool lazy) = 0;

        /// Sets whether profiling calls should be made.
        /// This is primarily a performance compromise, as the profiling calls go out on an EBus that may not exist if not activated, and will
        /// result in an expensive hash lookup if that is the case.
        virtual void SetProfilingActive(bool active) = 0;

        /// Returns true if profiling calls will be made.
        virtual bool IsProfilingActive() const = 0;

        /// All conforming allocators must call PostCreate() after their custom Create() method in order to be properly registered.
        virtual void PostCreate() = 0;

        /// All conforming allocators must call PreDestroy() before their custom Destroy() method in order to be properly deregistered.
        virtual void PreDestroy() = 0;

        /// All allocators must provide their deinitialization routine here.
        virtual void Destroy() = 0;

    protected:
        // The allocation source is made a direct member of the interface as a performance optimization.
        IAllocatorAllocate * m_allocationSource;
        IAllocatorAllocate* m_originalAllocationSource;

        template<class Allocator>
        friend class AllocatorStorage::StoragePolicyBase;

        friend class AllocatorManager;

        template<class Allocator>
        friend class AllocatorWrapper;

        friend class Debug::MemoryDriller;
    };
}

