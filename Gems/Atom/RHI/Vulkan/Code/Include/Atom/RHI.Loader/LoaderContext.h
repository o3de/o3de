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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Utility class for loading the Vulkan functions pointer using GLAD.
        class LoaderContext
        {
        public:
            //! Parameters for initializing the function loader
            struct Descriptor
            {
                VkInstance m_instance = VK_NULL_HANDLE;
                VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
                VkDevice m_device = VK_NULL_HANDLE;
                AZStd::vector<const char*> m_loadedLayers;
                AZStd::vector<const char*> m_loadedExtensions;
            };

            //! Creates a new instance of a loader context.
            static AZStd::unique_ptr<LoaderContext> Create();

            ~LoaderContext();

            //! Loads the function pointer using the instance, device and physical device provided.
            bool Init(const Descriptor& descriptor);
            //! Shutdown the glad loader context.
            void Shutdown();
            //! Returns a list of available instance layers.
            AZStd::vector<AZStd::string> GetInstanceLayerNames() const;
            //! Returns a list of available instance extensions.
            AZStd::vector<AZStd::string> GetInstanceExtensionNames(const char* layerName = nullptr) const;
            //! Returns the GLAD context with the loaded function pointers.
            GladVulkanContext& GetContext();
            //! Returns the constant GLAD context with the loaded function pointers.
            const GladVulkanContext& GetContext() const;

        private:
            LoaderContext() = default;

            //! Loads function pointers from the dynamic library directly (no trampoline).
            bool Preload();
            //! Loads extensions from a layer.
            void LoadLayerExtensions(const Descriptor& descriptor);
            //! Removes layers that were not loaded correctly.
            void FilterAvailableExtensions(const VkDevice device = VK_NULL_HANDLE);

            GladVulkanContext m_context = {};
        };
    } // namespace Vulkan
} // namespace AZ
