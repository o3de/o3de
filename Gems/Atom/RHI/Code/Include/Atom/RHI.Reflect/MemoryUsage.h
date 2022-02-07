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

namespace AZ
{
    namespace RHI
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

            //! This helper reserves memory in a thread-safe fashion. If the result exceeds the budget, the reservation is safely
            //! reverted and false is returned. otherwise, true is returned. Only m_reservedInBytes is affected.
            //!  @param sizeInBytes The amount of bytes to reserve.
            //! @return Whether the reservation was successful.
            bool TryReserveMemory(size_t sizeInBytes)
            {
                const size_t reservationInBytes = (m_reservedInBytes += sizeInBytes);

                // Check if we blew the budget. If so, undo the add and set the valid flag to false.
                if (m_budgetInBytes && reservationInBytes > m_budgetInBytes)
                {
                    m_reservedInBytes -= sizeInBytes;
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
                        m_budgetInBytes >= m_reservedInBytes,
                        "Reserved memory is larger than memory budget. Memory budget %zu Reserved %zu", m_budgetInBytes, m_reservedInBytes.load());
                    AZ_Assert(
                        m_reservedInBytes >= m_residentInBytes,
                        "Resident memory is larger than reserved memory. Reserved Memory %zu Resident memory %zu", m_reservedInBytes.load(),
                        m_residentInBytes.load());
                }
            }

            // The budget for the heap in bytes. A non-zero budget means the pool will reject reservation requests
            // once the budget is exceeded. A zero budget effectively disables this check. On certain platforms,
            // it may be unnecessary to budget certain heaps. Other platforms may require a non-zero budget for certain
            // heaps.
            size_t m_budgetInBytes = 0;

            // Number of bytes reserved on the heap for allocations. This value represents the allocation capacity for
            // the platform. It is validated against the budget and may not exceed it.
            AZStd::atomic_size_t m_reservedInBytes{ 0 };

            // Number of bytes physically allocated on the heap. This may not exceed the reservation. Certain platforms
            // may choose to transfer memory down the heap level hierarchy in response to memory trim events from the driver.
            AZStd::atomic_size_t m_residentInBytes{ 0 };
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
}
