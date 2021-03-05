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

#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to send light bar requests to connected input devices
    class InputLightBarRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId using EBus<>::Event,
        //! which should be handled by only one device that has connected to the bus using that id.
        //! Input requests can also be sent using EBus<>::Broadcast, in which case they'll be sent
        //! to all input devices that have connected to the input event bus regardless of their id.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input device connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId
        using BusIdType = InputDeviceId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the current light bar color of the input device. All calls to this method should be
        //! addressed to a specific input device otherwise all devices that support it will respond!
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \param[in] color The color to set the light bar
        virtual void SetLightBarColor(const AZ::Color& color) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Reset to default light bar color of the input device. All calls to this method should be
        //! addressed to a specific input device otherwise all devices that support it will respond!
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        virtual void ResetLightBarColor() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputLightBarRequests() = default;
    };
    using InputLightBarRequestBus = AZ::EBus<InputLightBarRequests>;
} // namespace AzFramework
