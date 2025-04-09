/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiCanvasUpdateNotificationBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiNavigationBus.h>
#include <LyShine/Bus/UiInteractableActionsBus.h>
#include "UiInteractableState.h"
#include "UiStateActionManager.h"
#include "UiNavigationSettings.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <LmbrCentral/Rendering/TextureAsset.h>

// Forward declarations
class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiInteractableComponent
    : public AZ::Component
    , public UiInteractableBus::Handler
    , public UiCanvasUpdateNotificationBus::Handler
    , public UiElementNotificationBus::Handler
    , public UiInteractableActionsBus::Handler
{
public: // member functions

    // NOTE: We don't use AZ_COMPONENT here because this is not a concrete class
    AZ_RTTI(UiInteractableComponent, "{A42EB486-1C89-434C-AD22-A3FC6CEEC46F}", AZ::Component);

    UiInteractableComponent();
    ~UiInteractableComponent() override;

    // UiInteractableInterface
    bool CanHandleEvent(AZ::Vector2 point) override;
    bool HandlePressed(AZ::Vector2 point, bool& shouldStayActive) override;
    bool HandleReleased(AZ::Vector2 point) override;
    bool HandleMultiTouchPressed(AZ::Vector2 point, int multiTouchIndex) override;
    bool HandleMultiTouchReleased(AZ::Vector2 point, int multiTouchIndex) override;
    bool HandleEnterPressed(bool& shouldStayActive) override;
    bool HandleEnterReleased() override;
    void InputPositionUpdate(AZ::Vector2 point) override;
    void MultiTouchPositionUpdate(AZ::Vector2 point, int multiTouchIndex) override;
    void LostActiveStatus() override;
    void HandleHoverStart() override;
    void HandleHoverEnd() override;
    void HandleReceivedHoverByNavigatingFromDescendant(AZ::EntityId descendantEntityId) override;
    bool IsPressed() override;
    bool IsHandlingEvents() override;
    void SetIsHandlingEvents(bool isHandlingEvents) override;
    bool IsHandlingMultiTouchEvents() override;
    void SetIsHandlingMultiTouchEvents(bool isHandlingMultiTouchEvents) override;
    bool GetIsAutoActivationEnabled() override;
    void SetIsAutoActivationEnabled(bool isEnabled) override;
    // ~UiInteractableInterface

    // UiCanvasUpdateNotification
    void Update(float deltaTime) override;
    // ~UiCanvasUpdateNotification

    // UiElementNotifications
    void OnUiElementFixup(AZ::EntityId canvasEntityId, AZ::EntityId parentEntityId) override;
    void OnUiElementAndAncestorsEnabledChanged(bool areElementAndAncestorsEnabled) override;
    // ~UiElementNotifications

    // UiInteractableActionsInterface
    const LyShine::ActionName& GetHoverStartActionName() override;
    void SetHoverStartActionName(const LyShine::ActionName& actionName) override;
    const LyShine::ActionName& GetHoverEndActionName() override;
    void SetHoverEndActionName(const LyShine::ActionName& actionName) override;
    const LyShine::ActionName& GetPressedActionName() override;
    void SetPressedActionName(const LyShine::ActionName& actionName) override;
    const LyShine::ActionName& GetReleasedActionName() override;
    void SetReleasedActionName(const LyShine::ActionName& actionName) override;
    const LyShine::ActionName& GetOutsideReleasedActionName() const override;
    void SetOutsideReleasedActionName(const LyShine::ActionName& actionName) override;
    OnActionCallback GetHoverStartActionCallback() override;
    void SetHoverStartActionCallback(OnActionCallback onActionCallback) override;
    OnActionCallback GetHoverEndActionCallback() override;
    void SetHoverEndActionCallback(OnActionCallback onActionCallback) override;
    OnActionCallback GetPressedActionCallback() override;
    void SetPressedActionCallback(OnActionCallback onActionCallback) override;
    OnActionCallback GetReleasedActionCallback() override;
    void SetReleasedActionCallback(OnActionCallback onActionCallback) override;
    // ~UiInteractableActionsInterface

public: // static member functions

    static void Reflect(AZ::ReflectContext* context);

protected: // types

    using StateActions = AZStd::vector<UiInteractableStateAction*>;

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    //! Compute the current Interactable state based on internal state flags
    virtual UiInteractableStatesInterface::State ComputeInteractableState();

    void OnHoverStateActionsChanged();
    void OnPressedStateActionsChanged();
    void OnDisabledStateActionsChanged();

    void TriggerHoverStartAction();
    void TriggerHoverEndAction();
    void TriggerPressedAction();
    void TriggerReleasedAction(bool releasedOutside = false);
    void TriggerReceivedHoverByNavigatingFromDescendantAction(AZ::EntityId descendantEntityId);

    virtual bool IsAutoActivationSupported();

protected: // data members

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // persistent data

    //! Selected/Hover state properties
    StateActions m_hoverStateActions;

    //! Pressed state properties
    StateActions m_pressedStateActions;

    //! Disabled state properties
    StateActions m_disabledStateActions;

    //! Action triggered on hover start
    LyShine::ActionName m_hoverStartActionName;

    //! Action triggered on hover end
    LyShine::ActionName m_hoverEndActionName;

    //! Action triggered on pressed
    LyShine::ActionName m_pressedActionName;

    //! Action triggered on release
    LyShine::ActionName m_releasedActionName;

    //! Action triggered on release outside
    LyShine::ActionName m_outsideReleasedActionName;

    //! If true, the interactable automatically becomes active when navigated to via gamepad/keyboard.
    //! Otherwise, a key press is needed to put the interactable in an active state 
    bool m_isAutoActivationEnabled;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // non-persistent data

    //! True if this interactable is accepting input (i.e. not in disabled state)
    bool m_isHandlingEvents;

    //! True if this interactable is handling multi-touch input events
    bool m_isHandlingMultiTouchEvents;

    //! True if this interactable is being hovered (can be true at the same time as m_isPressed)
    bool m_isHover;

    //! True if the interactable is in the pressed state (which can be true while dragging)
    bool m_isPressed;

    //! the viewport position at which the press event occured (only valid if m_isPressed is true)
    AZ::Vector2 m_pressedPoint;

    //! the multitouch position at which the press event occured (only valid if m_isPressed is true)
    int m_pressedMultiTouchIndex;

    //! The current interactable state. This is stored so that we can detect state changes.
    UiInteractableStatesInterface::State m_state;

    //! Callback triggered on hover start
    OnActionCallback m_hoverStartActionCallback;

    //! Callback triggered on hover end
    OnActionCallback m_hoverEndActionCallback;

    //! Callback triggered on pressed
    OnActionCallback m_pressedActionCallback;

    //! Callback triggered on release
    OnActionCallback m_releasedActionCallback;

    UiStateActionManager m_stateActionManager;
    UiNavigationSettings m_navigationSettings;

private: // static member functions

    //! Get the interactables that could be valid options for custom navigation from this interactable
    static LyShine::EntityArray GetNavigableInteractables(AZ::EntityId sourceEntity);

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // types

    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;

private: // member functions

    //! Methods used for controlling the Edit Context (the properties pane)
    EntityComboBoxVec PopulateNavigableEntityList();
    bool IsNavigationModeCustom() const;
};
