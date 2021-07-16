/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

// Forward declarations
class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiAnimateEntityInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiAnimateEntityInterface() {}

    //! Called when the animation system has updated the data members of an entity's components
    virtual void PropertyValuesChanged() = 0;

public: // static member data

    //! More than one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
};

typedef AZ::EBus<UiAnimateEntityInterface> UiAnimateEntityBus;
