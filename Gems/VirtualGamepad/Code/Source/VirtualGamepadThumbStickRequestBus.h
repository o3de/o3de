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

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class VirtualGamepadThumbStickRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = AZStd::string;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the current virtual thumb-stick axis values
        //! \return Current virtual thumb-stick axis values
        virtual AZ::Vector2 GetCurrentAxisValuesNormalized() const = 0;
    };
    using VirtualGamepadThumbStickRequestBus = AZ::EBus<VirtualGamepadThumbStickRequests>;
}
