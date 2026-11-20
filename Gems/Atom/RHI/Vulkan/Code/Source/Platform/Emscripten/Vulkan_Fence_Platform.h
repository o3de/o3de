/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Device.h>
#include <vulkan/vulkan.h>

namespace AZ::Vulkan
{
    static constexpr bool CrossDeviceFencesSupported = false;
    static constexpr const char* ExternalSemaphoreExtensionName = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
    static constexpr auto ExternalSemaphoreHandleTypeBit = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;

    [[maybe_unused]] static RHI::ResultCode ImportCrossDeviceSemaphore(
        [[maybe_unused]] const Device& originalDevice,
        [[maybe_unused]] VkSemaphore originalSemaphore,
        [[maybe_unused]] const Device& destinationDevice,
        [[maybe_unused]] VkSemaphore destinationSemaphore)
    {
        AZ_Assert(false, "Cross Device Fences are not supported on this platform");
        return RHI::ResultCode::Fail;
    }
} // namespace AZ::Vulkan
