/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannelId.h>
#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to send motion sensor requests to connected input devices
    class InputMotionSensorRequests : public AZ::EBusTraits
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
        //! Get the enabled state of a specific input channel. The majority of input channels cannot
        //! be disabled and are enabled by default, but motion sensor input can be explicitly turned
        //! on/off in order to preserve battery and so as to not generate a flood of unneeded events.
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \param[in] channelId Id of the input channel to check whether it is enabled or diabled
        //! \return True if the input channel is currently enabled, false otherwise
        virtual bool GetInputChannelEnabled(const InputChannelId& /*channelId*/) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the enabled state of a specific input channel. The majority of input channels cannot
        //! be disabled and are enabled by default, but motion sensor input can be explicitly turned
        //! on/off in order to preserve battery and so as to not generate a flood of unneeded events.
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \param[in] channelId Id of the input channel to be enabled or diabled
        //! \param[in] enabled Should the input channel be enabled (true) or disabled (false)?
        virtual void SetInputChannelEnabled(const InputChannelId& /*channelId*/, bool /*enabled*/) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputMotionSensorRequests() = default;
    };
    using InputMotionSensorRequestBus = AZ::EBus<InputMotionSensorRequests>;
} // namespace AzFramework
