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
//! Interface class that a dynamic layout component needs to implement. A dynamic layout component
//! clones a prototype element to achieve the desired number of children. The parent is resized
//! accordingly
class UiDynamicLayoutInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDynamicLayoutInterface() {}

    //! Clone a prototype element or remove cloned elements to end up with the specified number of children
    virtual void SetNumChildElements(int numChildren) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDynamicLayoutInterface> UiDynamicLayoutBus;
