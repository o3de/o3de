/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannelDigital.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for input channels that emit one dimensional digital input values and a position.
    //! Examples: touch
    class InputChannelDigitalWithPosition2D : public InputChannelDigital
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Raw digital with position 2D input event
        struct RawInputEvent
        {
            ////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            explicit RawInputEvent(float normalizedX, float normalizedY, bool digitalValue)
                : m_normalizedX(normalizedX)
                , m_normalizedY(normalizedY)
                , m_digitalValue(digitalValue)
            {}

            ////////////////////////////////////////////////////////////////////////////////////
            // Default copying
            AZ_DEFAULT_COPY(RawInputEvent);

            ////////////////////////////////////////////////////////////////////////////////////
            //! Default destructor
            ~RawInputEvent() = default;

            ////////////////////////////////////////////////////////////////////////////////////
            // Variables
            float m_normalizedX; //!< The normalized x position of the raw input event
            float m_normalizedY; //!< The normalized y position of the raw input event
            bool m_digitalValue; //!< The digital value of the raw input event (on|off)
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelDigitalWithPosition2D, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelDigitalWithPosition2D, "{5D3EC355-D359-47B7-9984-5B19D68FEC06}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        explicit InputChannelDigitalWithPosition2D(const InputChannelId& inputChannelId,
                                                   const InputDevice& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelDigitalWithPosition2D);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputChannelDigitalWithPosition2D() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the position data associated with the input channel
        //! \return Pointer to the position data
        const InputChannel::CustomData* GetCustomData() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::ResetState
        void ResetState() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::SimulateRawInputWithPosition2D
        void SimulateRawInputWithPosition2D(float rawValue,
                                            float normalizedX,
                                            float normalizedY) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process a raw input event, that will in turn update the channel's state based on whether
        //! it's active/engaged or inactive/idle, broadcasting an input event if the channel is left
        //! in a non-idle state. This function (or InputChannel::UpdateState) should only be called
        //! a max of once per channel per frame from InputDeviceRequests::TickInputDevice to ensure
        //! that input channels broadcast no more than one event each frame (and at the same time).
        //!
        //! Note that this function hides InputChannelDigital::ProcessRawInputEvent which is intended.
        //!
        //! \param[in] rawValues The raw digital value and position 2D to process
        void ProcessRawInputEvent(const RawInputEvent& rawValues);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannel::PositionData2D m_positionData;  //!< Current position data
    };
} // namespace AzFramework
