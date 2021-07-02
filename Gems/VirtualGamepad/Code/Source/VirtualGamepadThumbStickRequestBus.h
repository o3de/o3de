/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
