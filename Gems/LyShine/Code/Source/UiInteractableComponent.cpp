/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiInteractableComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

#include <LyShine/ISprite.h>
#include <LyShine/UiSerializeHelpers.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiInteractableNotificationBus Behavior context handler class
class BehaviorUiInteractableNotificationBusHandler
    : public UiInteractableNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiInteractableNotificationBusHandler, "{BBF912EB-8F45-4869-B1F0-19CDA9D16231}", AZ::SystemAllocator,
        OnHoverStart, OnHoverEnd, OnPressed, OnReleased, OnReceivedHoverByNavigatingFromDescendant);

    void OnHoverStart() override
    {
        Call(FN_OnHoverStart);
    }

    void OnHoverEnd() override
    {
        Call(FN_OnHoverEnd);
    }

    void OnPressed() override
    {
        Call(FN_OnPressed);
    }

    void OnReleased() override
    {
        Call(FN_OnReleased);
    }

    void OnReceivedHoverByNavigatingFromDescendant(AZ::EntityId descendantEntityId) override
    {
        Call(FN_OnReceivedHoverByNavigatingFromDescendant, descendantEntityId);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableComponent::UiInteractableComponent()
    : m_isAutoActivationEnabled(false)
    , m_isHandlingEvents(true)
    , m_isHandlingMultiTouchEvents(true)
    , m_isHover(false)
    , m_isPressed(false)
    , m_pressedPoint(0.0f, 0.0f)
    , m_pressedMultiTouchIndex(0)
    , m_state(UiInteractableStatesInterface::StateNormal)
    , m_hoverStartActionCallback(nullptr)
    , m_hoverEndActionCallback(nullptr)
    , m_pressedActionCallback(nullptr)
    , m_releasedActionCallback(nullptr)
{
    // Must be called in the same order as the states defined in UiInteractableStatesInterface
    m_stateActionManager.AddState(nullptr); // normal state has no state actions
    m_stateActionManager.AddState(&m_hoverStateActions);
    m_stateActionManager.AddState(&m_pressedStateActions);
    m_stateActionManager.AddState(&m_disabledStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableComponent::~UiInteractableComponent()
{
    if (m_isPressed && m_entity)
    {
        UiInteractableActiveNotificationBus::Event(GetEntityId(), &UiInteractableActiveNotificationBus::Events::ActiveCancelled);
        m_isPressed = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::CanHandleEvent([[maybe_unused]] AZ::Vector2 point)
{
    return m_isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::HandlePressed(AZ::Vector2 point, bool& shouldStayActive)
{
    bool handled = false;

    if (m_isHandlingEvents)
    {
        m_isPressed = true;
        m_pressedPoint = point;

        shouldStayActive = false;
        handled =  true;

        TriggerPressedAction();
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::HandleReleased([[maybe_unused]] AZ::Vector2 point)
{
    m_isPressed = false;

    TriggerReleasedAction();

    return m_isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::HandleMultiTouchPressed(AZ::Vector2 point, int multiTouchIndex)
{
    m_pressedMultiTouchIndex = multiTouchIndex;
    bool shouldStayActive = false;
    return m_isHandlingMultiTouchEvents && HandlePressed(point, shouldStayActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::HandleMultiTouchReleased(AZ::Vector2 point, int multiTouchIndex)
{
    AZ_UNUSED(multiTouchIndex);
    bool handled = m_isHandlingMultiTouchEvents && HandleReleased(point);
    m_pressedMultiTouchIndex = 0;
    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::HandleEnterPressed(bool& shouldStayActive)
{
    bool handled = false;

    if (m_isHandlingEvents)
    {
        m_isPressed = true;
        m_pressedPoint = AZ::Vector2(-1.0f, -1.0f);

        shouldStayActive = false;
        handled = true;

        TriggerPressedAction();
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::HandleEnterReleased()
{
    m_isPressed = false;

    TriggerReleasedAction();

    return m_isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::InputPositionUpdate(AZ::Vector2 point)
{
    if (m_isPressed)
    {
        AZ::EntityId parentDraggable;
        UiElementBus::EventResult(parentDraggable, GetEntityId(), &UiElementBus::Events::FindParentInteractableSupportingDrag, point);

        if (parentDraggable.IsValid())
        {
            const float containedDragThreshold = 5.0f;

            // offer the parent draggable the chance to become the active interactable
            bool handOff = false;
            UiInteractableBus::EventResult(
                handOff,
                parentDraggable,
                &UiInteractableBus::Events::OfferDragHandOff,
                GetEntityId(),
                m_pressedPoint,
                point,
                containedDragThreshold);

            if (handOff)
            {
                // interaction has been handed off to a container entity
                m_isPressed = false;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::MultiTouchPositionUpdate(AZ::Vector2 point, int multiTouchIndex)
{
    AZ_UNUSED(multiTouchIndex);
    if (m_isHandlingMultiTouchEvents)
    {
        InputPositionUpdate(point);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::LostActiveStatus()
{
    m_isPressed = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::HandleHoverStart()
{
    m_isHover = true;

    TriggerHoverStartAction();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::HandleHoverEnd()
{
    m_isHover = false;

    TriggerHoverEndAction();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::HandleReceivedHoverByNavigatingFromDescendant(AZ::EntityId descendantEntityId)
{
    TriggerReceivedHoverByNavigatingFromDescendantAction(descendantEntityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::IsPressed()
{
    return m_isPressed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::IsHandlingEvents()
{
    return m_isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetIsHandlingEvents(bool isHandlingEvents)
{
    m_isHandlingEvents = isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::IsHandlingMultiTouchEvents()
{
    return m_isHandlingMultiTouchEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetIsHandlingMultiTouchEvents(bool isHandlingMultiTouchEvents)
{
    m_isHandlingMultiTouchEvents = isHandlingMultiTouchEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::GetIsAutoActivationEnabled()
{
    return m_isAutoActivationEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetIsAutoActivationEnabled(bool isEnabled)
{
    m_isAutoActivationEnabled = isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiInteractableComponent::GetHoverStartActionName()
{
    return m_hoverStartActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetHoverStartActionName(const LyShine::ActionName& actionName)
{
    m_hoverStartActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiInteractableComponent::GetHoverEndActionName()
{
    return m_hoverEndActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetHoverEndActionName(const LyShine::ActionName& actionName)
{
    m_hoverEndActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiInteractableComponent::GetPressedActionName()
{
    return m_pressedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetPressedActionName(const LyShine::ActionName& actionName)
{
    m_pressedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiInteractableComponent::GetReleasedActionName()
{
    return m_releasedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetReleasedActionName(const LyShine::ActionName& actionName)
{
    m_releasedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetOutsideReleasedActionName(const LyShine::ActionName& actionName)
{
    m_outsideReleasedActionName = actionName;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiInteractableComponent::GetOutsideReleasedActionName() const
{
    return m_outsideReleasedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableActionsInterface::OnActionCallback UiInteractableComponent::GetHoverStartActionCallback()
{
    return m_hoverStartActionCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetHoverStartActionCallback(OnActionCallback onActionCallback)
{
    m_hoverStartActionCallback = onActionCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableActionsInterface::OnActionCallback UiInteractableComponent::GetHoverEndActionCallback()
{
    return m_hoverEndActionCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetHoverEndActionCallback(OnActionCallback onActionCallback)
{
    m_hoverEndActionCallback = onActionCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableActionsInterface::OnActionCallback UiInteractableComponent::GetPressedActionCallback()
{
    return m_pressedActionCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetPressedActionCallback(OnActionCallback onActionCallback)
{
    m_pressedActionCallback = onActionCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableActionsInterface::OnActionCallback UiInteractableComponent::GetReleasedActionCallback()
{
    return m_releasedActionCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::SetReleasedActionCallback(OnActionCallback onActionCallback)
{
    m_releasedActionCallback = onActionCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::Update(float /* deltaTime */)
{
    // This currently happens every frame. Needs optimization to just happen on events
    UiInteractableStatesInterface::State state = ComputeInteractableState();

    if (state != m_state)
    {
        m_stateActionManager.ResetAllOverrides();
        m_stateActionManager.ApplyStateActions(state);
        m_state = state;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::OnUiElementFixup(AZ::EntityId canvasEntityId, AZ::EntityId /*parentEntityId*/)
{
    bool isElementEnabled = false;
    UiElementBus::EventResult(isElementEnabled, GetEntityId(), &UiElementBus::Events::GetAreElementAndAncestorsEnabled);
    if (isElementEnabled)
    {
        UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::OnUiElementAndAncestorsEnabledChanged(bool areElementAndAncestorsEnabled)
{
    if (areElementAndAncestorsEnabled)
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        if (canvasEntityId.IsValid())
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }
    else
    {
        UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiInteractableComponent, AZ::Component>()
            ->Version(2, &VersionConverter)
            ->Field("IsHandlingEvents", &UiInteractableComponent::m_isHandlingEvents)
            ->Field("IsHandlingMultiTouchEvents", &UiInteractableComponent::m_isHandlingMultiTouchEvents)

            ->Field("HoverStateActions", &UiInteractableComponent::m_hoverStateActions)
            ->Field("PressedStateActions", &UiInteractableComponent::m_pressedStateActions)
            ->Field("DisabledStateActions", &UiInteractableComponent::m_disabledStateActions)

            ->Field("NavigationSettings", &UiInteractableComponent::m_navigationSettings)

            ->Field("IsAutoActivationEnabled", &UiInteractableComponent::m_isAutoActivationEnabled)

            ->Field("HoverStartActionName", &UiInteractableComponent::m_hoverStartActionName)
            ->Field("HoverEndActionName", &UiInteractableComponent::m_hoverEndActionName)

            ->Field("OutsideReleasedActionName", &UiInteractableComponent::m_outsideReleasedActionName)

            ->Field("PressedActionName", &UiInteractableComponent::m_pressedActionName)
            ->Field("ReleasedActionName", &UiInteractableComponent::m_releasedActionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiInteractableComponent>("Interactable", "Common settings for all interactable components");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement("CheckBox", &UiInteractableComponent::m_isHandlingEvents, "Input enabled",
                "When checked, this interactable will handle events.\n"
                "When unchecked, this interactable is drawn in the Disabled state.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));

            editInfo->DataElement("CheckBox", &UiInteractableComponent::m_isHandlingMultiTouchEvents, "Multi-touch input enabled",
                "When checked, this interactable will handle all multi-touch input events.\n"
                "When unchecked, this interactable will handle only primary touch input events.\n"
                "Will be ignored if the parent UICanvasComponent does not support multi-touch.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiInteractableComponent::IsHandlingEvents);

            // Navigation
            editInfo->DataElement(0, &UiInteractableComponent::m_navigationSettings, "Navigation",
                "How to navigate from this interactbale to the next interactable");

            editInfo->DataElement(0, &UiInteractableComponent::m_isAutoActivationEnabled, "Auto activate",
                "When checked, this interactable will automatically become active when navigated to with a gamepad/keyboard.\n"
                "When unchecked, a button press is required to activate/deactivate this interactable.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiInteractableComponent::IsAutoActivationSupported);

            // States Group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "States")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiInteractableComponent::m_hoverStateActions, "Hover", "The hover/selected state actions")
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &UiInteractableComponent::OnHoverStateActionsChanged);

                editInfo->DataElement(0, &UiInteractableComponent::m_pressedStateActions, "Pressed", "The pressed state actions")
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &UiInteractableComponent::OnPressedStateActionsChanged);

                editInfo->DataElement(0, &UiInteractableComponent::m_disabledStateActions, "Disabled", "The disabled state actions")
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &UiInteractableComponent::OnDisabledStateActionsChanged);
            }

            // Actions Group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiInteractableComponent::m_hoverStartActionName, "Hover start", "Action triggered on hover start");
                editInfo->DataElement(0, &UiInteractableComponent::m_hoverEndActionName, "Hover end", "Action triggered on hover end");
                editInfo->DataElement(0, &UiInteractableComponent::m_pressedActionName, "Pressed", "Action triggered on press");
                editInfo->DataElement(0, &UiInteractableComponent::m_releasedActionName, "Released", "Action triggered on release");
                editInfo->DataElement(0, &UiInteractableComponent::m_outsideReleasedActionName, "Outside Released", "Action triggered on release outside of element");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiInteractableBus>("UiInteractableBus")
            ->Event("IsHandlingEvents", &UiInteractableBus::Events::IsHandlingEvents)
            ->Event("SetIsHandlingEvents", &UiInteractableBus::Events::SetIsHandlingEvents)
            ->Event("IsHandlingMultiTouchEvents", &UiInteractableBus::Events::IsHandlingMultiTouchEvents)
            ->Event("SetIsHandlingMultiTouchEvents", &UiInteractableBus::Events::SetIsHandlingMultiTouchEvents)
            ->Event("GetIsAutoActivationEnabled", &UiInteractableBus::Events::GetIsAutoActivationEnabled)
            ->Event("LostActiveStatus", &UiInteractableBus::Events::LostActiveStatus)
            ->Event("SetIsAutoActivationEnabled", &UiInteractableBus::Events::SetIsAutoActivationEnabled);

        behaviorContext->EBus<UiInteractableActionsBus>("UiInteractableActionsBus")
            ->Event("GetHoverStartActionName", &UiInteractableActionsBus::Events::GetHoverStartActionName)
            ->Event("SetHoverStartActionName", &UiInteractableActionsBus::Events::SetHoverStartActionName)
            ->Event("GetHoverEndActionName", &UiInteractableActionsBus::Events::GetHoverEndActionName)
            ->Event("SetHoverEndActionName", &UiInteractableActionsBus::Events::SetHoverEndActionName)
            ->Event("GetPressedActionName", &UiInteractableActionsBus::Events::GetPressedActionName)
            ->Event("SetPressedActionName", &UiInteractableActionsBus::Events::SetPressedActionName)
            ->Event("GetReleasedActionName", &UiInteractableActionsBus::Events::GetReleasedActionName)
            ->Event("SetReleasedActionName", &UiInteractableActionsBus::Events::SetReleasedActionName);

        behaviorContext->Enum<(int)UiInteractableStatesInterface::StateNormal>("eUiInteractableState_Normal")
            ->Enum<(int)UiInteractableStatesInterface::StateHover>("eUiInteractableState_Hover")
            ->Enum<(int)UiInteractableStatesInterface::StatePressed>("eUiInteractableState_Pressed")
            ->Enum<(int)UiInteractableStatesInterface::StateDisabled>("eUiInteractableState_Disabled");

        behaviorContext->EBus<UiInteractableStatesBus>("UiInteractableStatesBus")
            ->Event("GetStateColor", &UiInteractableStatesBus::Events::GetStateColor)
            ->Event("SetStateColor", &UiInteractableStatesBus::Events::SetStateColor)
            ->Event("HasStateColor", &UiInteractableStatesBus::Events::HasStateColor)
            ->Event("GetStateAlpha", &UiInteractableStatesBus::Events::GetStateAlpha)
            ->Event("SetStateAlpha", &UiInteractableStatesBus::Events::SetStateAlpha)
            ->Event("HasStateAlpha", &UiInteractableStatesBus::Events::HasStateAlpha)
            ->Event("GetStateSpritePathname", &UiInteractableStatesBus::Events::GetStateSpritePathname)
            ->Event("SetStateSpritePathname", &UiInteractableStatesBus::Events::SetStateSpritePathname)
            ->Event("HasStateSprite", &UiInteractableStatesBus::Events::HasStateSprite)
            ->Event("GetStateFontPathname", &UiInteractableStatesBus::Events::GetStateFontPathname)
            ->Event("GetStateFontEffectIndex", &UiInteractableStatesBus::Events::GetStateFontEffectIndex)
            ->Event("SetStateFont", &UiInteractableStatesBus::Events::SetStateFont)
            ->Event("HasStateFont", &UiInteractableStatesBus::Events::HasStateFont);

        behaviorContext->EBus<UiInteractableNotificationBus>("UiInteractableNotificationBus")
            ->Handler<BehaviorUiInteractableNotificationBusHandler>();
    }

    UiInteractableStateAction::Reflect(context);
    UiInteractableStateColor::Reflect(context);
    UiInteractableStateAlpha::Reflect(context);
    UiInteractableStateSprite::Reflect(context);
    UiInteractableStateFont::Reflect(context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::Init()
{
    m_stateActionManager.Init(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::Activate()
{
    m_stateActionManager.Activate();
    m_navigationSettings.Activate(m_entity->GetId(), GetNavigableInteractables);

    UiInteractableBus::Handler::BusConnect(GetEntityId());
    UiInteractableActionsBus::Handler::BusConnect(GetEntityId());
    UiElementNotificationBus::Handler::BusConnect(GetEntityId());

    // The first time the component is activated the owning canvas will not be known. However if
    // the element is fixed up and then we deactivate and reactivate, OnUiElementFixup will
    // not get called again. So we need to connect to the UiCanvasUpdateNotificationBus here.
    // This assumes than on an element activate it will activate the UiElementComponent before
    // this component. We can rely on this because all UI components depend on UiElementService
    // as a required service.
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    if (canvasEntityId.IsValid())
    {
        bool isElementEnabled = false;
        UiElementBus::EventResult(isElementEnabled, GetEntityId(), &UiElementBus::Events::GetAreElementAndAncestorsEnabled);
        if (isElementEnabled)
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::Deactivate()
{
    m_stateActionManager.Deactivate();
    m_navigationSettings.Deactivate();

    UiInteractableBus::Handler::BusDisconnect();
    UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    UiElementNotificationBus::Handler::BusDisconnect();
    UiInteractableActionsBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStatesInterface::State UiInteractableComponent::ComputeInteractableState()
{
    UiInteractableStatesInterface::State state = UiInteractableStatesInterface::StateNormal;

    if (!m_isHandlingEvents)
    {
        // not handling events, use disabled state
        state = UiInteractableStatesInterface::StateDisabled;
    }
    else if (m_isPressed && m_isHover)
    {
        // We only use the pressed state when the button is pressed AND the mouse is over it
        state = UiInteractableStatesInterface::StatePressed;
    }
    else if (m_isHover || m_isPressed)
    {
        // we use the hover state for normal hover but also if the button is pressed but
        // the mouse is outside the button
        state = UiInteractableStatesInterface::StateHover;
    }

    return state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::OnHoverStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_hoverStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::OnPressedStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_pressedStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::OnDisabledStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_disabledStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::TriggerHoverStartAction()
{
    // if a C++ callback is registered for hover start then call it
    if (m_hoverStartActionCallback)
    {
        m_hoverStartActionCallback(GetEntityId());
    }

    UiInteractableNotificationBus::Event(GetEntityId(), &UiInteractableNotificationBus::Events::OnHoverStart);

    // Tell any action listeners about the event
    if (!m_hoverStartActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_hoverStartActionName);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::TriggerHoverEndAction()
{
    // if a C++ callback is registered for hover end then call it
    if (m_hoverEndActionCallback)
    {
        m_hoverEndActionCallback(GetEntityId());
    }

    UiInteractableNotificationBus::Event(GetEntityId(), &UiInteractableNotificationBus::Events::OnHoverEnd);

    // Tell any action listeners about the event
    if (!m_hoverEndActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_hoverEndActionName);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::TriggerPressedAction()
{
    // if a C++ callback is registered for pressed then call it
    if (m_pressedActionCallback)
    {
        m_pressedActionCallback(GetEntityId());
    }

    // Queue the event to prevent deletions during the input event
    UiInteractableNotificationBus::QueueEvent(GetEntityId(), &UiInteractableNotificationBus::Events::OnPressed);

    // Tell any action listeners about the event
    if (!m_pressedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        // Queue the event to prevent deletions during the input event
        UiCanvasNotificationBus::QueueEvent(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_pressedActionName);
        UiCanvasNotificationBus::QueueEvent(canvasEntityId, &UiCanvasNotificationBus::Events::OnActionMultitouch, GetEntityId(), m_pressedActionName, m_pressedPoint, m_pressedMultiTouchIndex);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::TriggerReleasedAction(bool releasedOutside)
{
    // if a C++ callback is registered for released then call it
    if (m_releasedActionCallback)
    {
        m_releasedActionCallback(GetEntityId());
    }

    // Queue the event to prevent deletions during the input event
    UiInteractableNotificationBus::QueueEvent(GetEntityId(), &UiInteractableNotificationBus::Events::OnReleased);

    if (releasedOutside && !m_outsideReleasedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        // Queue the event to prevent deletions during the input event

        UiCanvasNotificationBus::QueueEvent(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_outsideReleasedActionName);
        UiCanvasNotificationBus::QueueEvent(canvasEntityId, &UiCanvasNotificationBus::Events::OnActionMultitouch, GetEntityId(), m_outsideReleasedActionName, m_pressedPoint, m_pressedMultiTouchIndex);
        return;
    }

    // Tell any action listeners about the event
    if (!m_releasedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        // Queue the event to prevent deletions during the input event
        UiCanvasNotificationBus::QueueEvent(
            canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_releasedActionName);
        UiCanvasNotificationBus::QueueEvent(canvasEntityId, &UiCanvasNotificationBus::Events::OnActionMultitouch, GetEntityId(), m_releasedActionName, m_pressedPoint, m_pressedMultiTouchIndex);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableComponent::TriggerReceivedHoverByNavigatingFromDescendantAction(AZ::EntityId descendantEntityId)
{
    UiInteractableNotificationBus::Event(
        GetEntityId(), &UiInteractableNotificationBus::Events::OnReceivedHoverByNavigatingFromDescendant, descendantEntityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::IsAutoActivationSupported()
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiInteractableComponent::GetNavigableInteractables(AZ::EntityId entityId)
{
    // Get a list of all navigable elements
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, entityId, &UiElementBus::Events::GetCanvasEntityId);
    LyShine::EntityArray navigableElements;
    UiCanvasBus::Event(
        canvasEntityId,
        &UiCanvasBus::Events::FindElements,
        [entityId](const AZ::Entity* entity)
        {
            bool navigable = false;
            if (entity->GetId() != entityId)
            {
                if (UiInteractableBus::FindFirstHandler(entity->GetId()))
                {
                    UiNavigationInterface::NavigationMode navigationMode = UiNavigationInterface::NavigationMode::None;
                    UiNavigationBus::EventResult(navigationMode, entity->GetId(), &UiNavigationBus::Events::GetNavigationMode);
                    navigable = (navigationMode != UiNavigationInterface::NavigationMode::None);
                }
            }
            return navigable;
        },
        navigableElements);

    return navigableElements;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1:
    // - Need to move the navigation settings into a sub element UiNavigationSettings
    if (classElement.GetVersion() <= 1)
    {
        int navModeIndex = classElement.FindElement(AZ_CRC_CE("NavigationMode"));
        int navUpIndex = classElement.FindElement(AZ_CRC_CE("OnUpEntity"));
        int navDownIndex = classElement.FindElement(AZ_CRC_CE("OnDownEntity"));
        int navLeftIndex = classElement.FindElement(AZ_CRC_CE("OnLeftEntity"));
        int navRightIndex = classElement.FindElement(AZ_CRC_CE("OnRightEntity"));

        if (navModeIndex == -1 || navUpIndex == -1 || navDownIndex == -1 || navLeftIndex == -1 || navRightIndex == -1)
        {
            AZ_Error("Serialization", false, "UiInteractableComponent version conversion failed finding navigation fields");
            return false;
        }

        // Add the new UiNavigationSettings node
        int navSettingsIndex = classElement.AddElement<UiNavigationSettings>(context, "NavigationSettings");
        if (navSettingsIndex == -1)
        {
            AZ_Error("Serialization", false, "UiInteractableComponent version conversion failed when adding navigation settings");
            return false;
        }
        AZ::SerializeContext::DataElementNode& navSettingsNode = classElement.GetSubElement(navSettingsIndex);

        // for each of the fields to move, make a copy of the existing node and add it to the nav settings
        AZ::SerializeContext::DataElementNode navModeNode = classElement.GetSubElement(navModeIndex);
        navSettingsNode.AddElement(navModeNode);
        AZ::SerializeContext::DataElementNode navUpNode = classElement.GetSubElement(navUpIndex);
        navSettingsNode.AddElement(navUpNode);
        AZ::SerializeContext::DataElementNode navDownNode = classElement.GetSubElement(navDownIndex);
        navSettingsNode.AddElement(navDownNode);
        AZ::SerializeContext::DataElementNode navLeftNode = classElement.GetSubElement(navLeftIndex);
        navSettingsNode.AddElement(navLeftNode);
        AZ::SerializeContext::DataElementNode navRightNode = classElement.GetSubElement(navRightIndex);
        navSettingsNode.AddElement(navRightNode);

        // Remove the old nodes in reverse order since removing an index invalidates all indices after it
        classElement.RemoveElement(navRightIndex);
        classElement.RemoveElement(navLeftIndex);
        classElement.RemoveElement(navDownIndex);
        classElement.RemoveElement(navUpIndex);
        classElement.RemoveElement(navModeIndex);
    }

    return true;
}
