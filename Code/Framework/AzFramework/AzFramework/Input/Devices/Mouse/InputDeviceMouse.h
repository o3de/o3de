/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/InputDevice.h>
#include <AzFramework/Input/Channels/InputChannelDeltaWithSharedPosition2D.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedPosition2D.h>

#include <AzCore/std/chrono/clocks.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Defines a generic mouse input device, including the ids of all its associated input channels.
    //! Platform specific implementations are defined as private implementations so that creating an
    //! instance of this generic class will work correctly on any platform that supports mouse input,
    //! while providing access to the device name and associated channel ids on any platform through
    //! the 'null' implementation (primarily so that the editor can use them to setup input mappings).
    class InputDeviceMouse : public InputDevice,
                             public InputSystemCursorRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default sample rate for raw mouse movement events that aims to strike a balance between
        //! responsiveness and performance.
        static const AZ::u32 MovementSampleRateDefault;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Sample rate for raw mouse movement that will cause all events received in the same frame
        //! to be queued and dispatched as individual events. This results in maximum responsiveness
        //! but may potentially impact performance depending how many events happen over each frame.
        static const AZ::u32 MovementSampleRateQueueAll;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Sample rate for raw mouse movement that will cause all events received in the same frame
        //! to be accumulated and dispatched as a single event. Optimal for performance, but results
        //! in sluggish/unresponsive mouse movement, especially when running at low frame rates.
        static const AZ::u32 MovementSampleRateAccumulateAll;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The id used to identify the primary mouse input device
        static const InputDeviceId Id;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies a mouse (regardless of index)
        //! \param[in] inputDeviceId The input device id to check
        //! \return True if the input device id identifies a mouse, false otherwise
        static bool IsMouseDevice(const InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify standard mouse buttons. Though some mice support
        //! more than 5 buttons, it would be strange for a game to explicitly map them as this would
        //! exclude the majority of players who use a regular 3-button mouse. Developers should most
        //! likely expect players to assign additional mouse buttons to keyboard keys using software.
        //!
        //! Additionally, macOSX only supports three mouse buttons (left, right, and middle), so any
        //! cross-platform game should entirely ignore the 'Other1' and 'Other2' buttons, which have
        //! been implemented for windows simply to provide for backwards compatibility with CryInput.
        struct Button
        {
            static constexpr inline InputChannelId Left{"mouse_button_left"}; //!< The left mouse button
            static constexpr inline InputChannelId Right{"mouse_button_right"}; //!< The right mouse button
            static constexpr inline InputChannelId Middle{"mouse_button_middle"}; //!< The middle mouse button
            static constexpr inline InputChannelId Other1{"mouse_button_other1"}; //!< DEPRECATED: the x1 mouse button
            static constexpr inline InputChannelId Other2{"mouse_button_other2"}; //!< DEPRECATED: the x2 mouse button

            //!< All mouse button ids
            static constexpr inline AZStd::array All
            {
                Left,
                Right,
                Middle,
                Other1,
                Other2
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify mouse movement. These input channels represent
        //! raw mouse movement before any system cursor ballistics have been applied, and so don't
        //! directly correlate to the mouse position (which is queried directly from the system).
        struct Movement
        {
            static constexpr inline InputChannelId X{"mouse_delta_x"}; //!< Raw horizontal mouse movement over the last frame
            static constexpr inline InputChannelId Y{"mouse_delta_y"}; //!< Raw vertical mouse movement over the last frame
            static constexpr inline InputChannelId Z{"mouse_delta_z"}; //!< Raw mouse wheel movement over the last frame

            //!< All mouse movement ids
            static constexpr inline AZStd::array All
            {
                X,
                Y,
                Z
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Input channel id of the system cursor position normalized relative to the active window.
        //! The position obtained has had os ballistics applied, and is valid regardless of whether
        //! the system cursor is hidden or visible. When the system cursor has been constrained to
        //! the active window values will be in the [0.0, 1.0] range, but not when unconstrained.
        //! See also InputSystemCursorRequests::SetSystemCursorState and GetSystemCursorState.
        static constexpr inline InputChannelId SystemCursorPosition{"mouse_system_cursor_position"};

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMouse, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceMouse, "{A509CA9D-BEAA-4124-9AAD-7381E46EBDD4}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        explicit InputDeviceMouse(AzFramework::InputDeviceId id = Id);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceMouse);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMouse() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::GetInputChannelsById
        const InputChannelByIdMap& GetInputChannelsById() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::IsSupported
        bool IsSupported() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputSystemCursorRequests::SetSystemCursorState
        void SetSystemCursorState(SystemCursorState systemCursorState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputSystemCursorRequests::GetSystemCursorState
        SystemCursorState GetSystemCursorState() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputSystemCursorRequests::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputSystemCursorRequests::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the sample rate for raw mouse movement events
        //! \param[in] sampleRateHertz The raw movement sample rate in Hertz (cycles per second)
        void SetRawMovementSampleRate(AZ::u32 sampleRateHertz);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Alias for verbose container class
        using ButtonChannelByIdMap = AZStd::unordered_map<InputChannelId,
                                                          InputChannelDigitalWithSharedPosition2D*>;
        using MovementChannelByIdMap = AZStd::unordered_map<InputChannelId,
                                                            InputChannelDeltaWithSharedPosition2D*>;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannelByIdMap                    m_allChannelsById;       //!< All mouse channels
        ButtonChannelByIdMap                   m_buttonChannelsById;    //!< Mouse button channels
        MovementChannelByIdMap                 m_movementChannelsById;  //!< Mouse movement channels
        InputChannelDeltaWithSharedPosition2D* m_cursorPositionChannel; //!< Cursor position channel
        InputChannel::SharedPositionData2D     m_cursorPositionData2D;  //!< Shared cursor position

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations of mouse input devices
        class Implementation
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! \param[in] inputDevice Reference to the input device being implemented
            static Implementation* Create(InputDeviceMouse& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] inputDevice Reference to the input device being implemented
            Implementation(InputDeviceMouse& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Disable copying
            AZ_DISABLE_COPY_MOVE(Implementation);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            virtual ~Implementation();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query the connected state of the input device
            //! \return True if the input device is currently connected, false otherwise
            virtual bool IsConnected() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Attempt to set the state of the system cursor
            //! \param[in] systemCursorState The desired system cursor state
            virtual void SetSystemCursorState(SystemCursorState systemCursorState) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Get the current state of the system cursor
            //! \return The current state of the system cursor
            virtual SystemCursorState GetSystemCursorState() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Attempt to set the system cursor position normalized relative to the active window
            //! \param[in] positionNormalized The desired system cursor position normalized
            virtual void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Get the current system cursor position normalized relative to the active window. The
            //! position obtained has had os ballistics applied, and is valid regardless of whether
            //! the system cursor is hidden or visible. When the cursor has been constrained to the
            //! active window the values will be in the [0.0, 1.0] range, but not when unconstrained.
            //! See also InputSystemCursorRequests::SetSystemCursorState and GetSystemCursorState.
            //! \return The current system cursor position normalized relative to the active window
            virtual AZ::Vector2 GetSystemCursorPositionNormalized() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Tick/update the input device to broadcast all input events since the last frame
            virtual void TickInputDevice() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Set the sample rate for raw mouse movement events
            //! \param[in] sampleRateHertz The raw movement sample rate in Hertz (cycles per second)
            void SetRawMovementSampleRate(AZ::u32 sampleRateHertz);

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Queue raw button events to be processed in the next call to ProcessRawEventQueues.
            //! This function is not thread safe and so should only be called from the main thread.
            //! \param[in] inputChannelId The input channel id
            //! \param[in] rawButtonState The raw button state
            void QueueRawButtonEvent(const InputChannelId& inputChannelId, bool rawButtonState);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Queue raw movement events to be processed in the next call to ProcessRawEventQueues.
            //! This function is not thread safe and so should only be called from the main thread.
            //! \param[in] inputChannelId The input channel id
            //! \param[in] rawMovementDelta The raw movement delta
            void QueueRawMovementEvent(const InputChannelId& inputChannelId, float rawMovementDelta);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process raw input events that have been queued since the last call to this function.
            //! This function is not thread safe, and so should only be called from the main thread.
            void ProcessRawEventQueues();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the state of all this input device's associated input channels
            void ResetInputChannelStates();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Alias for verbose container class
            ///@{
            using RawButtonEventQueueByIdMap = AZStd::unordered_map<InputChannelId, AZStd::vector<bool>>;
            using RawMovementEventQueueByIdMap = AZStd::unordered_map<InputChannelId, AZStd::vector<float>>;
            ///@}

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            InputDeviceMouse&            m_inputDevice;                //!< Reference to the device
            AZStd::sys_time_t            m_rawMovementSampleRate;      //!< Raw movement sample rate
            RawButtonEventQueueByIdMap   m_rawButtonEventQueuesById;   //!< Raw button events by id
            RawMovementEventQueueByIdMap m_rawMovementEventQueuesById; //!< Raw movement events by id
            AZStd::chrono::system_clock::time_point m_timeOfLastRawMovementSample; //!< Time of the last raw movement sample
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the implementation of this input device
        //! \param[in] implementation The new implementation
        void SetImplementation(AZStd::unique_ptr<Implementation> impl) { m_pimpl = AZStd::move(impl); }

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Private pointer to the platform specific implementation
        AZStd::unique_ptr<Implementation> m_pimpl;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Helper class that handles requests to create a custom implementation for this device
        InputDeviceImplementationRequestHandler<InputDeviceMouse> m_implementationRequestHandler;
    };
} // namespace AzFramework
