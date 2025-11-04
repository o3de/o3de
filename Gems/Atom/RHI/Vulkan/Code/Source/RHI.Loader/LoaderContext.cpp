/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#define GLAD_VULKAN_IMPLEMENTATION
#include <vulkan/vulkan.h>

#include <Atom/RHI.Loader/LoaderContext.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Vulkan
    {
        AZStd::unique_ptr<LoaderContext> LoaderContext::Create()
        {
            AZStd::unique_ptr<LoaderContext> loader(aznew LoaderContext());
            if (!loader->Preload())
            {
                return nullptr;
            }
            return loader;
        }

        LoaderContext::~LoaderContext()
        {
            Shutdown();
        }

        bool LoaderContext::Init(const LoaderContext::Descriptor& descriptor)
        {
            VkInstance instance = descriptor.m_instance;
            int result = gladLoaderLoadVulkanContext(&m_context, instance, descriptor.m_physicalDevice, descriptor.m_device);
            if (instance != VK_NULL_HANDLE)
            {
                LoadLayerExtensions(descriptor);
            }
            FilterAvailableExtensions(descriptor.m_device);
            return result != 0;
        }

        void LoaderContext::Shutdown()
        {
            gladLoaderUnloadVulkanContext(&m_context);
        }

        bool LoaderContext::Preload()
        {
            // Load functions from the dynamic library until we have a Vulkan instance or device.
            int result = gladLoaderLoadVulkanContext(&m_context, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
            FilterAvailableExtensions();
            return result != 0;
        }

        void LoaderContext::LoadLayerExtensions(const LoaderContext::Descriptor& descriptor)
        {
            // GLAD doesn't support loading extensions from layers yet, only from the driver.
            // On some platforms (e.g. Android) the EXT_debug_utils extension (we use it for receiving validation messages) is provided by
            // the validation layer instead of the driver. Because of this we manually load the function pointers for the EXT_debug_utils
            // extension (after checking that the extension was loaded by the Vulkan instance).
            VkInstance vkInstance = descriptor.m_instance;
            // Check if the EXT_debug_utils function pointers were already loaded from the driver.
            if (!VK_INSTANCE_EXTENSION_SUPPORTED(m_context, EXT_debug_utils))
            {
                // Check if the EXT_debug_utils extension was loaded when creating the VkInstance
                const AZStd::vector<const char*>& loadedExtensions = descriptor.m_loadedExtensions;
                auto findExtensionIter = AZStd::find_if(
                    loadedExtensions.begin(),
                    loadedExtensions.end(),
                    [](const auto& extension)
                    {
                        return AZ::StringFunc::Equal(extension, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                    });

                if (findExtensionIter != loadedExtensions.end())
                {
                    // EXT_debug_utils extension is loaded, so now we look for the layer that is providing the extension (since it was not
                    // provided by the driver).
                    const AZStd::vector<const char*>& loadedLayers = descriptor.m_loadedLayers;
                    auto findLayerIter = AZStd::find_if(
                        loadedLayers.begin(),
                        loadedLayers.end(),
                        [&](const auto& layer)
                        {
                            auto instanceExtensions = GetInstanceExtensionNames(layer);
                            return AZStd::find(instanceExtensions.begin(), instanceExtensions.end(), *findExtensionIter) !=
                                instanceExtensions.end();
                        });

                    if (findLayerIter != loadedLayers.end())
                    {
                        // Extension is loaded and provided by a layer that is also loaded.
                        // We load the function pointers for the EXT_debug_utils extension manually.
                        m_context.EXT_debug_utils = 1;

                        if (vkInstance != VK_NULL_HANDLE)
                        {
                            m_context.CreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                                m_context.GetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT"));
                            m_context.DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                                m_context.GetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT"));
                            m_context.SubmitDebugUtilsMessageEXT = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(
                                m_context.GetInstanceProcAddr(vkInstance, "vkSubmitDebugUtilsMessageEXT"));
                        }

                        VkDevice device = descriptor.m_device;
                        if (device != VK_NULL_HANDLE)
                        {
                            m_context.CmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
                                m_context.GetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));
                            m_context.CmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
                                m_context.GetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));
                            m_context.CmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
                                m_context.GetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT"));
                            m_context.QueueBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(
                                m_context.GetDeviceProcAddr(device, "vkQueueBeginDebugUtilsLabelEXT"));
                            m_context.QueueEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(
                                m_context.GetDeviceProcAddr(device, "vkQueueEndDebugUtilsLabelEXT"));
                            m_context.QueueInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(
                                m_context.GetDeviceProcAddr(device, "vkQueueInsertDebugUtilsLabelEXT"));
                            m_context.SetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                                m_context.GetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
                            m_context.SetDebugUtilsObjectTagEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(
                                m_context.GetDeviceProcAddr(device, "vkSetDebugUtilsObjectTagEXT"));
                        }
                    }
                }
            }
        }

        AZStd::vector<AZStd::string> LoaderContext::GetInstanceLayerNames() const
        {
            AZStd::vector<AZStd::string> layerNames;
            uint32_t layerPropertyCount = 0;
            VkResult result = m_context.EnumerateInstanceLayerProperties(&layerPropertyCount, nullptr);
            if (result != VK_SUCCESS || layerPropertyCount == 0)
            {
                return layerNames;
            }

            AZStd::vector<VkLayerProperties> layerProperties(layerPropertyCount);
            result = m_context.EnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data());
            if (result != VK_SUCCESS)
            {
                return layerNames;
            }

            layerNames.reserve(layerNames.size() + layerProperties.size());
            for (uint32_t layerPropertyIndex = 0; layerPropertyIndex < layerPropertyCount; ++layerPropertyIndex)
            {
                layerNames.emplace_back(layerProperties[layerPropertyIndex].layerName);
            }

            return layerNames;
        }   

        AZStd::vector<AZStd::string> LoaderContext::GetInstanceExtensionNames(const char* layerName /*= nullptr*/) const
        {
            AZStd::vector<AZStd::string> extensionNames;
            uint32_t extPropertyCount = 0;
            VkResult result = m_context.EnumerateInstanceExtensionProperties(layerName, &extPropertyCount, nullptr);
            if (result != VK_SUCCESS || extPropertyCount == 0)
            {
                return extensionNames;
            }

            AZStd::vector<VkExtensionProperties> extProperties;
            extProperties.resize(extPropertyCount);

            result = m_context.EnumerateInstanceExtensionProperties(layerName, &extPropertyCount, extProperties.data());
            if (result != VK_SUCCESS)
            {
                return extensionNames;
            }

            extensionNames.reserve(extensionNames.size() + extProperties.size());
            for (uint32_t extPropertyIndex = 0; extPropertyIndex < extPropertyCount; extPropertyIndex++)
            {
                extensionNames.emplace_back(extProperties[extPropertyIndex].extensionName);
            }

            return extensionNames;
        }

        GladVulkanContext& LoaderContext::GetContext()
        {
            return m_context;
        }

        const GladVulkanContext& LoaderContext::GetContext() const
        {
            return m_context;
        }

        void LoaderContext::FilterAvailableExtensions(const VkDevice device)
        {
            // In some cases (like when running with the GPU profiler on Quest2) the extension is reported as available
            // but the function pointers do not load. Disable the extension if that's the case.
            if (device != VK_NULL_HANDLE &&
                m_context.EXT_debug_utils &&
                !m_context.CmdBeginDebugUtilsLabelEXT)
            {
                m_context.EXT_debug_utils = 0;
            }
        }

    } // namespace Vulkan
} // namespace AZ
