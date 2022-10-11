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
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Scope.h>
#include <RHI/Memory.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<AliasedHeap> AliasedHeap::Create()
        {
            return aznew AliasedHeap;
        }

        AZStd::unique_ptr<RHI::AliasingBarrierTracker> AliasedHeap::CreateBarrierTrackerInternal()
        {
            return AZStd::make_unique<AliasingBarrierTracker>(GetVulkanRHIDevice());
        }

        RHI::ResultCode AliasedHeap::InitInternal(RHI::Device& rhiDevice, const RHI::AliasedHeapDescriptor& descriptor)
        {
            DeviceObject::Init(rhiDevice);
            Device& device = static_cast<Device&>(rhiDevice);
            m_descriptor = static_cast<const Descriptor&>(descriptor);
          
            m_heapMemory = device.AllocateMemory(m_descriptor.m_budgetInBytes, m_descriptor.m_memoryTypeMask, m_descriptor.m_memoryFlags);
            return m_heapMemory ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
        }

        void AliasedHeap::ShutdownInternal()
        {
            Device& device = GetVulkanRHIDevice();
            device.QueueForRelease(m_heapMemory);
            m_heapMemory = nullptr;
            Base::ShutdownInternal();
        }

        void AliasedHeap::ShutdownResourceInternal(RHI::Resource& resource)
        {
            if (Buffer* buffer = azrtti_cast<Buffer*>(&resource))
            {
                buffer->m_memoryView = {};
                buffer->Invalidate();
            }
            else if (Image* image = azrtti_cast<Image*>(&resource))
            {
                image->m_memoryView = {};
                image->Invalidate();
            }
        }

        RHI::ResultCode AliasedHeap::InitImageInternal(const RHI::ImageInitRequest& request, size_t heapOffset)
        {
            Image* image = static_cast<Image*>(request.m_image);
            RHI::ImageDescriptor imageDescriptor = request.m_descriptor;
            // Add copy write flag because it's needed for clearing operations.
            imageDescriptor.m_bindFlags |= RHI::ImageBindFlags::CopyWrite;
            RHI::ResourceMemoryRequirements memoryRequirements = GetDevice().GetResourceMemoryRequirements(imageDescriptor);

            RHI::ResultCode result = image->Init(GetVulkanRHIDevice(), imageDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            MemoryView memoryView = MemoryView(m_heapMemory, heapOffset, memoryRequirements.m_sizeInBytes, memoryRequirements.m_alignmentInBytes, MemoryAllocationType::SubAllocated);
            result = image->BindMemoryView(memoryView, RHI::HeapMemoryLevel::Device);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            image->SetDescriptor(imageDescriptor);
            image->SetResidentSizeInBytes(memoryView.GetSize());
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode AliasedHeap::InitBufferInternal(const RHI::BufferInitRequest& request, size_t heapOffset)
        {
            Buffer* buffer = static_cast<Buffer*>(request.m_buffer);
            RHI::BufferDescriptor bufferDescriptor = request.m_descriptor;
            // Add copy write flag because it's needed for clearing operations.
            bufferDescriptor.m_bindFlags |= RHI::BufferBindFlags::CopyWrite;
            RHI::ResourceMemoryRequirements memoryRequirements = GetDevice().GetResourceMemoryRequirements(bufferDescriptor);

            RHI::Ptr<BufferMemory> bufferMemory = BufferMemory::Create();
            RHI::ResultCode result = bufferMemory->Init(GetVulkanRHIDevice(), MemoryView(m_heapMemory, heapOffset, memoryRequirements.m_sizeInBytes, memoryRequirements.m_alignmentInBytes, MemoryAllocationType::SubAllocated), bufferDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            BufferMemoryView memoryView = BufferMemoryView(bufferMemory, 0, memoryRequirements.m_sizeInBytes, memoryRequirements.m_alignmentInBytes, MemoryAllocationType::SubAllocated);

            result = buffer->Init(GetVulkanRHIDevice(), bufferDescriptor, memoryView);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            buffer->SetDescriptor(bufferDescriptor);
            return RHI::ResultCode::Success;
        }

        Device& AliasedHeap::GetVulkanRHIDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        const AliasedHeap::Descriptor& AliasedHeap::GetDescriptor() const
        {
            return m_descriptor;
        }        
    }
}
