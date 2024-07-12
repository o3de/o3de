/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "VirtualGamepadSystemComponent.h"
#include "InputDeviceVirtualGamepad.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("VirtualGamepadService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("VirtualGamepadService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("InputSystemService"));
        required.push_back(AZ_CRC_CE("LyShineService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<VirtualGamepadSystemComponent, AZ::Component>()
                ->Version(0)
                ->Field("ButtonNames", &VirtualGamepadSystemComponent::m_buttonNames)
                ->Field("ThumbStickNames", &VirtualGamepadSystemComponent::m_thumbStickNames);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<VirtualGamepadSystemComponent>("VirtualGamepad", "Provides an example of a virtual gamepad that can be used by mobile devices with touch screens in place of a physical gamepad.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &VirtualGamepadSystemComponent::m_buttonNames, "Button Names", "The button names made available by the virtual gamepad.")
                    ->DataElement(0, &VirtualGamepadSystemComponent::m_thumbStickNames, "Thumb-Stick Names", "The thumb-stick names made available by the virtual gamepad.")
                ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    VirtualGamepadSystemComponent::VirtualGamepadSystemComponent()
    {
        m_buttonNames =
        {
            "virtual_gamepad_button_a",
            "virtual_gamepad_button_b",
            "virtual_gamepad_button_x",
            "virtual_gamepad_button_y"
        };

        m_thumbStickNames =
        {
            "virtual_gamepad_thumbstick_l",
            "virtual_gamepad_thumbstick_r"
        };
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadSystemComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadSystemComponent::Activate()
    {
        VirtualGamepadRequestBus::Handler::BusConnect();
        m_virtualGamepad.reset(aznew InputDeviceVirtualGamepad(m_buttonNames,
                                                               m_thumbStickNames));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadSystemComponent::Deactivate()
    {
        m_virtualGamepad.reset(nullptr);
        VirtualGamepadRequestBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::unordered_set<AZStd::string>& VirtualGamepadSystemComponent::GetButtonNames() const
    {
        return m_buttonNames;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::unordered_set<AZStd::string>& VirtualGamepadSystemComponent::GetThumbStickNames() const
    {
        return m_thumbStickNames;
    }
} // namespace VirtualGamepad
