/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/VulkanMemoryAllocation.h>
#include <RHI/Device.h>

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

        size_t VulkanMemoryAllocation::GetOffset() const
        {
            // Need to get the allocation info in case the offset has changed since initial allocation
            return GetAllocationInfo().offset;
        }

        size_t VulkanMemoryAllocation::GetSize() const
        {
            return m_size;
        }

        size_t VulkanMemoryAllocation::GetBlockSize() const
        {
            auto& device = static_cast<Device&>(GetDevice());
            VmaAllocationInfo2 info;
            vmaGetAllocationInfo2(device.GetVmaAllocator(), m_vmaAllocation, &info);
            return info.blockSize;
        }

        CpuVirtualAddress VulkanMemoryAllocation::Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess)
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

        void VulkanMemoryAllocation::Unmap(size_t offset, RHI::HostMemoryAccess hostAccess)
        {
            Device& device = static_cast<Device&>(GetDevice());
            if (hostAccess == RHI::HostMemoryAccess::Write)
            {
                [[maybe_unused]] VkResult result = vmaFlushAllocation(device.GetVmaAllocator(), m_vmaAllocation, offset, VK_WHOLE_SIZE);
                AZ_Error("RHI", result == VK_SUCCESS, "Failed to flush vma allocations, error = %s", GetResultString(result));
            }

            vmaUnmapMemory(device.GetVmaAllocator(), m_vmaAllocation);
        }

        VmaAllocation VulkanMemoryAllocation::GetVmaAllocation() const
        {
            return m_vmaAllocation;
        }

        VkDeviceMemory VulkanMemoryAllocation::GetNativeDeviceMemory() const
        {
            return GetAllocationInfo().deviceMemory;
        }

        void VulkanMemoryAllocation::SetNameInternal(const AZStd::string_view& name)
        {
            auto& device = static_cast<Device&>(GetDevice());
            vmaSetAllocationName(device.GetVmaAllocator(), m_vmaAllocation, name.data());
        }

        void VulkanMemoryAllocation::Shutdown()
        {
            auto& device = static_cast<Device&>(GetDevice());
            vmaFreeMemory(device.GetVmaAllocator(), m_vmaAllocation);
        }

        VmaAllocationInfo VulkanMemoryAllocation::GetAllocationInfo() const
        {
            auto& device = static_cast<Device&>(GetDevice());
            VmaAllocationInfo info;
            vmaGetAllocationInfo(device.GetVmaAllocator(), m_vmaAllocation, &info);
            return info;
        }
    }
}
