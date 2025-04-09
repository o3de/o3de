/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Vulkan.h>
#include <RHI/AliasedHeap.h>
#include <RHI/AliasingBarrierTracker.h>
#include <RHI/Image.h>
#include <RHI/Buffer.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Scope.h>
#include <RHI/VulkanMemoryAllocation.h>
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
            return AZStd::make_unique<AliasingBarrierTracker>(GetVulkanRHIDevice(), GetDescriptor().m_budgetInBytes);
        }

        RHI::ResultCode AliasedHeap::InitInternal(RHI::Device& rhiDevice, const RHI::AliasedHeapDescriptor& descriptor)
        {
            DeviceObject::Init(rhiDevice);
            Device& device = static_cast<Device&>(rhiDevice);
            m_descriptor = static_cast<const Descriptor&>(descriptor);

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VmaAllocation vmaAllocation;
            VkMemoryRequirements memReq = m_descriptor.m_memoryRequirements;
            memReq.alignment = AZStd::max(memReq.alignment, descriptor.m_alignment);
            memReq.size = descriptor.m_budgetInBytes;
            VkResult vkResult = vmaAllocateMemory(device.GetVmaAllocator(), &memReq, &allocInfo, &vmaAllocation, nullptr);
            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_heapMemory = VulkanMemoryAllocation::Create();
            m_heapMemory->Init(device, vmaAllocation);

            return RHI::ResultCode::Success;
        }

        void AliasedHeap::ShutdownInternal()
        {
            Device& device = GetVulkanRHIDevice();
            device.QueueForRelease(m_heapMemory);
            m_heapMemory = nullptr;
            Base::ShutdownInternal();
        }

        void AliasedHeap::ShutdownResourceInternal(RHI::DeviceResource& resource)
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

        RHI::ResultCode AliasedHeap::InitImageInternal(const RHI::DeviceImageInitRequest& request, size_t heapOffset)
        {
            Image* image = static_cast<Image*>(request.m_image);
            RHI::ImageDescriptor imageDescriptor = request.m_descriptor;
            // Add copy write flag because it's needed for clearing operations.
            imageDescriptor.m_bindFlags |= RHI::ImageBindFlags::CopyWrite;
            RHI::ResourceMemoryRequirements memoryRequirements = GetDevice().GetResourceMemoryRequirements(imageDescriptor);

            RHI::ResultCode result =
                image->Init(
                    GetVulkanRHIDevice(),
                    imageDescriptor,
                    MemoryView(m_heapMemory, heapOffset, memoryRequirements.m_sizeInBytes));

            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            image->SetResidentSizeInBytes(memoryRequirements.m_sizeInBytes);
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode AliasedHeap::InitBufferInternal(const RHI::DeviceBufferInitRequest& request, size_t heapOffset)
        {
            Buffer* buffer = static_cast<Buffer*>(request.m_buffer);
            RHI::BufferDescriptor bufferDescriptor = request.m_descriptor;
            // Add copy write flag because it's needed for clearing operations.
            bufferDescriptor.m_bindFlags |= RHI::BufferBindFlags::CopyWrite;
            RHI::ResourceMemoryRequirements memoryRequirements = GetDevice().GetResourceMemoryRequirements(bufferDescriptor);

            RHI::Ptr<BufferMemory> bufferMemory = BufferMemory::Create();
            RHI::ResultCode result = bufferMemory->Init(
                GetVulkanRHIDevice(),
                MemoryView(m_heapMemory, heapOffset, memoryRequirements.m_sizeInBytes),
                BufferMemory::Descriptor(bufferDescriptor, RHI::HeapMemoryLevel::Device));
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            BufferMemoryView memoryView = BufferMemoryView(bufferMemory);

            buffer->SetDescriptor(bufferDescriptor);
            result = buffer->Init(GetVulkanRHIDevice(), bufferDescriptor, memoryView);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

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
