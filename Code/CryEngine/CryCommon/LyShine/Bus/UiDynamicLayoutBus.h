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
