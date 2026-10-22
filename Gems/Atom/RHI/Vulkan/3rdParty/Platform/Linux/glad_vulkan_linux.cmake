#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB)
    set(GLAD_VULKAN_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_XCB_KHR
    )
endif()

if(PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND)
    list(APPEND GLAD_VULKAN_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_WAYLAND_KHR
    )
endif()
