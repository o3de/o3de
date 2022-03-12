/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    class AllocatorManager;

    namespace Internal
    {
        /**
        * A shim schema that solves the problem of overriding lazily-created allocators especially, that perform allocations before the override happens.
        *
        * Any allocator that *might* be overridden at some point must have this shim installed as its allocation source. This is done automatically by
        * the AllocationManager; you generally do not have to interact with this shim directly at all.
        *
        * The shim will keep track of any allocations that occur, and if the allocator gets overridden it will ensure that prior allocations are
        * deallocated with the old schema rather than the new one.
        *
        * There is some performance cost to this intrusion, however, in most cases it's only temporary:
        *  * Once the application calls FinalizeConfiguration(), any non-overridden allocators will have their shims destroyed.
        *  * Any overridden allocators that do not have prior allocations will have their shims destroyed.
        *  * Any overridden allocator will automatically destroy its shim once the last of the prior allocations has been deallocated.
        *
        * Note that an allocator that gets overridden but has prior allocations that it never intends to deallocate (such as file-level statics that never
        * get changed) will keep its shim indefinitely. This is an unfortunate cost but only affects those allocators if they are being overridden.
        */
        class AllocatorOverrideShim
            : public IAllocatorAllocate
        {
            friend AllocatorManager;

        public:
            //---------------------------------------------------------------------
            // IAllocator implementation
            //---------------------------------------------------------------------
            pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = nullptr, const char* fileName = nullptr, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
            void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
            size_type Resize(pointer_type ptr, size_type newSize) override;
            pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
            size_type AllocationSize(pointer_type ptr) override;
            void GarbageCollect() override;
            size_type NumAllocatedBytes() const override;
            size_type Capacity() const override;
            size_type GetMaxAllocationSize() const override;
            size_type GetMaxContiguousAllocationSize() const override;
            IAllocatorAllocate* GetSubAllocator() override;

        private:
            /// Creates a shim using a custom memory source
            static AllocatorOverrideShim* Create(IAllocator* owningAllocator, IAllocatorAllocate* shimAllocationSource);
            static void Destroy(AllocatorOverrideShim* source);

            AllocatorOverrideShim(IAllocator* owningAllocator, IAllocatorAllocate* shimAllocationSource);

            /// Overrides the shim's memory source with a different memory source.
            void SetOverride(IAllocatorAllocate* source);

            /// Returns the override source.
            IAllocatorAllocate* GetOverride() const;

            /// Returns true if the shim has an override source set on it.
            bool IsOverridden() const;

            /// Returns true if there are orphaned allocations from before the shim had its override set.
            bool HasOrphanedAllocations() const;

            /// Called by the AllocatorManager to signify that the configuration has been finalized by the application.
            void SetFinalizedConfiguration();

        private:
            class StdAllocationSrc : public AZStdIAllocator
            {
            public:
                StdAllocationSrc(IAllocatorAllocate* schema = nullptr) : AZStdIAllocator(schema)
                {
                }
            };

            typedef AZStd::mutex mutex_type;
            typedef AZStd::lock_guard<mutex_type> lock_type;
            typedef AZStd::unordered_set<void*, AZStd::hash<void*>, AZStd::equal_to<void*>, StdAllocationSrc> AllocationSet;

            IAllocator* m_owningAllocator;
            IAllocatorAllocate* m_source;
            IAllocatorAllocate* m_overridingSource;
            IAllocatorAllocate* m_shimAllocationSource;
            AllocationSet m_records;
            mutex_type m_mutex;
            bool m_finalizedConfiguration = false;
        };
    }
}
