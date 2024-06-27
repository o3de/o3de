/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <RHI/AliasedHeap.h>
#include <RHI/AliasingBarrierTracker.h>
#include <RHI/Buffer.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<AliasedHeap> AliasedHeap::Create()
        {
            return aznew AliasedHeap;
        }

        AZStd::unique_ptr<RHI::AliasingBarrierTracker> AliasedHeap::CreateBarrierTrackerInternal()
        {
            return AZStd::make_unique<AliasingBarrierTracker>(GetMetalRHIDevice());
        }

        RHI::ResultCode AliasedHeap::InitInternal(RHI::Device& device, const RHI::AliasedHeapDescriptor& descriptor)
        {
            RHI::DeviceObject::Init(device);
            MTLHeapDescriptor* heapDescriptor = [[MTLHeapDescriptor alloc]init];
            heapDescriptor.type = MTLHeapTypePlacement;
            heapDescriptor.storageMode = MTLStorageModePrivate;
            heapDescriptor.size = descriptor.m_budgetInBytes;
            heapDescriptor.hazardTrackingMode = MTLHazardTrackingModeTracked;

            m_heap = [GetMetalRHIDevice().GetMtlDevice() newHeapWithDescriptor:heapDescriptor];
            AZ_Assert(m_heap, "Heap should not be null");
            [heapDescriptor release] ;
         
            return RHI::ResultCode::Success;
        }

        void AliasedHeap::ShutdownInternal()
        {
            [m_heap release];
            m_heap = nil;
        }

        void AliasedHeap::ShutdownResourceInternal(RHI::DeviceResource& resource)
        {
            Device& device = GetMetalRHIDevice();
            if (Buffer* buffer = azrtti_cast<Buffer*>(&resource))
            {
                device.QueueForRelease(buffer->GetMemoryView());
                buffer->m_memoryView = {};
            }
            else if(Image* image = azrtti_cast<Image*>(&resource))
            {
                device.QueueForRelease(image->GetMemoryView());
                image->m_memoryView = {};
            }
        }

        RHI::ResultCode AliasedHeap::InitImageInternal(const RHI::DeviceImageInitRequest& request, size_t heapOffset)
        {
            Image* image = static_cast<Image*>(request.m_image);
            RHI::ResourceMemoryRequirements memoryRequirements = GetDevice().GetResourceMemoryRequirements(request.m_descriptor);

            MTLSizeAndAlign textureSizeAndAlign;
            textureSizeAndAlign.align = memoryRequirements.m_alignmentInBytes;
            textureSizeAndAlign.size = memoryRequirements.m_sizeInBytes;

            MemoryView memoryView = GetMetalRHIDevice().CreateImagePlaced(request.m_descriptor, m_heap, heapOffset, textureSizeAndAlign);
            if (!memoryView.IsValid())
            {
                return RHI::ResultCode::Fail;
            }

            image->SetDescriptor(request.m_descriptor);
            image->m_memoryView = AZStd::move(memoryView);
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode AliasedHeap::InitBufferInternal(const RHI::DeviceBufferInitRequest& request, size_t heapOffset)
        {
            Buffer* buffer = static_cast<Buffer*>(request.m_buffer);
            RHI::ResourceMemoryRequirements memoryRequirements = GetDevice().GetResourceMemoryRequirements(request.m_descriptor);

            MTLSizeAndAlign bufferSizeAndAlign;
            bufferSizeAndAlign.align = memoryRequirements.m_alignmentInBytes;
            bufferSizeAndAlign.size = memoryRequirements.m_sizeInBytes;
                    
            MemoryView memoryView = GetMetalRHIDevice().CreateBufferPlaced(request.m_descriptor, m_heap, heapOffset, bufferSizeAndAlign);
            if (!memoryView.IsValid())
            {
                return RHI::ResultCode::Fail;
            }

            buffer->SetDescriptor(request.m_descriptor);
            buffer->m_memoryView = BufferMemoryView{ AZStd::move(memoryView), BufferMemoryType::Unique };

            return RHI::ResultCode::Success;
        }

        Device& AliasedHeap::GetMetalRHIDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }
    }
}
