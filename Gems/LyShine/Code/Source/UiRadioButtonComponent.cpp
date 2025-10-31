/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiRadioButtonComponent.h"
#include "UiRadioButtonGroupComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiRadioButtonGroupBus.h>
#include <LyShine/Bus/UiRadioButtonGroupCommunicationBus.h>

#include <LyShine/UiSerializeHelpers.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiRadioButtonNotificationBus Behavior context handler class
class UiRadioButtonNotificationBusBehaviorHandler
    : public UiRadioButtonNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiRadioButtonNotificationBusBehaviorHandler, "{182D0EB2-DAD6-4CFC-98E9-185863A78637}", AZ::SystemAllocator,
        OnRadioButtonStateChange);

    void OnRadioButtonStateChange(bool checked) override
    {
        Call(FN_OnRadioButtonStateChange, checked);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRadioButtonComponent::UiRadioButtonComponent()
    : m_isOn(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRadioButtonComponent::~UiRadioButtonComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiRadioButtonComponent::GetState()
{
    return m_isOn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiRadioButtonComponent::GetGroup()
{
    return m_group;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::SetCheckedEntity(AZ::EntityId entityId)
{
    m_optionalCheckedEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiRadioButtonComponent::GetCheckedEntity()
{
    return m_optionalCheckedEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::SetUncheckedEntity(AZ::EntityId entityId)
{
    m_optionalUncheckedEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiRadioButtonComponent::GetUncheckedEntity()
{
    return m_optionalUncheckedEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiRadioButtonComponent::GetTurnOnActionName()
{
    return m_turnOnActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::SetTurnOnActionName(const LyShine::ActionName& actionName)
{
    m_turnOnActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiRadioButtonComponent::GetTurnOffActionName()
{
    return m_turnOffActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::SetTurnOffActionName(const LyShine::ActionName& actionName)
{
    m_turnOffActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiRadioButtonComponent::GetChangedActionName()
{
    return m_changedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::SetChangedActionName(const LyShine::ActionName& actionName)
{
    m_changedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::SetState(bool isOn, bool sendNotifications)
{
    bool oldState = m_isOn;
    m_isOn = isOn;

    // if we changed state, send events and set the correct entity to display
    if (m_isOn != oldState)
    {
        if (m_optionalCheckedEntity.IsValid())
        {
            UiElementBus::Event(m_optionalCheckedEntity, &UiElementBus::Events::SetIsEnabled, m_isOn);
        }

        if (m_optionalUncheckedEntity.IsValid())
        {
            UiElementBus::Event(m_optionalUncheckedEntity, &UiElementBus::Events::SetIsEnabled, !m_isOn);
        }

        if (sendNotifications)
        {
            // Tell any action listeners about the event
            if (m_isOn && !m_turnOnActionName.empty())
            {
                AZ::EntityId canvasEntityId;
                UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
                UiCanvasNotificationBus::Event(
                    canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_turnOnActionName);
            }

            if (!m_isOn && !m_turnOffActionName.empty())
            {
                AZ::EntityId canvasEntityId;
                UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
                UiCanvasNotificationBus::Event(
                    canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_turnOffActionName);
            }

            if (!m_changedActionName.empty())
            {
                AZ::EntityId canvasEntityId;
                UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
                UiCanvasNotificationBus::Event(
                    canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_changedActionName);
            }

            UiRadioButtonNotificationBus::Event(GetEntityId(), &UiRadioButtonNotificationBus::Events::OnRadioButtonStateChange, m_isOn);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::SetGroup(AZ::EntityId group)
{
    m_group = group;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::InGamePostActivate()
{
    // Add this radio button to its group
    UiRadioButtonGroupCommunicationBus::Event(m_group, &UiRadioButtonGroupCommunicationBus::Events::RegisterRadioButton, GetEntityId());

    // Request to be set to its default state
    // If the default state is on
    if (m_isOn)
    {
        // Let the group know about it
        UiRadioButtonGroupBus::Event(m_group, &UiRadioButtonGroupBus::Events::SetState, GetEntityId(), true);
    }
    // Else if the default state is off
    else
    {
        // We need to make sure the on/off entities are displaying correctly, no need to go through group
        if (m_optionalCheckedEntity.IsValid())
        {
            UiElementBus::Event(m_optionalCheckedEntity, &UiElementBus::Events::SetIsEnabled, false);
        }

        if (m_optionalUncheckedEntity.IsValid())
        {
            UiElementBus::Event(m_optionalUncheckedEntity, &UiElementBus::Events::SetIsEnabled, true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiRadioButtonComponent::HandleReleased(AZ::Vector2 point)
{
    bool isInRect = false;
    UiTransformBus::EventResult(isInRect, GetEntityId(), &UiTransformBus::Events::IsPointInRect, point);
    if (isInRect)
    {
        return HandleReleasedCommon(point);
    }
    else
    {
        m_isPressed = false;

        return m_isHandlingEvents;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiRadioButtonComponent::HandleEnterReleased()
{
    AZ::Vector2 point(-1.0f, -1.0f);
    return HandleReleasedCommon(point);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiRadioButtonBus::Handler::BusConnect(GetEntityId());
    UiRadioButtonCommunicationBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonComponent::Deactivate()
{
    UiRadioButtonGroupCommunicationBus::Event(m_group, &UiRadioButtonGroupCommunicationBus::Events::UnregisterRadioButton, GetEntityId());

    UiInteractableComponent::Deactivate();
    UiRadioButtonBus::Handler::BusDisconnect(GetEntityId());
    UiRadioButtonCommunicationBus::Handler::BusDisconnect(GetEntityId());
    UiInitializationBus::Handler::BusDisconnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiRadioButtonComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiRadioButtonComponent, UiInteractableComponent>()
            ->Version(1)
        // Elements group
            ->Field("OptionalCheckedEntity", &UiRadioButtonComponent::m_optionalCheckedEntity)
            ->Field("OptionalUncheckedEntity", &UiRadioButtonComponent::m_optionalUncheckedEntity)
            ->Field("Group", &UiRadioButtonComponent::m_group)
        // Value group
            ->Field("IsChecked", &UiRadioButtonComponent::m_isOn)
        // Actions group
            ->Field("ChangedActionName", &UiRadioButtonComponent::m_changedActionName)
            ->Field("TurnOnActionName", &UiRadioButtonComponent::m_turnOnActionName)
            ->Field("TurnOffActionName", &UiRadioButtonComponent::m_turnOffActionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiRadioButtonComponent>("RadioButton", "An interactable component for RadioButton behavior.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiRadioButton.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiRadioButton.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Elements group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Elements")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiRadioButtonComponent::m_optionalCheckedEntity, "On", "The child element to show when RadioButton is in on state.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiRadioButtonComponent::PopulateChildEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiRadioButtonComponent::m_optionalUncheckedEntity, "Off", "The child element to show when RadioButton is in off state.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiRadioButtonComponent::PopulateChildEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiRadioButtonComponent::m_group, "Group", "The group this radio button belongs to.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiRadioButtonComponent::PopulateGroupsEntityList);
            }

            // Value group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Value")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiRadioButtonComponent::m_isOn, "Checked", "The initial state of the radio button.");
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiRadioButtonComponent::m_changedActionName, "Change", "The action triggered when value changes either way.");
                editInfo->DataElement(0, &UiRadioButtonComponent::m_turnOnActionName, "On", "The action triggered when turned on.");
                editInfo->DataElement(0, &UiRadioButtonComponent::m_turnOffActionName, "Off", "The action triggered when turned off.");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiRadioButtonBus>("UiRadioButtonBus")
            ->Event("GetState", &UiRadioButtonBus::Events::GetState)
            ->Event("GetGroup", &UiRadioButtonBus::Events::GetGroup)
            ->Event("GetCheckedEntity", &UiRadioButtonBus::Events::GetCheckedEntity)
            ->Event("SetCheckedEntity", &UiRadioButtonBus::Events::SetCheckedEntity)
            ->Event("GetUncheckedEntity", &UiRadioButtonBus::Events::GetUncheckedEntity)
            ->Event("SetUncheckedEntity", &UiRadioButtonBus::Events::SetUncheckedEntity)
            ->Event("GetTurnOnActionName", &UiRadioButtonBus::Events::GetTurnOnActionName)
            ->Event("SetTurnOnActionName", &UiRadioButtonBus::Events::SetTurnOnActionName)
            ->Event("GetTurnOffActionName", &UiRadioButtonBus::Events::GetTurnOffActionName)
            ->Event("SetTurnOffActionName", &UiRadioButtonBus::Events::SetTurnOffActionName)
            ->Event("GetChangedActionName", &UiRadioButtonBus::Events::GetChangedActionName)
            ->Event("SetChangedActionName", &UiRadioButtonBus::Events::SetChangedActionName);

        behaviorContext->EBus<UiRadioButtonNotificationBus>("UiRadioButtonNotificationBus")
            ->Handler<UiRadioButtonNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRadioButtonComponent::EntityComboBoxVec UiRadioButtonComponent::PopulateChildEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all child elements
    LyShine::EntityArray matchingElements;
    UiElementBus::Event(
        GetEntityId(),
        &UiElementBus::Events::FindDescendantElements,
        []([[maybe_unused]] const AZ::Entity* entity)
        {
            return true;
        },
        matchingElements);

    // add their names to the StringList and their IDs to the id list
    for (auto childEntity : matchingElements)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRadioButtonComponent::EntityComboBoxVec UiRadioButtonComponent::PopulateGroupsEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all elements in the canvas with the radio button group component
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    LyShine::EntityArray matchingElements;
    UiCanvasBus::Event(
        canvasEntityId,
        &UiCanvasBus::Events::FindElements,
        [](const AZ::Entity* entity)
        {
            return UiRadioButtonGroupBus::FindFirstHandler(entity->GetId()) != nullptr;
        },
        matchingElements);

    // Sort the elements by name
    AZStd::sort(matchingElements.begin(), matchingElements.end(),
        [](const AZ::Entity* e1, const AZ::Entity* e2) { return e1->GetName() < e2->GetName(); });

    // add their names to the StringList and their IDs to the id list
    for (auto childEntity : matchingElements)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiRadioButtonComponent::HandleReleasedCommon([[maybe_unused]] const AZ::Vector2& point)
{
    if (m_isHandlingEvents)
    {
        UiInteractableComponent::TriggerReleasedAction();

        UiRadioButtonGroupCommunicationBus::Event(
            m_group, &UiRadioButtonGroupCommunicationBus::Events::RequestRadioButtonStateChange, GetEntityId(), !m_isOn);
    }

    m_isPressed = false;

    return m_isHandlingEvents;
}
