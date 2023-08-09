/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Device/DeviceAttributesSystemComponent.h>
#include <AzFramework/Device/DeviceAttributes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzFramework
{
    DeviceAttributesSystemComponent::DeviceAttributesSystemComponent() = default;
    DeviceAttributesSystemComponent::~DeviceAttributesSystemComponent() = default;

    void DeviceAttributesSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DeviceAttributesSystemComponent, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DeviceAttributesSystemComponent>(
                    "AzFramework Device Attributes Component", "System component responsible for handling device attribute registration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }
    }

    void DeviceAttributesSystemComponent::Activate()
    {
        if (DeviceAttributeRegistrar::Get() == nullptr)
        {
            DeviceAttributeRegistrar::Register(this);
        }

        // register default attributes
        RegisterDeviceAttribute(AZStd::make_unique<DeviceAttributeDeviceModel>());
        RegisterDeviceAttribute(AZStd::make_unique<DeviceAttributeRAM>());
    }

    void DeviceAttributesSystemComponent::Deactivate()
    {
        if (DeviceAttributeRegistrar::Get() == this)
        {
            DeviceAttributeRegistrar::Unregister(this);
        }

        m_deviceAttributes.clear();
    }

    bool DeviceAttributesSystemComponent::RegisterDeviceAttribute(AZStd::unique_ptr<DeviceAttribute> deviceAttribute)
    {
        auto name = deviceAttribute->GetName();
        if (m_deviceAttributes.contains(name))
        {
            AZ_Warning(
                "DeviceAttributesSystemComponent", false,
                "Device attribute '%.*s' is already registered, ignoring new registration request.",
                AZ_STRING_ARG(name));
            return false;
        }

        m_deviceAttributes.emplace(name, AZStd::move(deviceAttribute) );
        return true;
    }

    void DeviceAttributesSystemComponent::VisitDeviceAttributes(const VisitInterfaceCallback& callback) const
    {
        for (const auto& [deviceAttribue, deviceAttributeInterface] : m_deviceAttributes)
        {
            if (!deviceAttributeInterface)
            {
                continue;
            }

            if (!callback(*deviceAttributeInterface.get()))
            {
                return;
            }
        }
    }

    DeviceAttribute* DeviceAttributesSystemComponent::FindDeviceAttribute(AZStd::string_view deviceAttribute) const
    {
        auto itr = m_deviceAttributes.find(deviceAttribute);
        return itr != m_deviceAttributes.end() ? itr->second.get() : nullptr;
    }

    void DeviceAttributesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DeviceAttributesSystemComponentService"));
    }

    void DeviceAttributesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DeviceAttributesSystemComponentService"));
    }

    void DeviceAttributesSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // AzFramework

