/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool IsAnyKeyOrButton(const InputChannel& inputChannel)
    {
        const InputChannelId& channelId = inputChannel.GetInputChannelId();
        const InputDeviceId& deviceId = inputChannel.GetInputDevice().GetInputDeviceId();
        if (InputDeviceGamepad::IsGamepadDevice(deviceId))
        {
            const auto& gamepadButtons = InputDeviceGamepad::Button::All;
            const auto& gamepadTriggers = InputDeviceGamepad::Trigger::All;
            return AZStd::find(gamepadButtons.cbegin(), gamepadButtons.cend(), channelId) != gamepadButtons.cend() ||
                   AZStd::find(gamepadTriggers.cbegin(), gamepadTriggers.cend(), channelId) != gamepadTriggers.cend();
        }

        if (InputDeviceKeyboard::IsKeyboardDevice(deviceId))
        {
            const auto& keyboardKeys = InputDeviceKeyboard::Key::All;
            return AZStd::find(keyboardKeys.cbegin(), keyboardKeys.cend(), channelId) != keyboardKeys.cend();
        }

        if (InputDeviceMouse::IsMouseDevice(deviceId))
        {
            const auto& mouseButtons = InputDeviceMouse::Button::All;
            return AZStd::find(mouseButtons.cbegin(), mouseButtons.cend(), channelId) != mouseButtons.cend();
        }

        if (InputDeviceTouch::IsTouchDevice(deviceId))
        {
            const auto& touches = InputDeviceTouch::Touch::All;
            return AZStd::find(touches.cbegin(), touches.cend(), channelId) != touches.cend();
        }

        return false;
    }
} // namespace AzFramework
