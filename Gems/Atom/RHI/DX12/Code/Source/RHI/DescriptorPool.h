/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Descriptor.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI/Allocator.h>

namespace AZ
{
    namespace DX12
    {
        //! This class defines a Descriptor pool which manages all the descriptors used for binding resources
        class DescriptorPool
        {
        public:
            DescriptorPool() = default;
            virtual ~DescriptorPool() = default;

            //! Initialize the native heap as well as init the allocators tracking the memory for descriptor handles
            virtual void Init(
                ID3D12DeviceX* device,
                D3D12_DESCRIPTOR_HEAP_TYPE type,
                D3D12_DESCRIPTOR_HEAP_FLAGS flags,
                uint32_t descriptorCountForHeap,
                uint32_t descriptorCountForAllocator);

            //! Initialize a descriptor pool mapping a range of descriptors from a parent heap. Descriptors are allocated
            //! using the RHI::PoolAllocator
            void InitPooledRange(DescriptorPool& parent, uint32_t offset, uint32_t count);

            ID3D12DescriptorHeap* GetPlatformHeap() const;

            //! Allocate a Descriptor handles
            DescriptorHandle AllocateHandle(uint32_t count = 1);
            //! Release a descriptor handle
            void ReleaseHandle(DescriptorHandle table);
            //! Allocate a range contiguous handles (i.e Descriptor table)
            virtual DescriptorTable AllocateTable(uint32_t count = 1);
            //! Release a range contiguous handles (i.e Descriptor table)
            virtual void ReleaseTable(DescriptorTable table);
            //! Garbage collection for freed handles or tables
            virtual void GarbageCollect();
            //Get native pointers from the heap
            virtual D3D12_CPU_DESCRIPTOR_HANDLE GetCpuPlatformHandleForTable(DescriptorTable handle) const;
            virtual D3D12_GPU_DESCRIPTOR_HANDLE GetGpuPlatformHandleForTable(DescriptorTable handle) const;
            D3D12_CPU_DESCRIPTOR_HANDLE GetCpuPlatformHandle(DescriptorHandle handle) const;
            D3D12_GPU_DESCRIPTOR_HANDLE GetGpuPlatformHandle(DescriptorHandle handle) const;

        protected:
            D3D12_DESCRIPTOR_HEAP_DESC m_desc;
            AZStd::mutex m_mutex;
            D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart = {};
            D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart = {};
            uint32_t m_stride = 0;
        private:

            // Native heap 
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;

            // Allocator used to manage the whole native heap. In the case of DescriptorPoolShaderVisibleCbvSrvUav this allocator
            // is used to manage the part of the heap that only manages static handles. 
            AZStd::unique_ptr<RHI::Allocator> m_allocator;
        };
    }
}
