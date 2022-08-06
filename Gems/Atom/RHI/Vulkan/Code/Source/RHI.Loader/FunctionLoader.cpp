/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#define GLAD_VULKAN_IMPLEMENTATION
#include <vulkan/vulkan.h>

#include <Atom/RHI.Loader/FunctionLoader.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/std/containers/vector.h>
#include <Vulkan_Traits_Platform.h>

namespace AZ
{
    namespace Vulkan
    {
        AZStd::unique_ptr<FunctionLoader> FunctionLoader::Create()
        {
            return AZStd::make_unique<FunctionLoader>();
        }

        bool FunctionLoader::Init()
        {
            return true;
        }

        void FunctionLoader::Shutdown()
        {
            ShutdownInternal();
        }

        FunctionLoader::~FunctionLoader()
        {
        }

        bool FunctionLoader::LoadProcAddresses(GladVulkanContext* context, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
        {
            return gladLoaderLoadVulkanContext(context, instance, physicalDevice, device) != 0;
        }

        void FunctionLoader::ShutdownInternal()
        {
            gladLoaderUnloadVulkan();
        }
    } // namespace Vulkan
} // namespace AZ
