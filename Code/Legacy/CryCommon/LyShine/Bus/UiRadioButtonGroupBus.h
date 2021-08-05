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
//! Interface class that a radio button group component needs to implement. A radio button group
//! component provides functionality to manage a group of radio buttons. A group of radio buttons
//! allows users to choose one of a predefined set of mutually exclusive options. No more than one
//! item may be selected.
class UiRadioButtonGroupInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiRadioButtonGroupInterface() {}

    //! Query the state of the radio button group
    //! \return     The currently checked radio button.
    virtual AZ::EntityId GetCheckedRadioButton() = 0;

    //! Set a radio button to a desired state
    virtual void SetState(AZ::EntityId radioButton, bool isOn) = 0;

    //! Get the allow uncheck flag
    virtual bool GetAllowUncheck() = 0;

    //! Set the allow uncheck flag
    virtual void SetAllowUncheck(bool allowUncheck) = 0;

    //! Add a new radio button to the group
    virtual void AddRadioButton(AZ::EntityId radioButton) = 0;

    //! Remove a radio button from the group
    virtual void RemoveRadioButton(AZ::EntityId radioButton) = 0;

    //! Query whether a radio button is in the group or not
    virtual bool ContainsRadioButton(AZ::EntityId radioButton) = 0;

    //! Get the action triggered when changed
    virtual const LyShine::ActionName& GetChangedActionName() = 0;

    //! Set the action triggered when changed
    virtual void SetChangedActionName(const LyShine::ActionName& actionName) = 0;

public: // static member data

        //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiRadioButtonGroupInterface> UiRadioButtonGroupBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiRadioButtonGroupNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiRadioButtonGroupNotifications() {}

    //! Notify listeners that the radio button group state has changed
    virtual void OnRadioButtonGroupStateChange([[maybe_unused]] AZ::EntityId checkedRadioButton) {}
};

typedef AZ::EBus<UiRadioButtonGroupNotifications> UiRadioButtonGroupNotificationBus;
