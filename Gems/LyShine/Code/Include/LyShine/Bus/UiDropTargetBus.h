/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDropTargetInterface
    : public AZ::ComponentBus
{
public: // types

    using DropState = int;
    enum
    {
        DropStateNormal = 0,
        DropStateValid,
        DropStateInvalid,

        NumDropStates
    };

public: // member functions

    virtual ~UiDropTargetInterface() {}

    //! Get the OnDrop action name
    virtual const LyShine::ActionName& GetOnDropActionName() = 0;

    //! Set the OnDrop action name
    virtual void SetOnDropActionName(const LyShine::ActionName& actionName) = 0;

    //! Called when mouse/touch enters the bounds of this drop target while dragging a UiDraggableComponent
    virtual void HandleDropHoverStart(AZ::EntityId draggable) = 0;

    //! Called on the currently drop hovered drop target component when mouse/touch moves outside of bounds
    virtual void HandleDropHoverEnd(AZ::EntityId draggable) = 0;

    //! Called when a draggable is dropped on this drop target
    virtual void HandleDrop(AZ::EntityId draggable) = 0;

    //! Get the state of the drop
    virtual DropState GetDropState() = 0;

    //! Set the state of the drop target.
    //! The state affects the visual state of the drop target and can be used to indicate when it has
    //! a valid draggable hovering over it.
    virtual void SetDropState(DropState dropState) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDropTargetInterface> UiDropTargetBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDropTargetNotifications
    : public AZ::ComponentBus
{
public:

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const bool EnableEventQueue = true;
    //////////////////////////////////////////////////////////////////////////

public: // member functions

    virtual ~UiDropTargetNotifications() {}

    //! Called on starting hovering over a drop target
    virtual void OnDropHoverStart(AZ::EntityId draggable) = 0;

    //! Called on ending hovering over a drop target
    virtual void OnDropHoverEnd(AZ::EntityId draggable) = 0;

    //! Called on drop
    virtual void OnDrop(AZ::EntityId draggable) = 0;
};

typedef AZ::EBus<UiDropTargetNotifications> UiDropTargetNotificationBus;
