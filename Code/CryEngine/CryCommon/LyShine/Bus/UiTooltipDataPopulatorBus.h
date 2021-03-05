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
//! Interface class that a tooltip component needs to implement. A tooltip component populates
//! a tooltip display element with data
class UiTooltipDataPopulatorInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiTooltipDataPopulatorInterface() {}

    //! Assign data to the specified tooltip display element
    virtual void PushDataToDisplayElement(AZ::EntityId displayEntityId) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiTooltipDataPopulatorInterface> UiTooltipDataPopulatorBus;
