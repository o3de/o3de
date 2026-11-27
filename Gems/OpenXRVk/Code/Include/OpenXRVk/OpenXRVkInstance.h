/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/std/containers/vector.h>
#include <OpenXRVk_Platform.h>
#include <OpenXRVk/OpenXRVkPhysicalDevice.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI.Reflect/Vulkan/VulkanBus.h>
#include <XR/XRInstance.h>

namespace OpenXRVk
{
    //! Vulkan specific XR instance back-end class that will help manage
    //! XR specific vulkan native objects
    class Instance final
        : public XR::Instance
        , public AZ::Vulkan::InstanceRequirementBus::Handler
        , public AZ::Vulkan::DeviceRequirementBus::Handler
        , public AZ::Vulkan::InstanceNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Instance, AZ::SystemAllocator);
        AZ_RTTI(Instance, "{1A62DF32-2909-431C-A938-B1E841A50768}", XR::Instance);

        static XR::Ptr<Instance> Create();

        //////////////////////////////////////////////////////////////////////////
        // XR::Instance overrides
        AZ::u32 GetNumPhysicalDevices() const override;
        AZ::RHI::ResultCode GetXRPhysicalDevice(AZ::RHI::XRPhysicalDeviceDescriptor* physicalDeviceDescriptor, int32_t index) override;
        //////////////////////////////////////////////////////////////////////////

        //! Enumerate supported extension names.
        XR::StringList GetInstanceExtensionNames(const char* layerName = nullptr) const;

        //! Enumerate supported layer names.
        XR::StringList GetInstanceLayerNames() const;

        //! Enumerate and log view configurations.
        void LogViewConfigurations();

        //! Enumerate and log environment blend mode.
        void LogEnvironmentBlendMode(XrViewConfigurationType type);

        //! Get the XRInstance.
        XrInstance GetXRInstance() const;

        //! Get System id.
        XrSystemId GetXRSystemId() const;

        //! Get native VkInstance.
        VkInstance GetNativeInstance() const;

        //! Get XR environment blend mode.
        XrEnvironmentBlendMode GetEnvironmentBlendMode() const;

        //! Get XR configuration type.
        XrViewConfigurationType GetViewConfigType() const;

        //! Get the number of views (aka eyes) according
        //! to the current XrViewConfigurationType.
        uint32_t GetViewCount() const;

    protected:
        // XR::Instance overrides...
        AZ::RHI::ResultCode InitInstanceInternal() override;
        void ShutdownInternal() override;

        // AZ::Vulkan::InstanceRequirementBus::Handler overrides..
        void CollectAdditionalRequiredInstanceExtensions(AZStd::vector<AZStd::string>& extensions) override;
        void CollectMinMaxVulkanAPIVersions(AZStd::vector<uint32_t>& min, AZStd::vector<uint32_t>& max) override;

        // AZ::Vulkan::DeviceRequirementBus::Handler overrides...
        void CollectAdditionalRequiredDeviceExtensions(AZStd::vector<AZStd::string>& extensions) override;
        void FilterSupportedDevices(AZStd::vector<VkPhysicalDevice>& supportedDevices) override;

        // AZ::Vulkan::InstanceNotificationBus::Handler overrides...
        virtual void OnInstanceCreated([[maybe_unused]] VkInstance instance);
        virtual void OnInstanceDestroyed();

    private:
        XrInstance m_xrInstance = XR_NULL_HANDLE;
        VkInstance m_xrVkInstance = VK_NULL_HANDLE;
        XrFormFactor m_formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        XrViewConfigurationType m_viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        XrEnvironmentBlendMode m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        XrSystemId m_xrSystemId = XR_NULL_SYSTEM_ID;
        XR::RawStringList m_requiredLayers;
        XR::RawStringList m_requiredExtensions;
        XR::StringList m_requireVulkanInstanceExtensions;
        XR::StringList m_requireVulkanDeviceExtensions;
        AZStd::vector<VkPhysicalDevice> m_supportedXRDevices;
        uint32_t m_minVulkanAPIVersion = 0;
        uint32_t m_maxVulkanAPIVersion = 0;
        //! At runtime the number of views (eyes) will be calculate according
        //! to the select view configuration in @m_viewConfigType.
        uint32_t m_viewCount = 0;
    };
}
