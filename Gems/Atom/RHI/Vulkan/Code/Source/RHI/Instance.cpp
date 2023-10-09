/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Vulkan.h>
#include <RHI/Device.h>
#include <RHI/Instance.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI.Reflect/Vulkan/VulkanBus.h>
#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Utils/Utils.h>

namespace AZ
{
    namespace Vulkan
    {
        static const uint32_t s_minVulkanSupportedVersion = VK_API_VERSION_1_0;

        static EnvironmentVariable<Instance> s_vulkanInstance;
        static constexpr const char* s_vulkanInstanceKey = "VulkanInstance";
        static const RawStringList s_emptyRawList = {};

        Instance& Instance::GetInstance()
        {
            if (!s_vulkanInstance)
            {
                s_vulkanInstance = Environment::FindVariable<Instance>(s_vulkanInstanceKey);
                if (!s_vulkanInstance)
                {
                    s_vulkanInstance = Environment::CreateVariable<Instance>(s_vulkanInstanceKey);
                }
            }

            return s_vulkanInstance.Get();
        }

        void Instance::Reset()
        {
            s_vulkanInstance.Reset();
        }

        Instance::~Instance()
        {
            Shutdown();
        }
        
        bool Instance::Init(const Descriptor& descriptor)
        {
            m_loaderContext = LoaderContext::Create();
            if (!m_loaderContext)
            {
                return false;
            }
#if defined(USE_NSIGHT_AFTERMATH)
            m_gpuCrashHandler.EnableGPUCrashDumps();
#endif

            m_descriptor = descriptor;
            const RHI::ValidationMode validation = GetValidationMode();
            if (validation != RHI::ValidationMode::Disabled)
            {
                char exeDirectory[AZ_MAX_PATH_LEN];
                AZ::Utils::GetExecutableDirectory(exeDirectory, AZ_ARRAY_SIZE(exeDirectory));

                //This env var (VK_LAYER_PATH) is used by the drivers to look for VkLayer_khronos_validation.dll
                AZ::Utils::SetEnv("VK_LAYER_PATH", exeDirectory, 1);

                RawStringList validationLayers = Debug::GetValidationLayers();
                m_descriptor.m_optionalLayers.insert(m_descriptor.m_optionalLayers.end(), validationLayers.begin(), validationLayers.end());
                RawStringList validationExtension = Debug::GetValidationExtensions();
                m_descriptor.m_optionalExtensions.insert(m_descriptor.m_optionalExtensions.end(), validationExtension.begin(), validationExtension.end());
            }
#if defined(AZ_VULKAN_USE_DEBUG_LABELS)
            m_descriptor.m_optionalExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
            
            uint32_t apiVersion = VK_API_VERSION_1_0;
            // vkEnumerateInstanceVersion is a Vulkan 1.1 function 
            // so if it's not available we assume Vulkan 1.0
            if (GetContext().EnumerateInstanceVersion && GetContext().EnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
            {
                AZ_Warning("Vulkan", false, "Failed to get instance version.");
                return false;
            }

            AZStd::vector<uint32_t> minVersions = { s_minVulkanSupportedVersion };
            AZStd::vector<uint32_t> maxVersions = { apiVersion };
            InstanceRequirementBus::Broadcast(&InstanceRequirementBus::Events::CollectMinMaxVulkanAPIVersions, minVersions, maxVersions);
            uint32_t minVersion = *AZStd::minmax_element(minVersions.begin(), minVersions.end()).second;
            uint32_t maxVersion = *AZStd::minmax_element(maxVersions.begin(), maxVersions.end()).first;
            if (apiVersion < minVersion)
            {
                AZ_Warning(
                    "Vulkan", 
                    false, 
                    "The current Vulkan version (%d.%d.%d) is lower that the minimun required (%d.%d.%d).",
                    VK_VERSION_MAJOR(apiVersion),
                    VK_VERSION_MINOR(apiVersion),
                    VK_VERSION_PATCH(apiVersion),
                    VK_VERSION_MAJOR(minVersion),
                    VK_VERSION_MINOR(minVersion),
                    VK_VERSION_PATCH(minVersion));

                return false;
            }                

            if (apiVersion > maxVersion)
            {
                // The max API version is the the maximum Vulkan Instance API version that the runtime has been tested on and is known to support.
                // Newer Vulkan Instance API versions might work if they are compatible.
                AZ_Warning(
                    "Vulkan",
                    false,
                    "The current Vulkan version (%d.%d.%d) is higher than the maximun tested version (%d.%d.%d).",
                    VK_VERSION_MAJOR(apiVersion),
                    VK_VERSION_MINOR(apiVersion),
                    VK_VERSION_PATCH(apiVersion),
                    VK_VERSION_MAJOR(maxVersion),
                    VK_VERSION_MINOR(maxVersion),
                    VK_VERSION_PATCH(maxVersion));
            }

            AZStd::vector<AZStd::string> collectedExtensions;
            InstanceRequirementBus::Broadcast(&InstanceRequirementBus::Events::CollectAdditionalRequiredInstanceExtensions, collectedExtensions);
            for (const auto& extension : collectedExtensions)
            {
                m_descriptor.m_requiredExtensions.push_back(extension.c_str());
            }
            
            m_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            m_appInfo.apiVersion = apiVersion;

            m_instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            m_instanceCreateInfo.pApplicationInfo = &m_appInfo;

            StringList instanceLayerNames = m_loaderContext->GetInstanceLayerNames();
            RawStringList optionalLayers = FilterList(m_descriptor.m_optionalLayers, instanceLayerNames);
            m_descriptor.m_requiredLayers.insert(m_descriptor.m_requiredLayers.end(), optionalLayers.begin(), optionalLayers.end());

            StringList instanceExtensions = m_loaderContext->GetInstanceExtensionNames();
            // Add the extensions provided by layers
            for (const auto& extension : m_descriptor.m_requiredLayers)
            {
                StringList layerExtensions = m_loaderContext->GetInstanceExtensionNames(extension);
                instanceExtensions.insert(instanceExtensions.end(), layerExtensions.begin(), layerExtensions.end());
            }

            RawStringList optionalExtensions = FilterList(m_descriptor.m_optionalExtensions, instanceExtensions);
            m_descriptor.m_requiredExtensions.insert(m_descriptor.m_requiredExtensions.end(), optionalExtensions.begin(), optionalExtensions.end());

            m_instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_descriptor.m_requiredLayers.size());
            m_instanceCreateInfo.ppEnabledLayerNames = m_descriptor.m_requiredLayers.data();
            m_instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_descriptor.m_requiredExtensions.size());
            m_instanceCreateInfo.ppEnabledExtensionNames = m_descriptor.m_requiredExtensions.data();

            // For instance creation/destruction use nullptr for VkAllocationCallbacks* as using VkSystemAllocator::Get() crashes RenderDoc when used with openxr
            // enabled projects. We think it's because RenderDoc is maybe injecting something when doing allocations. Using nullptr when USE_RENDERDOC or enableRenderDoc
            // is enabled is another option but it will not work for Android easily and will require further work, not to mention manually enabling this for Android RenderDoc.
            VkResult result = GetContext().CreateInstance(&m_instanceCreateInfo, nullptr, &m_instance);

            if (validation != RHI::ValidationMode::Disabled &&
                (result == VK_ERROR_LAYER_NOT_PRESENT || result == VK_ERROR_EXTENSION_NOT_PRESENT))
            {
                // Remove all validation layers and extensions and try again to create the Vulkan instance.
                // Some drivers report the validation layers as available but they fail to load them when creating the instance.
                AZ_Warning("Vulkan", false, R"(Disabling validation due to Instance creation failure. Error = "%s".)", GetResultString(result));
                RawStringList validationLayers = Debug::GetValidationLayers();
                RemoveRawStringList(m_descriptor.m_requiredLayers, validationLayers);
                RawStringList validationExtension = Debug::GetValidationExtensions();
                RemoveRawStringList(m_descriptor.m_requiredExtensions, validationExtension);

                m_descriptor.m_validationMode = RHI::ValidationMode::Disabled;
                m_instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_descriptor.m_requiredLayers.size());
                m_instanceCreateInfo.ppEnabledLayerNames = m_descriptor.m_requiredLayers.data();
                m_instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_descriptor.m_requiredExtensions.size());
                m_instanceCreateInfo.ppEnabledExtensionNames = m_descriptor.m_requiredExtensions.data();

                result = GetContext().CreateInstance(&m_instanceCreateInfo, nullptr, &m_instance);
            }

            if (result != VK_SUCCESS)
            {
                AZ_Warning("Vulkan", false, R"(Failed to create Vulkan instance. Error = "%s")", GetResultString(result));
                return false;
            }
            InstanceNotificationBus::Broadcast(&InstanceNotificationBus::Events::OnInstanceCreated, m_instance);

            // Now that we have created the instance, load the function pointers for it.
            LoaderContext::Descriptor loaderDescriptor;
            loaderDescriptor.m_instance = m_instance;
            loaderDescriptor.m_loadedExtensions = GetLoadedExtensions();
            loaderDescriptor.m_loadedLayers = GetLoadedLayers();
            if (!m_loaderContext->Init(loaderDescriptor))
            {
                AZ_Warning("Vulkan", false, "Failed to load function pointers for instance");
                return false;
            }

            CreateDebugMessenger();

            // Check that we have at least one device that meets the requirements.
            m_supportedDevices = EnumerateSupportedDevices(minVersion);           
            AZ_Warning("Vulkan", !m_supportedDevices.empty(), "Could not find any Vulkan supported device");
            return !m_supportedDevices.empty();
        }

        void Instance::Shutdown() 
        {
            ShutdownNativeInstance();
            if (m_loaderContext)
            {
                m_loaderContext->Shutdown();
                m_loaderContext = nullptr;
            }
        }

        void Instance::ShutdownNativeInstance()
        {
            if (m_instance != VK_NULL_HANDLE)
            {
                if (GetValidationMode() != RHI::ValidationMode::Disabled)
                {
                    Debug::ShutdownDebugMessages(GetContext(), m_instance);
                }
                m_supportedDevices.clear();
                InstanceNotificationBus::Broadcast(&InstanceNotificationBus::Events::OnInstanceDestroyed);

                //Using use nullptr for VkAllocationCallbacks*. Please see comments above related to Instance creation
                GetContext().DestroyInstance(m_instance, nullptr);
                m_instance = VK_NULL_HANDLE;
            }
        }

        const Instance::Descriptor& Instance::GetDescriptor() const
        {
            return m_descriptor;
        }

        RHI::PhysicalDeviceList Instance::GetSupportedDevices() const
        {
            return m_supportedDevices;
        }

        AZ::RHI::ValidationMode Instance::GetValidationMode() const
        {
            return m_descriptor.m_validationMode;
        }

        RHI::PhysicalDeviceList Instance::EnumerateSupportedDevices(uint32_t minVersion) const
        {
            // First get all available devices
            RHI::PhysicalDeviceList supportedDevices = PhysicalDevice::Enumerate();
            // Filter the ones that are not supported according to the Ebus function
            AZStd::vector<VkPhysicalDevice> supportedVkDevices;
            supportedVkDevices.reserve(supportedDevices.size());
            // Build an array of VkPhysicalDevices
            AZStd::transform(
                supportedDevices.begin(),
                supportedDevices.end(),
                AZStd::back_inserter(supportedVkDevices),
                [](const RHI::Ptr<RHI::PhysicalDevice>& physicalDevice)
                {
                    const PhysicalDevice* physical = static_cast<const PhysicalDevice*>(physicalDevice.get());
                    return physical->GetNativePhysicalDevice();
                });
            // Filter by VkPhysicalDevice
            DeviceRequirementBus::Broadcast(&DeviceRequirementBus::Events::FilterSupportedDevices, supportedVkDevices);
            // Remove all not supported devices
            AZStd::erase_if(
                supportedDevices,
                [&](const RHI::Ptr<RHI::PhysicalDevice>& physicalDevice)
                {
                    const PhysicalDevice* physical = static_cast<const PhysicalDevice*>(physicalDevice.get());
                    auto findIt = AZStd::find(supportedVkDevices.begin(), supportedVkDevices.end(), physical->GetNativePhysicalDevice());
                    return findIt == supportedVkDevices.end();
                });

            // Finally filter by API version and by required layers and extensions.
            for (auto it = supportedDevices.begin(); it != supportedDevices.end();)
            {
                PhysicalDevice* physicalDevice = static_cast<PhysicalDevice*>((*it).get());
                const VkPhysicalDeviceProperties& properties = physicalDevice->GetPhysicalDeviceProperties();
                bool shouldIgnore = false;
                // Check that the device supports the minimum required Vulkan version.
                if (properties.apiVersion < minVersion)
                {
                    AZ_Warning("Vulkan", false, "Ignoring device %s because the Vulkan version doesn't meet the minimum requirements.", properties.deviceName);
                    it = supportedDevices.erase(it);
                    continue;
                }

                // Check that it supports all required layers.
                auto layersName = physicalDevice->GetDeviceLayerNames();
                for (const AZStd::string& layerName : Device::GetRequiredLayers())
                {
                    auto findIt = AZStd::find(layersName.begin(), layersName.end(), layerName);
                    if (findIt == layersName.end())
                    {
                        AZ_Warning("Vulkan", false, "Ignoring device %s because required layer %s is not available.", properties.deviceName, layerName.c_str());
                        shouldIgnore = true;
                        break;
                    }
                }

                if (!shouldIgnore)
                {
                    // Check that it supports all required extensions.
                    auto extensionNames = physicalDevice->GetDeviceExtensionNames();
                    for (const AZStd::string& extensionName : Device::GetRequiredExtensions())
                    {
                        auto findIt = AZStd::find(extensionNames.begin(), extensionNames.end(), extensionName);
                        if (findIt == extensionNames.end())
                        {
                            AZ_Warning("Vulkan", false, "Ignoring device %s because required extension %s is not available.", properties.deviceName, extensionName.c_str());
                            shouldIgnore = true;
                            break;
                        }
                    }
                }

                it = shouldIgnore ? supportedDevices.erase(it) : it + 1;
            }
            return supportedDevices;
        }

        void Instance::CreateDebugMessenger()
        {
            auto validationMode = GetValidationMode();
            if (validationMode != RHI::ValidationMode::Disabled)
            {
                auto messagesTypeMask =
                    Debug::DebugMessageTypeFlag::Error | Debug::DebugMessageTypeFlag::Warning | Debug::DebugMessageTypeFlag::Performance;
                if (validationMode == RHI::ValidationMode::Verbose)
                {
                    messagesTypeMask |= Debug::DebugMessageTypeFlag::Debug | Debug::DebugMessageTypeFlag::Info;
                }

                Debug::InitDebugMessages(GetContext(), m_instance, messagesTypeMask);
            }
        }

        const RawStringList& Instance::GetLoadedLayers() const
        {
            if (m_instance == VK_NULL_HANDLE)
            {
                return s_emptyRawList;
            }

            return m_descriptor.m_requiredLayers;
        }
        const RawStringList& Instance::GetLoadedExtensions() const
        {
            if (m_instance == VK_NULL_HANDLE)
            {
                return s_emptyRawList;
            }

            return m_descriptor.m_requiredExtensions;
        }
    }
}
