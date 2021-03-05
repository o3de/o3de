/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
