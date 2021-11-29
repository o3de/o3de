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

        void DescriptorPool::CloneAllocator(RHI::Allocator* newAllocator)
        {
            m_allocator->Clone(newAllocator);
        }

        void DescriptorPool::ClearAllocator()
        {
            AZ_Assert(m_gpuStart.ptr, "Clearing the allocator is only supported for the gpu visible heap as only this heap can be compacted");
            static_cast<RHI::FreeListAllocator*>(m_allocator.get())
                ->Init(static_cast<RHI::FreeListAllocator*>(m_allocator.get())->GetDescriptor());
        }

        RHI::Allocator* DescriptorPool::GetAllocator() const
        {
            return m_allocator.get();
        }

        void DescriptorPoolShaderVisibleCbvSrvUav::Init(
            ID3D12DeviceX* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags,
            uint32_t descriptorCount,
            uint32_t staticHandlesCount)
        {
            //This pool manages two allocators. The allocator in the base class manages static handles
            Base::Init(device, type, flags, descriptorCount, staticHandlesCount);

            //This allocator manages dynamic handles associated with descriptor tables. This allows us to
            //reconstruct the full heap in a compact manner if it ever fragments. 
            RHI::FreeListAllocator::Descriptor descriptor;
            descriptor.m_alignmentInBytes = 1;
            descriptor.m_capacityInBytes = aznumeric_cast<uint32_t>(descriptorCount - staticHandlesCount);
            descriptor.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;

            RHI::FreeListAllocator* allocator = aznew RHI::FreeListAllocator();
            allocator->Init(descriptor);
            m_unboundedArrayAllocator.reset(allocator);

            //Cache the starting point of the dynamic section of the heap
            m_startingHandleIndex = staticHandlesCount;
        }

        DescriptorTable DescriptorPoolShaderVisibleCbvSrvUav::AllocateTable(uint32_t count)
        {
            RHI::VirtualAddress address;
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                address = m_unboundedArrayAllocator->Allocate(count, 1);
            }

            if (address.IsValid())
            {
                DescriptorHandle handle(m_desc.Type, m_desc.Flags, static_cast<uint32_t>(address.m_ptr));
                return DescriptorTable(handle, static_cast<uint16_t>(count));
            }
            else
            {
                return DescriptorTable{};
            }
        }

        void DescriptorPoolShaderVisibleCbvSrvUav::ReleaseTable(DescriptorTable table)
        {
            if (table.IsNull())
            {
                return;
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            m_unboundedArrayAllocator->DeAllocate(RHI::VirtualAddress::CreateFromOffset(table.GetOffset().m_index));
        }

        void DescriptorPoolShaderVisibleCbvSrvUav::GarbageCollect()
        {
            Base::GarbageCollect();
            m_unboundedArrayAllocator->GarbageCollect();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPoolShaderVisibleCbvSrvUav::GetCpuPlatformHandleForTable(DescriptorTable descTable) const
        {
            DescriptorHandle handle = descTable.GetOffset();
            AZ_Assert(handle.m_index != DescriptorHandle::NullIndex, "Index is invalid");
            return D3D12_CPU_DESCRIPTOR_HANDLE{ m_cpuStart.ptr + (m_startingHandleIndex * m_stride) + (handle.m_index * m_stride) };
        }

        D3D12_GPU_DESCRIPTOR_HANDLE DescriptorPoolShaderVisibleCbvSrvUav::GetGpuPlatformHandleForTable(DescriptorTable descTable) const
        {
            DescriptorHandle handle = descTable.GetOffset();
            AZ_Assert(handle.IsShaderVisible(), "Handle is not shader visible");
            AZ_Assert(handle.m_index != DescriptorHandle::NullIndex, "Index is invalid");
            return D3D12_GPU_DESCRIPTOR_HANDLE{ m_gpuStart.ptr + (m_startingHandleIndex * m_stride) + (handle.m_index * m_stride) };
        }

        void DescriptorPoolShaderVisibleCbvSrvUav::ClearAllocator()
        {
            Base::ClearAllocator();
            static_cast<RHI::FreeListAllocator*>(m_unboundedArrayAllocator.get())->Init(static_cast<RHI::FreeListAllocator*>(m_unboundedArrayAllocator.get())->GetDescriptor());
        }
    }
}
