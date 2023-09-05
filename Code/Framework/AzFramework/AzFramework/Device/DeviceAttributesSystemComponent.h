/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/Device/DeviceAttributeInterface.h>

namespace AzFramework
{
    class DeviceAttributeInterface;

    //! System component responsible for managing device attributes
    class DeviceAttributesSystemComponent final
        : public AZ::Component
        , public DeviceAttributeRegistrarInterface
    {
    public:
        AZ_COMPONENT(DeviceAttributesSystemComponent, "{C5ACED7D-FE7B-43F4-9414-8B2CAB51F229}", AZ::Component);

        DeviceAttributesSystemComponent();
        ~DeviceAttributesSystemComponent() override;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzFramework::DeviceAttributeRegistrarInterface
        DeviceAttribute* FindDeviceAttribute(AZStd::string_view deviceAttribute) const override;
        bool RegisterDeviceAttribute(AZStd::shared_ptr<DeviceAttribute> deviceAttribute) override;
        bool UnregisterDeviceAttribute(AZStd::string_view deviceAttribute) override;
        void VisitDeviceAttributes(const VisitInterfaceCallback&) const override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    private:
        AZ_DISABLE_COPY(DeviceAttributesSystemComponent);

        AZStd::unordered_map<AZStd::string, AZStd::shared_ptr<DeviceAttribute>> m_deviceAttributes;
    };
} // AzFramework

