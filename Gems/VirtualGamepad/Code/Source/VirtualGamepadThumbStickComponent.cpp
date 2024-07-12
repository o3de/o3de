/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "VirtualGamepadThumbStickComponent.h"

#include <VirtualGamepad/VirtualGamepadBus.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/std/sort.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    static const int InactiveTouchIndex = -1;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    static const int PrimaryTouchIndex = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("VirtualGamepadThumbStickService"));
        provided.push_back(AZ_CRC_CE("UiInteractableService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("VirtualGamepadThumbStickService"));
        incompatible.push_back(AZ_CRC_CE("UiInteractableService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<VirtualGamepadThumbStickComponent, AZ::Component>()
                ->Version(0)
                ->Field("AssignedInputChannelName", &VirtualGamepadThumbStickComponent::m_assignedInputChannelName)
                ->Field("ThumbStickImageCentre", &VirtualGamepadThumbStickComponent::m_thumbStickImageCentre)
                ->Field("ThumbStickImageRadial", &VirtualGamepadThumbStickComponent::m_thumbStickImageRadial)
                ->Field("CentreWhenPressed", &VirtualGamepadThumbStickComponent::m_centreWhenPressed)
                ->Field("AdjustPositionWhilePressed", &VirtualGamepadThumbStickComponent::m_adjustPositionWhilePressed)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<VirtualGamepadThumbStickComponent>("VirtualGamepadThumbStick", "A component that designates this entity as a virtual gamepad thumb-stick")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiVirtualThumbStick.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiVirtualThumbStick.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &VirtualGamepadThumbStickComponent::m_assignedInputChannelName,
                        "Input Channel", "The input channel that will be updated when the user interacts with this virtual control")
                        ->Attribute(AZ::Edit::Attributes::StringList, &VirtualGamepadThumbStickComponent::GetAssignableInputChannelNames)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &VirtualGamepadThumbStickComponent::m_thumbStickImageCentre,
                        "Thumb Stick Image Centre", "The child element that will be positioned at the centre of the virtual thumb-stick.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &VirtualGamepadThumbStickComponent::GetChildEntityIdNamePairs)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &VirtualGamepadThumbStickComponent::m_thumbStickImageRadial,
                        "Thumb Stick Image Radial", "The child element that will be positioned under the user's finger while the virtual thumb-stick is active.\n"
                                                    "The position of this image will always be clamped to the radial edge of the virtual thumb-stick centre image.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &VirtualGamepadThumbStickComponent::GetChildEntityIdNamePairs)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &VirtualGamepadThumbStickComponent::m_centreWhenPressed,
                        "Centre When Pressed", "Whether or not to centre the virtual thumb-stick when it is pressed.")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &VirtualGamepadThumbStickComponent::m_adjustPositionWhilePressed,
                        "Adjust Position While Pressed", "Whether or not to adjust the position of the virtual thumb-stick while it is active,\n"
                                                         "such that it will track the user's finger when it moves outside the thumb-stick radius.")
                ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::Activate()
    {
        m_activeTouchIndex = InactiveTouchIndex;
        m_currentAxisValuesNormalized = AZ::Vector2::CreateZero();
        m_currentViewportPositionPixels = AZ::Vector2::CreateZero();

        VirtualGamepadThumbStickRequestBus::Handler::BusConnect(m_assignedInputChannelName);
        UiInteractableBus::Handler::BusConnect(GetEntityId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::Deactivate()
    {
        UiInteractableBus::Handler::BusDisconnect(GetEntityId());
        VirtualGamepadThumbStickRequestBus::Handler::BusDisconnect(m_assignedInputChannelName);

        m_currentViewportPositionPixels = AZ::Vector2::CreateZero();
        m_currentAxisValuesNormalized = AZ::Vector2::CreateZero();
        m_activeTouchIndex = InactiveTouchIndex;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool VirtualGamepadThumbStickComponent::CanHandleEvent(AZ::Vector2 point)
    {
        AZ_UNUSED(point);
        return m_activeTouchIndex == InactiveTouchIndex;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool VirtualGamepadThumbStickComponent::HandlePressed(AZ::Vector2 point, bool& shouldStayActive)
    {
        shouldStayActive = false;
        return OnAnyTouchPressed(point, PrimaryTouchIndex);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool VirtualGamepadThumbStickComponent::HandleReleased(AZ::Vector2 point)
    {
        return OnAnyTouchReleased(point, PrimaryTouchIndex);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool VirtualGamepadThumbStickComponent::HandleMultiTouchPressed(AZ::Vector2 point, int multiTouchIndex)
    {
        return OnAnyTouchPressed(point, multiTouchIndex);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool VirtualGamepadThumbStickComponent::HandleMultiTouchReleased(AZ::Vector2 point, int multiTouchIndex)
    {
        return OnAnyTouchReleased(point, multiTouchIndex);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::InputPositionUpdate(AZ::Vector2 point)
    {
        OnAnyTouchPositionUpdate(point, PrimaryTouchIndex);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::MultiTouchPositionUpdate(AZ::Vector2 point, int multiTouchIndex)
    {
        OnAnyTouchPositionUpdate(point, multiTouchIndex);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 VirtualGamepadThumbStickComponent::GetCurrentAxisValuesNormalized() const
    {
        return m_currentAxisValuesNormalized;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool VirtualGamepadThumbStickComponent::OnAnyTouchPressed(AZ::Vector2 viewportPositionPixels,
                                                              int touchIndex)
    {
        if (m_activeTouchIndex != InactiveTouchIndex)
        {
            return false;
        }

        // Set the active touch index, current thumb-stick position, and axis values
        m_activeTouchIndex = touchIndex;
        m_currentAxisValuesNormalized = AZ::Vector2::CreateZero();

        // Store the default thumb-stick position and radius
        UiTransformInterface::RectPoints rectPoints;
        UiTransformBus::Event(m_thumbStickImageCentre,
                              &UiTransformInterface::GetViewportSpacePoints,
                              rectPoints);
        m_thumbStickPixelRadius = AZStd::max(rectPoints.GetAxisAlignedSize().GetX() * 0.5f, 1.0f);
        UiTransformBus::EventResult(m_defaultViewportPositionPixels,
                                    m_thumbStickImageCentre,
                                    &UiTransformInterface::GetViewportPosition);

        if (m_centreWhenPressed)
        {
            // Position both thumb-stick images at the touch start position
            m_currentViewportPositionPixels = viewportPositionPixels;
            UiTransformBus::Event(m_thumbStickImageCentre,
                                  &UiTransformInterface::SetViewportPosition,
                                  m_currentViewportPositionPixels);
            UiTransformBus::Event(m_thumbStickImageRadial,
                                  &UiTransformInterface::SetViewportPosition,
                                  m_currentViewportPositionPixels);
        }
        else
        {
            // Leave both thumb-sticks at their default position
            m_currentViewportPositionPixels = m_defaultViewportPositionPixels;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool VirtualGamepadThumbStickComponent::OnAnyTouchReleased(AZ::Vector2 viewportPositionPixels,
                                                               int touchIndex)
    {
        AZ_UNUSED(viewportPositionPixels);
        if (m_activeTouchIndex != touchIndex)
        {
            return false;
        }

        // Reset the active touch index, current thumb-stick position, and axis values
        m_activeTouchIndex = InactiveTouchIndex;
        m_currentViewportPositionPixels = AZ::Vector2::CreateZero();
        m_currentAxisValuesNormalized = AZ::Vector2::CreateZero();

        // Position both thumb-stick images at their default position
        UiTransformBus::Event(m_thumbStickImageCentre,
                              &UiTransformInterface::SetViewportPosition,
                              m_defaultViewportPositionPixels);
        UiTransformBus::Event(m_thumbStickImageRadial,
                              &UiTransformInterface::SetViewportPosition,
                              m_defaultViewportPositionPixels);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void VirtualGamepadThumbStickComponent::OnAnyTouchPositionUpdate(AZ::Vector2 viewportPositionPixels,
                                                                     [[maybe_unused]] int touchIndex)
    {
        // Calculate the current virtual thumb-stick axis values
        AZ::Vector2 pixelDelta = viewportPositionPixels - m_currentViewportPositionPixels;
        const float deltaLength = pixelDelta.GetLength();
        if (deltaLength > m_thumbStickPixelRadius)
        {
            if (m_adjustPositionWhilePressed)
            {
                // Reposition the centre virtual thumb-stick image so the press stays at the edge
                AZ::Vector2 radialDelta = pixelDelta;
                radialDelta.SetLength(deltaLength - m_thumbStickPixelRadius);
                m_currentViewportPositionPixels += radialDelta;
                UiTransformBus::Event(m_thumbStickImageCentre,
                                      &UiTransformInterface::SetViewportPosition,
                                      m_currentViewportPositionPixels);
            }

            // Clamp the pixel delta to the radius of the thumb-stick
            pixelDelta *= m_thumbStickPixelRadius / deltaLength;
        }

        // Position the radial thumb-stick image accordingly
        const AZ::Vector2 radialImagePosition = m_currentViewportPositionPixels + pixelDelta;
        UiTransformBus::Event(m_thumbStickImageRadial,
                              &UiTransformInterface::SetViewportPosition,
                              radialImagePosition);

        // Set the current normalized axis values
        m_currentAxisValuesNormalized.SetX(pixelDelta.GetX() / m_thumbStickPixelRadius);
        m_currentAxisValuesNormalized.SetY(-pixelDelta.GetY() / m_thumbStickPixelRadius);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AZStd::string> VirtualGamepadThumbStickComponent::GetAssignableInputChannelNames() const
    {
        AZStd::unordered_set<AZStd::string> buttonNames;
        VirtualGamepadRequestBus::BroadcastResult(buttonNames, &VirtualGamepadRequests::GetThumbStickNames);

        AZStd::vector<AZStd::string> assignableInputChannelNames;
        for (const AZStd::string& buttonName : buttonNames)
        {
            assignableInputChannelNames.push_back(buttonName);
        }
        AZStd::sort(assignableInputChannelNames.begin(), assignableInputChannelNames.end());
        return assignableInputChannelNames;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::string>> VirtualGamepadThumbStickComponent::GetChildEntityIdNamePairs() const
    {
        AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::string>> result;

        // Add a first entry for "None"
        result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

        // Get a list of all child elements and add them to the result
        LyShine::EntityArray childElements;
        UiElementBus::EventResult(childElements, GetEntityId(), &UiElementInterface::GetChildElements);
        for (const auto& childElement : childElements)
        {
            result.push_back(AZStd::make_pair(AZ::EntityId(childElement->GetId()), childElement->GetName()));
        }

        return result;
    }
} // namespace VirtualGamepad
