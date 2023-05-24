/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannelAnalog.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for input channels that emit one dimensional analog input values and a position.
    //! Example: touch with pressure and position
    class InputChannelAnalogWithPosition2D : public InputChannelAnalog
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Raw analog with position 2D input event
        struct RawInputEvent
        {
            ////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            explicit RawInputEvent(float normalizedX, float normalizedY, float analogValue)
                : m_normalizedX(normalizedX)
                , m_normalizedY(normalizedY)
                , m_analogValue(analogValue)
            {}

            ////////////////////////////////////////////////////////////////////////////////////
            // Default copying
            AZ_DEFAULT_COPY(RawInputEvent);

            ////////////////////////////////////////////////////////////////////////////////////
            //! Default destructor
            virtual ~RawInputEvent() = default;

            ////////////////////////////////////////////////////////////////////////////////////
            // Variables
            float m_normalizedX; //!< The normalized x position of the raw input event
            float m_normalizedY; //!< The normalized y position of the raw input event
            float m_analogValue; //!< The analog value of the raw input event (0: idle)
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelAnalogWithPosition2D, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelAnalogWithPosition2D, "{2C07314B-3294-41F8-9F14-FD5FE4048283}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        explicit InputChannelAnalogWithPosition2D(const InputChannelId& inputChannelId,
                                                  const InputDevice& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelAnalogWithPosition2D);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelAnalogWithPosition2D() override = default;

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
        //! Note that this function hides InputChannelAnalog::ProcessRawInputEvent which is intended.
        //!
        //! \param[in] rawValues The raw analog value and position 2D to process
        void ProcessRawInputEvent(const RawInputEvent& rawValues);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannel::PositionData2D m_positionData;  //!< Current position data
    };
} // namespace AzFramework
