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
//! Interface class that a radio button component needs to implement. This interface allows the
//! radio button group to communicate with the radio button itself
class UiRadioButtonCommunicationInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiRadioButtonCommunicationInterface() {}

    //! Used by the group to set the state of the radio button
    //! \param     The new desired state of the radio button.
    //! \param     Whether the button should inform listeners that it was changed.
    virtual void SetState(bool isOn, bool sendNotifications) = 0;

    //! Set the radio button group
    virtual void SetGroup(AZ::EntityId group) = 0;

public: // static member data

    //! Only one component on an entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiRadioButtonCommunicationInterface> UiRadioButtonCommunicationBus;
