/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Allocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * This class can be used to efficiently allocate small chunks of memory from an externally
         * managed source (DMA / Gpu memory). It will recycle freed blocks by deferring for a configurable
         * number of ticks. If the memory is being used as GPU local memory, its common for the CPU to write
         * to that memory and for the GPU to read it several frames later. The garbage collection latency can
         * be set to match the maximum number of buffered frames, so the user can allocate and free at will
         * without stomping over regions of memory being read.
         */
        class PoolAllocator final
            : public Allocator
        {
        public:
            struct Descriptor : Allocator::Descriptor
            {
                /// The size of each element in the allocator.
                uint32_t m_elementSize = 0;
            };

            AZ_CLASS_ALLOCATOR(PoolAllocator, AZ::SystemAllocator, 0);

            PoolAllocator() = default;

            void Init(const Descriptor& descriptor);

            inline VirtualAddress Allocate()
            {
                return Allocate(m_descriptor.m_elementSize, 1);
            }

            //////////////////////////////////////////////////////////////////////////
            // Allocator
            void Shutdown() override;
            VirtualAddress Allocate(size_t byteCount, size_t byteAlignment) override;
            void DeAllocate(VirtualAddress allocation) override;
            void GarbageCollect() override;
            void GarbageCollectForce() override;
            size_t GetAllocationCount() const override;
            size_t GetAllocatedByteCount() const override;
            const Descriptor& GetDescriptor() const override;
            //////////////////////////////////////////////////////////////////////////

        private:
            Descriptor m_descriptor;
            uint32_t m_elementCount = 0;

            struct Garbage
            {
                Garbage(uint32_t index, uint32_t garbageCollectCycle)
                    : m_index{ index }
                    , m_garbageCollectCycle{ garbageCollectCycle }
                {}

                uint32_t m_index;
                uint32_t m_garbageCollectCycle = 0;
            };

            bool IsGarbageReady(Garbage& garbage) const;

            AZStd::queue<Garbage> m_garbage;
            AZStd::vector<uint32_t> m_freeList;
            uint32_t m_garbageCollectCycle = 0;
            uint32_t m_allocationCountTotal = 0;
        };
    }
}
