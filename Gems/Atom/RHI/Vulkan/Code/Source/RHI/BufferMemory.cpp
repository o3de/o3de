/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI/RHIBus.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom_RHI_Vulkan_Platform.h>
#include <AzCore/Math/MathIntrinsics.h>
#include <RHI/BufferMemory.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<BufferMemory> BufferMemory::Create()
        {
            return aznew BufferMemory();
        }

        RHI::ResultCode BufferMemory::Init(Device& device, const MemoryView& memoryView, const Descriptor& descriptor)
        {
            BufferCreateInfo createInfo = device.BuildBufferCreateInfo(descriptor);
            // Create the buffer using a specific memory location. This function doesn't allocate new memory, it binds
            // the provided memory (with the proper offset) to the buffer.
            VkResult vkResult = vmaCreateAliasingBuffer2(
                device.GetVmaAllocator(),
                memoryView.GetAllocation()->GetVmaAllocation(),
                memoryView.GetOffset(),
                createInfo.GetCreateInfo(),
                &m_vkBuffer);

            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            Base::Init(device);
            m_memoryView = memoryView;
            m_descriptor = descriptor;

            return result;
        }

        RHI::ResultCode BufferMemory::Init(Device& device, const Descriptor& descriptor)
        {
            Base::Init(device);
            m_descriptor = descriptor;
            BufferCreateInfo createInfo = device.BuildBufferCreateInfo(descriptor);
            struct
            {
                size_t m_alignment = 0;
                void operator=(size_t value)
                {
                    m_alignment = AZStd::max(m_alignment, value);
                }
            } alignment;
            RHI::RHIRequirementRequestBus::BroadcastResult(alignment, &RHI::RHIRequirementsRequest::GetRequiredAlignment, device);
            auto updatedCreateInfo{ createInfo.GetCreateInfo() };
            if (alignment.m_alignment && updatedCreateInfo->size > alignment.m_alignment)
            {
                updatedCreateInfo->size =
                    ((updatedCreateInfo->size + alignment.m_alignment - 1) / alignment.m_alignment) * alignment.m_alignment;
            }

            VmaAllocationCreateInfo allocInfo = GetVmaAllocationCreateInfo(descriptor.m_heapMemoryLevel);
            VmaAllocation vmaAlloc;
            VkResult vkResult;
            // Creates the buffer, allocates new memory and bind it to the buffer.
            vkResult = vmaCreateBufferWithAlignment(
                device.GetVmaAllocator(),
                createInfo.GetCreateInfo(),
                &allocInfo,
                RHI::IsPowerOfTwo(descriptor.m_alignment) ? descriptor.m_alignment : 1,
                &m_vkBuffer,
                &vmaAlloc,
                nullptr);

            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            RHI::Ptr<VulkanMemoryAllocation> alloc = VulkanMemoryAllocation::Create();
            alloc->Init(device, vmaAlloc);
            m_memoryView = MemoryView(alloc);
            m_sharingMode = createInfo.GetCreateInfo()->sharingMode;
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode BufferMemory::InitWithExternalHostMemory(Device& device, const Descriptor& descriptor)
        {
            struct
            {
                size_t m_alignment = 0;
                void operator=(size_t value)
                {
                    m_alignment = AZStd::max(m_alignment, value);
                }
            } alignment;
            RHI::RHIRequirementRequestBus::BroadcastResult(alignment, &RHI::RHIRequirementsRequest::GetRequiredAlignment, device);
            for (int deviceIndex = 0; deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount(); deviceIndex++)
            {
                auto currentDevice = RHI::RHISystemInterface::Get()->GetDevice(deviceIndex);
                if (currentDevice->GetFeatures().m_crossDeviceHostMemory)
                {
                    alignment = static_cast<const PhysicalDevice&>(currentDevice->GetPhysicalDevice())
                                    .GetExternalMemoryHostProperties()
                                    .minImportedHostPointerAlignment;
                }
            }
            m_allocatedHostMemorySize = RHI::AlignUp(descriptor.m_byteCount, alignment.m_alignment);
            m_allocatedHostMemory.reset(AZ_OS_MALLOC(m_allocatedHostMemorySize, alignment.m_alignment));
            return InitWithExternalHostMemory(device, descriptor, m_allocatedHostMemory.get(), m_allocatedHostMemorySize);
        }

        RHI::ResultCode BufferMemory::InitWithExternalHostMemory(
            Device& device, const Descriptor& descriptor, void* allocatedHostMemory, size_t allocatedMemoryHostSize)
        {
            Base::Init(device);
            m_descriptor = descriptor;

            AZ_Assert(
                static_cast<const PhysicalDevice&>(device.GetPhysicalDevice())
                    .IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::ExternalMemoryHost),
                "External Host Memory not supported");

            AZ_Assert(
                descriptor.m_heapMemoryLevel == RHI::HeapMemoryLevel::Host, "Cannot create External Host Memory Buffer in Device Memory");

            BufferCreateInfo createInfo = device.BuildBufferCreateInfo(descriptor);
            createInfo.GetCreateInfo()->size = allocatedMemoryHostSize;

            auto& memProps = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice()).GetMemoryProperties();
            VkMemoryHostPointerPropertiesEXT hostMemoryProps{};
            hostMemoryProps.sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;
            auto vkResult = device.GetContext().GetMemoryHostPointerPropertiesEXT(
                device.GetNativeDevice(), VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, allocatedHostMemory, &hostMemoryProps);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            int32_t bestMemoryTypeIndex = -1;
            VkMemoryPropertyFlags bestMemoryFlags{};

            for (auto memoryTypeIndex{ 0U }; memoryTypeIndex < memProps.memoryTypeCount; ++memoryTypeIndex)
            {
                auto flags{ memProps.memoryTypes[memoryTypeIndex].propertyFlags };

                if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                    continue;

                if (RHI::CheckBit(hostMemoryProps.memoryTypeBits, memoryTypeIndex) &&
                    (bestMemoryTypeIndex == -1 ||
                     (az_popcnt_u32(static_cast<uint32_t>(bestMemoryFlags)) > az_popcnt_u32(static_cast<uint32_t>(flags)))))
                {
                    bestMemoryFlags = flags;
                    bestMemoryTypeIndex = memoryTypeIndex;
                }
            }
            AZ_Assert(bestMemoryTypeIndex >= 0, "Could not find memory type index for imported host memory");

            VkImportMemoryHostPointerInfoEXT importInfo{};
            importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
            importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
            importInfo.pHostPointer = allocatedHostMemory;

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = allocatedMemoryHostSize;
            allocInfo.memoryTypeIndex = bestMemoryTypeIndex;
            allocInfo.pNext = &importInfo;

            VkDeviceMemory memory;
            vkResult = device.GetContext().AllocateMemory(device.GetNativeDevice(), &allocInfo, nullptr, &memory);
            result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            VkExternalMemoryBufferCreateInfo externalMemoryBufferCreateInfo{};
            externalMemoryBufferCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
            externalMemoryBufferCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
            if (createInfo.GetCreateInfo()->pNext)
            {
                // This might happen if a Gem adds external memory flags via CollectExternalMemoryRequirements
                // We add the the given memory flags to our external flags here
                auto data = static_cast<const VkExternalMemoryBufferCreateInfo*>(createInfo.GetCreateInfo()->pNext);
                AZ_Assert(data->sType == VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO, "Unknown BufferCreateInfo extension");
                externalMemoryBufferCreateInfo.handleTypes |= data->handleTypes;
            }
            createInfo.GetCreateInfo()->pNext = &externalMemoryBufferCreateInfo;

            vkResult = device.GetContext().CreateBuffer(device.GetNativeDevice(), createInfo.GetCreateInfo(), nullptr, &m_vkBuffer);
            result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            vkResult = device.GetContext().BindBufferMemory(device.GetNativeDevice(), m_vkBuffer, memory, 0);
            result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            RHI::Ptr<VulkanMemoryAllocation> alloc = VulkanMemoryAllocation::Create();
            alloc->Init(device, memory, allocatedMemoryHostSize);
            m_memoryView = MemoryView(alloc);
            m_sharingMode = createInfo.GetCreateInfo()->sharingMode;
            return RHI::ResultCode::Success;
        }

        CpuVirtualAddress BufferMemory::Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess)
        {
            return m_memoryView.GetAllocation()->Map(m_memoryView.GetOffset() + offset, size, hostAccess);
        }

        void BufferMemory::Unmap(size_t offset, RHI::HostMemoryAccess hostAccess)
        {
            return m_memoryView.GetAllocation()->Unmap(m_memoryView.GetOffset() + offset, hostAccess);
        }

        const VkBuffer BufferMemory::GetNativeBuffer()
        {
            return m_vkBuffer;
        }

        const BufferMemory::Descriptor& BufferMemory::GetDescriptor() const
        {
            return m_descriptor;
        }

        VkSharingMode BufferMemory::GetSharingMode() const
        {
            return m_sharingMode;
        }

        size_t BufferMemory::GetSize() const
        {
            return m_memoryView.GetSize();
        }

        const MemoryView& BufferMemory::GetMemoryView() const
        {
            return m_memoryView;
        }

        void* BufferMemory::GetAllocatedHostMemory() const
        {
            return m_allocatedHostMemory.get();
        }

        size_t BufferMemory::GetAllocatedHostMemorySize() const
        {
            return m_allocatedHostMemorySize;
        }

        size_t BufferMemory::GetAllocationSize() const
        {
            return m_memoryView.GetAllocation()->GetSize();
        }

        VkDeviceMemory BufferMemory::GetNativeDeviceMemory() const
        {
            return m_memoryView.GetNativeDeviceMemory();
        }

        size_t BufferMemory::GetMemoryViewOffset() const
        {
            return m_memoryView.GetOffset();
        }

        void BufferMemory::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_vkBuffer), name.data(), VK_OBJECT_TYPE_BUFFER, static_cast<Device&>(GetDevice()));
            }

             m_memoryView.SetName(name);
        }

        void BufferMemory::Shutdown()
        {
            if (m_vkBuffer != VK_NULL_HANDLE)
            {
                Device& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroyBuffer(device.GetNativeDevice(), m_vkBuffer, VkSystemAllocator::Get());
                m_vkBuffer = VK_NULL_HANDLE;
            }

            m_memoryView = MemoryView();
        }

        BufferMemory::Descriptor::Descriptor(const RHI::BufferDescriptor& desc, RHI::HeapMemoryLevel memoryLevel)
            : RHI::BufferDescriptor(desc)
            , m_heapMemoryLevel(memoryLevel)
        {
        }
    } // namespace Vulkan
}
