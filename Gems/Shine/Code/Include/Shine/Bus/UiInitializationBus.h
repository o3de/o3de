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
class UiInitializationInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiInitializationInterface() {}

    //! Initialize the component after it has been created as part of a set of entities.
    //! I.e. After a group of entities has been created and activated from a load or clone operation
    //! in the game (not in the UI Editor) this is called on each created element.
    //! This allows the component to perform operations that rely on related entities being
    //! activated.
    virtual void InGamePostActivate() = 0;

public: // static member functions

    static const char* GetUniqueName() { return "UiInitializationInterface"; }

public: // static member data

    //! Multiple components on an entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
};

typedef AZ::EBus<UiInitializationInterface> UiInitializationBus;
