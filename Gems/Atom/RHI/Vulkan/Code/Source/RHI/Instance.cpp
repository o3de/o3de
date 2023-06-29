/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>
#include <RHI/Instance.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>
#include <Atom/RHI.Loader/FunctionLoader.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Utils/Utils.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        static const uint32_t s_minVulkanSupportedVersion = VK_API_VERSION_1_0;

        static EnvironmentVariable<Instance> s_vulkanInstance;
        static constexpr const char* s_vulkanInstanceKey = "VulkanInstance";

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
            
            m_functionLoader = FunctionLoader::Create();
            if (!m_functionLoader->Init() ||
                !m_functionLoader->LoadProcAddresses(&m_context, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE))
            {
                AZ_Warning("Vulkan", false, "Could not initialize function loader.");
                return false;
            }

            uint32_t apiVersion = VK_API_VERSION_1_0;
            // vkEnumerateInstanceVersion is a Vulkan 1.1 function 
            // so if it's not available we assume Vulkan 1.0
            if (m_context.EnumerateInstanceVersion && m_context.EnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
            {
                AZ_Warning("Vulkan", false, "Failed to get instance version.");
                return false;
            }

            if (apiVersion < s_minVulkanSupportedVersion)
            {
                AZ_Warning(
                    "Vulkan", 
                    false, 
                    "The current Vulkan version (%d.%d.%d) is lower that the minimun required (%d.%d.%d).",
                    VK_VERSION_MAJOR(apiVersion),
                    VK_VERSION_MINOR(apiVersion),
                    VK_VERSION_PATCH(apiVersion),
                    VK_VERSION_MAJOR(s_minVulkanSupportedVersion),
                    VK_VERSION_MINOR(s_minVulkanSupportedVersion),
                    VK_VERSION_PATCH(s_minVulkanSupportedVersion));

                return false;
            }                

            
            m_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            m_appInfo.apiVersion = apiVersion;

            m_instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            m_instanceCreateInfo.pApplicationInfo = &m_appInfo;

            StringList instanceLayerNames = GetInstanceLayerNames();
            RawStringList optionalLayers = FilterList(m_descriptor.m_optionalLayers, instanceLayerNames);
            m_descriptor.m_requiredLayers.insert(m_descriptor.m_requiredLayers.end(), optionalLayers.begin(), optionalLayers.end());

            StringList instanceExtensions = GetInstanceExtensionNames();
            RawStringList optionalExtensions = FilterList(m_descriptor.m_optionalExtensions, instanceExtensions);
            m_descriptor.m_requiredExtensions.insert(m_descriptor.m_requiredExtensions.end(), optionalExtensions.begin(), optionalExtensions.end());

            m_instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_descriptor.m_requiredLayers.size());
            m_instanceCreateInfo.ppEnabledLayerNames = m_descriptor.m_requiredLayers.data();
            m_instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_descriptor.m_requiredExtensions.size());
            m_instanceCreateInfo.ppEnabledExtensionNames = m_descriptor.m_requiredExtensions.data();

            // For instance creation/destruction use nullptr for VkAllocationCallbacks* as using VkSystemAllocator::Get() crashes RenderDoc when used with openxr
            // enabled projects. We think it's because RenderDoc is maybe injecting something when doing allocations. Using nullptr when USE_RENDERDOC or enableRenderDoc
            // is enabled is another option but it will not work for Android easily and will require further work, not to mention manually enabling this for Android RenderDoc.
            VkResult result = m_context.CreateInstance(&m_instanceCreateInfo, nullptr, &m_instance);

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

                result = m_context.CreateInstance(&m_instanceCreateInfo, nullptr, &m_instance);
            }

            if (result != VK_SUCCESS)
            {
                AZ_Warning("Vulkan", false, R"(Failed to create Vulkan instance. Error = "%s")", GetResultString(result));
                return false;
            }

            // Now that we have created the instance, load the function pointers for it.
            m_functionLoader->LoadProcAddresses(&m_context, m_instance, VK_NULL_HANDLE, VK_NULL_HANDLE);

            CreateDebugMessenger();

            // Check that we have at least one device that meets the requirements.
            m_supportedDevices = EnumerateSupportedDevices();

            AZ_Warning("Vulkan", !m_supportedDevices.empty(), "Could not find any Vulkan supported device");
            return !m_supportedDevices.empty();
        }

        void Instance::Shutdown() 
        {
            //Only destroy VkInstance if created locally and not passed in by XR module
            if (!m_isXRInstanceCreated)
            {
                ShutdownNativeInstance();
            }
            
            if (m_functionLoader)
            {
                m_functionLoader->Shutdown();
            }
            m_functionLoader = nullptr;
        }

        void Instance::ShutdownNativeInstance()
        {
            if (m_instance != VK_NULL_HANDLE)
            {
                if (GetValidationMode() != RHI::ValidationMode::Disabled)
                {
                    Debug::ShutdownDebugMessages(m_context, m_instance);
                }
                m_supportedDevices.clear();

                //Using use nullptr for VkAllocationCallbacks*. Please see comments above related to Instance creation
                m_context.DestroyInstance(m_instance, nullptr);
                m_instance = VK_NULL_HANDLE;
            }
        }

        const Instance::Descriptor& Instance::GetDescriptor() const
        {
            return m_descriptor;
        }

        StringList Instance::GetInstanceLayerNames() const
        {
            StringList layerNames;
            uint32_t layerPropertyCount = 0;
            VkResult result = m_context.EnumerateInstanceLayerProperties(&layerPropertyCount, nullptr);
            if (IsError(result) || layerPropertyCount == 0)
            {
                return layerNames;
            }

            AZStd::vector<VkLayerProperties> layerProperties(layerPropertyCount);
            result = m_context.EnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data());
            if (IsError(result))
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

        StringList Instance::GetInstanceExtensionNames(const char* layerName /*= nullptr*/) const
        {
            StringList extensionNames;
            uint32_t extPropertyCount = 0;
            VkResult result = m_context.EnumerateInstanceExtensionProperties(layerName, &extPropertyCount, nullptr);
            if (IsError(result) || extPropertyCount == 0)
            {
                return extensionNames;
            }

            AZStd::vector<VkExtensionProperties> extProperties;
            extProperties.resize(extPropertyCount);

            result = m_context.EnumerateInstanceExtensionProperties(layerName, &extPropertyCount, extProperties.data());
            if (IsError(result))
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

        RHI::PhysicalDeviceList Instance::GetSupportedDevices() const
        {
            return m_supportedDevices;
        }

        AZ::RHI::ValidationMode Instance::GetValidationMode() const
        {
            return m_descriptor.m_validationMode;
        }

        RHI::PhysicalDeviceList Instance::EnumerateSupportedDevices() const
        {
            RHI::PhysicalDeviceList supportedDevices = PhysicalDevice::Enumerate();
            for (auto it = supportedDevices.begin(); it != supportedDevices.end();)
            {
                PhysicalDevice* physicalDevice = static_cast<PhysicalDevice*>((*it).get());
                const VkPhysicalDeviceProperties& properties = physicalDevice->GetPhysicalDeviceProperties();
                bool shouldIgnore = false;
                // Check that the device supports the minimum required Vulkan version.
                if (properties.apiVersion < s_minVulkanSupportedVersion)
                {
                    AZ_Warning("Vulkan", false, "Ignoring device %s because the Vulkan version doesn't meet the minimum requirements.", properties.deviceName);
                    shouldIgnore = true;
                }

                if (!shouldIgnore)
                {
                    // Check that it supports all required layers.
                    auto layersName = physicalDevice->GetDeviceLayerNames();
                    for (const char* layerName : Device::GetRequiredLayers())
                    {
                        auto findIt = AZStd::find(layersName.begin(), layersName.end(), layerName);
                        if (findIt == layersName.end())
                        {
                            AZ_Warning("Vulkan", false, "Ignoring device %s because required layer %s is not available.", properties.deviceName, layerName);
                            shouldIgnore = true;
                            break;
                        }
                    }

                }

                if (!shouldIgnore)
                {
                    // Check that it supports all required extensions.
                    auto extensionNames = physicalDevice->GetDeviceExtensionNames();
                    for (const char* extensionName : Device::GetRequiredExtensions())
                    {
                        auto findIt = AZStd::find(extensionNames.begin(), extensionNames.end(), extensionName);
                        if (findIt == extensionNames.end())
                        {
                            AZ_Warning("Vulkan", false, "Ignoring device %s because required extension %s is not available.", properties.deviceName, extensionName);
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

                Debug::InitDebugMessages(m_context, m_instance, messagesTypeMask);
            }
        }

        void Instance::UpdateNativeInstance(RHI::XRRenderingInterface* xrSystem)
        {
            if (m_isXRInstanceCreated)
            {
                AZ_Warning("Vulkan", false, "XR Vulkan instance is already created");
                return;
            }

            RHI::Ptr<XRInstanceDescriptor> xrInstanceDescriptor = aznew XRInstanceDescriptor();
            xrInstanceDescriptor->m_inputData.m_createInfo = &m_instanceCreateInfo;

            // Init the new native instance for XR
            AZ::RHI::ResultCode result = xrSystem->InitNativeInstance(xrInstanceDescriptor.get());
            AZ_Warning("Vulkan", result == RHI::ResultCode::Success, "Xr instance creation was not successful");
            if (result == RHI::ResultCode::Success)
            {
                //Delete existing VkInstance
                ShutdownNativeInstance();

                //Update the native object from the passed by the XR module
                m_instance = xrInstanceDescriptor->m_outputData.m_xrVkInstance;
                //Update the context from the passed by the XR module
                m_context = xrInstanceDescriptor->m_outputData.m_context;

                //Re-add support for validation with the updated VkInstance
                CreateDebugMessenger();

                //Get number of  Physical devices
                uint32_t numPhysicalDevices = xrSystem->GetNumPhysicalDevices();
                    
                //Clear any existing physical devices
                m_supportedDevices.clear();
                m_supportedDevices.reserve(numPhysicalDevices);

                //Re-populate physical devices from XR module
                for (uint32_t i = 0; i < numPhysicalDevices; i++)
                {
                    RHI::Ptr<XRPhysicalDeviceDescriptor> xrPhysicalDeviceDesc = aznew XRPhysicalDeviceDescriptor();
                    xrSystem->GetXRPhysicalDevice(xrPhysicalDeviceDesc.get(), i);

                    RHI::Ptr<PhysicalDevice> physicalDevice = aznew PhysicalDevice;
                    physicalDevice->Init(xrPhysicalDeviceDesc->m_outputData.m_xrVkPhysicalDevice);
                    m_supportedDevices.emplace_back(physicalDevice);
                }
                m_isXRInstanceCreated = true;
            }
        }
    }
}
