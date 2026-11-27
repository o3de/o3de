/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVk/InputDeviceXRController.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Input/Utils/AdjustAnalogInputForDeadZone.h>

// Debug Draw
#include <AzCore/Console/IConsole.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace OpenXRVk
{
    // Cvar to enable/disable debug drawing of xr controller data on screen.
    // No "on change" function defined here, just read the state of the bool
    // elsewhere in the draw function.
    AZ_CVAR(bool, xr_DebugDrawInput, 0,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "Turn off/on debug drawing of XR Input state");

} // namespace OpenXRVk


namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceXRController::IsXRControllerDevice(const InputDeviceId& inputDeviceId)
    {
        // Only need to check the name (crc) to check the device is an xr controller type.
        return (inputDeviceId.GetNameCrc32() == IdForIndex0.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
#define BEHAVIOR_XR_CONSTANT(channel)       Constant(channel.GetName(), BehaviorConstant(channel.GetName()))

            behaviorContext->Class<InputDeviceXRController>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(IdForIndex0.GetName()))
                // Standard digital buttons...
                ->BEHAVIOR_XR_CONSTANT(Button::A)
                ->BEHAVIOR_XR_CONSTANT(Button::B)
                ->BEHAVIOR_XR_CONSTANT(Button::X)
                ->BEHAVIOR_XR_CONSTANT(Button::Y)
                ->BEHAVIOR_XR_CONSTANT(Button::Home)
                ->BEHAVIOR_XR_CONSTANT(Button::Menu)
                ->BEHAVIOR_XR_CONSTANT(Button::L3)
                ->BEHAVIOR_XR_CONSTANT(Button::R3)
                // Touch capacitive...
                ->BEHAVIOR_XR_CONSTANT(Button::TA)
                ->BEHAVIOR_XR_CONSTANT(Button::TB)
                ->BEHAVIOR_XR_CONSTANT(Button::TX)
                ->BEHAVIOR_XR_CONSTANT(Button::TY)
                ->BEHAVIOR_XR_CONSTANT(Button::TLStick)
                ->BEHAVIOR_XR_CONSTANT(Button::TRStick)
                ->BEHAVIOR_XR_CONSTANT(Button::TLRest)
                ->BEHAVIOR_XR_CONSTANT(Button::TRRest)
                ->BEHAVIOR_XR_CONSTANT(Button::TLTrig)
                ->BEHAVIOR_XR_CONSTANT(Button::TRTrig)
                // Analog triggers...
                ->BEHAVIOR_XR_CONSTANT(Trigger::LTrigger)
                ->BEHAVIOR_XR_CONSTANT(Trigger::RTrigger)
                ->BEHAVIOR_XR_CONSTANT(Trigger::LGrip)
                ->BEHAVIOR_XR_CONSTANT(Trigger::RGrip)
                // Thumbsticks (1D)...
                ->BEHAVIOR_XR_CONSTANT(ThumbStickAxis1D::LX)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickAxis1D::LY)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickAxis1D::RX)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickAxis1D::RY)
                // Thumbsticks (2D)...
                ->BEHAVIOR_XR_CONSTANT(ThumbStickAxis2D::L)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickAxis2D::R)
                // Thumbstick directions...
                ->BEHAVIOR_XR_CONSTANT(ThumbStickDirection::LU)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickDirection::LD)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickDirection::LL)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickDirection::LR)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickDirection::RU)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickDirection::RD)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickDirection::RL)
                ->BEHAVIOR_XR_CONSTANT(ThumbStickDirection::RR)
                // Position (3D)...
                ->BEHAVIOR_XR_CONSTANT(ControllerPosePosition::LPos)
                ->BEHAVIOR_XR_CONSTANT(ControllerPosePosition::RPos)
                // Orientation (quaternion)...
                ->BEHAVIOR_XR_CONSTANT(ControllerPoseOrientation::LOrient)
                ->BEHAVIOR_XR_CONSTANT(ControllerPoseOrientation::ROrient)
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Default constructor
    //! Using the default constructor will not create an implementation.  It is then up to the user
    //! to call InputDeviceXRController::SetImplementation and supply either a unique_ptr<Implementation>
    //! or an ImplementationFactory function.
    InputDeviceXRController::InputDeviceXRController()
        : InputDeviceXRController(InputDeviceId(Name, 0), nullptr)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceXRController::InputDeviceXRController(const InputDeviceId& inputDeviceId,
                                                     ImplementationFactory implFactoryFn)
        : InputDevice(inputDeviceId)
    {
        // Create all digital button input channels
        for (const InputChannelId& channelId : Button::All)
        {
            auto channel = aznew InputChannelDigital(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_buttonChannelsById[channelId] = channel;
        }

        // Create all analog trigger input channels
        for (const InputChannelId& channelId : Trigger::All)
        {
            auto channel = aznew InputChannelAnalog(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_triggerChannelsById[channelId] = channel;
        }

        // Create all 1D thumb-stick input channels
        for (const InputChannelId& channelId : ThumbStickAxis1D::All)
        {
            auto channel = aznew InputChannelAxis1D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_thumbStick1DChannelsById[channelId] = channel;
        }

        // Create all 2D thumb-stick input channels
        for (const InputChannelId& channelId : ThumbStickAxis2D::All)
        {
            auto channel = aznew InputChannelAxis2D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_thumbStick2DChannelsById[channelId] = channel;
        }

        // Create all analog thumb-stick direction input channels
        for (const InputChannelId& channelId : ThumbStickDirection::All)
        {
            auto channel = aznew InputChannelAnalog(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_thumbStickDirectionChannelsById[channelId] = channel;
        }

        // Create all 3D controller position input channels
        for (const InputChannelId& channelId : ControllerPosePosition::All)
        {
            auto channel = aznew InputChannelAxis3D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_controllerPositionChannelsById[channelId] = channel;
        }

        // Create all Quat controller orientation input channels
        for (const InputChannelId& channelId : ControllerPoseOrientation::All)
        {
            auto channel = aznew InputChannelQuaternion(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_controllerOrientationChannelsById[channelId] = channel;
        }

        // Create the custom implementation
        SetImplementation(AZStd::move(implFactoryFn));

        // Connect to haptic feedback request bus
        InputHapticFeedbackRequestBus::Handler::BusConnect(GetInputDeviceId());

        // Debug Draw
        DebugDisplayEventBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceXRController::~InputDeviceXRController()
    {
        // Debug Draw
        DebugDisplayEventBus::Handler::BusDisconnect();

        // Disconnect from haptic feedback request bus
        InputHapticFeedbackRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        // Destroy the custom implementation
        m_impl.reset();

        // Destroy all input channels
        for (const auto& channelById : m_allChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceXRController::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceXRController::IsSupported() const
    {
        return m_impl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceXRController::IsConnected() const
    {
        return m_impl ? m_impl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::TickInputDevice()
    {
        if (m_impl)
        {
            m_impl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized)
    {
        if (m_impl)
        {
            m_impl->SetVibration(leftMotorSpeedNormalized, rightMotorSpeedNormalized);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::SetImplementation(AZStd::unique_ptr<Implementation> impl)
    {
        m_impl = AZStd::move(impl);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::SetImplementation(const ImplementationFactory& implFactoryFn)
    {
        if (implFactoryFn)
        {
            m_impl.reset(implFactoryFn(*this));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceXRController::Implementation* InputDeviceXRController::GetImplementation() const
    {
        return m_impl.get();
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! InputDeviceXRController::Implementation
    ////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceXRController::Implementation::Implementation(InputDeviceXRController& inputDevice)
        : m_inputDevice(inputDevice)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::Implementation::BroadcastInputDeviceConnectedEvent() const
    {
        m_inputDevice.BroadcastInputDeviceConnectedEvent();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::Implementation::BroadcastInputDeviceDisconnectedEvent() const
    {
        m_inputDevice.BroadcastInputDeviceDisconnectedEvent();
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! InputDeviceXRController::Implementation::RawXRControllerState
    ////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceXRController::Implementation::RawXRControllerState::RawXRControllerState(ButtonIdToBitMaskMap digitalButtonMap)
        : m_buttonIdsToBitMasks(AZStd::move(digitalButtonMap))
        , m_triggerMaxValue(1.f)
        , m_gripMaxValue(1.f)
        , m_thumbStickMaxValue(1.f)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::Implementation::RawXRControllerState::Reset()
    {
        m_digitalButtonStates = 0;
        m_leftTriggerState = 0.f;
        m_rightTriggerState = 0.f;
        m_leftGripState = 0.f;
        m_rightGripState = 0.f;
        m_leftThumbStickXState = 0.f;
        m_leftThumbStickYState = 0.f;
        m_rightThumbStickXState = 0.f;
        m_rightThumbStickYState = 0.f;
        m_leftPositionState = AZ::Vector3::CreateZero();
        m_rightPositionState = AZ::Vector3::CreateZero();
        m_leftOrientationState = AZ::Quaternion::CreateIdentity();
        m_rightOrientationState = AZ::Quaternion::CreateIdentity();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceXRController::Implementation::RawXRControllerState::GetDigitalButtonState(const InputChannelId& channelId) const
    {
        if (auto it = m_buttonIdsToBitMasks.find(channelId); it != m_buttonIdsToBitMasks.end())
        {
            return (m_digitalButtonStates & it->second) != 0;
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputDeviceXRController::Implementation::RawXRControllerState::GetLeftTriggerAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeAnalogInput(m_leftTriggerState, m_triggerDeadZoneValue, m_triggerMaxValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputDeviceXRController::Implementation::RawXRControllerState::GetRightTriggerAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeAnalogInput(m_rightTriggerState, m_triggerDeadZoneValue, m_triggerMaxValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputDeviceXRController::Implementation::RawXRControllerState::GetLeftGripAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeAnalogInput(m_leftGripState, m_gripDeadZoneValue, m_gripMaxValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputDeviceXRController::Implementation::RawXRControllerState::GetRightGripAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeAnalogInput(m_rightGripState, m_gripDeadZoneValue, m_gripMaxValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceXRController::Implementation::RawXRControllerState::GetLeftThumbStickAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeThumbStickInput(m_leftThumbStickXState, m_leftThumbStickYState,
                                                            m_leftThumbStickDeadZoneValue, m_thumbStickMaxValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceXRController::Implementation::RawXRControllerState::GetRightThumbStickAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeThumbStickInput(m_rightThumbStickXState, m_rightThumbStickYState,
                                                            m_rightThumbStickDeadZoneValue, m_thumbStickMaxValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceXRController::Implementation::RawXRControllerState::GetLeftThumbStickNormalizedValues() const
    {
        return AZ::Vector2(m_leftThumbStickXState / m_thumbStickMaxValue,
                           m_leftThumbStickYState / m_thumbStickMaxValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceXRController::Implementation::RawXRControllerState::GetRightThumbStickNormalizedValues() const
    {
        return AZ::Vector2(m_rightThumbStickXState / m_thumbStickMaxValue,
                           m_rightThumbStickYState / m_thumbStickMaxValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::Implementation::ProcessRawControllerState([[maybe_unused]] const RawXRControllerState& rawControllerState)
    {
        // Update digital button channels...
        for (const auto& [channelId, bitMask] : rawControllerState.m_buttonIdsToBitMasks)
        {
            const bool buttonState = (rawControllerState.m_digitalButtonStates & bitMask) != 0;
            m_inputDevice.m_buttonChannelsById[channelId]->ProcessRawInputEvent(buttonState);
        }

        using xrc = InputDeviceXRController;

        // Update the analog triggers...
        const float triggerL = rawControllerState.GetLeftTriggerAdjustedForDeadZoneAndNormalized();
        const float triggerR = rawControllerState.GetRightTriggerAdjustedForDeadZoneAndNormalized();
        const float gripL = rawControllerState.GetLeftGripAdjustedForDeadZoneAndNormalized();
        const float gripR = rawControllerState.GetRightGripAdjustedForDeadZoneAndNormalized();
        m_inputDevice.m_triggerChannelsById[xrc::Trigger::LTrigger]->ProcessRawInputEvent(triggerL);
        m_inputDevice.m_triggerChannelsById[xrc::Trigger::RTrigger]->ProcessRawInputEvent(triggerR);
        m_inputDevice.m_triggerChannelsById[xrc::Trigger::LGrip]->ProcessRawInputEvent(gripL);
        m_inputDevice.m_triggerChannelsById[xrc::Trigger::RGrip]->ProcessRawInputEvent(gripR);

        // Update thumb-stick channels...
        const AZ::Vector2 leftThumbStick = rawControllerState.GetLeftThumbStickAdjustedForDeadZoneAndNormalized();
        const AZ::Vector2 leftThumbStickPreDeadZone = rawControllerState.GetLeftThumbStickNormalizedValues();
        const float leftStickUp = AZ::GetClamp(leftThumbStick.GetY(), s_thumbStickCenterValue, s_thumbStickMaxValue);
        const float leftStickDown = fabsf(AZ::GetClamp(leftThumbStick.GetY(), s_thumbStickMinValue, s_thumbStickCenterValue));
        const float leftStickLeft = fabsf(AZ::GetClamp(leftThumbStick.GetX(), s_thumbStickMinValue, s_thumbStickCenterValue));
        const float leftStickRight = AZ::GetClamp(leftThumbStick.GetX(), s_thumbStickCenterValue, s_thumbStickMaxValue);
        const AZ::Vector2 rightThumbStick = rawControllerState.GetRightThumbStickAdjustedForDeadZoneAndNormalized();
        const AZ::Vector2 rightThumbStickPreDeadZone = rawControllerState.GetRightThumbStickNormalizedValues();
        const float rightStickUp = AZ::GetClamp(rightThumbStick.GetY(), s_thumbStickCenterValue, s_thumbStickMaxValue);
        const float rightStickDown = fabsf(AZ::GetClamp(rightThumbStick.GetY(), s_thumbStickMinValue, s_thumbStickCenterValue));
        const float rightStickLeft = fabsf(AZ::GetClamp(rightThumbStick.GetX(), s_thumbStickMinValue, s_thumbStickCenterValue));
        const float rightStickRight = AZ::GetClamp(rightThumbStick.GetX(), s_thumbStickCenterValue, s_thumbStickMaxValue);

        m_inputDevice.m_thumbStick2DChannelsById[xrc::ThumbStickAxis2D::L]->ProcessRawInputEvent(leftThumbStick, &leftThumbStickPreDeadZone);
        m_inputDevice.m_thumbStick1DChannelsById[xrc::ThumbStickAxis1D::LX]->ProcessRawInputEvent(leftThumbStick.GetX());
        m_inputDevice.m_thumbStick1DChannelsById[xrc::ThumbStickAxis1D::LY]->ProcessRawInputEvent(leftThumbStick.GetY());
        m_inputDevice.m_thumbStickDirectionChannelsById[xrc::ThumbStickDirection::LU]->ProcessRawInputEvent(leftStickUp);
        m_inputDevice.m_thumbStickDirectionChannelsById[xrc::ThumbStickDirection::LD]->ProcessRawInputEvent(leftStickDown);
        m_inputDevice.m_thumbStickDirectionChannelsById[xrc::ThumbStickDirection::LL]->ProcessRawInputEvent(leftStickLeft);
        m_inputDevice.m_thumbStickDirectionChannelsById[xrc::ThumbStickDirection::LR]->ProcessRawInputEvent(leftStickRight);
        m_inputDevice.m_thumbStick2DChannelsById[xrc::ThumbStickAxis2D::R]->ProcessRawInputEvent(rightThumbStick, &rightThumbStickPreDeadZone);
        m_inputDevice.m_thumbStick1DChannelsById[xrc::ThumbStickAxis1D::RX]->ProcessRawInputEvent(rightThumbStick.GetX());
        m_inputDevice.m_thumbStick1DChannelsById[xrc::ThumbStickAxis1D::RY]->ProcessRawInputEvent(rightThumbStick.GetY());
        m_inputDevice.m_thumbStickDirectionChannelsById[xrc::ThumbStickDirection::RU]->ProcessRawInputEvent(rightStickUp);
        m_inputDevice.m_thumbStickDirectionChannelsById[xrc::ThumbStickDirection::RD]->ProcessRawInputEvent(rightStickDown);
        m_inputDevice.m_thumbStickDirectionChannelsById[xrc::ThumbStickDirection::RL]->ProcessRawInputEvent(rightStickLeft);
        m_inputDevice.m_thumbStickDirectionChannelsById[xrc::ThumbStickDirection::RR]->ProcessRawInputEvent(rightStickRight);

        // Position update...
        m_inputDevice.m_controllerPositionChannelsById[xrc::ControllerPosePosition::LPos]
            ->ProcessRawInputEvent(rawControllerState.m_leftPositionState);
        m_inputDevice.m_controllerPositionChannelsById[xrc::ControllerPosePosition::RPos]
            ->ProcessRawInputEvent(rawControllerState.m_rightPositionState);

        // Orientation update...
        m_inputDevice.m_controllerOrientationChannelsById[xrc::ControllerPoseOrientation::LOrient]
            ->ProcessRawInputEvent(rawControllerState.m_leftOrientationState);
        m_inputDevice.m_controllerOrientationChannelsById[xrc::ControllerPoseOrientation::ROrient]
            ->ProcessRawInputEvent(rawControllerState.m_rightOrientationState);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 InputDeviceXRController::Implementation::GetInputDeviceIndex() const
    {
        return m_inputDevice.GetInputDeviceId().GetIndex();
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Debug Draw Related Functions
    ////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(AZ_RELEASE_BUILD)

    static AZ::Transform GetCameraTransformFromCurrentView()
    {
        if (const auto viewportContextMgr = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            viewportContextMgr != nullptr)
        {
            if (const AZ::RPI::ViewportContextPtr viewportContext = viewportContextMgr->GetDefaultViewportContext();
                viewportContext != nullptr)
            {
                if (const AZ::RPI::ViewPtr view = viewportContext->GetDefaultView();
                    view != nullptr)
                {
                    return view->GetCameraTransform();
                }
            }
        }
        return AZ::Transform::CreateIdentity();
    }

    static void DrawControllerAxes(DebugDisplayRequests& debugDisplay, const AZ::Vector3& position, const AZ::Quaternion& orientation)
    {
        static const AZ::Color axisColorX(1.f, 0.f, 0.f, 0.9f);
        static const AZ::Color axisColorY(0.f, 1.f, 0.f, 0.9f);
        static const AZ::Color axisColorZ(0.f, 0.f, 1.f, 0.9f);

        const auto cameraTransform = GetCameraTransformFromCurrentView();
        const AZ::Vector3& cameraPosition = cameraTransform.GetTranslation();
        const AZ::Vector3 controllerPosition = cameraPosition + position;

        const AZ::Transform controllerTransform = AZ::Transform::CreateFromQuaternionAndTranslation(orientation, controllerPosition);
        debugDisplay.SetColor(axisColorX);
        debugDisplay.DrawLine(controllerPosition, controllerPosition + controllerTransform.GetBasisX());
        debugDisplay.SetColor(axisColorY);
        debugDisplay.DrawLine(controllerPosition, controllerPosition + controllerTransform.GetBasisY());
        debugDisplay.SetColor(axisColorZ);
        debugDisplay.DrawLine(controllerPosition, controllerPosition + controllerTransform.GetBasisZ());
    }

#endif // !AZ_RELEASE_BUILD

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::CheckDebugDrawCheat() const
    {
#if !defined(AZ_RELEASE_BUILD)
        // This looks for specific controller input and will toggle the debug draw cvar.
        const auto& rawControllerData = m_impl->GetRawState();
        using xrc = InputDeviceXRController;
        static bool cheatWasPressed = false;

        // Menu button + Left Trigger pulled past 0.9 will toggle.
        // To avoid button bounce, block re-toggle until the menu button is released.
        const bool menuPressed = rawControllerData.GetDigitalButtonState(xrc::Button::Menu);
        const float leftTrigger = rawControllerData.GetLeftTriggerAdjustedForDeadZoneAndNormalized();
        if (menuPressed)
        {
            if (!cheatWasPressed && leftTrigger > 0.9f)
            {
                cheatWasPressed = true;
                OpenXRVk::xr_DebugDrawInput = !OpenXRVk::xr_DebugDrawInput;
            }
        }
        else
        {
            cheatWasPressed = false;
        }
#endif // !AZ_RELEASE_BUILD
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceXRController::DrawGlobalDebugInfo()
    {
#if !defined(AZ_RELEASE_BUILD)
        CheckDebugDrawCheat();

        if (!OpenXRVk::xr_DebugDrawInput)
        {
            return;
        }

        DebugDisplayRequestBus::BusPtr debugDisplayBus;
        DebugDisplayRequestBus::Bind(debugDisplayBus, g_defaultSceneEntityDebugDisplayId);
        DebugDisplayRequests* debugDisplay{ DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus) };
        if (!debugDisplay || !IsSupported())
        {
            return;
        }

        // Save previous draw state
        const AZ::u32 oldDrawState{ debugDisplay->GetState() };

        // ... draw data to the screen ...
        const auto& rawControllerData = m_impl->GetRawState();
        DrawControllerAxes(*debugDisplay, rawControllerData.m_leftPositionState, rawControllerData.m_leftOrientationState);
        DrawControllerAxes(*debugDisplay, rawControllerData.m_rightPositionState, rawControllerData.m_rightOrientationState);

        float drawX = 20.f;     // current draw X
        float drawY = 20.f;     // current draw Y
        constexpr float textSize = 0.8f;
        constexpr float lineHeight = 15.f;

        const AZ::Color whiteColor{ 1.f, 1.f, 1.f, 1.f };
        const AZ::Color pressedColor{ 0.f, 1.f, 0.2f, 1.f };
        const AZ::Color touchedColor{ 0.7f, 0.5f, 0.2f, 1.f };
        const AZ::Color defaultColor{ 0.2f, 0.2f, 0.2f, 0.8f };

        auto printButtonWithTouchState = [&](const InputChannelId& buttonChannel,
            const InputChannelId& touchedButtonChannel, const char* buttonText)
        {
            AZStd::string text{ buttonText };
            if (rawControllerData.GetDigitalButtonState(buttonChannel))
            {
                text.append(" Pressed");
                debugDisplay->SetColor(pressedColor);
            }
            else if (rawControllerData.GetDigitalButtonState(touchedButtonChannel))
            {
                text.append(" Touched");
                debugDisplay->SetColor(touchedColor);
            }
            else
            {
                debugDisplay->SetColor(defaultColor);
            }
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, text.c_str());

            drawY += lineHeight;
        };

        auto printButtonState = [&](const InputChannelId& buttonChannel, const char* buttonText)
        {
            AZStd::string text{ buttonText };
            if (rawControllerData.GetDigitalButtonState(buttonChannel))
            {
                text.append(" Pressed");
                debugDisplay->SetColor(pressedColor);
            }
            else
            {
                debugDisplay->SetColor(defaultColor);
            }
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, text.c_str());

            drawY += lineHeight;
        };

        auto printButtonTouchOnlyState = [&](const InputChannelId& touchChannel, const char* buttonText)
        {
            AZStd::string text{ buttonText };
            if (rawControllerData.GetDigitalButtonState(touchChannel))
            {
                text.append(" Touched");
                debugDisplay->SetColor(touchedColor);
            }
            else
            {
                debugDisplay->SetColor(defaultColor);
            }
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, text.c_str());

            drawY += lineHeight;
        };

        auto printAnalogWithTouchState = [&](const InputChannelId& touchedChannel, const char* analogText, float value)
        {
            AZStd::string text{ analogText };
            if (!AZ::IsClose(value, 0.f))
            {
                text.append(AZStd::string::format(" Pressed: %.2f", value));
                debugDisplay->SetColor(pressedColor);
            }
            else if (rawControllerData.GetDigitalButtonState(touchedChannel))
            {
                text.append(" Touched");
                debugDisplay->SetColor(touchedColor);
            }
            else
            {
                debugDisplay->SetColor(defaultColor);
            }
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, text.c_str());

            drawY += lineHeight;
        };

        auto printAnalogState = [&](const char* analogText, float value)
        {
            AZStd::string text{ analogText };
            if (!AZ::IsClose(value, 0.f))
            {
                text.append(AZStd::string::format(" = %.2f", value));
                debugDisplay->SetColor(pressedColor);
            }
            else
            {
                debugDisplay->SetColor(defaultColor);
            }
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, text.c_str());

            drawY += lineHeight;
        };

        auto print2DThumbStickWithTouchState = [&](const InputChannelId& touchedChannel, const char* thumbStickText, float xvalue, float yvalue)
        {
            AZStd::string text{ thumbStickText };
            if (!AZ::IsClose(xvalue, 0.f) || !AZ::IsClose(yvalue, 0.f))
            {
                text.append(AZStd::string::format(" Pressed: (%.2f, %.2f)", xvalue, yvalue));
                debugDisplay->SetColor(pressedColor);
            }
            else if (rawControllerData.GetDigitalButtonState(touchedChannel))
            {
                text.append(" Touched");
                debugDisplay->SetColor(touchedColor);
            }
            else
            {
                debugDisplay->SetColor(defaultColor);
            }
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, text.c_str());

            drawY += lineHeight;
        };

        auto printVector3 = [&](const AZ::Vector3& vec, const char* vectorText)
        {
            AZStd::string str{ AZStd::string::format("%s = (%.2f, %.2f, %.2f)", vectorText, vec.GetX(), vec.GetY(), vec.GetZ()) };
            debugDisplay->SetColor(whiteColor);
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, str.c_str());

            drawY += lineHeight;
        };

        auto printMatrix3x4 = [&](const AZ::Matrix3x4& matx, const char* matrixText)
        {
            debugDisplay->SetColor(whiteColor);
            AZStd::string str{ AZStd::string::format("%s:", matrixText) };
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, str.c_str());
            drawY += lineHeight;

            AZ::Vector3 col0, col1, col2, col3;
            matx.GetColumns(&col0, &col1, &col2, &col3);
            str = AZStd::string::format("    | %.2f    %.2f    %.2f    %.2f |", col0.GetX(), col1.GetX(), col2.GetX(), col3.GetX());
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, str.c_str());
            drawY += lineHeight;
            str = AZStd::string::format("    | %.2f    %.2f    %.2f    %.2f |", col0.GetY(), col1.GetY(), col2.GetY(), col3.GetY());
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, str.c_str());
            drawY += lineHeight;
            str = AZStd::string::format("    | %.2f    %.2f    %.2f    %.2f |", col0.GetZ(), col1.GetZ(), col2.GetZ(), col3.GetZ());
            debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, str.c_str());
            drawY += lineHeight;
        };


        using xrc = InputDeviceXRController;

        // Left controller...
        debugDisplay->SetColor(whiteColor);
        debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, "Left XR Controller");
        drawY += lineHeight;

        printButtonWithTouchState(xrc::Button::X, xrc::Button::TX, "X");
        printButtonWithTouchState(xrc::Button::Y, xrc::Button::TY, "Y");
        printButtonState(xrc::Button::L3, "L3");
        printButtonState(xrc::Button::Menu, "Menu");
        printButtonTouchOnlyState(xrc::Button::TLRest, "L ThumbRest");
        printAnalogWithTouchState(xrc::Button::TLTrig, "L Trigger", rawControllerData.m_leftTriggerState);
        printAnalogState("L Grip", rawControllerData.m_leftGripState);
        print2DThumbStickWithTouchState(xrc::Button::TLStick, "L Thumb-Stick",
            rawControllerData.m_leftThumbStickXState, rawControllerData.m_leftThumbStickYState);

        drawY += (2.f * lineHeight);

        // Right controller...
        debugDisplay->SetColor(whiteColor);
        debugDisplay->Draw2dTextLabel(drawX, drawY, textSize, "Right XR Controller");
        drawY += lineHeight;

        printButtonWithTouchState(xrc::Button::A, xrc::Button::TA, "A");
        printButtonWithTouchState(xrc::Button::B, xrc::Button::TB, "B");
        printButtonState(xrc::Button::R3, "R3");
        printButtonState(xrc::Button::Home, "Home");
        printButtonTouchOnlyState(xrc::Button::TRRest, "R ThumbRest");
        printAnalogWithTouchState(xrc::Button::TLTrig, "R Trigger", rawControllerData.m_rightTriggerState);
        printAnalogState("R Grip", rawControllerData.m_rightGripState);
        print2DThumbStickWithTouchState(xrc::Button::TRStick, "R Thumb-Stick",
            rawControllerData.m_rightThumbStickXState, rawControllerData.m_rightThumbStickYState);

        drawY += (2.f * lineHeight);

        // Positions and Orientation
        printVector3(rawControllerData.m_leftPositionState, "Left Controller Position");
        printMatrix3x4(AZ::Matrix3x4::CreateFromQuaternion(rawControllerData.m_leftOrientationState), "Left Controller Orientation");
        printVector3(rawControllerData.m_rightPositionState, "Right Controller Position");
        printMatrix3x4(AZ::Matrix3x4::CreateFromQuaternion(rawControllerData.m_rightOrientationState), "Right Controller Orientation");

        // Restore previous state
        debugDisplay->SetState(oldDrawState);
#endif // !AZ_RELEASE_BUILD
    }

} // namespace AzFramework
