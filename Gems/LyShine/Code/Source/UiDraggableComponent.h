/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiDraggableBus.h>
#include <LyShine/UiComponentTypes.h>

#include "UiInteractableComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDraggableComponent
    : public UiInteractableComponent
    , public UiDraggableBus::Handler
{
public: // types

    //! These are the visual states
    enum
    {
        StateDragNormal = UiInteractableStatesInterface::NumStates,
        StateDragValid,
        StateDragInvalid
    };

public: // member functions

    AZ_COMPONENT(UiDraggableComponent, LyShine::UiDraggableComponentUuid, UiInteractableComponent);

    UiDraggableComponent();
    ~UiDraggableComponent() override;

    // UiInteractableInterface
    bool HandlePressed(AZ::Vector2 point, bool& shouldStayActive) override;
    bool HandleReleased(AZ::Vector2 point) override;
    bool HandleEnterPressed(bool& shouldStayActive) override;
    bool HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys) override;
    void InputPositionUpdate(AZ::Vector2 point) override;
    bool DoesSupportDragHandOff(AZ::Vector2 startPoint) override;
    bool OfferDragHandOff(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float dragThreshold) override;
    void LostActiveStatus() override;
    // ~UiInteractableInterface

    // UiDraggableInterface
    DragState GetDragState() override;
    void SetDragState(DragState dragState) override;
    void RedoDrag(AZ::Vector2 point) override;

    void SetAsProxy(AZ::EntityId originalDraggableId, AZ::Vector2 point) override;
    void ProxyDragEnd(AZ::Vector2 point) override;
    bool IsProxy() override;
    AZ::EntityId GetOriginalFromProxy() override;

    bool GetCanDropOnAnyCanvas() override;
    void SetCanDropOnAnyCanvas(bool anyCanvas) override;
    // ~UiDraggableInterface

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    // UiInteractableInterface
    UiInteractableStatesInterface::State ComputeInteractableState() override;
    // ~UiInteractableInterface

    void OnDragNormalStateActionsChanged();
    void OnDragValidStateActionsChanged();
    void OnDragInvalidStateActionsChanged();


protected: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiInteractableService"));
        provided.push_back(AZ_CRC_CE("UiNavigationService"));
        provided.push_back(AZ_CRC_CE("UiStateActionsService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiInteractableService"));
        incompatible.push_back(AZ_CRC_CE("UiNavigationService"));
        incompatible.push_back(AZ_CRC_CE("UiStateActionsService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void Reflect(AZ::ReflectContext* context);

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiDraggableComponent);

    //! Look for an interactable drop target at the given point
    AZ::EntityId GetDropTargetUnderDraggable(AZ::Vector2 point, bool ignoreInteractables);

    //! Used to detect when we have started a drag
    bool CheckForDragOrHandOffToParent(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float childDragThreshold, bool& handOffDone);

    //! Common code for each frame of drag operation
    void DoDrag(AZ::Vector2 viewportPoint, bool ignoreInteractables);

    //! Common code for the end of a drag
    void EndDragOperation(AZ::Vector2 viewportPoint, bool ignoreInteractables);

    //! Find the drop target elements that we can navigate to
    void FindNavigableDropTargetElements(AZ::EntityId ignoreElement, LyShine::EntityArray& result);

    //! Find the closest drop target to the draggable (used for keyboard navigation)
    AZ::EntityId FindClosestNavigableDropTarget();

private: // static member functions

    //! Perform a recursive search for a valid drop target in all canvases
    static AZ::EntityId FindDropTargetOrInteractableOnAllCanvases(
        AZ::Vector2 point, AZ::EntityId ignoreElement, bool ignoreInteractables);

    //! Perform a recursive search for a valid drop target in the given canvas
    static AZ::EntityId FindDropTargetOrInteractableOnCanvas(AZ::EntityId canvasEntityId,
        AZ::Vector2 point, AZ::EntityId ignoreElement, bool ignoreInteractables);

    //! Perform a recursive search for a valid drop target
    static AZ::EntityId FindDropTargetOrInteractableUnderCursor(AZ::EntityId element,
        AZ::Vector2 point, AZ::EntityId ignoreElement, bool ignoreInteractables);

private: // persistent data

    //! Dragging state action properties - allow visual states to be defined
    StateActions m_dragNormalStateActions;
    StateActions m_dragValidStateActions;
    StateActions m_dragInvalidStateActions;

private: // data

    //! True when a drag has started
    bool m_isDragging = false;

    //! True when interactable can be manipulated by key input
    bool m_isActive = false;

    //! This tracks the drop target that the draggable is hovering over (if any)
    AZ::EntityId m_hoverDropTarget;

    //! The drag state indicates the state that we want to communicate to the user about the drag
    DragState m_dragState = DragState::Normal;

    //! For a proxy draggable this stores the ID of the draggable that it is a proxy for
    AZ::EntityId m_isProxyFor;

    //! If true this draggable will search for drop targets on any canvas
    bool m_canDropOnAnyCanvas = false;
};
