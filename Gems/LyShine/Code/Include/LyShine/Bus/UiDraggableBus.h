/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDraggableInterface
    : public AZ::ComponentBus
{
public: // types

    //! States that the component can be in during a drag. Lua scripts can switch the state to alert the user
    enum class DragState
    {
        Normal,
        Valid,
        Invalid
    };

public: // member functions

    virtual ~UiDraggableInterface() {}

    //! Get the state of the drag
    virtual DragState GetDragState() = 0;

    //! Set the state of the drag. This is only relevant during a drag.
    //! The state affects the visual state of the draggable and can be used to indicate when it is
    //! over a valid drop target.
    virtual void SetDragState(DragState dragState) = 0;

    //! Redo the drag, this is not usually needed but if a UiDraggableNotificationBus handler causes
    //! drop targets to move, and keyboard or console navigation is being used, it can be needed.
    //! In that case the handler should call this method after moving drop targets.
    virtual void RedoDrag(AZ::Vector2 point) = 0;

    //! Set this draggable element to be a proxy for another draggable element and start a drag on
    //! this draggable element at the specified point
    virtual void SetAsProxy(AZ::EntityId originalDraggableId, AZ::Vector2 point) = 0;

    //! Conclude the drag of a proxy. This should be called from the OnDragEnd callback of the proxy and
    //! will result in calling OnDragEnd on the draggable element that this is a proxy for
    virtual void ProxyDragEnd(AZ::Vector2 point) = 0;
        
    //! Check if this draggable element is a proxy
    virtual bool IsProxy() = 0;

    //! Get the original draggable element that this element is a proxy for
    //! Returns an invalid entity id if this is not a proxy
    virtual AZ::EntityId GetOriginalFromProxy() = 0;

    //! Get the flag that indicates if this draggable can be dropped on any canvas
    virtual bool GetCanDropOnAnyCanvas() = 0;

    //! Set the flag that indicates if this draggable can be dropped on any canvas
    virtual void SetCanDropOnAnyCanvas(bool anyCanvas) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDraggableInterface> UiDraggableBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDraggableNotifications
    : public AZ::ComponentBus
{
public:

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const bool EnableEventQueue = true;
    //////////////////////////////////////////////////////////////////////////

public: // member functions

    virtual ~UiDraggableNotifications() {}

    //! Called on drag start
    virtual void OnDragStart(AZ::Vector2 position) = 0;

    //! Called on position change during drag
    virtual void OnDrag(AZ::Vector2 position) = 0;

    //! Called on drag end
    virtual void OnDragEnd(AZ::Vector2 position) = 0;
};

typedef AZ::EBus<UiDraggableNotifications> UiDraggableNotificationBus;
