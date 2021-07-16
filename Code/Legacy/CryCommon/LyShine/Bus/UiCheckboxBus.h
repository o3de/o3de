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
class UiCheckboxInterface
    : public AZ::ComponentBus
{
public: // types

    //! params: sending entity id, new state
    typedef AZStd::function<void(AZ::EntityId, AZ::Vector2, bool)> StateChangeCallback;

public: // member functions

    virtual ~UiCheckboxInterface() {}

    //! Query the state of the checkbox
    //! \return     The current state for the checkbox.
    virtual bool GetState() = 0;

    //! Manually override the state of the checkbox
    //! \param isOn     The new desired state of the checkbox.
    virtual void SetState(bool checked) = 0;

    //! Toggle the state of the checkbox
    //! \return     The new state of the checkbox.
    virtual bool ToggleState() = 0;

    //! Get the state change callback
    virtual StateChangeCallback GetStateChangeCallback() = 0;

    //! Set the state change callback
    virtual void SetStateChangeCallback(StateChangeCallback onChange) = 0;

    //! Set the optional checked (ON) entity
    virtual void SetCheckedEntity(AZ::EntityId entityId) = 0;

    //! Get the optional checked (ON) entity
    virtual AZ::EntityId GetCheckedEntity() = 0;

    //! Set the optional unchecked (OFF) entity
    virtual void SetUncheckedEntity(AZ::EntityId entityId) = 0;

    //! Get the optional unchecked (OFF) entity
    virtual AZ::EntityId GetUncheckedEntity() = 0;

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

typedef AZ::EBus<UiCheckboxInterface> UiCheckboxBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCheckboxNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiCheckboxNotifications() {}

    //! Notify listeners that the checkbox state has changed
    virtual void OnCheckboxStateChange([[maybe_unused]] bool checked) {}
};

typedef AZ::EBus<UiCheckboxNotifications> UiCheckboxNotificationBus;
