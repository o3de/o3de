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
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! This interface can to be implemented by any component that wants to modify transform properties
//! of elements are runtime using the Layout system. The methods in this interface will be called
//! by the LayoutManager whenever the element is told to recompute its layout. Because an element
//! might have multiple components that implement this interface, the handlers will be sorted by
//! priority (lower priority number gets called earlier).
class UiLayoutControllerInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiLayoutControllerInterface() {}

    //! Set elements' width transform properties
    virtual void ApplyLayoutWidth() = 0;

    //! Set elements' height transform properties
    virtual void ApplyLayoutHeight() = 0;

public: // static member data

    //! Events are ordered, each handler may set its priority
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;

    //! Priority will be used for ordering, lower priority number means it gets called earlier
    struct BusHandlerOrderCompare
    {
        AZ_FORCE_INLINE bool operator()(const UiLayoutControllerInterface* left, const UiLayoutControllerInterface* right) const { return left->GetPriority() < right->GetPriority(); }
    };

protected: // member data

    static const unsigned int k_defaultPriority = 100;  // Default is 100, make it lower to get called earlier, higher to get called later
    virtual unsigned int GetPriority() const { return k_defaultPriority; }
};

typedef AZ::EBus<UiLayoutControllerInterface> UiLayoutControllerBus;
