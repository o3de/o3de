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
    static constexpr bool CrossDeviceFencesSupported = true;
    static constexpr auto ExternalSemaphoreHandleTypeBit = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    static constexpr const char* ExternalSemaphoreExtensionName = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;

    [[maybe_unused]] static RHI::ResultCode ImportCrossDeviceSemaphore(
        const Device& originalDevice, VkSemaphore originalSemaphore, const Device& destinationDevice, VkSemaphore destinationSemaphore)
    {
        int fd;
        VkSemaphoreGetFdInfoKHR semaphoreGetFdInfoKHR = {};
        semaphoreGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
        semaphoreGetFdInfoKHR.pNext = NULL;
        semaphoreGetFdInfoKHR.semaphore = originalSemaphore;
        semaphoreGetFdInfoKHR.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
        [[maybe_unused]] const VkResult error = originalDevice.GetContext().GetSemaphoreFdKHR(originalDevice.GetNativeDevice(), &semaphoreGetFdInfoKHR, &fd);
        AZ_Assert(error == VK_SUCCESS, "Could not retrieve semaphore handle");

        VkImportSemaphoreFdInfoKHR importInfo{};
        importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
        importInfo.pNext = nullptr;
        importInfo.semaphore = destinationSemaphore;
        importInfo.flags = 0;
        importInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
        importInfo.fd = fd;
        auto result = destinationDevice.GetContext().ImportSemaphoreFdKHR(destinationDevice.GetNativeDevice(), &importInfo);
        return ConvertResult(result);
    }

} // namespace AZ::Vulkan
