/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCheckboxComponent.h"
#include "Sprite.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiVisualBus.h>

#include <LyShine/ISprite.h>
#include <LyShine/UiSerializeHelpers.h>
#include "UiSerialize.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCheckboxNotificationBus Behavior context handler class
class UiCheckboxNotificationBusBehaviorHandler
    : public UiCheckboxNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiCheckboxNotificationBusBehaviorHandler, "{718A00EF-119B-4616-9235-F55790640A1E}", AZ::SystemAllocator,
        OnCheckboxStateChange);

    void OnCheckboxStateChange(bool checked) override
    {
        Call(FN_OnCheckboxStateChange, checked);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCheckboxComponent::UiCheckboxComponent()
    : m_isOn(false)
    , m_optionalCheckedEntity()
    , m_optionalUncheckedEntity()
    , m_onChange()
    , m_turnOnActionName()
    , m_turnOffActionName()
    , m_changedActionName()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCheckboxComponent::~UiCheckboxComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCheckboxComponent::GetState()
{
    return m_isOn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::SetState(bool isOn)
{
    m_isOn = isOn;

    if (m_optionalCheckedEntity.IsValid())
    {
        UiElementBus::Event(m_optionalCheckedEntity, &UiElementBus::Events::SetIsEnabled, m_isOn);
    }

    if (m_optionalUncheckedEntity.IsValid())
    {
        UiElementBus::Event(m_optionalUncheckedEntity, &UiElementBus::Events::SetIsEnabled, !m_isOn);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCheckboxComponent::ToggleState()
{
    SetState(!m_isOn);

    return m_isOn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCheckboxComponent::StateChangeCallback UiCheckboxComponent::GetStateChangeCallback()
{
    return m_onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::SetStateChangeCallback(UiCheckboxComponent::StateChangeCallback onChange)
{
    m_onChange = onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::SetCheckedEntity(AZ::EntityId entityId)
{
    m_optionalCheckedEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCheckboxComponent::GetCheckedEntity()
{
    return m_optionalCheckedEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::SetUncheckedEntity(AZ::EntityId entityId)
{
    m_optionalUncheckedEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCheckboxComponent::GetUncheckedEntity()
{
    return m_optionalUncheckedEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiCheckboxComponent::GetTurnOnActionName()
{
    return m_turnOnActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::SetTurnOnActionName(const LyShine::ActionName& actionName)
{
    m_turnOnActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiCheckboxComponent::GetTurnOffActionName()
{
    return m_turnOffActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::SetTurnOffActionName(const LyShine::ActionName& actionName)
{
    m_turnOffActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiCheckboxComponent::GetChangedActionName()
{
    return m_changedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::SetChangedActionName(const LyShine::ActionName& actionName)
{
    m_changedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::InGamePostActivate()
{
    SetState(m_isOn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCheckboxComponent::HandleReleased(AZ::Vector2 point)
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
bool UiCheckboxComponent::HandleEnterReleased()
{
    AZ::Vector2 point(-1.0f, -1.0f);
    return HandleReleasedCommon(point);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiCheckboxBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCheckboxComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiCheckboxBus::Handler::BusDisconnect(GetEntityId());
    UiInitializationBus::Handler::BusDisconnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiCheckboxComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiCheckboxComponent, UiInteractableComponent>()
            ->Version(3, &VersionConverter)
        // Elements group
            ->Field("OptionalCheckedEntity", &UiCheckboxComponent::m_optionalCheckedEntity)
            ->Field("OptionalUncheckedEntity", &UiCheckboxComponent::m_optionalUncheckedEntity)
        // Value group
            ->Field("IsChecked", &UiCheckboxComponent::m_isOn)
        // Actions group
            ->Field("ChangedActionName", &UiCheckboxComponent::m_changedActionName)
            ->Field("TurnOnActionName", &UiCheckboxComponent::m_turnOnActionName)
            ->Field("TurnOffActionName", &UiCheckboxComponent::m_turnOffActionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiCheckboxComponent>("Checkbox", "An interactable component for Checkbox/Toggle behavior.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiCheckbox.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiCheckbox.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Elements group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Elements")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiCheckboxComponent::m_optionalCheckedEntity, "On", "The child element to show when Checkbox is in on state.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiCheckboxComponent::PopulateChildEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiCheckboxComponent::m_optionalUncheckedEntity, "Off", "The child element to show when Checkbox is in off state.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiCheckboxComponent::PopulateChildEntityList);
            }

            // Value group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Value")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiCheckboxComponent::m_isOn, "Checked", "The initial state of the Checkbox.");
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiCheckboxComponent::m_changedActionName, "Change", "The action triggered when value changes either way.");
                editInfo->DataElement(0, &UiCheckboxComponent::m_turnOnActionName, "On", "The action triggered when turned on.");
                editInfo->DataElement(0, &UiCheckboxComponent::m_turnOffActionName, "Off", "The action triggered when turned off.");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiCheckboxBus>("UiCheckboxBus")
            ->Event("GetState", &UiCheckboxBus::Events::GetState)
            ->Event("SetState", &UiCheckboxBus::Events::SetState)
            ->Event("ToggleState", &UiCheckboxBus::Events::ToggleState)
            ->Event("GetCheckedEntity", &UiCheckboxBus::Events::GetCheckedEntity)
            ->Event("SetCheckedEntity", &UiCheckboxBus::Events::SetCheckedEntity)
            ->Event("GetUncheckedEntity", &UiCheckboxBus::Events::GetUncheckedEntity)
            ->Event("SetUncheckedEntity", &UiCheckboxBus::Events::SetUncheckedEntity)
            ->Event("GetTurnOnActionName", &UiCheckboxBus::Events::GetTurnOnActionName)
            ->Event("SetTurnOnActionName", &UiCheckboxBus::Events::SetTurnOnActionName)
            ->Event("GetTurnOffActionName", &UiCheckboxBus::Events::GetTurnOffActionName)
            ->Event("SetTurnOffActionName", &UiCheckboxBus::Events::SetTurnOffActionName)
            ->Event("GetChangedActionName", &UiCheckboxBus::Events::GetChangedActionName)
            ->Event("SetChangedActionName", &UiCheckboxBus::Events::SetChangedActionName);

        behaviorContext->EBus<UiCheckboxNotificationBus>("UiCheckboxNotificationBus")
            ->Handler<UiCheckboxNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCheckboxComponent::EntityComboBoxVec UiCheckboxComponent::PopulateChildEntityList()
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
bool UiCheckboxComponent::HandleReleasedCommon(const AZ::Vector2& point)
{
    if (m_isHandlingEvents)
    {
        SetState(!m_isOn);

        if (m_onChange)
        {
            m_onChange(GetEntityId(), point, m_isOn);
        }

        UiInteractableComponent::TriggerReleasedAction();

        // Tell any action listeners about the event
        if (m_isOn && !m_turnOnActionName.empty())
        {
            AZ::EntityId canvasEntityId;
            UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
            UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_turnOnActionName);
        }

        if (!m_isOn && !m_turnOffActionName.empty())
        {
            AZ::EntityId canvasEntityId;
            UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
            UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_turnOffActionName);
        }

        if (!m_changedActionName.empty())
        {
            AZ::EntityId canvasEntityId;
            UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
            UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_changedActionName);
        }

        UiCheckboxNotificationBus::Event(GetEntityId(), &UiCheckboxNotificationBus::Events::OnCheckboxStateChange, m_isOn);
    }

    m_isPressed = false;

    return m_isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCheckboxComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to 2:
    // - Need to convert AZStd::string sprites to AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>
    if (classElement.GetVersion() < 2)
    {
        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "SelectedSprite"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "DisabledSprite"))
        {
            return false;
        }
    }

    // Conversion from version 2 to 3:
    if (classElement.GetVersion() < 3)
    {
        // find the base class (AZ::Component)
        // NOTE: in very old versions there may not be a base class because the base class was not serialized
        int componentBaseClassIndex = classElement.FindElement(AZ_CRC_CE("BaseClass1"));

        // If there was a base class, make a copy and remove it
        AZ::SerializeContext::DataElementNode componentBaseClassNode;
        if (componentBaseClassIndex != -1)
        {
            // make a local copy of the component base class node
            componentBaseClassNode = classElement.GetSubElement(componentBaseClassIndex);

            // remove the component base class from the button
            classElement.RemoveElement(componentBaseClassIndex);
        }

        // Add a new base class (UiInteractableComponent)
        int interactableBaseClassIndex = classElement.AddElement<UiInteractableComponent>(context, "BaseClass1");
        AZ::SerializeContext::DataElementNode& interactableBaseClassNode = classElement.GetSubElement(interactableBaseClassIndex);

        // if there was previously a base class...
        if (componentBaseClassIndex != -1)
        {
            // copy the component base class into the new interactable base class
            // Since AZ::Component is now the base class of UiInteractableComponent
            interactableBaseClassNode.AddElement(componentBaseClassNode);
        }

        // Move the selected/hover state to the base class
        if (!UiSerialize::MoveToInteractableStateActions(context, classElement, "HoverStateActions",
                "SelectedColor", "SelectedAlpha", "SelectedSprite"))
        {
            return false;
        }

        // Move the disabled state to the base class
        if (!UiSerialize::MoveToInteractableStateActions(context, classElement, "DisabledStateActions",
                "DisabledColor", "DisabledAlpha", "DisabledSprite"))
        {
            return false;
        }
    }

    return true;
}
