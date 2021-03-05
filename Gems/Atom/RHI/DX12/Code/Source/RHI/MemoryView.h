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

#include <RHI/Memory.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <Atom/RHI/MemoryAllocation.h>

namespace AZ
{
    namespace DX12
    {
        using MemoryAllocation = RHI::MemoryAllocation<Memory>;
        /**
         * Represents a view into GPU memory. It contains a smart pointer to the ID3D12Resource. The
         * contents are interpreted slightly differently for buffer and image resources:
         *
         * Buffers are basically just treated as memory. Offsets, size, and alignment are relative to the base resource. The
         * GPU virtual address is valid and offset to match the view range. The CPU address can be mapped and also assumes the range
         * of the view. If you need more fine grained control, the resource itself is easily accessible.
         *
         * Images are more restricted. In effect, the only 'view' supported is the full mapping of the resource. The offset
         * must be 0. The size and alignment describe the full ID3D12Resource, and the memory cannot be mapped or accessed
         * through the GPU virtual address (since image layouts are opaque).
         */

        enum class MemoryViewType 
        {
            Image = 0,
            Buffer
        };

        class MemoryView
        {
            friend class BufferMemoryAllocator;
            friend class ShaderResourceGroupPool;

        public:
            MemoryView() = default;
            MemoryView(RHI::Ptr<Memory> memory, size_t offset, size_t size, size_t alignment, MemoryViewType viewType);
            MemoryView(const MemoryAllocation& memAllocation, MemoryViewType viewType);

            /// Supports copy and move construction / assignment.
            MemoryView(const MemoryView& rhs) = default;
            MemoryView(MemoryView&& rhs) = default;
            MemoryView& operator=(const MemoryView& rhs) = default;
            MemoryView& operator=(MemoryView&& rhs) = default;

            bool IsValid() const;

            /// Returns the offset relative to the base memory address in bytes.
            size_t GetOffset() const;

            /// Returns the size of the memory view region in bytes.
            size_t GetSize() const;

            /// Returns the alignment of the memory view region in bytes.
            size_t GetAlignment() const;

            /// Returns a pointer to the memory chunk this view is sub-allocated from.
            Memory* GetMemory() const;

            /// A convenience method to map the resource region spanned by the view for CPU access.
            CpuVirtualAddress Map(RHI::HostMemoryAccess hostAccess) const;

            /// A convenience method for unmapping the resource region spanned by the view.
            void Unmap(RHI::HostMemoryAccess hostAccess) const;

            /// Returns the GPU address, offset to match the view.
            GpuVirtualAddress GetGpuAddress() const;

            /// Sets the name of the ID3D12Resource.
            void SetName(const AZStd::string_view& name);

        private:
            void Construct();

            /// The GPU address of the memory view, offset from the base memory location.
            GpuVirtualAddress m_gpuAddress = 0;

            MemoryAllocation m_memoryAllocation;

            MemoryViewType m_viewType;
        };
    }
}
