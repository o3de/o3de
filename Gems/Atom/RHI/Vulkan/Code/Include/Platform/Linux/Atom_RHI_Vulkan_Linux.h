/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/algorithm.h>
#include <vulkan/vulkan.h>
#include <limits.h>
#include <RHI/Vulkan.h>

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND && PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/WaylandConnectionManager.h>
inline const char* GetVulkanSurfaceExtensionName()
{
    if (AzFramework::WaylandConnectionManagerInterface::Get())
    {
        return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    }

    return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
}

//Let a function at runtime figure it out
#define AZ_VULKAN_SURFACE_EXTENSION_NAME GetVulkanSurfaceExtensionName()
#else
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#define AZ_VULKAN_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#define AZ_VULKAN_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif

#endif
