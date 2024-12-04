/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Interface/Vulkan/RHIVulkanInterface.h>

#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/Image.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/TimelineSemaphoreFence.h>

namespace AZ
{
    namespace Vulkan
    {
        VkDevice GetDeviceNativeHandle(RHI::Device& device)
        {
            AZ_Assert(azrtti_cast<Device*>(&device), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            return static_cast<Device&>(device).GetNativeDevice();
        }

        VkPhysicalDevice GetPhysicalDeviceNativeHandle(const RHI::PhysicalDevice& device)
        {
            AZ_Assert(azrtti_cast<const PhysicalDevice*>(&device), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            return static_cast<const PhysicalDevice&>(device).GetNativePhysicalDevice();
        }

        VkSemaphore GetFenceNativeHandle(RHI::DeviceFence& fence)
        {
            AZ_Assert(azrtti_cast<Fence*>(&fence), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            auto& vulkanFence = static_cast<Fence&>(fence);
            auto timelineSemaphoreFence = azrtti_cast<TimelineSemaphoreFence*>(&vulkanFence.GetFenceBase());
            AZ_Assert(timelineSemaphoreFence, "Cannot return a VkSemaphore from a binary fence");
            return timelineSemaphoreFence->GetNativeSemaphore();
        }

        uint64_t GetFencePendingValue(RHI::DeviceFence& fence)
        {
            AZ_Assert(azrtti_cast<Fence*>(&fence), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            auto& vulkanFence = static_cast<Fence&>(fence);
            auto timelineSemaphoreFence = azrtti_cast<TimelineSemaphoreFence*>(&vulkanFence.GetFenceBase());
            AZ_Assert(timelineSemaphoreFence, "Cannot return a VkSemaphore from a binary fence");
            return timelineSemaphoreFence->GetPendingValue();
        }

        VkBuffer GetNativeBuffer(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            return static_cast<Buffer&>(buffer).GetBufferMemoryView()->GetNativeBuffer();
        }

        VkDeviceMemory GetBufferMemory(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            return static_cast<Buffer&>(buffer).GetBufferMemoryView()->GetNativeDeviceMemory();
        }

        size_t GetBufferMemoryViewSize(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            return static_cast<Buffer&>(buffer).GetBufferMemoryView()->GetSize();
        }

        size_t GetBufferAllocationSize(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            auto& vulkanBuffer = static_cast<Buffer&>(buffer);
            if (vulkanBuffer.GetBufferMemoryView()->GetAllocation()->GetMemoryView().GetAllocation()->GetVmaAllocation())
            {
                return vulkanBuffer.GetBufferMemoryView()->GetAllocation()->GetMemoryView().GetAllocation()->GetBlockSize();
            }
            else
            {
                return vulkanBuffer.GetBufferMemoryView()->GetAllocation()->GetAllocationSize();
            }
        }

        size_t GetBufferAllocationOffset(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            auto& vulkanBuffer = static_cast<Buffer&>(buffer);
            if (vulkanBuffer.GetBufferMemoryView()->GetAllocation()->GetMemoryView().GetAllocation()->GetVmaAllocation())
            {
                return vulkanBuffer.GetBufferMemoryView()->GetAllocation()->GetMemoryView().GetAllocation()->GetOffset() +
                    vulkanBuffer.GetBufferMemoryView()->GetAllocation()->GetMemoryViewOffset() +
                    vulkanBuffer.GetBufferMemoryView()->GetOffset();
            }
            else
            {
                return vulkanBuffer.GetBufferMemoryView()->GetAllocation()->GetMemoryViewOffset() +
                    vulkanBuffer.GetBufferMemoryView()->GetOffset();
            }
        }

        VkImage GetNativeImage(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            return static_cast<Image&>(image).GetNativeImage();
        }

        VkDeviceMemory GetImageMemory(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            return static_cast<Image&>(image).GetMemoryView().GetNativeDeviceMemory();
        }

        size_t GetImageMemoryViewSize(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            return static_cast<Image&>(image).GetMemoryView().GetSize();
        }

        size_t GetImageAllocationSize(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            auto& vulkanImage = static_cast<Image&>(image);
            if (vulkanImage.GetMemoryView().GetAllocation()->GetVmaAllocation())
            {
                return vulkanImage.GetMemoryView().GetAllocation()->GetBlockSize();
            }
            else
            {
                return vulkanImage.GetMemoryView().GetAllocation()->GetSize();
            }
        }

        size_t GetImageAllocationOffset(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a Vulkan RHI object", __FUNCTION__);
            auto& vulkanImage = static_cast<Image&>(image);
            return vulkanImage.GetMemoryView().GetAllocation()->GetOffset() + vulkanImage.GetMemoryView().GetOffset();
        }
    } // namespace Vulkan
} // namespace AZ
