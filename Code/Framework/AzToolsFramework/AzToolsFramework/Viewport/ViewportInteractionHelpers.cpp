/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzToolsFramework/Viewport/ViewportInteractionHelpers.h>


namespace AzToolsFramework::ViewportInteraction
{
    MouseButton Helpers::GetMouseButton(const AzFramework::InputChannel& inputChannel)
    {
        using AzToolsFramework::ViewportInteraction::MouseButton;
        using InputButton = AzFramework::InputDeviceMouse::Button;
        const AzFramework::InputChannelId& id = inputChannel.GetInputChannelId();
        if (id == InputButton::Left)
        {
            return MouseButton::Left;
        }
        if (id == InputButton::Middle)
        {
            return MouseButton::Middle;
        }
        if (id == InputButton::Right)
        {
            return MouseButton::Right;
        }
        return MouseButton::None;
    }

    bool Helpers::IsMouseMove(const AzFramework::InputChannel& inputChannel)
    {
        return inputChannel.GetInputChannelId() == AzFramework::InputDeviceMouse::SystemCursorPosition;
    }

    KeyboardModifier Helpers::GetKeyboardModifier(const AzFramework::InputChannel& inputChannel)
    {
        using AzToolsFramework::ViewportInteraction::KeyboardModifier;
        using Key = AzFramework::InputDeviceKeyboard::Key;
        const auto& id = inputChannel.GetInputChannelId();
        if (id == Key::ModifierAltL || id == Key::ModifierAltR)
        {
            return KeyboardModifier::Alt;
        }
        if (id == Key::ModifierCtrlL || id == Key::ModifierCtrlR)
        {
            return KeyboardModifier::Ctrl;
        }
        if (id == Key::ModifierShiftL || id == Key::ModifierShiftR)
        {
            return KeyboardModifier::Shift;
        }
        return KeyboardModifier::None;
    }
} // namespace AzToolsFramework::ViewportInteraction
