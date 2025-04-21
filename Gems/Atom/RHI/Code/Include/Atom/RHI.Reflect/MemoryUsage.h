/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZ::RHI
{
    struct HeapMemoryTransfer
    {
        HeapMemoryTransfer() = default;
        HeapMemoryTransfer(const HeapMemoryTransfer&);
        HeapMemoryTransfer& operator=(const HeapMemoryTransfer&);

        /// Memory transferred in bytes, reset on a regular cycle (e.g. per frame).
        AZStd::atomic_size_t m_bytesPerFrame = {0};

        /// Memory transferred in bytes, accumulated across heap / pool lifetime.
        size_t m_accumulatedInBytes = 0;
    };

    //! Tracks memory usage for a specific heap in the system. The data is expected to adhere to the following constraints:
    //!  1) Reserved <= Budget (unless the budget is 0).
    //!  2) Resident <= Reserved.
    struct HeapMemoryUsage
    {
        HeapMemoryUsage() = default;
        HeapMemoryUsage(const HeapMemoryUsage&);
        HeapMemoryUsage& operator=(const HeapMemoryUsage&);

        //! This helper function checks whether a new allocation is within the budget
        bool CanAllocate(size_t sizeInBytes) const
        {
            if (m_budgetInBytes && (m_usedResidentInBytes + sizeInBytes) > m_budgetInBytes)
            {
                return false;
            }
            return true;
        }

        //! Helper function to validate sizes
        void Validate()
        {
            if (Validation::IsEnabled())
            {
                AZ_Assert(
                    m_budgetInBytes >= m_usedResidentInBytes || m_budgetInBytes == 0,
                    "Used resident memory is larger than memory budget. Memory budget %zu resident %zu", m_budgetInBytes, m_usedResidentInBytes.load());
            }
        }

        // The budget for the heap in bytes. A non-zero budget means the pool will reject reservation requests
        // once the budget is exceeded. A zero budget effectively disables this check. On certain platforms,
        // it may be unnecessary to budget certain heaps. Other platforms may require a non-zero budget for certain
        // heaps.
        size_t m_budgetInBytes = 0;

        // For heaps that suballocate in a manner that results in fragmentation, this quantity is computed as
        // 1 - (largest free block byte size) / (total free memory). When the free memory equals the largest block size, this
        // measure is 0. As the largest free block size decreases relative to the amount of free memory, this approaches 1.
        // Fragmentation may be expensive to compute on demand, so it is currently computed as a side-effect of the ReportMemoryUsage
        // routines. As this is the only quantity that changes during the memory statistics gathering process, we opt to mark it mutable
        // here instead of marking the entire routine mutable.
        mutable float m_fragmentation{ 0 };

        // Total number of bytes allocated on the physical memory. This may not exceed the budget if it's non zero.
        AZStd::atomic_size_t m_totalResidentInBytes{ 0 };

        // Number of bytes are used for resources or objects. This usually tracks the sub-allocations out of the total resident.
        // It may not exceed the total resident.
        AZStd::atomic_size_t m_usedResidentInBytes{ 0 };

        // Number of bytes used by Unique Allocations.
        AZStd::atomic_size_t m_uniqueAllocationBytes{ 0 };

        // Platform specific allocator statistics in a JSON format.
        AZStd::string m_extraStats;
    };

    //!
    //! Describes memory usage metrics of a resource pool. Resource pools *can* associate with a single
    //! device memory heap (i.e. a single GPU) and the host memory heap. Certain pools on specific platforms
    //! may not require one or the other. In this case, the memory usage / budget will report empty values for
    //! that heap type.
    struct PoolMemoryUsage
    {
        PoolMemoryUsage() = default;
        PoolMemoryUsage(const PoolMemoryUsage&) = default;
        PoolMemoryUsage& operator=(const PoolMemoryUsage&) = default;

        HeapMemoryUsage& GetHeapMemoryUsage(HeapMemoryLevel memoryType)
        {
            return m_memoryUsagePerLevel[static_cast<size_t>(memoryType)];
        }

        const HeapMemoryUsage& GetHeapMemoryUsage(HeapMemoryLevel memoryType) const
        {
            return m_memoryUsagePerLevel[static_cast<size_t>(memoryType)];
        }

        /// The memory heap usages of this pool for each level in the hierarchy.
        AZStd::array<HeapMemoryUsage, HeapMemoryLevelCount> m_memoryUsagePerLevel;

        /// Tracks data pulled (via a DMA controller) from host memory to device memory.
        HeapMemoryTransfer m_transferPull;

        /// Tracks data pushed (via a DMA controller) from device memory to host memory.
        HeapMemoryTransfer m_transferPush;
    };
}
