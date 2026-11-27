/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVk/OpenXRVkPhysicalDevice.h>
#include <OpenXRVk/OpenXRVkUtils.h>

namespace OpenXRVk::PhysicalDevice
{
    AZStd::vector<VkPhysicalDevice> EnumerateDeviceList(XrSystemId xrSystemId, XrInstance xrInstance, VkInstance vkInstance)
    {
        PFN_xrGetVulkanGraphicsDeviceKHR pfnGetVulkanGraphicsDeviceKHR = nullptr;
        XrResult result = xrGetInstanceProcAddr(
            xrInstance, "xrGetVulkanGraphicsDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsDeviceKHR));
        ASSERT_IF_UNSUCCESSFUL(result);

        //TODO::Look into api that can retreive multiple physical devices if connected
        VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
        result = pfnGetVulkanGraphicsDeviceKHR(xrInstance, xrSystemId, vkInstance, &vkPhysicalDevice);
        ASSERT_IF_UNSUCCESSFUL(result);

        return {vkPhysicalDevice};
    }
}
