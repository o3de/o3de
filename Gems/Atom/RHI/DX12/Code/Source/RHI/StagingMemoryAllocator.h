/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <RHI/MemoryPageAllocator.h>
#include <RHI/MemorySubAllocator.h>
#include <Atom/RHI/ThreadLocalContext.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    namespace RHI
    {
        class MemoryStatisticsBuilder;
    }

    namespace DX12
    {
        class Device;

        class StagingMemoryAllocator
        {
        public:
            StagingMemoryAllocator();
            StagingMemoryAllocator(const StagingMemoryAllocator&) = delete;

            struct Descriptor
            {
                Device* m_device = nullptr;
                uint32_t m_mediumPageSizeInBytes = 0;
                uint32_t m_largePageSizeInBytes = 0;
                uint32_t m_collectLatency = 0;
            };

            void Init(const Descriptor& descriptor);

            void Shutdown();

            void GarbageCollect();

            MemoryView Allocate(size_t sizeInBytes, size_t alignmentInBytes);

            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const;

            MemoryPageAllocator& GetMediumPageAllocator();

        private:
            MemoryView AllocateUnique(size_t sizeInBytes);

            Device* m_device = nullptr;

            // Memory usage is shared betweeen large and medium page allocators.
            RHI::HeapMemoryUsage m_memoryUsage;

            /**
             * Small allocations are done from a thread-local linear allocator that pulls
             * pages from a central page allocator. This allows small allocations to be very
             * low contention.
             */
            MemoryPageAllocator m_mediumPageAllocator;
            RHI::ThreadLocalContext<MemoryLinearSubAllocator> m_mediumBlockAllocators;

            /**
             * Large allocations are done through a separate page pool (with large pages) and
             * uses a lock. We should have few large allocations per frame for things like streaming
             * image or geometry uploads.
             */
            MemoryPageAllocator m_largePageAllocator;
            MemoryLinearSubAllocator m_largeBlockAllocator;
            AZStd::mutex m_largeBlockMutex;
        };
    }
}
