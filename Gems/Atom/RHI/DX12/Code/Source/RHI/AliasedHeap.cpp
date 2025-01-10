/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/AliasedHeap.h>
#include <RHI/AliasingBarrierTracker.h>
#include <RHI/Image.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/Scope.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace DX12
    {
        static size_t CalculateHeapAlignment(D3D12_HEAP_FLAGS heapFlags)
        {
            /**
             * Heap alignment is the alignment of the actual heap we are allocating, not the base alignment of
             * of sub-allocations from the heap. That is confusing in the D3D12 docs. The heap itself is
             * required to be 4MB aligned if it holds MSAA textures. Therefore, this simple metric just
             * forces 4MB alignment of the heap for all textures, because our chances of having an MSAA target
             * across the whole frame is high, and the amount of internal fragmentation is low relative to the full
             * heap size.
             * To simply test the flags for equality, we mask the D3D12_HEAP_FLAG_SHARED prior to testing.
             */
            auto maskedFlags = heapFlags & ~(D3D12_HEAP_FLAG_SHARED);
            if (D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES == maskedFlags ||
                D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES == maskedFlags ||
                D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES == maskedFlags)
            {
                return D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
            }
            else
            {
                return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            }
        }

        RHI::Ptr<AliasedHeap> AliasedHeap::Create()
        {
            return aznew AliasedHeap;
        }

        AZStd::unique_ptr<RHI::AliasingBarrierTracker> AliasedHeap::CreateBarrierTrackerInternal()
        {
            return AZStd::make_unique<AliasingBarrierTracker>();
        }

        RHI::ResultCode AliasedHeap::InitInternal(
            RHI::Device& rhiDevice,
            const RHI::AliasedHeapDescriptor& descriptor)
        {
            Device& device = static_cast<Device&>(rhiDevice);
            m_descriptor = static_cast<const Descriptor&>(descriptor);
            
            D3D12_HEAP_DESC heapDesc;
            heapDesc.Alignment = AZStd::max(CalculateHeapAlignment(m_descriptor.m_heapFlags), descriptor.m_alignment);
            // Even the dx12 document said 'but non-aligned SizeInBytes is also supported', but it may lead to TDR issue with some graphics cards
            // Such as nvidia 2070, 2080
            heapDesc.SizeInBytes = RHI::AlignUp<size_t>(descriptor.m_budgetInBytes, heapDesc.Alignment);
            heapDesc.Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            heapDesc.Flags = m_descriptor.m_heapFlags;

            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            heapMemoryUsage.m_totalResidentInBytes = heapDesc.SizeInBytes;
            heapMemoryUsage.m_usedResidentInBytes = heapDesc.SizeInBytes;

            Microsoft::WRL::ComPtr<ID3D12Heap> heap;
            device.AssertSuccess(device.GetDevice()->CreateHeap(&heapDesc, IID_GRAPHICS_PPV_ARGS(heap.GetAddressOf())));
            m_heap = heap.Get();
            
            return m_heap ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
        }

        void AliasedHeap::ShutdownInternal()
        {
            m_heap = nullptr;
            Base::ShutdownInternal();
        }

        void AliasedHeap::ShutdownResourceInternal(RHI::DeviceResource& resource)
        {
            Device& device = GetDX12RHIDevice();
            if (Buffer* buffer = azrtti_cast <Buffer*>(&resource))
            {
                device.QueueForRelease(buffer->GetMemoryView());
                buffer->m_memoryView = {};
            }
            else if (Image* image = azrtti_cast < Image * > (&resource))
            {
                device.QueueForRelease(image->GetMemoryView());
                image->m_memoryView = {};
            }
        }

        RHI::ResultCode AliasedHeap::InitBufferInternal(const RHI::DeviceBufferInitRequest& request, size_t heapOffset)
        {
            const RHI::BufferDescriptor& descriptor = request.m_descriptor;
            Buffer* buffer = static_cast<Buffer*>(request.m_buffer);

            MemoryView memoryView =
                GetDX12RHIDevice().CreateBufferPlaced(
                    descriptor,
                    D3D12_RESOURCE_STATE_COMMON,
                    m_heap.get(),
                    heapOffset);

            if (!memoryView.IsValid())
            {
                return RHI::ResultCode::OutOfMemory;
            }

            buffer->SetDescriptor(descriptor);
            buffer->m_memoryView = BufferMemoryView{ AZStd::move(memoryView), BufferMemoryType::Unique };
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode AliasedHeap::InitImageInternal(const RHI::DeviceImageInitRequest& request, size_t heapOffset)
        {
            const RHI::ImageDescriptor& descriptor = request.m_descriptor;
            Image* image = static_cast<Image*>(request.m_image);

            image->SetDescriptor(descriptor);

            /// Ownership is managed by the cache.
            MemoryView memoryView =
                GetDX12RHIDevice().CreateImagePlaced(
                    descriptor,
                    request.m_optimizedClearValue,
                    image->GetInitialResourceState(),
                    m_heap.get(),
                    heapOffset);

            image->GenerateSubresourceLayouts();
            image->m_residentSizeInBytes = memoryView.GetSize();
            image->m_memoryView = AZStd::move(memoryView);
            return RHI::ResultCode::Success;
        }

        Device& AliasedHeap::GetDX12RHIDevice()
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        const AliasedHeap::Descriptor& AliasedHeap::GetDescriptor() const
        {
            return m_descriptor;
        }
    }
}
