/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/HeapAllocator.h>

namespace AZ
{
    namespace DX12
    {
        void HeapFactory::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_getHeapMemoryUsageFunction, "You must supply a valid function for getting heap memory usage.");

            m_descriptor = descriptor;

            // use image alignment (since it's compatible with alignments for buffer and constant buffer) for all the resource types
            m_descriptor.m_pageSizeInBytes = RHI::AlignUp(m_descriptor.m_pageSizeInBytes, Alignment::Image);

            // deny buffers if resource type doesn't include buffer
            if ((descriptor.m_resourceTypeFlags & ResourceTypeFlags::Buffer) != ResourceTypeFlags::Buffer)
            {
                m_heapFlags |= D3D12_HEAP_FLAG_DENY_BUFFERS;
            }

            // deny textures if resource type doesn't include image
            if ((descriptor.m_resourceTypeFlags & ResourceTypeFlags::Image) != ResourceTypeFlags::Image)
            {
                m_heapFlags |= D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
            }

            // deny render target and depth stencil if resource type doesn't include render target
            if ((descriptor.m_resourceTypeFlags & ResourceTypeFlags::RenderTarget) != ResourceTypeFlags::RenderTarget)
            {
                m_heapFlags |= D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
            }

            // heap type for default or upload or readback
            m_heapType = ConvertHeapType(descriptor.m_heapMemoryLevel, descriptor.m_hostMemoryAccess);
        }

        RHI::Ptr<Heap> HeapFactory::CreateObject()
        {
            AZ_PROFILE_SCOPE(RHI, "Create heap Page: size %dk", m_descriptor.m_pageSizeInBytes/1024);

            RHI::HeapMemoryUsage* heapMemoryUsage = m_descriptor.m_getHeapMemoryUsageFunction();
            if (!heapMemoryUsage->CanAllocate(m_descriptor.m_pageSizeInBytes))
            {
                AZ_Warning("HeapFactory", false, "Heap allocation failed: reach the memory budget");
                return nullptr;
            }
           
            Device* device = static_cast<Device*>(m_descriptor.m_device);

            CD3DX12_HEAP_DESC heapDesc( m_descriptor.m_pageSizeInBytes, m_heapType, 0, m_heapFlags);
                
            Microsoft::WRL::ComPtr<ID3D12Heap> heap;
            if (device->AssertSuccess(device->GetDevice()->CreateHeap(&heapDesc, IID_GRAPHICS_PPV_ARGS(heap.GetAddressOf()))))
            {
                heapMemoryUsage->m_totalResidentInBytes += m_descriptor.m_pageSizeInBytes;
                return heap.Get();
            }

            AZ_Warning("HeapFactory", false, "Heap allocation failed: failed to create a dx12 heap");
            return nullptr;
        }

        void HeapFactory::ShutdownObject(Heap& object, [[maybe_unused]] bool isPoolShutdown)
        {
            m_descriptor.m_device->QueueForRelease(&object);

            RHI::HeapMemoryUsage* heapMemoryUsage = m_descriptor.m_getHeapMemoryUsageFunction();
            heapMemoryUsage->m_totalResidentInBytes -= m_descriptor.m_pageSizeInBytes;
            heapMemoryUsage->Validate();
        }

        bool HeapFactory::CollectObject([[maybe_unused]] Heap& object)
        {
            return m_descriptor.m_recycleOnCollect;
        }

        const HeapFactory::Descriptor& HeapFactory::GetDescriptor() const
        {
            return m_descriptor;
        }
    }
}
