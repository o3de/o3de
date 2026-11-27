/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputHapticFeedbackRequestBus.h>
#include <AzFramework/Input/Channels/InputChannelAnalog.h>
#include <AzFramework/Input/Channels/InputChannelAxis1D.h>
#include <AzFramework/Input/Channels/InputChannelAxis2D.h>
#include <AzFramework/Input/Channels/InputChannelAxis3D.h>
#include <AzFramework/Input/Channels/InputChannelDigital.h>
#include <AzFramework/Input/Channels/InputChannelQuaternion.h>
#include <AzFramework/Input/Devices/InputDevice.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>


namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Defines a generic XR controller pair device, including the ids of all associated input
    //! channels. Platform specifics are defined as private implementations so that creating an
    //! instance of this generic class will work correctly on any platform supporting this type of
    //! hand-held XR controllers.
    class InputDeviceXRController
        : public InputDevice
        , public InputHapticFeedbackRequestBus::Handler
        , public AzFramework::DebugDisplayEventBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The name used to identify an XR Controller input device
        static constexpr const char* Name{ "xr_controller" };
        static constexpr InputDeviceId IdForIndex0{ Name, 0 };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies an XR controller (regardless of index)
        //! @param inputDeviceId The input device id to check
        //! @return True if the input device id identifies as an XR controller, False otherwise
        static bool IsXRControllerDevice(const InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify XR Controller digital button inputs
        struct Button
        {
            static constexpr InputChannelId A{ "xr_controller_button_a" }; //!< The right-hand A button
            static constexpr InputChannelId B{ "xr_controller_button_b" }; //!< The right-hand B button
            static constexpr InputChannelId X{ "xr_controller_button_x" }; //!< The left-hand X button
            static constexpr InputChannelId Y{ "xr_controller_button_y" }; //!< The left-hand Y button
            static constexpr InputChannelId Home{ "xr_controller_button_home" }; //!< The right-hand "Home" button
            static constexpr InputChannelId Menu{ "xr_controller_button_menu" }; //!< The left-hand "Menu" button
            static constexpr InputChannelId L3{ "xr_controller_button_l3" }; //!< The left-hand thumb-stick click button
            static constexpr InputChannelId R3{ "xr_controller_button_r3" }; //!< The right-hand thumb-stick click button

            static constexpr InputChannelId TA{ "xr_controller_touch_button_a" }; //!< The A button touch detection
            static constexpr InputChannelId TB{ "xr_controller_touch_button_b" }; //!< The B button touch detection
            static constexpr InputChannelId TX{ "xr_controller_touch_button_x" }; //!< The X button touch detection
            static constexpr InputChannelId TY{ "xr_controller_touch_button_y" }; //!< The Y button touch detection
            static constexpr InputChannelId TLStick{ "xr_controller_touch_thumbstick_l" }; //!< The left thumb-stick touch detection
            static constexpr InputChannelId TRStick{ "xr_controller_touch_thumbstick_r" }; //!< The right thumb-stick touch detection
            static constexpr InputChannelId TLRest{ "xr_controller_touch_thumbrest_l" }; //!< The left thumb-rest touch detection
            static constexpr InputChannelId TRRest{ "xr_controller_touch_thumbrest_r" }; //!< The right thumb-rest touch detection
            static constexpr InputChannelId TLTrig{ "xr_controller_touch_trigger_l" }; //!< The left trigger touch detection
            static constexpr InputChannelId TRTrig{ "xr_controller_touch_trigger_r" }; //!< The right trigger touch detection

            //! All digital XR controller button ids
            static constexpr AZStd::array All
            {
                A,
                B,
                X,
                Y,
                Home,
                Menu,
                L3,
                R3,
                TA,
                TB,
                TX,
                TY,
                TLStick,
                TRStick,
                TLRest,
                TRRest,
                TLTrig,
                TRTrig,
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify XR Controller analog inputs
        struct Trigger
        {
            static constexpr InputChannelId LTrigger{ "xr_controller_trigger_l" }; //!< The left-hand trigger
            static constexpr InputChannelId RTrigger{ "xr_controller_trigger_r" }; //!< The right-hand trigger
            static constexpr InputChannelId LGrip{ "xr_controller_grip_l" }; //!< The left-hand grip
            static constexpr InputChannelId RGrip{ "xr_controller_grip_r" }; //!< The right-hand grip

            //! All analog XR Controller input ids
            static constexpr AZStd::array All
            {
                LTrigger,
                RTrigger,
                LGrip,
                RGrip
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify XR Controller 1D axis inputs
        struct ThumbStickAxis1D
        {
            static constexpr InputChannelId LX{ "xr_controller_thumbstick_l_x" }; //!< X-axis of the left-hand thumb-stick
            static constexpr InputChannelId LY{ "xr_controller_thumbstick_l_y" }; //!< Y-axis of the left-hand thumb-stick
            static constexpr InputChannelId RX{ "xr_controller_thumbstick_r_x" }; //!< X-axis of the right-hand thumb-stick
            static constexpr InputChannelId RY{ "xr_controller_thumbstick_r_y" }; //!< Y-axis of the right-hand thumb-stick

            //! All 1D axis XR Controller input ids
            static constexpr AZStd::array All
            {
                LX,
                LY,
                RX,
                RY
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify XR Controller 2D axis inputs
        struct ThumbStickAxis2D
        {
            static constexpr InputChannelId L{ "xr_controller_thumbstick_l" }; //!< The left-hand thumb-stick
            static constexpr InputChannelId R{ "xr_controller_thumbstick_r" }; //!< The right-hand thumb-stick

            //! All 2D axis XR Controller input ids
            static constexpr AZStd::array All
            {
                L,
                R
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify XR Controller thumb-stick directions
        struct ThumbStickDirection
        {
            static constexpr InputChannelId LU{ "xr_controller_thumbstick_l_up" }; //!< Up on the left-hand thumb-stick
            static constexpr InputChannelId LD{ "xr_controller_thumbstick_l_down" }; //!< Down on the left-hand thumb-stick
            static constexpr InputChannelId LL{ "xr_controller_thumbstick_l_left" }; //!< Left on the left-hand thumb-stick
            static constexpr InputChannelId LR{ "xr_controller_thumbstick_l_right" }; //!< Right on the left-hand thumb-stick
            static constexpr InputChannelId RU{ "xr_controller_thumbstick_r_up" }; //!< Up on the right-hand thumb-stick
            static constexpr InputChannelId RD{ "xr_controller_thumbstick_r_down" }; //!< Down on the right-hand thumb-stick
            static constexpr InputChannelId RL{ "xr_controller_thumbstick_r_left" }; //!< Left on the right-hand thumb-stick
            static constexpr InputChannelId RR{ "xr_controller_thumbstick_r_right" }; //!< Right on the right-hand thumb-stick

            //! All thumb-stick directional XR Controller input ids
            static constexpr AZStd::array All
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
        //! All the input channel ids that identify XR Controller 3D axis inputs
        struct ControllerPosePosition
        {
            static constexpr InputChannelId LPos{ "xr_controller_position_l" }; //!< The left-hand position
            static constexpr InputChannelId RPos{ "xr_controller_position_r" }; //!< The right-hand position

            //! All XR Controller position input ids
            static constexpr AZStd::array All
            {
                LPos,
                RPos,
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify XR Controller orientation inputs
        struct ControllerPoseOrientation
        {
            static constexpr InputChannelId LOrient{ "xr_controller_orientation_l" }; //!< The left-hand orientation
            static constexpr InputChannelId ROrient{ "xr_controller_orientation_r" }; //!< The right-hand orientation

            //! All XR Controller orientation input ids
            static constexpr AZStd::array All
            {
                LOrient,
                ROrient
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceXRController, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceXRController, "{31FC6155-5902-46E3-9CB7-C7E7673FE4CC}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // The internal implementation class that is passed to the constructor
        class Implementation
        {
        public:
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! @param inputDevice Reference to the input device being implemented
            static Implementation* Create(InputDeviceXRController& inputDevice);

            Implementation(InputDeviceXRController& inputDevice);
            AZ_DISABLE_COPY_MOVE(Implementation);
            virtual ~Implementation() = default;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query for a path representing an input channel
            //! Used for initializing Xr inputs.
            virtual AZStd::string GetInputChannelPath(const InputChannelId& channelId) const = 0;

            virtual AZStd::string GetInputDeviceProfilePath() const = 0;
            virtual AZStd::string GetLeftHandSubPath() const = 0;
            virtual AZStd::string GetRightHandSubPath() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Register a callback function for this implementation to call
            //! This callback is called during tick updates and allows for platform code to run
            //! updates to refresh state of the controller data.
            //! @param callbackFn The callback function
            using TickCallbackFn = AZStd::function<void()>;
            virtual void RegisterTickCallback(TickCallbackFn callbackFn) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query the connected state of the device
            //! @return True if the input device is currently connected, False otherwise
            virtual bool IsConnected() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Set the current vibration speed of the motors
            //! @param leftMotorSpeedNormalized Speed of the left motor
            //! @param rightMotorSpeedNormalized Speed of the right motor
            virtual void SetVibration(float leftMotorSpeedNormalized,
                                      float rightMotorSpeedNormalized) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Tick/update the input device to broadcast all input events since the last frame
            virtual void TickInputDevice() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Broadcast an event when the input device connects to the system
            void BroadcastInputDeviceConnectedEvent() const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Broadcast an event when the input device disconnects from the system
            void BroadcastInputDeviceDisconnectedEvent() const;

            using ButtonIdToBitMaskMap = AZStd::unordered_map<InputChannelId, AZ::u32>;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Platform agnostic representation of raw XR Controller state
            struct RawXRControllerState
            {
                ////////////////////////////////////////////////////////////////////////////////////
                //! Constructor
                //! @param digitalButtonMap A map of digital button ids by bitmask
                explicit RawXRControllerState(ButtonIdToBitMaskMap digitalButtonMap);

                AZ_DISABLE_COPY_MOVE(RawXRControllerState);
                ~RawXRControllerState() = default;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Reset the raw xr controller data
                void Reset();

                ////////////////////////////////////////////////////////////////////////////////////
                //! Gets a digital button's current state
                bool GetDigitalButtonState(const InputChannelId& channelId) const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the left trigger value adjusted for the dead zone and normalized
                //! @return The adjusted left trigger value
                float GetLeftTriggerAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the right trigger value adjusted for the dead zone and normalized
                //! @return The adjusted right trigger value
                float GetRightTriggerAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the left grip value adjusted for the dead zone and normalized
                //! @return The adjusted left grip value
                float GetLeftGripAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the right grip value adjusted for the dead zone and normalized
                //! @return The adjusted right grip value
                float GetRightGripAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the left thumb-stick values adjusted for the dead zone and normalized
                //! @return The adjusted left thumb-stick values
                AZ::Vector2 GetLeftThumbStickAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the right thumb-stick values adjusted for the dead zone and normalized
                //! @return The adjusted right thumb-stick values
                AZ::Vector2 GetRightThumbStickAdjustedForDeadZoneAndNormalized() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the left thumb-stick values normalized with no dead zone applied
                //! @return The normalized left thumb-stick values
                AZ::Vector2 GetLeftThumbStickNormalizedValues() const;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Get the right thumb-stick values normalized with no dead zone applied
                //! @return The normalized right thumb-stick values
                AZ::Vector2 GetRightThumbStickNormalizedValues() const;

                const ButtonIdToBitMaskMap m_buttonIdsToBitMasks;

                ////////////////////////////////////////////////////////////////////////////////////
                // Raw Data
                AZ::u32 m_digitalButtonStates = 0;          //!< The state of all digital buttons
                float m_leftTriggerState = 0.f;             //!< The left trigger value
                float m_rightTriggerState = 0.f;            //!< The right trigger value
                float m_leftGripState = 0.f;                //!< The left grip value
                float m_rightGripState = 0.f;               //!< The right grip value
                float m_leftThumbStickXState = 0.f;         //!< The left thumb-stick x-axis
                float m_leftThumbStickYState = 0.f;         //!< The left thumb-stick y-axis
                float m_rightThumbStickXState = 0.f;        //!< The right thumb-stick x-axis
                float m_rightThumbStickYState = 0.f;        //!< The right thumb-stick y-axis

                float m_triggerMaxValue = 0.f;              //!< The maximum value of the analog triggers
                float m_triggerDeadZoneValue = 0.f;         //!< The dead zone value of the analog triggers
                float m_gripMaxValue = 0.f;                 //!< The maximum value of the grip triggers
                float m_gripDeadZoneValue = 0.f;            //!< The dead zone value of the grip triggers
                float m_thumbStickMaxValue = 0.f;           //!< The maximum value of the thumb-sticks
                float m_leftThumbStickDeadZoneValue = 0.f;  //!< The left thumb-stick dead zone value
                float m_rightThumbStickDeadZoneValue = 0.f; //!< The right thumb-stick dead zone value

                float m_leftMotorVibrationValue = 0.f;      //!< The vibration amount of the left motor
                float m_rightMotorVibrationValue = 0.f;     //!< The vibration amount of the right motor

                AZ::Vector3 m_leftPositionState = AZ::Vector3::CreateZero();                //!< The left controller position
                AZ::Vector3 m_rightPositionState = AZ::Vector3::CreateZero();               //!< The right controller position
                AZ::Quaternion m_leftOrientationState = AZ::Quaternion::CreateIdentity();   //!< The left controller orientation
                AZ::Quaternion m_rightOrientationState = AZ::Quaternion::CreateIdentity();  //!< The right controller orientation
            }; // struct RawXRControllerState

            virtual RawXRControllerState& GetRawState() = 0;

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process a controller state that has been obtained since the last call to this function.
            //! @param rawControllerState The raw controller state
            void ProcessRawControllerState(const RawXRControllerState& rawControllerState);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the state of all this input device's associated input channels
            void ResetInputChannelStates();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! @see AzFramework::InputDeviceId::GetIndex
            AZ::u32 GetInputDeviceIndex() const;

        private:
            InputDeviceXRController& m_inputDevice;
        }; // class Implementation

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Alias for the function type used to create a custom implementation for this input device
        using ImplementationFactory = AZStd::function<Implementation*(InputDeviceXRController&)>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        InputDeviceXRController();
        explicit InputDeviceXRController(const InputDeviceId& inputDeviceId,
                                         ImplementationFactory implFactoryFn = &Implementation::Create);
        AZ_DISABLE_COPY_MOVE(InputDeviceXRController);
        ~InputDeviceXRController() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AzFramework::InputDevice interface
        const InputChannelByIdMap& GetInputChannelsById() const override;
        bool IsSupported() const override;
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AzFramework::InputDeviceRequests interface
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AzFramework::InputHapticFeedbackRequests interface
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the implementation of this input device
        //! @param impl The Implementation to use
        void SetImplementation(AZStd::unique_ptr<Implementation> impl);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the implementation of this input device
        //! @param implFactoryFn The Implementation factory create function to use
        void SetImplementation(const ImplementationFactory& implFactoryFn);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the non-owning pointer to the implementation of this input device
        //! @return The raw implementation pointer
        Implementation* GetImplementation() const;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AzFramework::DebugDisplayEventBus interface
        void DrawGlobalDebugInfo() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Helper routine that checks for a specific controller input combo and will toggle the debug draw.
        //! Allows users to quickly display/hide the debug information for the xr controllers without having
        //! to type anything into a console.
        //! Currently bound to: Button::Menu + Trigger::LTrigger
        //! That is, hold the left controller's Menu button and squeeze the left trigger fully.
        void CheckDebugDrawCheat() const;

        static constexpr float s_thumbStickMaxValue{ 1.f };
        static constexpr float s_thumbStickMinValue{ -1.f };
        static constexpr float s_thumbStickCenterValue{ 0.f };

        ////////////////////////////////////////////////////////////////////////////////////////////
        using ButtonChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelDigital*>;
        using TriggerChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAnalog*>;
        using ThumbStickAxis1DChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis1D*>;
        using ThumbStickAxis2DChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis2D*>;
        using ThumbStickDirectionChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAnalog*>;
        using ControllerAxis3DChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis3D*>;
        using ControllerPoseChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelQuaternion*>;

        InputChannelByIdMap m_allChannelsById{}; //!< All XR Controller input channels by id
        ButtonChannelByIdMap m_buttonChannelsById{}; //!< All digital button channels by id
        TriggerChannelByIdMap m_triggerChannelsById{}; //!< All analog trigger channels by id
        ThumbStickAxis1DChannelByIdMap m_thumbStick1DChannelsById{}; //!< All thumb-stick 1D axis channels by id
        ThumbStickAxis2DChannelByIdMap m_thumbStick2DChannelsById{}; //!< All thumb-stick 2D axis channels by id
        ThumbStickDirectionChannelByIdMap m_thumbStickDirectionChannelsById{}; //!< All thumb-stick direction channels by id
        ControllerAxis3DChannelByIdMap m_controllerPositionChannelsById{}; //!< All controller position channels by id
        ControllerPoseChannelByIdMap m_controllerOrientationChannelsById{}; //!< All controller orientation channels by id

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Private pointer to the platform implementation
        AZStd::unique_ptr<Implementation> m_impl;
    };

} // namespace AzFramework
