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

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiInteractionMaskInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiInteractionMaskInterface() {}

    //! Check whether this element is masking the given point
    virtual bool IsPointMasked(AZ::Vector2 point) = 0;

public: // static member functions

    static const char* GetUniqueName() { return "UIInteractionMaskInterface"; }

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiInteractionMaskInterface> UiInteractionMaskBus;
