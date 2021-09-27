/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputHapticFeedbackRequestBus.h>
#include <AzFramework/Input/Buses/Requests/InputLightBarRequestBus.h>
#include <AzFramework/Input/Channels/InputChannelAnalog.h>
#include <AzFramework/Input/Channels/InputChannelAxis1D.h>
#include <AzFramework/Input/Channels/InputChannelAxis2D.h>
#include <AzFramework/Input/Channels/InputChannelDigital.h>
#include <AzFramework/Input/Devices/InputDevice.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Defines a generic game-pad input device, including the ids of all associated input channels.
    //! Platform specific implementations are defined as private implementations so that creating an
    //! instance of this generic class will work correctly on any platform supporting game-pad input,
    //! while providing access to the device name and associated channel ids on any platform through
    //! the 'null' implementation (primarily so that the editor can use them to setup input mappings).
    class InputDeviceGamepad : public InputDevice
                             , public InputHapticFeedbackRequestBus::Handler
                             , public InputLightBarRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The name used to identify any game-pad input device
        static const char* Name;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The id used to identify a game-pad input device with a specific index
        ///@{
        static const InputDeviceId IdForIndex0;
        static const InputDeviceId IdForIndex1;
        static const InputDeviceId IdForIndex2;
        static const InputDeviceId IdForIndex3;
        static const InputDeviceId IdForIndexN(AZ::u32 n);
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies a gamepad (regardless of index)
        //! \param[in] inputDeviceId The input device id to check
        //! \return True if the input device id identifies a gamepad, false otherwise
        static bool IsGamepadDevice(const InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the maximum number of gamepads that are supported on the current platform
        //! \return The maximum number of gamepads that are supported on the current platform
        static AZ::u32 GetMaxSupportedGamepads();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify game-pad digital button input
        struct Button
        {
            static constexpr inline InputChannelId A{"gamepad_button_a"}; //!< The bottom diamond face button
            static constexpr inline InputChannelId B{"gamepad_button_b"}; //!< The right diamond face button
            static constexpr inline InputChannelId X{"gamepad_button_x"}; //!< The left diamond face button
            static constexpr inline InputChannelId Y{"gamepad_button_y"}; //!< The top diamond face button
            static constexpr inline InputChannelId L1{"gamepad_button_l1"}; //!< The top-left shoulder bumper button
            static constexpr inline InputChannelId R1{"gamepad_button_r1"}; //!< The top-right shoulder bumper button
            static constexpr inline InputChannelId L3{"gamepad_button_l3"}; //!< The left thumb-stick click button
            static constexpr inline InputChannelId R3{"gamepad_button_r3"}; //!< The right thumb-stick click button
            static constexpr inline InputChannelId DU{"gamepad_button_d_up"}; //!< The up directional pad button
            static constexpr inline InputChannelId DD{"gamepad_button_d_down"}; //!< The down directional pad button
            static constexpr inline InputChannelId DL{"gamepad_button_d_left"}; //!< The left directional pad button
            static constexpr inline InputChannelId DR{"gamepad_button_d_right"}; //!< The right directional pad button
            static constexpr inline InputChannelId Start{"gamepad_button_start"}; //!< The start/pause/options button
            static constexpr inline InputChannelId Select{"gamepad_button_select"}; //!< The select/back button

            //!< All digital game-pad button ids
            static constexpr inline AZStd::array All
            {
                A,
                B,
                X,
                Y,
                L1,
                R1,
                L3,
                R3,
                DU,
                DD,
                DL,
                DR,
                Start,
                Select
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify game-pad analog trigger input
        struct Trigger
        {
            static constexpr inline InputChannelId L2{"gamepad_trigger_l2"}; //!< The bottom-left shoulder trigger
            static constexpr inline InputChannelId R2{"gamepad_trigger_r2"}; //!< The bottom-right shoulder trigger

            //!< All analog game-pad trigger ids
            static constexpr inline AZStd::array All
            {
                L2,
                R2
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify game-pad thumb-stick 2D axis input
        struct ThumbStickAxis2D
        {
            static constexpr inline InputChannelId L{"gamepad_thumbstick_l"}; //!< The left-hand thumb-stick
            static constexpr inline InputChannelId R{"gamepad_thumbstick_r"}; //!< The right-hand thumb-stick

            //!< All game-pad thumb-stick 2D axis input channel ids
            static constexpr inline AZStd::array All
            {
                L,
                R
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify game-pad thumb-stick 1D axis input
        struct ThumbStickAxis1D
        {
            static constexpr inline InputChannelId LX{"gamepad_thumbstick_l_x"}; //!< X-axis of the left-hand thumb-stick
            static constexpr inline InputChannelId LY{"gamepad_thumbstick_l_y"}; //!< Y-axis of the left-hand thumb-stick
            static constexpr inline InputChannelId RX{"gamepad_thumbstick_r_x"}; //!< X-axis of the right-hand thumb-stick
            static constexpr inline InputChannelId RY{"gamepad_thumbstick_r_y"}; //!< Y-axis of the right-hand thumb-stick

            //!< All game-pad thumb-stick 1D axis input channel ids
            static constexpr inline AZStd::array All
            {
                LX,
                LY,
                RX,
                RY
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify game-pad thumb-stick directional input
        struct ThumbStickDirection
        {
            static constexpr inline InputChannelId LU{"gamepad_thumbstick_l_up"}; //!< Up on the left-hand thumb-stick
            static constexpr inline InputChannelId LD{"gamepad_thumbstick_l_down"}; //!< Down on the left-hand thumb-stick
            static constexpr inline InputChannelId LL{"gamepad_thumbstick_l_left"}; //!< Left on the left-hand thumb-stick
            static constexpr inline InputChannelId LR{"gamepad_thumbstick_l_right"}; //!< Right on the left-hand thumb-stick
            static constexpr inline InputChannelId RU{"gamepad_thumbstick_r_up"}; //!< Up on the left-hand thumb-stick
            static constexpr inline InputChannelId RD{"gamepad_thumbstick_r_down"}; //!< Down on the left-hand thumb-stick
            static constexpr inline InputChannelId RL{"gamepad_thumbstick_r_left"}; //!< Left on the left-hand thumb-stick
            static constexpr inline InputChannelId RR{"gamepad_thumbstick_r_right"}; //!< Right on the left-hand thumb-stick

            //!< All game-pad thumb-stick directional input channel ids
            static constexpr inline AZStd::array All
            {
                LU,
                LD,
                LL,
                LR,
                RU,
                RD,
                RL,
                RR
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceGamepad, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceGamepad, "{16652E28-4B60-4852-BBD0-CB6A2D1B7377}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        explicit InputDeviceGamepad();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] index Index of the game-pad device
        explicit InputDeviceGamepad(AZ::u32 index);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceGamepad);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceGamepad() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::GetAssignedLocalUserId
        LocalUserId GetAssignedLocalUserId() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::PromptLocalUserSignIn
        void PromptLocalUserSignIn() const override;

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
        //! \ref AzFramework::InputDeviceRequests::GetPhysicalKeyOrButtonText
        void GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                        AZStd::string& o_keyOrButtonText) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputHapticFeedbackRequests::SetVibration
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputLightBarRequests::SetLightBarColor
        void SetLightBarColor(const AZ::Color& color) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputLightBarRequests::ResetLightBarColor
        void ResetLightBarColor() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Alias for verbose container class
        using ButtonChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelDigital*>;
        using TriggerChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAnalog*>;
        using ThumbStickAxis1DChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis1D*>;
        using ThumbStickAxis2DChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis2D*>;
        using ThumbStickDirectionChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAnalog*>;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannelByIdMap               m_allChannelsById;                 //!< All game-pad input channels by id
        ButtonChannelByIdMap              m_buttonChannelsById;              //!< All digital button channels by id
        TriggerChannelByIdMap             m_triggerChannelsById;             //!< All analog trigger channels by id
        ThumbStickAxis1DChannelByIdMap    m_thumbStickAxis1DChannelsById;    //!< All thumb-stick axis 1D channels by id
        ThumbStickAxis2DChannelByIdMap    m_thumbStickAxis2DChannelsById;    //!< All thumb-stick axis 2D channels by id
        ThumbStickDirectionChannelByIdMap m_thumbStickDirectionChannelsById; //!< All thumb-stick direction channels by id

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations of game-pad input devices
        class Implementation
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! \param[in] inputDevice Reference to the input device being implemented
            static Implementation* Create(InputDeviceGamepad& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] inputDevice Reference to the input device being implemented
            Implementation(InputDeviceGamepad& inputDevice);

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
            //! Prompt a local user sign-in request from this input device
            virtual void PromptLocalUserSignIn() const {}

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query the connected state of the input device
            //! \return True if the input device is currently connected, false otherwise
            virtual bool IsConnected() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Set the current vibration (force-feedback) speed of the gamepads motors
            //! \param[in] leftMotorSpeedNormalized Speed of the left (large/low frequency) motor
            //! \param[in] rightMotorSpeedNormalized Speed of the right (small/high frequency) motor
            virtual void SetVibration(float leftMotorSpeedNormalized,
                                      float rightMotorSpeedNormalized) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Set the current light bar color of the gamepad (if one exists)
            //! \param[in] color The color to set the gamepad's light bar
            virtual void SetLightBarColor(const AZ::Color& color) { AZ_UNUSED(color); }

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the light bar color of the gamepad (if one exists) to it's default
            virtual void ResetLightBarColor() {}

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Get the text displayed on the physical key/button associated with an input channel.
            //! \param[in] inputChannelId The input channel id whose key or button text to return
            //! \param[out] o_keyOrButtonText The text displayed on the physical key/button if found
            //! \return True if o_keyOrButtonText was set, false otherwise
            virtual bool GetPhysicalKeyOrButtonText(const InputChannelId& /*inputChannelId*/,
                                                    AZStd::string& /*o_keyOrButtonText*/) const { return false; }

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Tick/update the input device to broadcast all input events since the last frame
            virtual void TickInputDevice() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Broadcast an event when the input device connects to the system
            void BroadcastInputDeviceConnectedEvent() const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Broadcast an event when the input device disconnects from the system
            void BroadcastInputDeviceDisconnectedEvent() const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Alias for verbose container class
            using DigitalButtonIdByBitMaskMap = AZStd::unordered_map<AZ::u32, const InputChannelId*>;

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Platform agnostic representation of a raw game-pad state
            struct RawGamepadState
            {
                ////////////////////////////////////////////////////////////////////////////////////
                //! Constructor
                //! \param[in] digitalButtonIdsByBitMask A map of digital button ids by bitmask
                RawGamepadState(const DigitalButtonIdByBitMaskMap& digitalButtonIdsByBitMask);

                ////////////////////////////////////////////////////////////////////////////////////
                // Disable copying
                AZ_DISABLE_COPY_MOVE(RawGamepadState);

                ////////////////////////////////////////////////////////////////////////////////////
                //! Default destructor
                ~RawGamepadState() = default;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Reset the raw gamepad state
                void Reset();

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the left trigger value adjusted for the dead zone and normalized
                //! \return The left trigger value adjusted for the dead zone and normalized
                float GetLeftTriggerAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the right trigger value adjusted for the dead zone and normalized
                //! \return The right trigger value adjusted for the dead zone and normalized
                float GetRightTriggerAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the left thumb-stick values adjusted for the dead zone and normalized
                //! \return The left thumb-stick values adjusted for the dead zone and normalized
                AZ::Vector2 GetLeftThumbStickAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the right thumb-stick values adjusted for the dead zone and normalized
                //! \return The right thumb-stick values adjusted for the dead zone and normalized
                AZ::Vector2 GetRightThumbStickAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the left thumb-stick values normalized with no dead zone applied
                //! \return The left thumb-stick values normalized with no dead zone applied
                AZ::Vector2 GetLeftThumbStickNormalizedValues() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the right thumb-stick values normalized with no dead zone applied
                //! \return The right thumb-stick values normalized with no dead zone applied
                AZ::Vector2 GetRightThumbStickNormalizedValues() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! The map of digital button ids by bitmask
                const DigitalButtonIdByBitMaskMap m_digitalButtonIdsByBitMask;

                ////////////////////////////////////////////////////////////////////////////////////
                // Variables
                AZ::u32 m_digitalButtonStates;     //!< The state of all digital buttons
                float   m_triggerButtonLState;     //!< The state of the left trigger button
                float   m_triggerButtonRState;     //!< The state of the right trigger button
                float   m_thumbStickLeftXState;    //!< The state of the left thumb-stick x-axis
                float   m_thumbStickLeftYState;    //!< The state of the left thumb-stick y-axis
                float   m_thumbStickRightXState;   //!< The state of the right thumb-stick x-axis
                float   m_thumbStickRightYState;   //!< The state of the right thumb-stick y-axis
                float   m_triggerMaximumValue;     //!< The analog trigger maximum value
                float   m_triggerDeadZoneValue;    //!< The analog trigger dead zone value
                float   m_thumbStickMaximumValue;  //!< The thumb-stick maximum radius value
                float   m_thumbStickLeftDeadZone;  //!< The left thumb-stick radial dead zone value
                float   m_thumbStickRightDeadZone; //!< The right thumb-stick radial dead zone value
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process a game-pad state that has been obtained since the last call to this function.
            //! This function is not thread safe, and so should only be called from the main thread.
            //! \param[in] rawGamepadState The raw game-pad state
            void ProcessRawGamepadState(const RawGamepadState& rawGamepadState);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the state of all this input device's associated input channels
            void ResetInputChannelStates();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AzFramework::InputDeviceId::GetIndex
            AZ::u32 GetInputDeviceIndex() const;

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            InputDeviceGamepad& m_inputDevice; //!< Reference to the input device
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
        InputDeviceImplementationRequestHandler<InputDeviceGamepad> m_implementationRequestHandler;
    };
} // namespace AzFramework
