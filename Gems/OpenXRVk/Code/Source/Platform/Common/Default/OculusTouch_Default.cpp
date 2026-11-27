/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OculusTouch_Default.h"

#include <OpenXRVk_Platform.h>

namespace OpenXRVk
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Map of digital button ids keyed by their button bitmask
    const AzFramework::InputDeviceXRController::Implementation::ButtonIdToBitMaskMap GetButtonIdToBitMaskMap()
    {
        using xrc = AzFramework::InputDeviceXRController;
        return {
            { xrc::Button::A, (1 << 0) },
            { xrc::Button::B, (1 << 1) },
            { xrc::Button::X, (1 << 2) },
            { xrc::Button::Y, (1 << 3) },
            { xrc::Button::Home, (1 << 4) },
            { xrc::Button::Menu, (1 << 5) },
            { xrc::Button::L3, (1 << 6) },
            { xrc::Button::R3, (1 << 7) },
            { xrc::Button::TA, (1 << 8) },
            { xrc::Button::TB, (1 << 9) },
            { xrc::Button::TX, (1 << 10) },
            { xrc::Button::TY, (1 << 11) },
            { xrc::Button::TLStick, (1 << 12) },
            { xrc::Button::TRStick, (1 << 13) },
            { xrc::Button::TLRest, (1 << 14) },
            { xrc::Button::TRRest, (1 << 15) },
            { xrc::Button::TLTrig, (1 << 16) },
            { xrc::Button::TRTrig, (1 << 17) },
        };
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceOculusTouch::InputDeviceOculusTouch(AzFramework::InputDeviceXRController& inputDevice)
        : AzFramework::InputDeviceXRController::Implementation(inputDevice)
        , m_rawControllerState(GetButtonIdToBitMaskMap())
    {
        // These are guesses for now
        m_rawControllerState.m_triggerMaxValue = 1.f;
        m_rawControllerState.m_triggerDeadZoneValue = 0.1f;
        m_rawControllerState.m_thumbStickMaxValue = 1.f;
        m_rawControllerState.m_leftThumbStickDeadZoneValue = 0.1f;
        m_rawControllerState.m_rightThumbStickDeadZoneValue = 0.1f;

        using xrc = AzFramework::InputDeviceXRController;
        m_xrPathMap = {
            { xrc::Button::A, "/user/hand/right/input/a/click" },
            { xrc::Button::B, "/user/hand/right/input/b/click" },
            { xrc::Button::X, "/user/hand/left/input/x/click" },
            { xrc::Button::Y, "/user/hand/left/input/y/click" },
            { xrc::Button::Home, "/user/hand/right/input/system/click" },
            { xrc::Button::Menu, "/user/hand/left/input/menu/click" },
            { xrc::Button::L3, "/user/hand/left/input/thumbstick/click" },
            { xrc::Button::R3, "/user/hand/right/input/thumbstick/click" },
            { xrc::Button::TA, "/user/hand/right/input/a/touch" },
            { xrc::Button::TB, "/user/hand/right/input/b/touch" },
            { xrc::Button::TX, "/user/hand/left/input/x/touch" },
            { xrc::Button::TY, "/user/hand/left/input/y/touch" },
            { xrc::Button::TLStick, "/user/hand/left/input/thumbstick/touch" },
            { xrc::Button::TRStick, "/user/hand/right/input/thumbstick/touch" },
            { xrc::Button::TLRest, "/user/hand/left/input/thumbrest/touch" },
            { xrc::Button::TRRest, "/user/hand/right/input/thumbrest/touch" },
            { xrc::Button::TLTrig, "/user/hand/left/input/trigger/touch" },
            { xrc::Button::TRTrig, "/user/hand/right/input/trigger/touch" },
            { xrc::Trigger::LTrigger, "/user/hand/left/input/trigger/value" },
            { xrc::Trigger::RTrigger, "/user/hand/right/input/trigger/value" },
            { xrc::Trigger::LGrip, "/user/hand/left/input/squeeze/value" },
            { xrc::Trigger::RGrip, "/user/hand/right/input/squeeze/value" },
            { xrc::ThumbStickAxis1D::LX, "/user/hand/left/input/thumbstick/x" },
            { xrc::ThumbStickAxis1D::LY, "/user/hand/left/input/thumbstick/y" },
            { xrc::ThumbStickAxis1D::RX, "/user/hand/right/input/thumbstick/x" },
            { xrc::ThumbStickAxis1D::RY, "/user/hand/right/input/thumbstick/y" },
            { xrc::ControllerPosePosition::LPos, "/user/hand/left/input/grip/pose" },
            { xrc::ControllerPosePosition::RPos, "/user/hand/right/input/grip/pose" },
            { xrc::ControllerPoseOrientation::LOrient, "/user/hand/left/input/aim/pose" },
            { xrc::ControllerPoseOrientation::ROrient, "/user/hand/right/input/aim/pose" },
        };
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceOculusTouch::~InputDeviceOculusTouch() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string InputDeviceOculusTouch::GetInputChannelPath(const AzFramework::InputChannelId& channelId) const
    {
        if (const auto it = m_xrPathMap.find(channelId);
            it != m_xrPathMap.end())
        {
            return{ it->second };
        }
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string InputDeviceOculusTouch::GetInputDeviceProfilePath() const
    {
        return { "/interaction_profiles/oculus/touch_controller" };
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string InputDeviceOculusTouch::GetLeftHandSubPath() const
    {
        return { "/user/hand/left" };
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string InputDeviceOculusTouch::GetRightHandSubPath() const
    {
        return { "/user/hand/right" };
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceOculusTouch::RegisterTickCallback(TickCallbackFn callbackFn)
    {
        m_tickCallback = callbackFn;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceOculusTouch::RawXRControllerState& InputDeviceOculusTouch::GetRawState()
    {
        return m_rawControllerState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceOculusTouch::IsConnected() const
    {
        return m_isConnected;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceOculusTouch::SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized)
    {
        if (m_isConnected)
        {
            // Set vibration values on the raw data structure, they will be consumed on the next tick
            m_rawControllerState.m_leftMotorVibrationValue = leftMotorSpeedNormalized;
            m_rawControllerState.m_rightMotorVibrationValue = rightMotorSpeedNormalized;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceOculusTouch::TickInputDevice()
    {
        // DEPRECATED. This whole file will be removed soon.
    }

} // namespace OpenXRVk
