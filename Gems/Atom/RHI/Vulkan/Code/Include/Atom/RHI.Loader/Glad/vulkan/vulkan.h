/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once
#include <Atom/RHI.Loader/Glad/Vulkan_Platform.h>
#include <glad/vulkan.h>

#define VK_EXTENSION_SUPPORTED(_Extension) (GLAD_VK_ ## _Extension == 1)
#define VK_INSTANCE_EXTENSION_SUPPORTED(_Extension) VK_EXTENSION_SUPPORTED(_Extension)
#define VK_DEVICE_EXTENSION_SUPPORTED(_Extension) VK_EXTENSION_SUPPORTED(_Extension)
