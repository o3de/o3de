/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class VirtualGamepadRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the list of button names made available by the virtual gamepad
        //! \return The list of button names made available by the virtual gamepad
        virtual const AZStd::unordered_set<AZStd::string>& GetButtonNames() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the list of thumb-stick names made available by the virtual gamepad
        //! \return The list of thumb-stick names made available by the virtual gamepad
        virtual const AZStd::unordered_set<AZStd::string>& GetThumbStickNames() const = 0;
    };
    using VirtualGamepadRequestBus = AZ::EBus<VirtualGamepadRequests>;
}
