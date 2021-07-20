/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/DescriptorPool.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/FreeListAllocator.h>
#include <Atom/RHI/PoolAllocator.h>

namespace AZ
{
    namespace DX12
    {
        void DescriptorPool::Init(
            ID3D12DeviceX* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags,
            uint32_t descriptorCount)
        {
            m_Desc.Type = type;
            m_Desc.Flags = flags;
            m_Desc.NumDescriptors = descriptorCount;
            m_Desc.NodeMask = 1;

            ID3D12DescriptorHeap* heap;
            DX12::AssertSuccess(device->CreateDescriptorHeap(&m_Desc, IID_GRAPHICS_PPV_ARGS(&heap)));
            heap->SetName(L"DescriptorHeap");

            m_DescriptorHeap.Attach(heap);
            m_Stride = device->GetDescriptorHandleIncrementSize(m_Desc.Type);

            m_CpuStart = heap->GetCPUDescriptorHandleForHeapStart();
            m_GpuStart = {};

            if (RHI::CheckBitsAny(flags, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
            {
                m_GpuStart = heap->GetGPUDescriptorHandleForHeapStart();
            }

            const bool isGpuVisible = RHI::CheckBitsAll(flags, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            if (isGpuVisible)
            {
                RHI::FreeListAllocator::Descriptor descriptor;
                descriptor.m_alignmentInBytes = 1;
                descriptor.m_capacityInBytes = descriptorCount;
                descriptor.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;

                RHI::FreeListAllocator* allocator = aznew RHI::FreeListAllocator();
                allocator->Init(descriptor);
                m_allocator.reset(allocator);
            }
            else
            {
                // Non-shader-visible heaps don't require contiguous descriptors. Therefore, we can allocate
                // them using a block allocator.
                RHI::PoolAllocator::Descriptor descriptor;
                descriptor.m_alignmentInBytes = 1;
                descriptor.m_elementSize = 1;
                descriptor.m_capacityInBytes = descriptorCount;
                descriptor.m_garbageCollectLatency = 0;

                RHI::PoolAllocator* allocator = aznew RHI::PoolAllocator();
                allocator->Init(descriptor);
                m_allocator.reset(allocator);
            }
        }

        DescriptorTable DescriptorPool::Allocate(uint32_t count)
        {
            RHI::VirtualAddress address;
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                address = m_allocator->Allocate(count, 1);
            }

            if (address.IsValid())
            {
                DescriptorHandle handle(m_Desc.Type, m_Desc.Flags, static_cast<uint32_t>(address.m_ptr));
                return DescriptorTable(handle, static_cast<uint16_t>(count));
            }
            else
            {
                return DescriptorTable{};
            }
        }

        void DescriptorPool::Release(DescriptorTable table)
        {
            if (table.IsNull())
            {
                return;
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            m_allocator->DeAllocate(RHI::VirtualAddress::CreateFromOffset(table.GetOffset().m_index));
        }

        void DescriptorPool::GarbageCollect()
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            m_allocator->GarbageCollect();
        }

        ID3D12DescriptorHeap* DescriptorPool::GetPlatformHeap() const
        {
            return m_DescriptorHeap.Get();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPool::GetCpuPlatformHandle(DescriptorHandle handle) const
        {
            AZ_Assert(handle.m_index != DescriptorHandle::NullIndex, "Index is invalid");
            return D3D12_CPU_DESCRIPTOR_HANDLE{ m_CpuStart.ptr + handle.m_index * m_Stride };
        }

        D3D12_GPU_DESCRIPTOR_HANDLE DescriptorPool::GetGpuPlatformHandle(DescriptorHandle handle) const
        {
            AZ_Assert(handle.IsShaderVisible(), "Handle is not shader visible");
            AZ_Assert(handle.m_index != DescriptorHandle::NullIndex, "Index is invalid");
            return D3D12_GPU_DESCRIPTOR_HANDLE{ m_GpuStart.ptr + handle.m_index * m_Stride };
        }
    }
}
