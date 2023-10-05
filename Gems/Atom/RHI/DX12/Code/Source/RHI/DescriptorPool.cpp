/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
            uint32_t descriptorCountForHeap,
            uint32_t descriptorCountForAllocator)
        {
            m_desc.Type = type;
            m_desc.Flags = flags;
            m_desc.NumDescriptors = descriptorCountForHeap;
            m_desc.NodeMask = 1;

            ID3D12DescriptorHeap* heap;
            DX12::AssertSuccess(device->CreateDescriptorHeap(&m_desc, IID_GRAPHICS_PPV_ARGS(&heap)));
            heap->SetName(L"DescriptorHeap");

            m_descriptorHeap.Attach(heap);
            m_stride = device->GetDescriptorHandleIncrementSize(m_desc.Type);

            m_cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
            m_gpuStart = {};

            const bool isGpuVisible = RHI::CheckBitsAll(flags, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            if (isGpuVisible)
            {
                m_gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
            }

            if (isGpuVisible)
            {
                RHI::FreeListAllocator::Descriptor descriptor;
                descriptor.m_alignmentInBytes = 1;

                //It is possible for descriptorCountForAllocator to not match descriptorCountForHeap for DescriptorPoolShaderVisibleCbvSrvUav
                //heaps in which case descriptorCountForAllocator defines the number of static handles 
                descriptor.m_capacityInBytes = aznumeric_cast<uint32_t>(descriptorCountForAllocator);
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
                descriptor.m_capacityInBytes = aznumeric_cast<uint32_t>(descriptorCountForAllocator);
                descriptor.m_garbageCollectLatency = 0;

                RHI::PoolAllocator* allocator = aznew RHI::PoolAllocator();
                allocator->Init(descriptor);
                m_allocator.reset(allocator);
            }
        }

        void DescriptorPool::InitPooledRange(DescriptorPool& parent, uint32_t offset, uint32_t count)
        {
            m_desc = parent.m_desc;
            m_descriptorHeap.Attach(parent.GetPlatformHeap());
            m_stride = parent.m_stride;
            m_cpuStart = parent.m_cpuStart;
            m_cpuStart.ptr += m_stride * offset;

            bool shaderVisible = RHI::CheckBitsAll(m_desc.Flags, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            if (shaderVisible)
            {
                m_gpuStart = parent.m_gpuStart;
                m_gpuStart.ptr += m_stride * offset;
            }

            // NOTE: The range is currently only used for the static descriptor region of the shader visible heap, so
            // we leverage the PoolAllocator since these descriptors are allocated one-at-a-time (fragmentation free).
            RHI::PoolAllocator::Descriptor desc;
            desc.m_alignmentInBytes = 1;
            desc.m_elementSize = 1;
            desc.m_capacityInBytes = count;
            desc.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;
            m_allocator = AZStd::make_unique<RHI::PoolAllocator>();
            static_cast<RHI::PoolAllocator*>(m_allocator.get())->Init(desc);
        }

        DescriptorHandle DescriptorPool::AllocateHandle(uint32_t count)
        {
            RHI::VirtualAddress address;
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                address = m_allocator->Allocate(count, 1);
            }

            if (address.IsValid())
            {
                DescriptorHandle handle(m_desc.Type, m_desc.Flags, static_cast<uint32_t>(address.m_ptr));
                return handle;
            }
            else
            {
                return DescriptorHandle{};
            }
        }

        void DescriptorPool::ReleaseHandle(DescriptorHandle handle)
        {
            if (handle.IsNull())
            {
                return;
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            m_allocator->DeAllocate(RHI::VirtualAddress::CreateFromOffset(handle.m_index));
        }

        DescriptorTable DescriptorPool::AllocateTable(uint32_t count)
        {
            return DescriptorTable(AllocateHandle(count), static_cast<uint16_t>(count));
        }

        void DescriptorPool::ReleaseTable(DescriptorTable table)
        {
            ReleaseHandle(table.GetOffset());
        }

        void DescriptorPool::GarbageCollect()
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            m_allocator->GarbageCollect();
        }

        ID3D12DescriptorHeap* DescriptorPool::GetPlatformHeap() const
        {
            return m_descriptorHeap.Get();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPool::GetCpuPlatformHandle(DescriptorHandle handle) const
        {
            AZ_Assert(handle.m_index != DescriptorHandle::NullIndex, "Index is invalid");
            return D3D12_CPU_DESCRIPTOR_HANDLE{ m_cpuStart.ptr + handle.m_index * m_stride };
        }

        D3D12_GPU_DESCRIPTOR_HANDLE DescriptorPool::GetGpuPlatformHandle(DescriptorHandle handle) const
        {
            AZ_Assert(handle.IsShaderVisible(), "Handle is not shader visible");
            AZ_Assert(handle.m_index != DescriptorHandle::NullIndex, "Index is invalid");
            return D3D12_GPU_DESCRIPTOR_HANDLE{ m_gpuStart.ptr + (handle.m_index * m_stride) };
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPool::GetCpuPlatformHandleForTable(DescriptorTable descTable) const
        {
            DescriptorHandle handle = descTable.GetOffset();
            AZ_Assert(handle.m_index != DescriptorHandle::NullIndex, "Index is invalid");
            return D3D12_CPU_DESCRIPTOR_HANDLE{ m_cpuStart.ptr + handle.m_index * m_stride };
        }

        D3D12_GPU_DESCRIPTOR_HANDLE DescriptorPool::GetGpuPlatformHandleForTable(DescriptorTable descTable) const
        {
            DescriptorHandle handle = descTable.GetOffset();
            AZ_Assert(handle.IsShaderVisible(), "Handle is not shader visible");
            AZ_Assert(handle.m_index != DescriptorHandle::NullIndex, "Index is invalid");
            return D3D12_GPU_DESCRIPTOR_HANDLE{ m_gpuStart.ptr + (handle.m_index * m_stride) };
        }
    }
}
