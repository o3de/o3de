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
//! Interface class that a radio button component needs to implement. A radio button component
//! provides functionality for a button that can be turned on or off and is dependent on the other
//! radio buttons in the same radio button group. This interface is designed to be used in
//! conjunction with the UiRadioButtonGroupInterface, which describes the group for radio buttons.
class UiRadioButtonInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiRadioButtonInterface() {}

    //! Query the state of the radio button
    //! \return     The current state for the radio button.
    virtual bool GetState() = 0;

    //! Get the radio button group
    virtual AZ::EntityId GetGroup() = 0;

    //! Get the optional checked (ON) entity
    virtual AZ::EntityId GetCheckedEntity() = 0;

    //! Set the optional checked (ON) entity
    virtual void SetCheckedEntity(AZ::EntityId entityId) = 0;

    //! Get the optional unchecked (OFF) entity
    virtual AZ::EntityId GetUncheckedEntity() = 0;

    //! Set the optional unchecked (OFF) entity
    virtual void SetUncheckedEntity(AZ::EntityId entityId) = 0;

    //! Get the action triggered when turned on
    virtual const LyShine::ActionName& GetTurnOnActionName() = 0;

    //! Set the action triggered when turned on
    virtual void SetTurnOnActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the action triggered when turned off
    virtual const LyShine::ActionName& GetTurnOffActionName() = 0;

    //! Set the action triggered when turned off
    virtual void SetTurnOffActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the action triggered when changed
    virtual const LyShine::ActionName& GetChangedActionName() = 0;

    //! Set the action triggered when changed
    virtual void SetChangedActionName(const LyShine::ActionName& actionName) = 0;

public: // static member data

        //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiRadioButtonInterface> UiRadioButtonBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiRadioButtonNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiRadioButtonNotifications() {}

    //! Notify listeners that the radio button state has changed
    virtual void OnRadioButtonStateChange([[maybe_unused]] bool checked) {}
};

typedef AZ::EBus<UiRadioButtonNotifications> UiRadioButtonNotificationBus;
