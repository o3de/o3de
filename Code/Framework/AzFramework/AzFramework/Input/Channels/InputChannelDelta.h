/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannel.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for input channels that emit one dimensional delta input values, where the 'delta' and
    //! 'value' is one and the same, unlike other input channels that calculate the delta themselves.
    class InputChannelDelta : public InputChannel
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelDelta, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelDelta, "{3AE8E55C-08E2-4258-B42E-3C3B6304B5D2}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        explicit InputChannelDelta(const InputChannelId& inputChannelId,
                                   const InputDevice& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelDelta);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelDelta() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the one dimensional delta/value of the input channel, same as GetDelta
        //! \return The current delta/value of the input channel
        float GetValue() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the one dimensional delta/value of the input channel, same as GetValue
        //! \return The current delta/value of the input channel
        float GetDelta() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::ResetState
        void ResetState() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::SimulateRawInput
        void SimulateRawInput(float rawValue) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process a raw input event, that will in turn update the channel's state based on whether
        //! it's active/engaged or inactive/idle, broadcasting an input event if the channel is left
        //! in a non-idle state. This function (or InputChannel::UpdateState) should only be called
        //! a max of once per channel per frame from InputDeviceRequests::TickInputDevice to ensure
        //! that input channels broadcast no more than one event each frame (and at the same time).
        //! \param[in] rawValue The raw delta value to process
        void ProcessRawInputEvent(float rawValue);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        float m_delta;    //!< The current delta/value of the input channel
    };
} // namespace AzFramework
