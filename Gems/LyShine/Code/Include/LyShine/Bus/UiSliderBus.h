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
class UiSliderInterface
    : public AZ::ComponentBus
{
public: // types

    //! params: sending entity id, newValue, newPosition
    typedef AZStd::function<void(AZ::EntityId, float)> ValueChangeCallback;

public: // member functions

    virtual ~UiSliderInterface() {}

    //! Query the value of the slider
    //! \return     The current value for the slider.
    virtual float GetValue() = 0;

    //! Manually override the value of the slider
    //! \param isOn     The new desired value of the slider.
    virtual void SetValue(float value) = 0;

    virtual float GetMinValue() = 0;
    virtual void SetMinValue(float value) = 0;
    virtual float GetMaxValue() = 0;
    virtual void SetMaxValue(float value) = 0;
    virtual float GetStepValue() = 0;
    virtual void SetStepValue(float step) = 0;

    //! Get the callback invoked while the value is changing
    virtual ValueChangeCallback GetValueChangingCallback() = 0;

    //! Set the callback invoked while the value is changing
    virtual void SetValueChangingCallback(ValueChangeCallback onChange) = 0;

    //! Get the action triggered while the value is changing
    virtual const LyShine::ActionName& GetValueChangingActionName() = 0;

    //! Set the action triggered while the value is changing
    virtual void SetValueChangingActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the callback invoked when the value is done changing
    virtual ValueChangeCallback GetValueChangedCallback() = 0;

    //! Set the callback invoked when the value is done changing
    virtual void SetValueChangedCallback(ValueChangeCallback onChange) = 0;

    //! Get the action triggered when the value is done changing
    virtual const LyShine::ActionName& GetValueChangedActionName() = 0;

    //! Set the action triggered when the value is done changing
    virtual void SetValueChangedActionName(const LyShine::ActionName& actionName) = 0;

    //! Set the optional track entity
    virtual void SetTrackEntity(AZ::EntityId entityId) = 0;

    //! Get the optional track entity
    virtual AZ::EntityId GetTrackEntity() = 0;

    //! Set the optional fill entity
    virtual void SetFillEntity(AZ::EntityId entityId) = 0;

    //! Get the optional fill entity
    virtual AZ::EntityId GetFillEntity() = 0;

    //! Set the optional manipulator entity
    virtual void SetManipulatorEntity(AZ::EntityId entityId) = 0;

    //! Get the optional manipulator entity
    virtual AZ::EntityId GetManipulatorEntity() = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiSliderInterface> UiSliderBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiSliderNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiSliderNotifications() {}

    //! Notify listeners that the slider value is changing
    virtual void OnSliderValueChanging([[maybe_unused]] float value) {}

    //! Notify listeners that the slider value is done changing
    virtual void OnSliderValueChanged([[maybe_unused]] float value) {}
};

typedef AZ::EBus<UiSliderNotifications> UiSliderNotificationBus;
