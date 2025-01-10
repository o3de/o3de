/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/PhysicalDevice.h>

#include <vulkan/vulkan.h>

namespace AZ
{
    namespace Vulkan
    {
        //! It is important to note that the usage of these functions requires care from the user.
        //! This includes:
        //! - Synchronizing with the renderer by waiting on a fence from the FrameGraph before
        //!   starting execution as well as signaling the FrameGraph to continue execution
        //! - Leaving the GPU in a valid state
        //! - Returning resources in a valid state

        //! Provide access to native device handles
        VkDevice GetDeviceNativeHandle(RHI::Device& device);
        VkPhysicalDevice GetPhysicalDeviceNativeHandle(const RHI::PhysicalDevice& device);

        //! Provide access to native fence handle and value
        VkSemaphore GetFenceNativeHandle(RHI::DeviceFence& fence);
        uint64_t GetFencePendingValue(RHI::DeviceFence& fence);

        //! Provide access to native VkBuffer, VKDeviceMemory as well as size and offset
        VkBuffer GetNativeBuffer(RHI::DeviceBuffer& buffer);
        VkDeviceMemory GetBufferMemory(RHI::DeviceBuffer& buffer);
        size_t GetBufferMemoryViewSize(RHI::DeviceBuffer& buffer);
        size_t GetBufferAllocationSize(RHI::DeviceBuffer& buffer);
        size_t GetBufferAllocationOffset(RHI::DeviceBuffer& buffer);

        //! Provide access to native VkImage, VKDeviceMemory as well as size and offset
        VkImage GetNativeImage(RHI::DeviceImage& image);
        VkDeviceMemory GetImageMemory(RHI::DeviceImage& image);
        size_t GetImageMemoryViewSize(RHI::DeviceImage& image);
        size_t GetImageAllocationSize(RHI::DeviceImage& image);
        size_t GetImageAllocationOffset(RHI::DeviceImage& image);
    } // namespace Vulkan
} // namespace AZ
