/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>
#include <AzFramework/Input/Devices/InputDeviceId.h>
#include <AzFramework/Input/User/LocalUserId.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/typetraits/is_base_of.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputDevice;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all input channels that represent the current state of a single input source.
    //! Derived classes should provide additional functions that allow their parent input devices to
    //! update the state and value(s) of the input channel as raw input is received from the system,
    //! and they can (optionally) override the virtual GetCustomData function to return custom data.
    class InputChannel : public InputChannelRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! State of the input channel (not all channels will go through all states)
        enum class State
        {
            Idle,    //!< Examples: inactive or idle, not currently emitting input events
            Began,   //!< Examples: button pressed, trigger engaged, thumb-stick exits deadzone
            Updated, //!< Examples: button held, trigger changed, thumb-stick is outside deadzone
            Ended    //!< Examples: button released, trigger released, thumb-stick enters deadzone
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Snapshot of an input channel that can be constructed, copied, and stored independently.
        struct Snapshot
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] inputChannel The input channel used to initialize the snapshot
            Snapshot(const InputChannel& inputChannel);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] channelId The channel id used to initialize the snapshot
            //! \param[in] deviceId The device id used to initialize the snapshot
            //! \param[in] state The state used to initialize the snapshot
            Snapshot(const InputChannelId& channelId,
                     const InputDeviceId& deviceId,
                     State state);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] channelId The channel id used to initialize the snapshot
            //! \param[in] deviceId The device id used to initialize the snapshot
            //! \param[in] state The state used to initialize the snapshot
            //! \param[in] value The value used to initialize the snapshot
            //! \param[in] delta The delta used to initialize the snapshot
            Snapshot(const InputChannelId& channelId,
                     const InputDeviceId& deviceId,
                     State state,
                     float value,
                     float delta);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            InputChannelId m_channelId;      //!< The channel id of the input channel
            InputDeviceId m_deviceId;        //!< The device id of the input channel
            State m_state;                   //!< The state of the input channel
            float m_value;                   //!< The value of the input channel
            float m_delta;                   //!< The delta of the input channel
            LocalUserId m_localUserId;       //!< The local user id assigned to the input device
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base struct from which to derive all custom input data
        struct CustomData
        {
            AZ_CLASS_ALLOCATOR(CustomData, AZ::SystemAllocator);
            AZ_RTTI(CustomData, "{887E38BB-64AF-4F4E-A1AE-C1B02371F9EC}");
            virtual ~CustomData() = default;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom data struct for input channels associated with a 2D position
        struct PositionData2D : public CustomData
        {
            AZ_CLASS_ALLOCATOR(PositionData2D, AZ::SystemAllocator);
            AZ_RTTI(PositionData2D, "{354437EC-6BFD-41D4-A0F2-7740018D3589}", CustomData);
            virtual ~PositionData2D() = default;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Convenience function to convert the normalized position to screen space coordinates
            //! \param[in] screenWidth The width of the screen to use in the conversion
            //! \param[in] screenHeight The height of the screen to use in the conversion
            //! \return The position in screen space coordinates
            AZ::Vector2 ConvertToScreenSpaceCoordinates(float screenWidth, float screenHeight) const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Update both m_normalizedPosition and m_normalizedPositionDelta given a new position
            //! \param[in] newNormalizedPosition The new normalized position
            void UpdateNormalizedPositionAndDelta(const AZ::Vector2& newNormalizedPosition);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Normalized screen coordinates, where the top-left of the screen is at (0.0, 0.0) and
            //! the bottom-right is at (1.0, 1.0)
            AZ::Vector2 m_normalizedPosition = AZ::Vector2(0.5f, 0.5f);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The delta between the current normalized position and the last one
            AZ::Vector2 m_normalizedPositionDelta = AZ::Vector2::CreateZero();
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for verbose shared_ptr class
        using SharedPositionData2D = AZStd::shared_ptr<InputChannel::PositionData2D>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannel, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannel, "{1C88625D-D297-4A1C-AE07-E17F88D138F3}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel
        //! \param[in] inputDevice Input device that owns the input channel
        InputChannel(const InputChannelId& inputChannelId,
                     const InputDevice& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputChannel() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::GetInputChannel
        const InputChannel* GetInputChannel() const final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input channel's id
        //! \return Id of the input channel
        const InputChannelId& GetInputChannelId() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input channel's device
        //! \return Input device that owns the input channel
        const InputDevice& GetInputDevice() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Direct access to the input channel's current state
        //! \return The current state of the input channel
        State GetState() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Indirect access to the input channel's current state
        //! \return True if the input channel is currently in the specified state, false otherwise
        bool IsStateIdle() const;
        bool IsStateBegan() const;
        bool IsStateUpdated() const;
        bool IsStateEnded() const;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Indirect access to the input channel's current state
        //! \return True if the channel is in the 'Began' or 'Updated' states, false otherwise
        bool IsActive() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the one dimensional float value of the input channel
        //! \return The current one dimensional float value of the input channel
        virtual float GetValue() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the one dimensional float delta of the input channel
        //! \return The current one dimensional float delta of the input channel
        virtual float GetDelta() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to any custom data provided by the input channel
        //! \return Pointer to the custom data if it exists, nullptr otherwise
        virtual const CustomData* GetCustomData() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to any custom data of a specific type provided by the input channel
        //! \tparam CustomDataType The specific type of custom data to be returned if it exists
        //! \return Pointer to the data if it exists and is of type CustomDataType, nullptr othewise
        template<class CustomDataType> const CustomDataType* GetCustomData() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Update the channel's state based on whether it is active/engaged or inactive/idle, which
        //! will broadcast an input event if the channel is left in a non-idle state. Should only be
        //! called a maximum of once per channel per frame from InputDeviceRequests::TickInputDevice
        //! to ensure input channels broadcast no more than one event each frame (at the same time).
        //! \param[in] isChannelActive Whether the input channel is currently active/engaged
        //! \return Whether the update resulted in a state transition (was m_state changed)
        bool UpdateState(bool isChannelActive);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::ResetState
        void ResetState() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        const InputChannelId    m_inputChannelId;   //!< Id of the input channel
        const InputDevice&      m_inputDevice;      //!< Input device that owns the input channel
        State                   m_state;            //!< Current state of the input channel
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Inline Implementation
    template<class CustomDataType>
    inline const CustomDataType* InputChannel::GetCustomData() const
    {
        static_assert((AZStd::is_base_of<CustomData, CustomDataType>::value),
            "Custom input data must inherit from InputChannel::CustomData");

        const CustomData* customData = GetCustomData();
        return customData ? azdynamic_cast<const CustomDataType*>(customData) : nullptr;
    }
} // namespace AzFramework
