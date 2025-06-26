/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/VulkanMemoryAllocation.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<VulkanMemoryAllocation> VulkanMemoryAllocation::Create()
        {
            return aznew VulkanMemoryAllocation();
        }

        void VulkanMemoryAllocation::Init(Device& device, const VmaAllocation& alloc)
        {
            RHI::DeviceObject::Init(device);
            m_vmaAllocation = alloc;
            m_size = GetAllocationInfo().size;
        }

        void VulkanMemoryAllocation::Init(Device& device, const VkDeviceMemory memory, size_t size)
        {
            RHI::DeviceObject::Init(device);
            m_memory = memory;
            m_size = size;
        }

        size_t VulkanMemoryAllocation::GetOffset() const
        {
            if (m_vmaAllocation)
            {
                // Need to get the allocation info in case the offset has changed since initial allocation
                return GetAllocationInfo().offset;
            }
            else
            {
                return 0;
            }
        }

        size_t VulkanMemoryAllocation::GetSize() const
        {
            return m_size;
        }

        size_t VulkanMemoryAllocation::GetBlockSize() const
        {
            if (m_vmaAllocation)
            {
                auto& device = static_cast<Device&>(GetDevice());
                VmaAllocationInfo2 info;
                vmaGetAllocationInfo2(device.GetVmaAllocator(), m_vmaAllocation, &info);
                return info.blockSize;
            }
            else
            {
                return m_size;
            }
        }

        CpuVirtualAddress VulkanMemoryAllocation::Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess)
        {
            if (m_vmaAllocation)
            {
                Device& device = static_cast<Device&>(GetDevice());
                void* mappedPtr;
                VkResult vkResult = vmaMapMemory(device.GetVmaAllocator(), m_vmaAllocation, &mappedPtr);
                if (vkResult != VK_SUCCESS)
                {
                    AZ_Error("RHI", false, "Failed to map vma buffer, error = %s", GetResultString(vkResult));
                    return nullptr;
                }

                if (hostAccess == RHI::HostMemoryAccess::Read)
                {
                    // VMA will check if this is necessary
                    vkResult = vmaInvalidateAllocation(device.GetVmaAllocator(), m_vmaAllocation, offset, size);
                    if (vkResult != VK_SUCCESS)
                    {
                        AZ_Error("RHI", false, "Failed to InvalidateAllocation vma buffer, error = %s", GetResultString(vkResult));
                        return nullptr;
                    }
                }

                return static_cast<CpuVirtualAddress>(mappedPtr) + offset;
            }
            else
            {
                void* mappedPtr;
                auto& device = static_cast<Device&>(GetDevice());
                auto vkResult = device.GetContext().MapMemory(device.GetNativeDevice(), m_memory, offset, size, 0, &mappedPtr);
                if (vkResult != VK_SUCCESS)
                {
                    AZ_Error("RHI", false, "Failed to map vulkan memory, error = %s", GetResultString(vkResult));
                    return nullptr;
                }
                return static_cast<CpuVirtualAddress>(mappedPtr);
            }
        }

        void VulkanMemoryAllocation::Unmap(size_t offset, RHI::HostMemoryAccess hostAccess)
        {
            if (m_vmaAllocation)
            {
                Device& device = static_cast<Device&>(GetDevice());
                if (hostAccess == RHI::HostMemoryAccess::Write)
                {
                    [[maybe_unused]] VkResult result = vmaFlushAllocation(device.GetVmaAllocator(), m_vmaAllocation, offset, VK_WHOLE_SIZE);
                    AZ_Error("RHI", result == VK_SUCCESS, "Failed to flush vma allocations, error = %s", GetResultString(result));
                }

                vmaUnmapMemory(device.GetVmaAllocator(), m_vmaAllocation);
            }
            else
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().UnmapMemory(device.GetNativeDevice(), m_memory);
            }
        }

        VmaAllocation VulkanMemoryAllocation::GetVmaAllocation() const
        {
            return m_vmaAllocation;
        }

        VkDeviceMemory VulkanMemoryAllocation::GetNativeDeviceMemory() const
        {
            if (m_vmaAllocation)
            {
                return GetAllocationInfo().deviceMemory;
            }
            else
            {
                return m_memory;
            }
        }

        void VulkanMemoryAllocation::SetNameInternal(const AZStd::string_view& name)
        {
            if (m_vmaAllocation)
            {
                auto& device = static_cast<Device&>(GetDevice());
                vmaSetAllocationName(device.GetVmaAllocator(), m_vmaAllocation, name.data());
            }
        }

        void VulkanMemoryAllocation::Shutdown()
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (m_vmaAllocation)
            {
                vmaFreeMemory(device.GetVmaAllocator(), m_vmaAllocation);
            }
            else
            {
                device.GetContext().FreeMemory(device.GetNativeDevice(), m_memory, nullptr);
            }
        }

        VmaAllocationInfo VulkanMemoryAllocation::GetAllocationInfo() const
        {
            AZ_Assert(m_vmaAllocation, "Cannot get allocation information from a non-vma allocation");
            auto& device = static_cast<Device&>(GetDevice());
            VmaAllocationInfo info;
            vmaGetAllocationInfo(device.GetVmaAllocator(), m_vmaAllocation, &info);
            return info;
        }
    }
}
