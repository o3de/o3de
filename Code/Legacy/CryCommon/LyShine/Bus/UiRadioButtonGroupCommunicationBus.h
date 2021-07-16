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
//! Interface class that a radio button group component needs to implement. This interface allows
//! a radio button to communicate with the radio button group itself
class UiRadioButtonGroupCommunicationInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiRadioButtonGroupCommunicationInterface() {}

    //! Registers a radio button within a group
    //! \param      The radio button to add
    //! \return     Whether the radio button was successfully added to the group or not
    virtual bool RegisterRadioButton(AZ::EntityId radioButton) = 0;

    //! Unregisters a radio button within a group
    //! \param      The radio button to remove
    virtual void UnregisterRadioButton(AZ::EntityId radioButton) = 0;

    //! Called when a radio button wants to change state due to user action (i.e. by clicking it)
    //! \param      The radio button to change the state of
    //! \param      The new radio button state
    virtual void RequestRadioButtonStateChange(AZ::EntityId radioButton, bool newState) = 0;

public: // static member data

    //! Only one component on an entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiRadioButtonGroupCommunicationInterface> UiRadioButtonGroupCommunicationBus;
