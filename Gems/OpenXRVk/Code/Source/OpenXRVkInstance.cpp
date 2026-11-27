/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVk/OpenXRVkInstance.h>
#include <OpenXRVk/OpenXRVkUtils.h>
#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <AzCore/Casting/numeric_cast.h>
#include <OpenXRVkCommon.h>

namespace OpenXRVk
{
    XR::Ptr<Instance> Instance::Create()
    {
        return aznew Instance;
    }

    XR::StringList Instance::GetInstanceExtensionNames(const char* layerName /*= nullptr*/) const
    {
        XR::StringList extensionNames;
        AZ::u32 extPropertyCount = 0;
        XrResult result = xrEnumerateInstanceExtensionProperties(layerName, 0, &extPropertyCount, nullptr);
        if (IsError(result) || extPropertyCount == 0)
        {
            return extensionNames;
        }

        AZStd::vector<XrExtensionProperties> extProperties;
        extProperties.resize(extPropertyCount);

        for (XrExtensionProperties& extension : extProperties)
        {
            extension.type = XR_TYPE_EXTENSION_PROPERTIES;
        }

        result = xrEnumerateInstanceExtensionProperties(layerName, aznumeric_cast<AZ::u32>(extProperties.size()), &extPropertyCount, extProperties.data());
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

    XR::StringList Instance::GetInstanceLayerNames() const
    {
        XR::StringList layerNames;
        AZ::u32 layerPropertyCount = 0;
        XrResult result = xrEnumerateApiLayerProperties(0, &layerPropertyCount, nullptr);
        if (IsError(result) || layerPropertyCount == 0)
        {
            return layerNames;
        }

        AZStd::vector<XrApiLayerProperties> layerProperties(layerPropertyCount);
        for (XrApiLayerProperties& layer : layerProperties)
        {
            layer.type = XR_TYPE_API_LAYER_PROPERTIES;
        }

        result = xrEnumerateApiLayerProperties(aznumeric_cast<AZ::u32>(layerProperties.size()), &layerPropertyCount, layerProperties.data());
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

    AZ::RHI::ResultCode Instance::InitInstanceInternal()
    {
        if (!Platform::OpenXRInitializeLoader())
        {
            AZ_Error("OpenXRVk", false, "Could not initialize xr loader.");
            return AZ::RHI::ResultCode::Fail;
        }

        XR::RawStringList optionalLayers;
        XR::RawStringList optionalExtensions = { XR_KHR_VULKAN_ENABLE_EXTENSION_NAME };

        XR::StringList instanceLayerNames = GetInstanceLayerNames();
        XR::RawStringList supportedLayers = FilterList(optionalLayers, instanceLayerNames);
        m_requiredLayers.insert(m_requiredLayers.end(), supportedLayers.begin(), supportedLayers.end());

        XR::StringList instanceExtensions = GetInstanceExtensionNames();
        XR::RawStringList supportedExtensions = FilterList(optionalExtensions, instanceExtensions);
        m_requiredExtensions.insert(m_requiredExtensions.end(), supportedExtensions.begin(), supportedExtensions.end());

        if (m_validationMode == AZ::RHI::ValidationMode::Enabled)
        {
            AZ_Printf("OpenXRVk", "Available Extensions: (%i)\n", instanceExtensions.size());
            for (const AZStd::string& extension : instanceExtensions)
            {
                AZ_Printf("OpenXRVk", "Name=%s\n", extension.c_str());
            }

            AZ_Printf("OpenXRVk", "Extensions to enable: (%i)\n", m_requiredExtensions.size());
            for (const char* extension : m_requiredExtensions)
            {
                AZ_Printf("OpenXRVk", "Name=%s\n", extension);
            }

            AZ_Printf("OpenXRVk", "Available Layers: (%i)\n", instanceLayerNames.size());
            for (const AZStd::string& layerName : instanceLayerNames)
            {
                AZ_Printf("OpenXRVk", "Name=%s \n", layerName.c_str());
            }
        }

        AZ_Assert(m_xrInstance == XR_NULL_HANDLE, "XR Instance is already initialized");
        XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
        createInfo.next = nullptr;
        createInfo.enabledExtensionCount = aznumeric_cast<AZ::u32>(m_requiredExtensions.size());
        createInfo.enabledExtensionNames = m_requiredExtensions.data();
        createInfo.enabledApiLayerCount = aznumeric_cast<AZ::u32>(supportedLayers.size());
        createInfo.enabledApiLayerNames = supportedLayers.data();

        azstrncpy(createInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "O3deApp", 4);
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

        //Create XR instance
        XrResult result = xrCreateInstance(&createInfo, &m_xrInstance);
        if(IsError(result))
        {
            AZ_Warning("OpenXRVk", false, "Failed to create XR instance");
            return AZ::RHI::ResultCode::Fail;
        }

        if (m_validationMode == AZ::RHI::ValidationMode::Enabled)
        {
            XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
            result = xrGetInstanceProperties(m_xrInstance, &instanceProperties);
            if (IsSuccess(result))
            {
                AZStd::string verStr = AZStd::string::format("%d.%d.%d",
                XR_VERSION_MAJOR(instanceProperties.runtimeVersion),
                XR_VERSION_MINOR(instanceProperties.runtimeVersion),
                XR_VERSION_PATCH(instanceProperties.runtimeVersion));
                AZ_Printf("OpenXRVk", "Instance RuntimeName=%s RuntimeVersion=%s\n", instanceProperties.runtimeName, verStr.c_str());
            }
        }

        AZ_Assert(m_xrInstance != XR_NULL_HANDLE, "XR Instance is Null");
        AZ_Assert(m_xrSystemId == XR_NULL_SYSTEM_ID, "XR System id already initialized");

        //TODO::Add support for handheld display
        m_formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
        systemInfo.formFactor = m_formFactor;
        result = xrGetSystem(m_xrInstance, &systemInfo, &m_xrSystemId);
        if (IsError(result))
        {
            AZ_Warning("OpenXRVk", false, "Failed to get XR System id");
            return AZ::RHI::ResultCode::Fail;
        }

        //TODO::Add support for other view configuration types
        m_viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        m_viewCount = 0;
        result = xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId, m_viewConfigType, 0, &m_viewCount, nullptr);
        if (IsError(result))
        {
            PrintXrError("OpenXRVk", result, "Failed to read the number of views for the configuration type: %u.", aznumeric_cast<uint32_t>(m_viewConfigType));
            return AZ::RHI::ResultCode::Fail;
        }
        AZ_Assert(m_viewCount > 0, "View count should be greater than 0.");

        //TODO::Add support for other environment blend types
        m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

        // Query the runtime Vulkan API version requirements
        XrGraphicsRequirementsVulkan2KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
        PFN_xrGetVulkanGraphicsRequirementsKHR pfnGetVulkanGraphicsRequirementsKHR = nullptr;
        result = xrGetInstanceProcAddr(m_xrInstance, "xrGetVulkanGraphicsRequirementsKHR",
                                       reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsRequirementsKHR));
        WARN_IF_UNSUCCESSFUL(result);

        XrGraphicsRequirementsVulkanKHR legacyRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR };
        result = pfnGetVulkanGraphicsRequirementsKHR(m_xrInstance, m_xrSystemId, &legacyRequirements);
        WARN_IF_UNSUCCESSFUL(result);
        graphicsRequirements.maxApiVersionSupported = legacyRequirements.maxApiVersionSupported;
        graphicsRequirements.minApiVersionSupported = legacyRequirements.minApiVersionSupported;

        m_minVulkanAPIVersion = VK_MAKE_API_VERSION(
            0,
            XR_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported),
            XR_VERSION_MINOR(graphicsRequirements.minApiVersionSupported),
            XR_VERSION_PATCH(graphicsRequirements.minApiVersionSupported));

        m_maxVulkanAPIVersion = VK_MAKE_API_VERSION(
            0,
            XR_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported),
            XR_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported),
            XR_VERSION_PATCH(graphicsRequirements.maxApiVersionSupported));

        if (m_validationMode == AZ::RHI::ValidationMode::Enabled)
        {
            AZ_Printf("OpenXRVk", "graphicsRequirements.maxApiVersionSupported %d.%d.%d\n",
            XR_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported),
            XR_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported),
            XR_VERSION_PATCH(graphicsRequirements.maxApiVersionSupported));

            AZ_Printf("OpenXRVk", "graphicsRequirements.minApiVersionSupported %d.%d.%d\n",
            XR_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported),
            XR_VERSION_MINOR(graphicsRequirements.minApiVersionSupported),
            XR_VERSION_PATCH(graphicsRequirements.minApiVersionSupported));

            AZ_Printf("OpenXRVk", "Using system %d for form factor %s\n", m_xrSystemId, to_string(m_formFactor));
            LogViewConfigurations();
        }

        PFN_xrGetVulkanInstanceExtensionsKHR pfnGetVulkanInstanceExtensionsKHR = nullptr;
        result = xrGetInstanceProcAddr(m_xrInstance, "xrGetVulkanInstanceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanInstanceExtensionsKHR));
        ASSERT_IF_UNSUCCESSFUL(result);

        AZ::u32 extensionNamesSize = 0;
        result = pfnGetVulkanInstanceExtensionsKHR(m_xrInstance, m_xrSystemId, 0, &extensionNamesSize, nullptr);
        ASSERT_IF_UNSUCCESSFUL(result);

        AZStd::vector<char> extensionNames(extensionNamesSize);
        result = pfnGetVulkanInstanceExtensionsKHR(m_xrInstance, m_xrSystemId, extensionNamesSize, &extensionNamesSize, &extensionNames[0]);
        ASSERT_IF_UNSUCCESSFUL(result);

        AZStd::vector<const char*> requiredInstanceExtensions = ParseExtensionString(&extensionNames[0]);
        m_requireVulkanInstanceExtensions.insert(m_requireVulkanInstanceExtensions.end(), requiredInstanceExtensions.begin(), requiredInstanceExtensions.end());
        if (m_validationMode == AZ::RHI::ValidationMode::Enabled)
        {
            AZ_Printf("OpenXRVk", "Vulkan instance extensions to enable: (%i)\n", requiredInstanceExtensions.size());
            for (const AZStd::string& extension : requiredInstanceExtensions)
            {
                AZ_Printf("OpenXRVk", "Name=%s\n", extension.c_str());
            }
        }

        AZ::Vulkan::InstanceRequirementBus::Handler::BusConnect();

        PFN_xrGetVulkanDeviceExtensionsKHR pfnGetVulkanDeviceExtensionsKHR = nullptr;
        result = xrGetInstanceProcAddr(
            m_xrInstance,
            "xrGetVulkanDeviceExtensionsKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanDeviceExtensionsKHR));
        ASSERT_IF_UNSUCCESSFUL(result);

        AZ::u32 deviceExtensionNamesSize = 0;
        result = pfnGetVulkanDeviceExtensionsKHR(m_xrInstance, m_xrSystemId, 0, &deviceExtensionNamesSize, nullptr);
        ASSERT_IF_UNSUCCESSFUL(result);

        AZStd::vector<char> deviceExtensionNames(deviceExtensionNamesSize);
        result = pfnGetVulkanDeviceExtensionsKHR(
            m_xrInstance, m_xrSystemId, deviceExtensionNamesSize, &deviceExtensionNamesSize, &deviceExtensionNames[0]);
        ASSERT_IF_UNSUCCESSFUL(result);

        AZStd::vector<const char*> requiredDeviceExtensions = ParseExtensionString(&deviceExtensionNames[0]);
        m_requireVulkanDeviceExtensions.insert(m_requireVulkanDeviceExtensions.end(), requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
        if (m_validationMode == AZ::RHI::ValidationMode::Enabled)
        {
            AZ_Printf("OpenXRVk", "Vulkan device extensions to enable: (%i)\n", requiredDeviceExtensions.size());
            for (const AZStd::string& extension : requiredDeviceExtensions)
            {
                AZ_Printf("OpenXRVk", "Name=%s\n", extension.c_str());
            }
        }

        AZ::Vulkan::DeviceRequirementBus::Handler::BusConnect();
        AZ::Vulkan::InstanceNotificationBus::Handler::BusConnect();

        return AZ::RHI::ResultCode::Success;
    }

    void Instance::LogViewConfigurations()
    {
        AZ::u32 viewConfigTypeCount = 0;
        XrResult result = xrEnumerateViewConfigurations(m_xrInstance, m_xrSystemId, 0, &viewConfigTypeCount, nullptr);
        RETURN_IF_UNSUCCESSFUL(result);

        AZStd::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
        result = xrEnumerateViewConfigurations(m_xrInstance, m_xrSystemId, viewConfigTypeCount, &viewConfigTypeCount, viewConfigTypes.data());
        RETURN_IF_UNSUCCESSFUL(result);

        AZ_Warning("OpenXRVk", aznumeric_cast<AZ::u32>(viewConfigTypes.size()) == viewConfigTypeCount, "Size Mismatch");

        AZ_Printf("OpenXRVk", "Available View Configuration Types: (%d)\n", viewConfigTypeCount);
        for (XrViewConfigurationType viewConfigType : viewConfigTypes)
        {
            AZ_Printf("OpenXRVk", "View Configuration Type: %s %s\n", to_string(viewConfigType),
            viewConfigType == m_viewConfigType ? "(Selected)" : "");

            XrViewConfigurationProperties viewConfigProperties{ XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
            result = xrGetViewConfigurationProperties(m_xrInstance, m_xrSystemId, viewConfigType, &viewConfigProperties);
            RETURN_IF_UNSUCCESSFUL(result);

            AZ_Printf("OpenXRVk", "View configuration FovMutable=%s\n", viewConfigProperties.fovMutable == XR_TRUE ? "True" : "False");

            AZ::u32 viewCount = 0;
            result = xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId, viewConfigType, 0, &viewCount, nullptr);
            RETURN_IF_UNSUCCESSFUL(result);
            if (viewCount > 0)
            {
                AZStd::vector<XrViewConfigurationView> views(viewCount);
                for (XrViewConfigurationView& view : views)
                {
                    view.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                }
                result = xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId, viewConfigType, viewCount, &viewCount, views.data());
                RETURN_IF_UNSUCCESSFUL(result);

                for (uint32_t i = 0; i < views.size(); i++)
                {
                    const XrViewConfigurationView& view = views[i];
                    AZ_Printf(
                        "OpenXRVk", "View [%d]: Recommended Width=%d Height=%d SampleCount=%d\n", i, view.recommendedImageRectWidth,
                        view.recommendedImageRectHeight, view.recommendedSwapchainSampleCount);
                    AZ_Printf(
                        "OpenXRVk", "View [%d]:     Maximum Width=%d Height=%d SampleCount=%d\n", i, view.maxImageRectWidth,
                        view.maxImageRectHeight, view.maxSwapchainSampleCount);
                }
            }
            else
            {
                AZ_Printf("OpenXRVk", "Empty view configuration type\n");
            }

            LogEnvironmentBlendMode(viewConfigType);
        }
    }

    void Instance::LogEnvironmentBlendMode(XrViewConfigurationType type)
    {
        AZ::u32 count = 0;
        XrResult result = xrEnumerateEnvironmentBlendModes(m_xrInstance, m_xrSystemId, type, 0, &count, nullptr);
        AZ_Warning("OpenXRVk", count > 0, "BlendModes not supported");
        RETURN_IF_UNSUCCESSFUL(result);

        AZ_Printf("OpenXRVk", "Available Environment Blend Mode count : (%d)\n", count);

        AZStd::vector<XrEnvironmentBlendMode> blendModes(count);
        result = xrEnumerateEnvironmentBlendModes(m_xrInstance, m_xrSystemId, type, count, &count, blendModes.data());
        RETURN_IF_UNSUCCESSFUL(result);

        [[maybe_unused]] bool blendModeFound = false;
        for (XrEnvironmentBlendMode mode : blendModes)
        {
            const bool blendModeMatch = (mode == m_environmentBlendMode);
            AZ_Printf("OpenXRVk", "Environment Blend Mode (%s) : %s\n", to_string(mode), blendModeMatch ? "(Selected)" : "");
            blendModeFound |= blendModeMatch;
        }
    }

    void Instance::ShutdownInternal()
    {
        AZ::Vulkan::InstanceNotificationBus::Handler::BusDisconnect();
        AZ::Vulkan::DeviceRequirementBus::Handler::BusDisconnect();
        AZ::Vulkan::InstanceRequirementBus::Handler::BusDisconnect();

        m_supportedXRDevices.clear();
        m_xrVkInstance = VK_NULL_HANDLE;
    }

    void Instance::CollectAdditionalRequiredInstanceExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.insert(extensions.end(), m_requireVulkanInstanceExtensions.begin(), m_requireVulkanInstanceExtensions.end());
    }

    void Instance::CollectMinMaxVulkanAPIVersions(AZStd::vector<uint32_t>& min, AZStd::vector<uint32_t>& max)
    {
        min.push_back(m_minVulkanAPIVersion);
        max.push_back(m_maxVulkanAPIVersion);
    }

    void Instance::CollectAdditionalRequiredDeviceExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.insert(extensions.end(), m_requireVulkanDeviceExtensions.begin(), m_requireVulkanDeviceExtensions.end());
    }

    void Instance::FilterSupportedDevices(AZStd::vector<VkPhysicalDevice>& supportedDevices)
    {
        AZStd::erase_if(supportedDevices, [&](const auto& physicalDevice)
        {
            return AZStd::find(m_supportedXRDevices.begin(), m_supportedXRDevices.end(), physicalDevice) == m_supportedXRDevices.end();
        });
    }

    void Instance::OnInstanceCreated(VkInstance instance)
    {
        m_xrVkInstance = instance;
        m_supportedXRDevices = PhysicalDevice::EnumerateDeviceList(m_xrSystemId, m_xrInstance, m_xrVkInstance);
    }

    void Instance::OnInstanceDestroyed()
    {
        m_xrVkInstance = XR_NULL_HANDLE;
        m_supportedXRDevices.clear();
    }

    AZ::RHI::ResultCode Instance::GetXRPhysicalDevice(AZ::RHI::XRPhysicalDeviceDescriptor* physicalDeviceDescriptor, int32_t index)
    {
        AZ::Vulkan::XRPhysicalDeviceDescriptor* xrPhysicalDeviceDescriptor = static_cast<AZ::Vulkan::XRPhysicalDeviceDescriptor*>(physicalDeviceDescriptor);
        if (xrPhysicalDeviceDescriptor && (index < m_supportedXRDevices.size()))
        {
            xrPhysicalDeviceDescriptor->m_outputData.m_xrVkPhysicalDevice = m_supportedXRDevices[index];
            return AZ::RHI::ResultCode::Success;
        }
        return AZ::RHI::ResultCode::Fail;
    }

    AZ::u32 Instance::GetNumPhysicalDevices() const
    {
        return aznumeric_cast<AZ::u32>(m_supportedXRDevices.size());
    }

    XrInstance Instance::GetXRInstance() const
    {
        return m_xrInstance;
    }

    XrSystemId Instance::GetXRSystemId() const
    {
        return m_xrSystemId;
    }

    VkInstance Instance::GetNativeInstance() const
    {
        return m_xrVkInstance;
    }

    XrEnvironmentBlendMode Instance::GetEnvironmentBlendMode() const
    {
        return m_environmentBlendMode;
    }

    XrViewConfigurationType Instance::GetViewConfigType() const
    {
        return m_viewConfigType;
    }

    uint32_t Instance::GetViewCount() const
    {
        return m_viewCount;
    }
}
