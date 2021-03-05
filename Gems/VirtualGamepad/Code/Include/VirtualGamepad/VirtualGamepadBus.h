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
