/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Vulkan.h>
#include <RHI/Instance.h>
#include <RHI/FunctionLoader.h>

#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Vulkan
    {
        void LoadLayerExtensions(GladVulkanContext* context, VkDevice device)
        {
            // GLAD doesn't support loading extensions from layers yet, only from the driver.
            // On some platforms (e.g. Android) the EXT_debug_utils extension (we use it for receiving validation messages) is provided by
            // the validation layer instead of the driver. Because of this we manually load the function pointers for the EXT_debug_utils
            // extension (after checking that the extension was loaded by the Vulkan instance).
            auto& instance = Instance::GetInstance();
            VkInstance vkInstance = instance.GetNativeInstance();
            // Check if the EXT_debug_utils function pointers were already loaded from the driver.
            if (!VK_INSTANCE_EXTENSION_SUPPORTED((*context), EXT_debug_utils))
            {
                // Check if the EXT_debug_utils extension was loaded when creating the VkInstance
                const RawStringList& loadedExtensions = instance.GetLoadedExtensions();
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
                    const RawStringList& loadedLayers = instance.GetLoadedLayers();
                    auto findLayerIter = AZStd::find_if(
                        loadedLayers.begin(),
                        loadedLayers.end(),
                        [&](const auto& layer)
                        {
                            StringList instanceExtensions = instance.GetInstanceExtensionNames(layer);
                            return AZStd::find(instanceExtensions.begin(), instanceExtensions.end(), *findExtensionIter) !=
                                instanceExtensions.end();
                        });

                    if (findLayerIter != loadedLayers.end())
                    {
                        // Extension is loaded and provided by a layer that is also loaded.
                        // We load the function pointers for the EXT_debug_utils extension manually.
                        context->EXT_debug_utils = 1;

                        if (vkInstance != VK_NULL_HANDLE)
                        {
                            context->CreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                                context->GetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT"));
                            context->DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                                context->GetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT"));
                            context->SubmitDebugUtilsMessageEXT = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(
                                context->GetInstanceProcAddr(vkInstance, "vkSubmitDebugUtilsMessageEXT"));
                        }

                        if (device != VK_NULL_HANDLE)
                        {
                            context->CmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
                                context->GetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));
                            context->CmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
                                context->GetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));
                            context->CmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
                                context->GetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT"));
                            context->QueueBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(
                                context->GetDeviceProcAddr(device, "vkQueueBeginDebugUtilsLabelEXT"));
                            context->QueueEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(
                                context->GetDeviceProcAddr(device, "vkQueueEndDebugUtilsLabelEXT"));
                            context->QueueInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(
                                context->GetDeviceProcAddr(device, "vkQueueInsertDebugUtilsLabelEXT"));
                            context->SetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                                context->GetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
                            context->SetDebugUtilsObjectTagEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(
                                context->GetDeviceProcAddr(device, "vkSetDebugUtilsObjectTagEXT"));
                        }
                    }
                }
            }
        }

        bool FunctionLoader::LoadProcAddresses(GladVulkanContext* context, VkPhysicalDevice physicalDevice, VkDevice device)
        {
            VkInstance instance = Instance::GetInstance().GetNativeInstance();
            int result = gladLoaderLoadVulkanContext(context, instance, physicalDevice, device);
            if (instance != VK_NULL_HANDLE)
            {
                LoadLayerExtensions(context, device);
            }
            return result != 0;
        }

        void FunctionLoader::UnloadContext(GladVulkanContext* context)
        {
            gladLoaderUnloadVulkanContext(context);
        }
    } // namespace Vulkan
} // namespace AZ
