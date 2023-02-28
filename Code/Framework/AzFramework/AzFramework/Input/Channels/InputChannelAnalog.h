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
    //! Class for input channels that emit one dimensional analog input values.
    //! Example: game-pad trigger
    class InputChannelAnalog : public InputChannel
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelAnalog, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelAnalog, "{C3E76C92-0D00-45F1-AF03-EFF3F1910A0D}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        explicit InputChannelAnalog(const InputChannelId& inputChannelId,
                                    const InputDevice& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelAnalog);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelAnalog() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the one dimensional analog value of the input channel
        //! \return The current analogue value of the input channel
        float GetValue() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the one dimensional analog delta of the input channel
        //! \return Difference between the current and last reported analog values
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
        //! \param[in] rawValue The raw analog value to process
        void ProcessRawInputEvent(float rawValue);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        float m_value; //!< Current analog value of the input channel
        float m_delta; //!< Difference between the current and last reported analog values
    };
} // namespace AzFramework
