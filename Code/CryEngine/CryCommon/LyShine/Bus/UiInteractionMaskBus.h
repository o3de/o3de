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
