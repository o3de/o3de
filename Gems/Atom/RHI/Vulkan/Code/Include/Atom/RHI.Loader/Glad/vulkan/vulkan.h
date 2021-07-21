/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <Atom/RHI.Loader/Glad/Vulkan_Platform.h>
#include <glad/vulkan.h>

#define VK_EXTENSION_SUPPORTED(_Extension) (GLAD_VK_ ## _Extension == 1)
#define VK_INSTANCE_EXTENSION_SUPPORTED(_Extension) VK_EXTENSION_SUPPORTED(_Extension)
#define VK_DEVICE_EXTENSION_SUPPORTED(_Extension) VK_EXTENSION_SUPPORTED(_Extension)
