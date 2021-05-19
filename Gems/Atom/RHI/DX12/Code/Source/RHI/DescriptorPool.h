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
        class DescriptorPool
        {
        public:
            DescriptorPool() = default;

            void Init(
                ID3D12DeviceX* device,
                D3D12_DESCRIPTOR_HEAP_TYPE type,
                D3D12_DESCRIPTOR_HEAP_FLAGS flags,
                uint32_t descriptorCount);

            ID3D12DescriptorHeap* GetPlatformHeap() const;

            DescriptorTable Allocate(uint32_t count = 1);

            void Release(DescriptorTable table);

            void GarbageCollect();

            D3D12_CPU_DESCRIPTOR_HANDLE GetCpuPlatformHandle(DescriptorHandle handle) const;
            D3D12_GPU_DESCRIPTOR_HANDLE GetGpuPlatformHandle(DescriptorHandle handle) const;

        private:
            D3D12_CPU_DESCRIPTOR_HANDLE m_CpuStart = {};
            D3D12_GPU_DESCRIPTOR_HANDLE m_GpuStart = {};
            D3D12_CPU_DESCRIPTOR_HANDLE m_NullDescriptor = {};
            uint32_t m_Stride = 0;
            D3D12_DESCRIPTOR_HEAP_DESC m_Desc;
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
            AZStd::mutex m_mutex;
            AZStd::unique_ptr<RHI::Allocator> m_allocator;
        };
    }
}
