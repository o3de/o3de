/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiDraggableComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/UiDropTargetBus.h>
#include <LyShine/Bus/UiInteractionMaskBus.h>
#include <LyShine/Bus/UiNavigationBus.h>

#include "UiNavigationHelpers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiDraggableNotificationBus Behavior context handler class
class UiDraggableNotificationBusBehaviorHandler
    : public UiDraggableNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiDraggableNotificationBusBehaviorHandler, "{7EEA2A71-AB29-4F1D-AC76-4BE7237AB99B}", AZ::SystemAllocator,
        OnDragStart, OnDrag, OnDragEnd);

    void OnDragStart(AZ::Vector2 position) override
    {
        Call(FN_OnDragStart, position);
    }

    void OnDrag(AZ::Vector2 position) override
    {
        Call(FN_OnDrag, position);
    }

    void OnDragEnd(AZ::Vector2 position) override
    {
        Call(FN_OnDragEnd, position);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDraggableComponent::UiDraggableComponent()
{
    // Must be called in the same order as the states defined in UiDraggableInterface
    m_stateActionManager.AddState(&m_dragNormalStateActions);
    m_stateActionManager.AddState(&m_dragValidStateActions);
    m_stateActionManager.AddState(&m_dragInvalidStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDraggableComponent::~UiDraggableComponent()
{
    // delete all the state actions now rather than letting the base class do it automatically
    // because the m_stateActionManager has pointers to members in this derived class.
    m_stateActionManager.ClearStates();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDraggableComponent::HandlePressed(AZ::Vector2 point, bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandlePressed(point, shouldStayActive);

    if (handled)
    {
        // NOTE: Drag start does not happen until the mouse actually starts moving so HandlePressed does
        // not do much. Reset these member variables just in case they did not get reset in end drag
        m_isDragging = false;
        m_dragState = DragState::Normal;
        m_hoverDropTarget.SetInvalid();
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDraggableComponent::HandleReleased(AZ::Vector2 point)
{
    EndDragOperation(point, false);

    if (m_isPressed && m_isHandlingEvents)
    {
        UiInteractableComponent::TriggerReleasedAction();
    }

    m_isPressed = false;
    m_pressedPoint = AZ::Vector2(0.0f, 0.0f);

    return m_isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDraggableComponent::HandleEnterPressed(bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandleEnterPressed(shouldStayActive);

    if (handled)
    {
        AZ::Vector2 point(0.0f, 0.0f);
        UiTransformBus::EventResult(point, GetEntityId(), &UiTransformBus::Events::GetViewportSpacePivot);

        // if we are not yet in the dragging state do some tests to see if we should be
        if (!m_isDragging)
        {
            // the draggable will stay active after released so that arrow keys can be used to place it
            // over a drop target
            shouldStayActive = true;
            m_isActive = true;

            // the drag was valid for this draggable, we are now dragging
            m_isDragging = true;
            m_dragState = DragState::Normal;

            UiDraggableNotificationBus::QueueEvent(GetEntityId(), &UiDraggableNotificationBus::Events::OnDragStart, point);

            m_hoverDropTarget.SetInvalid();

            // find closest drop target to the draggable's center
            AZ::EntityId closestDropTarget = FindClosestNavigableDropTarget();
            if (closestDropTarget.IsValid())
            {
                UiTransformBus::EventResult(point, closestDropTarget, &UiTransformBus::Events::GetViewportPosition);
            }

            DoDrag(point, true);
        }
    }

    return handled;
}

/////////////////////////////////////////////////////////////////
bool UiDraggableComponent::HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys)
{
    if (!m_isHandlingEvents)
    {
        return false;
    }

    // don't accept key input while in pressed state
    if (m_isPressed)
    {
        return false;
    }

    bool result = false;

    const UiNavigationHelpers::Command command = UiNavigationHelpers::MapInputChannelIdToUiNavigationCommand(inputSnapshot.m_channelId, activeModifierKeys);
    if (command == UiNavigationHelpers::Command::Up ||
        command == UiNavigationHelpers::Command::Down ||
        command == UiNavigationHelpers::Command::Left ||
        command == UiNavigationHelpers::Command::Right)
    {
        AZ::EntityId closestDropTarget = FindClosestNavigableDropTarget();

        AZ::EntityId newElement;
        if (m_hoverDropTarget.IsValid())
        {
            LyShine::EntityArray navigableElements;
            FindNavigableDropTargetElements(m_hoverDropTarget, navigableElements);

            auto isValidDropTarget = [](AZ::EntityId entityId)
                {
                    bool isEnabled = false;
                    UiElementBus::EventResult(isEnabled, entityId, &UiElementBus::Events::IsEnabled);

                    if (isEnabled && UiDropTargetBus::FindFirstHandler(entityId))
                    {
                        return true;
                    }

                    return false;
                };

            newElement = UiNavigationHelpers::GetNextElement(m_hoverDropTarget, command,
                    navigableElements, closestDropTarget, isValidDropTarget);
        }
        else
        {
            // find closest drop target to the draggable's center
            newElement = closestDropTarget;
        }

        if (newElement.IsValid())
        {
            AZ::Vector2 point(0.0f, 0.0f);
            UiTransformBus::EventResult(point, newElement, &UiTransformBus::Events::GetViewportSpacePivot);
            DoDrag(point, true);
        }

        result = true;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::InputPositionUpdate(AZ::Vector2 point)
{
    if (m_isPressed)
    {
        // if we are not yet in the dragging state do some tests to see if we should be
        if (!m_isDragging)
        {
            bool handOffDone = false;
            bool dragDetected = CheckForDragOrHandOffToParent(GetEntityId(), m_pressedPoint, point, 0.0f, handOffDone);

            if (dragDetected)
            {
                if (handOffDone)
                {
                    // the drag was handed off to a parent, this draggable is no longer active
                    m_isPressed = false;
                }
                else
                {
                    // the drag was valid for this draggable, we are now dragging
                    m_isDragging = true;
                    m_dragState = DragState::Normal;

                    UiDraggableNotificationBus::QueueEvent(GetEntityId(), &UiDraggableNotificationBus::Events::OnDragStart, point);

                    m_hoverDropTarget.SetInvalid();
                }
            }
        }

        // if we are now in the dragging state do the drag update and handle start/end of drop hover
        if (m_isDragging)
        {
            DoDrag(point, false);
        }
    }
}

/////////////////////////////////////////////////////////////////
bool UiDraggableComponent::DoesSupportDragHandOff(AZ::Vector2 startPoint)
{
    // this component does support hand-off, so long as the start point is in its bounds
    // i.e. if there is a child interactable element such as a button or checkbox and the user
    // drags it, then the drag can get handed off to the parent draggable element
    bool isPointInRect = false;
    UiTransformBus::EventResult(isPointInRect, GetEntityId(), &UiTransformBus::Events::IsPointInRect, startPoint);
    return isPointInRect;
}

/////////////////////////////////////////////////////////////////
bool UiDraggableComponent::OfferDragHandOff(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float dragThreshold)
{
    // A child interactable element is offering to hand-off a drag interaction to this element

    bool handedOffToParent = false;
    bool dragDetected = CheckForDragOrHandOffToParent(currentActiveInteractable, startPoint, currentPoint, dragThreshold, handedOffToParent);

    if (dragDetected)
    {
        if (!handedOffToParent)
        {
            // a drag was detected and it was not handed off to a parent, so this draggable is now taking the handoff
            m_isPressed = true;
            m_pressedPoint = startPoint;

            // tell the canvas that this is now the active interactable
            UiInteractableActiveNotificationBus::Event(
                currentActiveInteractable, &UiInteractableActiveNotificationBus::Events::ActiveChanged, GetEntityId(), false);

            // start the drag
            m_isDragging = true;
            m_dragState = DragState::Normal;
            UiDraggableNotificationBus::QueueEvent(GetEntityId(), &UiDraggableNotificationBus::Events::OnDragStart, currentPoint);
            m_hoverDropTarget.SetInvalid();

            // Send the OnDrag and any OnDropHoverStart immediately so that it doesn't require another frame to
            // update.
            DoDrag(currentPoint, false);
        }
    }

    return dragDetected;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::LostActiveStatus()
{
    // this is called when keyboard or console operation is being used and Enter was used to end the
    // operation.

    UiInteractableComponent::LostActiveStatus();

    AZ::Vector2 viewportPoint;
    UiTransformBus::EventResult(viewportPoint, GetEntityId(), &UiTransformBus::Events::GetViewportSpacePivot);

    EndDragOperation(viewportPoint, true);

    m_isActive = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDraggableInterface::DragState UiDraggableComponent::GetDragState()
{
    return m_dragState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::SetDragState(DragState dragState)
{
    m_dragState = dragState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::RedoDrag(AZ::Vector2 point)
{
    DoDrag(point, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::SetAsProxy(AZ::EntityId originalDraggableId, AZ::Vector2 point)
{
    // find the originalDraggable by Id
    AZ::Entity* originalDraggable = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(originalDraggable, &AZ::ComponentApplicationBus::Events::FindEntity, originalDraggableId);
    if (!originalDraggable)
    {
        AZ_Warning("UI", false, "SetAsProxy: Cannot find original draggable");
        return;
    }

    // Find the UiDraggableComponent on the originalDraggable
    UiDraggableComponent* originalComponent = originalDraggable->FindComponent<UiDraggableComponent>();
    if (!originalComponent)
    {
        AZ_Warning("UI", false, "SetAsProxy: Cannot find draggable component");
        return;
    }

    // Set the isProxyFor member variable, this indicates that this is a proxy
    m_isProxyFor = originalDraggableId;

    // put this draggable into the drag state and copy some of the state from the original
    m_isPressed = true;
    m_pressedPoint = originalComponent->m_pressedPoint;
    m_isActive = originalComponent->m_isActive;

    // tell the proxy draggable's canvas that this is now the active interactable
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    UiCanvasBus::Event(canvasEntityId, &UiCanvasBus::Events::ForceActiveInteractable, GetEntityId(), m_isActive, m_pressedPoint);

    // start the drag on the proxy
    m_isDragging = true;
    m_dragState = DragState::Normal;
    UiDraggableNotificationBus::QueueEvent(GetEntityId(), &UiDraggableNotificationBus::Events::OnDragStart, point);
    m_hoverDropTarget.SetInvalid();

    // Send the OnDrag and any OnDropHoverStart immediately so that it doesn't require another frame to
    // update.
    DoDrag(point, false);

    // Turn of these flags on the original, this stops it responding to HandleReleased, InputPositionUpdate, etc
    // If the original is on a different canvas to the proxy then the original withh still get these functions called.
    // They just won't do anything.
    originalComponent->m_isDragging = false;
    originalComponent->m_isPressed = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::ProxyDragEnd(AZ::Vector2 point)
{
    AZ::Entity* originalDraggable = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(originalDraggable, &AZ::ComponentApplicationBus::Events::FindEntity, m_isProxyFor);
    if (!originalDraggable)
    {
        AZ_Warning("UI", false, "ProxyDragEnd: Cannot find original draggable");
        return;
    }

    UiDraggableComponent* originalComponent = originalDraggable->FindComponent<UiDraggableComponent>();
    if (!originalComponent)
    {
        AZ_Warning("UI", false, "ProxyDragEnd: Cannot find draggable component on original");
        return;
    }

    // we don't want the proxy to get in the way of the search for a drop target under the original
    // draggable so disable interaction on it
    m_isHandlingEvents = false;

    originalComponent->m_isPressed = true;
    originalComponent->m_isDragging = true;
    originalComponent->HandleReleased(point);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDraggableComponent::IsProxy()
{
    return (m_isProxyFor.IsValid()) ? true : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDraggableComponent::GetOriginalFromProxy()
{
    return m_isProxyFor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDraggableComponent::GetCanDropOnAnyCanvas()
{
    return m_canDropOnAnyCanvas;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::SetCanDropOnAnyCanvas(bool anyCanvas)
{
    m_canDropOnAnyCanvas = anyCanvas;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiDraggableBus::Handler::BusConnect(m_entity->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiDraggableBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStatesInterface::State UiDraggableComponent::ComputeInteractableState()
{
    UiInteractableStatesInterface::State state = UiInteractableStatesInterface::StateNormal;

    if (!m_isHandlingEvents)
    {
        state = UiInteractableStatesInterface::StateDisabled;
    }
    else if (m_isDragging)
    {
        switch (m_dragState)
        {
        case DragState::Normal:
            state = StateDragNormal;
            break;
        case DragState::Valid:
            state = StateDragValid;
            break;
        case DragState::Invalid:
            state = StateDragInvalid;
            break;
        }
    }
    else if (m_isPressed || m_isActive)
    {
        // To support keyboard/console we stay in pressed state when active
        state = UiInteractableStatesInterface::StatePressed;
    }
    else if (m_isHover)
    {
        state = UiInteractableStatesInterface::StateHover;
    }

    return state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::OnDragNormalStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_dragNormalStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::OnDragValidStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_dragValidStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::OnDragInvalidStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_dragInvalidStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiDraggableComponent, UiInteractableComponent>()
            ->Version(1)
            ->Field("DragNormalStateActions", &UiDraggableComponent::m_dragNormalStateActions)
            ->Field("DragValidStateActions", &UiDraggableComponent::m_dragValidStateActions)
            ->Field("DragInvalidStateActions", &UiDraggableComponent::m_dragInvalidStateActions);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiDraggableComponent>("Draggable", "An interactable component for drag and drop behavior");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiDraggable.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiDraggable.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Drag States")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiDraggableComponent::m_dragNormalStateActions, "Normal", "The normal drag state actions")
                ->Attribute(AZ::Edit::Attributes::AddNotify, &UiDraggableComponent::OnDragNormalStateActionsChanged);

            editInfo->DataElement(0, &UiDraggableComponent::m_dragValidStateActions, "Valid", "The valid drag state actions")
                ->Attribute(AZ::Edit::Attributes::AddNotify, &UiDraggableComponent::OnDragValidStateActionsChanged);

            editInfo->DataElement(0, &UiDraggableComponent::m_dragInvalidStateActions, "Invalid", "The invalid drag state actions")
                ->Attribute(AZ::Edit::Attributes::AddNotify, &UiDraggableComponent::OnDragInvalidStateActionsChanged);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiDraggableInterface::DragState::Normal>("eUiDragState_Normal")
            ->Enum<(int)UiDraggableInterface::DragState::Valid>("eUiDragState_Valid")
            ->Enum<(int)UiDraggableInterface::DragState::Invalid>("eUiDragState_Invalid");

        behaviorContext->EBus<UiDraggableBus>("UiDraggableBus")
            ->Event("GetDragState", &UiDraggableBus::Events::GetDragState)
            ->Event("SetDragState", &UiDraggableBus::Events::SetDragState)
            ->Event("RedoDrag", &UiDraggableBus::Events::RedoDrag)
            ->Event("SetAsProxy", &UiDraggableBus::Events::SetAsProxy)
            ->Event("ProxyDragEnd", &UiDraggableBus::Events::ProxyDragEnd)
            ->Event("IsProxy", &UiDraggableBus::Events::IsProxy)
            ->Event("GetOriginalFromProxy", &UiDraggableBus::Events::GetOriginalFromProxy)
            ->Event("GetCanDropOnAnyCanvas", &UiDraggableBus::Events::GetCanDropOnAnyCanvas)
            ->Event("SetCanDropOnAnyCanvas", &UiDraggableBus::Events::SetCanDropOnAnyCanvas);

        behaviorContext->EBus<UiDraggableNotificationBus>("UiDraggableNotificationBus")
            ->Handler<UiDraggableNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDraggableComponent::GetDropTargetUnderDraggable(AZ::Vector2 point, bool ignoreInteractables)
{
    AZ::EntityId result;

    AZ::EntityId canvasEntity;
    UiElementBus::EventResult(canvasEntity, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);

    // We will ignore this element and all its children in the search
    AZ::EntityId ignoreElement = GetEntityId();

    // Look for a drop target under the mouse position

    // recursively check the children of the canvas (in reverse order since children are in front of parent)
    if (m_canDropOnAnyCanvas)
    {
        result = FindDropTargetOrInteractableOnAllCanvases(point, ignoreElement, ignoreInteractables);
    }
    else
    {
        result = FindDropTargetOrInteractableOnCanvas(canvasEntity, point, ignoreElement, ignoreInteractables);
    }

    // The result could be an interactable that is not a drop target since an interactable in front of a drop target
    // can block dropping on it (unless it is the child of the drop target)
    if (!UiDropTargetBus::FindFirstHandler(result))
    {
        result.SetInvalid();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDraggableComponent::CheckForDragOrHandOffToParent([[maybe_unused]] AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float childDragThreshold, [[maybe_unused]] bool& handOffDone)
{
    // Currently a draggable never hands off the drag to a parent since a drag in any direction is valid.
    // Potentially this could change if we allowed, for example, a scroll box containing draggables where
    // dragging up and down scrolled the scroll box and dragging left and right initiated drag and drop.
    // In that case we would need a property to say in which direction a draggable can be dragged.
    bool result = false;

    // Possibly this should be a user defined property since it defines how much movement constitutes a drag start
    const float normalDragThreshold = 3.0f;

    float dragThreshold = normalDragThreshold;
    if (childDragThreshold > 0.0f)
    {
        dragThreshold = childDragThreshold;
    }
    float dragThresholdSq = dragThreshold * dragThreshold;

    // calculate how much we have dragged
    AZ::Vector2 dragVector = currentPoint - startPoint;
    float dragDistanceSq = dragVector.GetLengthSq();
    if (dragDistanceSq > dragThresholdSq)
    {
        // we dragged above the threshold value
        result = true;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::DoDrag(AZ::Vector2 viewportPoint, bool ignoreInteractables)
{
    // In the case where a proxy has been created in the OnDragStart handler we would no longer
    // be in the dragging state, in that case do nothing here
    if (!m_isDragging)
    {
        return;
    }

    // Send the OnDrag notification
    UiDraggableNotificationBus::QueueEvent(GetEntityId(), &UiDraggableNotificationBus::Events::OnDrag, viewportPoint);

    AZ::EntityId dropEntity = GetDropTargetUnderDraggable(viewportPoint, ignoreInteractables);

    // if we have a drop hover entity and we are no longer hovering over it
    if (m_hoverDropTarget.IsValid() && m_hoverDropTarget != dropEntity)
    {
        // end the drop hover
        UiDropTargetBus::Event(m_hoverDropTarget, &UiDropTargetBus::Events::HandleDropHoverEnd, GetEntityId());
        m_hoverDropTarget.SetInvalid();
    }

    // if we do not have a drop hover entity and we are hovering over a drop target
    if (!m_hoverDropTarget.IsValid() && dropEntity.IsValid())
    {
        // start a drop hover
        UiDropTargetBus::Event(dropEntity, &UiDropTargetBus::Events::HandleDropHoverStart, GetEntityId());
        m_hoverDropTarget = dropEntity;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::EndDragOperation(AZ::Vector2 viewportPoint, bool ignoreInteractables)
{
    if (m_isDragging)
    {
        // If we were hovering over a drop target then forget it, we will recompute what we are over now
        if (m_hoverDropTarget.IsValid())
        {
            UiDropTargetBus::Event(m_hoverDropTarget, &UiDropTargetBus::Events::HandleDropHoverEnd, GetEntityId());
            m_hoverDropTarget.SetInvalid();
        }

        // Search for a drop target before calling OnDragEnd in case OnDragEnd moves the drop target that we are over
        AZ::EntityId dropEntity = GetDropTargetUnderDraggable(viewportPoint, ignoreInteractables);

        // send a drag end notification
        UiDraggableNotificationBus::QueueEvent(GetEntityId(), &UiDraggableNotificationBus::Events::OnDragEnd, viewportPoint);

        // If there was a drop target under the cursor then send it a message to handle this draggable being dropped on it
        if (dropEntity.IsValid())
        {
            UiDropTargetBus::Event(dropEntity, &UiDropTargetBus::Events::HandleDrop, GetEntityId());
        }

        m_isDragging = false;
        m_dragState = DragState::Normal;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDraggableComponent::FindNavigableDropTargetElements(AZ::EntityId ignoreElement, LyShine::EntityArray& result)
{
    AZ::EntityId canvasEntity;
    UiElementBus::EventResult(canvasEntity, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);

    LyShine::EntityArray elements;
    UiCanvasBus::EventResult(elements, canvasEntity, &UiCanvasBus::Events::GetChildElements);

    AZStd::list<AZ::Entity*> elementList(elements.begin(), elements.end());
    while (!elementList.empty())
    {
        auto entity = elementList.front();
        elementList.pop_front();

        if (ignoreElement.IsValid() && entity->GetId() == ignoreElement)
        {
            continue; // this is the element to ignore, ignore its children also
        }

        // Check if the element is enabled
        bool isEnabled = false;
        UiElementBus::EventResult(isEnabled, entity->GetId(), &UiElementBus::Events::IsEnabled);
        if (!isEnabled)
        {
            continue;
        }

        bool isDropTarget = false;
        if (UiDropTargetBus::FindFirstHandler(entity->GetId()))
        {
            isDropTarget = true;
        }

        UiNavigationInterface::NavigationMode navigationMode = UiNavigationInterface::NavigationMode::None;
        UiNavigationBus::EventResult(navigationMode, entity->GetId(), &UiNavigationBus::Events::GetNavigationMode);
        bool isNavigable = (navigationMode != UiNavigationInterface::NavigationMode::None);

        if (isDropTarget && isNavigable)
        {
            result.push_back(entity);
        }
        else
        {
            LyShine::EntityArray childElements;
            UiElementBus::EventResult(childElements, entity->GetId(), &UiElementBus::Events::GetChildElements);
            elementList.insert(elementList.end(), childElements.begin(), childElements.end());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDraggableComponent::FindClosestNavigableDropTarget()
{
    UiTransformInterface::RectPoints srcPoints;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetViewportSpacePoints, srcPoints);
    AZ::Vector2 srcCenter = srcPoints.GetCenter();

    LyShine::EntityArray dropTargets;
    FindNavigableDropTargetElements(AZ::EntityId(), dropTargets);

    float shortestDist = FLT_MAX;
    AZ::EntityId closestElement;
    for (auto dropTarget : dropTargets)
    {
        UiTransformInterface::RectPoints destPoints;
        UiTransformBus::Event(dropTarget->GetId(), &UiTransformBus::Events::GetViewportSpacePoints, destPoints);

        AZ::Vector2 destCenter = destPoints.GetCenter();

        float dist = (destCenter - srcCenter).GetLengthSq();

        if (dist < shortestDist)
        {
            shortestDist = dist;
            closestElement = dropTarget->GetId();
        }
    }

    return closestElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDraggableComponent::FindDropTargetOrInteractableOnAllCanvases(
    AZ::Vector2 point, AZ::EntityId ignoreElement, bool ignoreInteractables)
{
    AZ::EntityId result;

    UiCanvasManagerInterface::CanvasEntityList canvases;
    UiCanvasManagerBus::BroadcastResult(canvases, &UiCanvasManagerBus::Events::GetLoadedCanvases);

    // reverse iterate over the loaded canvases so that the front most canvas gets first chance to
    // handle the event
    for (auto iter = canvases.rbegin(); iter != canvases.rend() && !result.IsValid(); ++iter)
    {
        AZ::EntityId canvasEntityId = *iter;

        result = FindDropTargetOrInteractableOnCanvas(canvasEntityId, point, ignoreElement, ignoreInteractables);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDraggableComponent::FindDropTargetOrInteractableOnCanvas(AZ::EntityId canvasEntityId,
    AZ::Vector2 point, AZ::EntityId ignoreElement, bool ignoreInteractables)
{
    AZ::EntityId result;

    // recursively check the children of the canvas (in reverse order since children are in front of parent)
    int numChildren = 0;
    UiCanvasBus::EventResult(numChildren, canvasEntityId, &UiCanvasBus::Events::GetNumChildElements);
    for (int i = numChildren - 1; !result.IsValid() && i >= 0; i--)
    {
        AZ::EntityId child;
        UiCanvasBus::EventResult(child, canvasEntityId, &UiCanvasBus::Events::GetChildElementEntityId, i);

        if (child != ignoreElement)
        {
            result = FindDropTargetOrInteractableUnderCursor(child, point, ignoreElement, ignoreInteractables);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDraggableComponent::FindDropTargetOrInteractableUnderCursor(AZ::EntityId element,
    AZ::Vector2 point, AZ::EntityId ignoreElement, bool ignoreInteractables)
{
    AZ::EntityId result;

    bool isEnabled = false;
    UiElementBus::EventResult(isEnabled, element, &UiElementBus::Events::IsEnabled);
    if (!isEnabled)
    {
        // Nothing to do
        return result;
    }

    // first check the children (in reverse order since children are in front of parent)
    {
        // if this element is masking children at this point then don't check the children
        bool isMasked = false;
        UiInteractionMaskBus::EventResult(isMasked, element, &UiInteractionMaskBus::Events::IsPointMasked, point);
        if (!isMasked)
        {
            int numChildren = 0;
            UiElementBus::EventResult(numChildren, element, &UiElementBus::Events::GetNumChildElements);
            for (int i = numChildren - 1; !result.IsValid() && i >= 0; i--)
            {
                AZ::EntityId child;
                UiElementBus::EventResult(child, element, &UiElementBus::Events::GetChildEntityId, i);

                if (child != ignoreElement)
                {
                    result = FindDropTargetOrInteractableUnderCursor(child, point, ignoreElement, ignoreInteractables);
                }
            }
        }
    }

    // if no match then check this element
    if (!result.IsValid())
    {
        // if the point is in this element's rect
        bool isInRect = false;
        UiTransformBus::EventResult(isInRect, element, &UiTransformBus::Events::IsPointInRect, point);
        if (isInRect)
        {
            // If this element has a drop target component
            if (UiDropTargetBus::FindFirstHandler(element))
            {
                // this is the drop target under the cursor
                result = element;
            }
            // else if this element has an interactable component
            else if (!ignoreInteractables && UiInteractableBus::FindFirstHandler(element))
            {
                // check if this interactable component is in a state where it can handle an event at the given point
                bool canHandle = false;
                UiInteractableBus::EventResult(canHandle, element, &UiInteractableBus::Events::CanHandleEvent, point);
                if (canHandle)
                {
                    // in this case the interaction is blocked unless this interactable has a parent that is
                    // a drop target
                    AZ::EntityId parent;
                    UiElementBus::EventResult(parent, element, &UiElementBus::Events::GetParentEntityId);
                    while (parent.IsValid())
                    {
                        bool isInParentRect = false;
                        UiTransformBus::EventResult(isInParentRect, parent, &UiTransformBus::Events::IsPointInRect, point);
                        if (isInParentRect && UiDropTargetBus::FindFirstHandler(parent))
                        {
                            // We found a parent drop target and the cursor is in its rect,
                            // this is considered the drop target under the cursor
                            result = parent;
                            break;
                        }
                        UiElementBus::EventResult(parent, parent, &UiElementBus::Events::GetParentEntityId);
                    }

                    if (!result.IsValid())
                    {
                        // no parent drop target was found, return this blocking interactable
                        result = element;
                    }
                }
            }
        }
    }

    return result;
}
