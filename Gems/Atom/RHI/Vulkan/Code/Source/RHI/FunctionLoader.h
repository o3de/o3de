/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Utility functions to load/unload Vulkan function pointers using GLAD
        namespace FunctionLoader
        {
            //! Load the function pointers into the context. If device is NULL, only instance functions pointers are loaded.
            //! If instance is not yet created, function pointers are loaded from the dynamic lybrary directly.
            bool LoadProcAddresses(GladVulkanContext* context, VkPhysicalDevice physicalDevice, VkDevice device);

            //! Unload resources used by the GladVulkanContext
            void UnloadContext(GladVulkanContext* context);
        };
    } // namespace Vulkan
} // namespace AZ
