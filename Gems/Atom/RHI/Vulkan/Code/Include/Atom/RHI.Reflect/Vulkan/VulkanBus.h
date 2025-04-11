/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ::Vulkan
{
    //! Ebus for collecting any additional requirements for creating a Vulkan instance.
    class InstanceRequirementsRequest
        : public AZ::EBusTraits
    {
    public:
        virtual ~InstanceRequirementsRequest() = default;

        //! Collects any additional instance extensions needed for creating the Vulkan instance.
        virtual void CollectAdditionalRequiredInstanceExtensions([[maybe_unused]] AZStd::vector<AZStd::string>& extensions){};
        //! Collects the min/max versions required for creating the Vulkan instance.
        virtual void CollectMinMaxVulkanAPIVersions(
            [[maybe_unused]] AZStd::vector<uint32_t>& min, [[maybe_unused]] AZStd::vector<uint32_t>& max){};

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
    };

    using InstanceRequirementBus = AZ::EBus<InstanceRequirementsRequest>;

    //! Ebus for collecting requirements for creating a Vulkan device.
    class DeviceRequirementsRequest
        : public AZ::EBusTraits
    {
    public:
        virtual ~DeviceRequirementsRequest() = default;

        //! Collects any additional device extensions needed for creating the Vulkan device.
        virtual void CollectAdditionalRequiredDeviceExtensions([[maybe_unused]] AZStd::vector<AZStd::string>& extensions){};
        //! Removes Vulkan devices that are not supported from a list of available devices.
        virtual void FilterSupportedDevices([[maybe_unused]] AZStd::vector<VkPhysicalDevice>& supportedDevices){};

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
    };

    using DeviceRequirementBus = AZ::EBus<DeviceRequirementsRequest>;

    //! Ebus for collecting requirements external handle requirements for creating memory/semaphores
    class ExternalHandleRequirementsRequest : public AZ::EBusTraits
    {
    public:
        virtual ~ExternalHandleRequirementsRequest() = default;

        //! Collects requirements for external memory when allocating memory
        virtual void CollectExternalMemoryRequirements([[maybe_unused]] VkExternalMemoryHandleTypeFlagsKHR& flags){};
        //! Collect requirements for semaphore export when creating timeline semaphores
        virtual void CollectSemaphoreExportHandleTypes([[maybe_unused]] VkExternalSemaphoreHandleTypeFlags& flags){};

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using MutexType = AZStd::mutex;
        static constexpr bool LocklessDispatch = true;
    };

    using ExternalHandleRequirementBus = AZ::EBus<ExternalHandleRequirementsRequest>;

    //! Notifications related to Vulkan instance operations.
    class InstanceNotification
        : public AZ::EBusTraits
    {
    public:
        virtual ~InstanceNotification() = default;

        //! Signals that the Vulkan instance has been created.
        virtual void OnInstanceCreated([[maybe_unused]] VkInstance instance){};
        //! Signals that the Vulkan instance has been destroyed.
        virtual void OnInstanceDestroyed(){};

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
    };

    using InstanceNotificationBus = AZ::EBus<InstanceNotification>;
}
