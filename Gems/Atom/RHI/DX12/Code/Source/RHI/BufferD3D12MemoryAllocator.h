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
        //! Utility class to forward buffer allocations to AMD's D3D12MemoryAllocator library.
        class BufferD3D12MemoryAllocator
        {
        public:
            BufferD3D12MemoryAllocator() = default;
            BufferD3D12MemoryAllocator(const BufferD3D12MemoryAllocator&) = delete;

            // Use the same descriptor as BufferPageAllocator to enable exact API match.
            using Descriptor = MemoryPageAllocator::Descriptor;

            //! Do any internal initialization
            void Init(const Descriptor& descriptor);

            //! placeholder to match BufferMemoryAllocator api
            void Shutdown();

            //! placeholder to match BufferMemoryAllocator api
            void GarbageCollect();

            // Allocate space for a buffer
            BufferMemoryView Allocate(size_t sizeInBytes, size_t overrideAlignment = 0);

            // Release space previously allocated for a buffer
            void DeAllocate(const BufferMemoryView& memory);

            //! placeholder to match BufferMemoryAllocator api
            float ComputeFragmentation() const;

        private:

            Descriptor m_descriptor;
        };
    }
}
