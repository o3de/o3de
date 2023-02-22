/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <vulkan/vulkan.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class DynamicModuleHandle;

    namespace Vulkan
    {
        class FunctionLoader
        {
        public:
            AZ_CLASS_ALLOCATOR(FunctionLoader, AZ::SystemAllocator);

            static AZStd::unique_ptr<FunctionLoader> Create();

            bool Init();
            void Shutdown();

            ~FunctionLoader();

            bool LoadProcAddresses(GladVulkanContext* context, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

        private:
            void ShutdownInternal();
        };
    } // namespace Vulkan
} // namespace AZ
