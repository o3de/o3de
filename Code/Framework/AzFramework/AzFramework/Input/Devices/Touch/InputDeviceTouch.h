/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Devices/InputDevice.h>
#include <AzFramework/Input/Channels/InputChannelAnalogWithPosition2D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Defines a generic touch input device, including the ids of all its associated input channels.
    //! Platform specific implementations are defined as private implementations so that creating an
    //! instance of this generic class will work correctly on any platform that supports touch input,
    //! while providing access to the device name and associated channel ids on any platform through
    //! the 'null' implementation (primarily so that the editor can use them to setup input mappings).
    class InputDeviceTouch : public InputDevice
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The id used to identify the primary touch input device
        static const InputDeviceId Id;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies a touch device (regardless of index)
        //! \param[in] inputDeviceId The input device id to check
        //! \return True if the input device id identifies a touch device, false otherwise
        static bool IsTouchDevice(const InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify touches. The maximum number of active touches to
        //! track is arbitrary, but ten seems to be more than sufficient for most game applications.
        struct Touch
        {
            static constexpr inline InputChannelId Index0{"touch_index_0"}; //!< Touch index 0
            static constexpr inline InputChannelId Index1{"touch_index_1"}; //!< Touch index 1
            static constexpr inline InputChannelId Index2{"touch_index_2"}; //!< Touch index 2
            static constexpr inline InputChannelId Index3{"touch_index_3"}; //!< Touch index 3
            static constexpr inline InputChannelId Index4{"touch_index_4"}; //!< Touch index 4
            static constexpr inline InputChannelId Index5{"touch_index_5"}; //!< Touch index 5
            static constexpr inline InputChannelId Index6{"touch_index_6"}; //!< Touch index 6
            static constexpr inline InputChannelId Index7{"touch_index_7"}; //!< Touch index 7
            static constexpr inline InputChannelId Index8{"touch_index_8"}; //!< Touch index 8
            static constexpr inline InputChannelId Index9{"touch_index_9"}; //!< Touch index 9

            //!< All touch input channel ids
            static constexpr inline AZStd::array All
            {
                Index0,
                Index1,
                Index2,
                Index3,
                Index4,
                Index5,
                Index6,
                Index7,
                Index8,
                Index9
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceTouch, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceTouch, "{796E4C57-4D6C-4DAA-8367-9026509E86EF}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        InputDeviceTouch();

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceTouch);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceTouch() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::GetAssignedLocalUserId
        LocalUserId GetAssignedLocalUserId() const override;

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

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for verbose container class
        using TouchChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAnalogWithPosition2D*>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannelByIdMap m_allChannelsById;   //!< All touch channels by id
        TouchChannelByIdMap m_touchChannelsById; //!< All touch channels by id

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations of touch input devices
        class Implementation
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! \param[in] inputDevice Reference to the input device being implemented
            static Implementation* Create(InputDeviceTouch& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] inputDevice Reference to the input device being implemented
            Implementation(InputDeviceTouch& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Disable copying
            AZ_DISABLE_COPY_MOVE(Implementation);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            virtual ~Implementation();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Access to the input device's currently assigned local user id
            //! \return Id of the local user currently assigned to the input device
            virtual LocalUserId GetAssignedLocalUserId() const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query the connected state of the input device
            //! \return True if the input device is currently connected, false otherwise
            virtual bool IsConnected() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Tick/update the input device to broadcast all input events since the last frame
            virtual void TickInputDevice() = 0;

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Platform agnostic representation of a raw touch event
            struct RawTouchEvent : public InputChannelAnalogWithPosition2D::RawInputEvent
            {
                ////////////////////////////////////////////////////////////////////////////////////
                //! State of the raw touch event
                enum class State
                {
                    Began,
                    Moved,
                    Ended
                };

                ////////////////////////////////////////////////////////////////////////////////////
                //! Constructor
                explicit RawTouchEvent(
                    float normalizedX,
                    float normalizedY,
                    float pressure,
                    AZ::u32 index,
                    State state);

                ////////////////////////////////////////////////////////////////////////////////////
                // Default copying
                AZ_DEFAULT_COPY(RawTouchEvent);

                ////////////////////////////////////////////////////////////////////////////////////
                //! Default destructor
                ~RawTouchEvent() override = default;

                ////////////////////////////////////////////////////////////////////////////////////
                // Variables
                AZ::u32 m_index; //!< The index of the raw touch event
                State m_state;   //!< The state of the raw touch event
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Queue raw touch events to be processed in the next call to ProcessRawEventQueues.
            //! This function is not thread safe and so should only be called from the main thread.
            //! \param[in] rawTouchEvent The raw touch event
            void QueueRawTouchEvent(const RawTouchEvent& rawTouchEvent);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process raw input events that have been queued since the last call to this function.
            //! This function is not thread safe, and so should only be called from the main thread.
            void ProcessRawEventQueues();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the state of all this input device's associated input channels
            void ResetInputChannelStates();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Alias for verbose container class
            using RawTouchEventQueueByIdMap = AZStd::unordered_map<InputChannelId, AZStd::vector<RawTouchEvent>>;

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            InputDeviceTouch&         m_inputDevice;             //!< Reference to the input device
            RawTouchEventQueueByIdMap m_rawTouchEventQueuesById; //!< Raw touch event queues by id
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
        InputDeviceImplementationRequestHandler<InputDeviceTouch> m_implementationRequestHandler;
    };
} // namespace AzFramework
