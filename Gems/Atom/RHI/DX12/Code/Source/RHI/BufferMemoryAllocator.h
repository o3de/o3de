/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FreeListAllocator.h>
#include <RHI/BufferMemoryView.h>
#include <RHI/MemorySubAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class BufferMemoryAllocator
        {
        public:
            BufferMemoryAllocator() = default;
            BufferMemoryAllocator(const BufferMemoryAllocator&) = delete;

            using Descriptor = MemoryPageAllocator::Descriptor;

            void Init(const Descriptor& descriptor);

            void Shutdown();

            void GarbageCollect();

            BufferMemoryView Allocate(size_t sizeInBytes, size_t overrideAlignment = 0);

            void DeAllocate(const BufferMemoryView& memory);

            float ComputeFragmentation() const;

        private:
            BufferMemoryView AllocateUnique(const RHI::BufferDescriptor& bufferDescriptor);

            void DeAllocateUnique(const BufferMemoryView& memoryView);

            Descriptor m_descriptor;
            MemoryPageAllocator m_pageAllocator;
            bool m_usePageAllocator = true;

            AZStd::mutex m_subAllocatorMutex;
            MemoryFreeListSubAllocator m_subAllocator;
            size_t m_subAllocationAlignment = Alignment::Buffer;
        };
    }
}
