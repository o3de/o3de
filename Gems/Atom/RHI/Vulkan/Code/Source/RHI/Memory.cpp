/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/parallel/lock.h>
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <RHI/Memory.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Memory> Memory::Create()
        {
            return aznew Memory();
        }

        RHI::ResultCode Memory::Init(Device& device, const Descriptor& descriptor)
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceMemoryProperties& memoryProps = physicalDevice.GetMemoryProperties();
            AZ_Assert(descriptor.m_memoryTypeIndex < memoryProps.memoryTypeCount, "Memory type index is invalid.");
            m_memoryPropertyFlags = memoryProps.memoryTypes[descriptor.m_memoryTypeIndex].propertyFlags;

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = descriptor.m_sizeInBytes;
            allocInfo.memoryTypeIndex = descriptor.m_memoryTypeIndex;

            VkMemoryAllocateFlagsInfo memAllocInfo{};
            if (ShouldApplyDeviceAddressBit(descriptor.m_bufferBindFlags))
            {
                memAllocInfo.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
            }
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
            
            allocInfo.pNext = &memAllocInfo;
            VkDeviceMemory deviceMemory;
            VkResult vkResult = vkAllocateMemory(device.GetNativeDevice(), &allocInfo, nullptr, &deviceMemory);
            AZ_Error(
                "Vulkan",
                vkResult == VK_SUCCESS || vkResult == VK_ERROR_OUT_OF_HOST_MEMORY || vkResult == VK_ERROR_OUT_OF_DEVICE_MEMORY,
                "Error %d while trying to allocate %llu bytes from memory type %d", vkResult, static_cast<unsigned long long>(descriptor.m_sizeInBytes), descriptor.m_memoryTypeIndex);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            Base::Init(device);
            m_descriptor = descriptor;
            m_nativeDeviceMemory = deviceMemory;
            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        const VkDeviceMemory Memory::GetNativeDeviceMemory()
        {
            return m_nativeDeviceMemory;
        }

        const Memory::Descriptor& Memory::GetDescriptor() const
        {
            return m_descriptor;
        }

        CpuVirtualAddress Memory::Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            auto& device = static_cast<Device&>(GetDevice());
            if (!m_mappedMemory)
            {
                VkResult result = vkMapMemory(device.GetNativeDevice(), m_nativeDeviceMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&m_mappedMemory));
                AssertSuccess(result);
                if (result != VK_SUCCESS)
                {
                    return nullptr;
                }
            }

            if (hostAccess == RHI::HostMemoryAccess::Read && 
                !RHI::CheckBitsAny(m_memoryPropertyFlags, static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
            {
                VkMappedMemoryRange readRange = {};
                readRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                readRange.memory = m_nativeDeviceMemory;
                readRange.offset = offset;
                readRange.size = size;
                vkInvalidateMappedMemoryRanges(device.GetNativeDevice(), 1, &readRange);
            }

            m_mappings[offset] = size;
            return m_mappedMemory + offset;
        }

        void Memory::Unmap(size_t offset, RHI::HostMemoryAccess hostAccess)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            AZ_Assert(m_mappedMemory, "Memory is not mapped");
            auto it = m_mappings.find(offset);
            if (it == m_mappings.end())
            {
                AZ_Assert(false, "Trying to unmap an invalid allocation. offset=%x", offset);
                return;
            }

            if (hostAccess == RHI::HostMemoryAccess::Write &&
                !RHI::CheckBitsAny(m_memoryPropertyFlags, static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
            {
                auto& device = static_cast<Device&>(GetDevice());
                VkMappedMemoryRange writeRange = {};
                writeRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                writeRange.memory = m_nativeDeviceMemory;
                writeRange.offset = offset;
                writeRange.size = it->second;
                vkFlushMappedMemoryRanges(device.GetNativeDevice(), 1, &writeRange);
            }

            m_mappings.erase(it);
        }

        void Memory::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeDeviceMemory), name.data(), VK_OBJECT_TYPE_DEVICE_MEMORY, static_cast<Device&>(GetDevice()));
            }
        }

        void Memory::Shutdown()
        {
            Device& device = static_cast<Device&>(GetDevice());
            if (m_mappedMemory)
            {
                AZ_Assert(m_mappings.empty(), "Shutting down and there's still memory mappings left");
                vkUnmapMemory(device.GetNativeDevice(), m_nativeDeviceMemory);
                m_mappedMemory = nullptr;
            }
            
            if (m_nativeDeviceMemory)
            {
                vkFreeMemory(device.GetNativeDevice(), m_nativeDeviceMemory, nullptr);
                m_nativeDeviceMemory = VK_NULL_HANDLE;
            }

            Base::Shutdown();
        }
    }
}
