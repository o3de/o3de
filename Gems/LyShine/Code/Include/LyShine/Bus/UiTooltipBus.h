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
//! Interface class that a tooltip component needs to implement. A tooltip component provides
//! data that is assigned to a tooltip display element
class UiTooltipInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiTooltipInterface() {}

    //! Get the tooltip text
    virtual AZStd::string GetText() = 0;

    //! Set the tooltip text
    virtual void SetText(const AZStd::string& text) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiTooltipInterface> UiTooltipBus;
