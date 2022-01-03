#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (${PAL_TRAIT_LINUX_WINDOW_MANAGER} STREQUAL "xcb")
    set(GLAD_VULKAN_COMPILE_DEFINITIONS 
        VK_USE_PLATFORM_XCB_KHR
    )
elseif(PAL_TRAIT_LINUX_WINDOW_MANAGER STREQUAL "wayland")
    set(GLAD_VULKAN_COMPILE_DEFINITIONS 
        VK_USE_PLATFORM_WAYLAND_KHR
    )
else()
    message(FATAL_ERROR, "Linux Window Manager ${PAL_TRAIT_LINUX_WINDOW_MANAGER} is not recognized")
endif()
