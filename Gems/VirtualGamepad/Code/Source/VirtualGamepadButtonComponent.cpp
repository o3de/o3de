/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "VirtualGamepadButtonComponent.h"

#include <VirtualGamepad/VirtualGamepadBus.h>

#include <LyShine/Bus/UiInteractableBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/std/sort.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadButtonComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("VirtualGamepadButtonService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadButtonComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("VirtualGamepadButtonService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadButtonComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiInteractableService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadButtonComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadButtonComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<VirtualGamepadButtonComponent, AZ::Component>()
                ->Version(0)
                ->Field("AssignedInputChannelName", &VirtualGamepadButtonComponent::m_assignedInputChannelName)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<VirtualGamepadButtonComponent>("VirtualGamepadButton", "A component that designates this entity as a virtual gamepad button")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiVirtualButton.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiVirtualButton.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &VirtualGamepadButtonComponent::m_assignedInputChannelName,
                        "Input Channel", "The input channel that will be updated when the user interacts with this virtual control")
                        ->Attribute(AZ::Edit::Attributes::StringList, &VirtualGamepadButtonComponent::GetAssignableInputChannelNames)
                ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadButtonComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadButtonComponent::Activate()
    {
        VirtualGamepadButtonRequestBus::Handler::BusConnect(m_assignedInputChannelName);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadButtonComponent::Deactivate()
    {
        VirtualGamepadButtonRequestBus::Handler::BusDisconnect(m_assignedInputChannelName);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool VirtualGamepadButtonComponent::IsPressed() const
    {
        bool isPressed = false;
        UiInteractableBus::EventResult(isPressed,
                                       GetEntityId(),
                                       &UiInteractableInterface::IsPressed);
        return isPressed;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AZStd::string> VirtualGamepadButtonComponent::GetAssignableInputChannelNames() const
    {
        AZStd::unordered_set<AZStd::string> buttonNames;
        VirtualGamepadRequestBus::BroadcastResult(buttonNames, &VirtualGamepadRequests::GetButtonNames);

        AZStd::vector<AZStd::string> assignableInputChannelNames;
        for (const AZStd::string& buttonName : buttonNames)
        {
            assignableInputChannelNames.push_back(buttonName);
        }
        AZStd::sort(assignableInputChannelNames.begin(), assignableInputChannelNames.end());
        return assignableInputChannelNames;
    }
} // namespace VirtualGamepad
