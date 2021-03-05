/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
