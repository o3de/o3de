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
        class BufferD3D12MemoryAllocator
        {
        public:
            BufferD3D12MemoryAllocator() = default;
            BufferD3D12MemoryAllocator(const BufferD3D12MemoryAllocator&) = delete;

            using Descriptor = MemoryPageAllocator::Descriptor;

            void Init(const Descriptor& descriptor);

            void Shutdown();

            void GarbageCollect();

            BufferMemoryView Allocate(size_t sizeInBytes, size_t overrideAlignment = 0);

            void DeAllocate(const BufferMemoryView& memory);

            float ComputeFragmentation() const;

        private:

            Descriptor m_descriptor;
        };
    }
}
