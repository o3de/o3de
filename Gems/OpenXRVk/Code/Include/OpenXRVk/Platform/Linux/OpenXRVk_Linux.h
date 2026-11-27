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
#include <limits.h>

#include <vulkan/vulkan.h>

 // Tell OpenXR what platform code we'll be using
#if defined(PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB)
    #define XR_USE_PLATFORM_XCB
#endif

#if defined(PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND)
    #define XR_USE_PLATFORM_WAYLAND
#endif
#define XR_USE_GRAPHICS_API_VULKAN

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
