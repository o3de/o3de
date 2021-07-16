/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that a scroller component needs to implement. A scroller component provides
//! functionality to control the scrolling of scrollable content
//! (e.g. UiScrollBarComponent)
class UiScrollerInterface
    : public AZ::ComponentBus
{
public: // types

    //! params: sending entity id, newValue, newPosition
    typedef AZStd::function<void(AZ::EntityId, float)> ValueChangeCallback;

    //! Scroller orientation
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

public: // member functions

    virtual ~UiScrollerInterface() {}

    //! Get the current value for the scrollbar (0 - 1)
    virtual float GetValue() = 0;

    //! Set the value of the scrollbar (0 - 1)
    virtual void SetValue(float value) = 0;

    //! Get the orientation of the scroller
    virtual Orientation GetOrientation() = 0;

    //! Set the orientation of the scroller
    virtual void SetOrientation(Orientation orientation) = 0;

    //! Get the scrollable entity
    virtual AZ::EntityId GetScrollableEntity() = 0;

    //! Set the scrollable entity
    virtual void SetScrollableEntity(AZ::EntityId entityId) = 0;

    //! Get the callback invoked while the value is changing
    virtual ValueChangeCallback GetValueChangingCallback() = 0;

    //! Set the callback invoked while the value is changing
    virtual void SetValueChangingCallback(ValueChangeCallback onChange) = 0;

    //! Get the callback invoked when the value is done changing
    virtual ValueChangeCallback GetValueChangedCallback() = 0;

    //! Set the callback invoked when the value is done changing
    virtual void SetValueChangedCallback(ValueChangeCallback onChange) = 0;

    //! Get the action triggered while the value is changing
    virtual const LyShine::ActionName& GetValueChangingActionName() = 0;

    //! Set the action triggered while the value is changing
    virtual void SetValueChangingActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the action triggered when the value is done changing
    virtual const LyShine::ActionName& GetValueChangedActionName() = 0;

    //! Set the action triggered when the value is done changing
    virtual void SetValueChangedActionName(const LyShine::ActionName& actionName) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiScrollerInterface> UiScrollerBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement in order to get notifications when values of the
//! scroller change
class UiScrollerNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiScrollerNotifications(){}

    //! Called when the scroller value (0 - 1) is changing
    virtual void OnScrollerValueChanging(float value) = 0;

    //! Called when the scroller value (0 - 1) has been changed
    virtual void OnScrollerValueChanged(float value) = 0;
};

typedef AZ::EBus<UiScrollerNotifications> UiScrollerNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that scrollables need to implement in order to get notifications when the scroller
//! changes the value
class UiScrollerToScrollableNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiScrollerToScrollableNotifications(){}

    //! Called when the scroller is changing the scroll value (0 - 1)
    virtual void OnValueChangingByScroller(float value) = 0;

    //! Called when the scroller is done changing the scroll value (0 - 1)
    virtual void OnValueChangedByScroller(float value) = 0;
};

typedef AZ::EBus<UiScrollerToScrollableNotifications> UiScrollerToScrollableNotificationBus;
