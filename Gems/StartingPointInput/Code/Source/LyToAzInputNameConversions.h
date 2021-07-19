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
#include <unordered_map>

namespace Input
{
    using namespace AzFramework;

    //////////////////////////////////////////////////////////////////////////
    inline AZStd::string ConvertInputDeviceName(AZStd::string inputDeviceName)
    {
        // Using std::unordered_map instead of AZStd to avoid allocator issues
        static const std::unordered_map<std::string, std::string> map =
        {
            { "mouse", InputDeviceMouse::Id.GetName() },
            { "keyboard", InputDeviceKeyboard::Id.GetName() },
            { "gamepad", InputDeviceGamepad::Name },
            { "game console controller", InputDeviceGamepad::Name },
            { "other game console controller", InputDeviceGamepad::Name },
            { "Oculus Touch Controller", "oculus_controllers" },
            { "OpenVR Controller", "openvr_controllers" }
        };

        const auto& it = map.find(inputDeviceName.c_str());
        return it != map.end() ? it->second.c_str() : inputDeviceName.c_str();
    }

    //////////////////////////////////////////////////////////////////////////
    inline AZStd::string ConvertInputEventName(AZStd::string inputEventName)
    {
        // Using std::unordered_map instead of AZStd to avoid allocator issues
        static const std::unordered_map<std::string, std::string> map =
        {
            { "mouse1", InputDeviceMouse::Button::Left.GetName() },
            { "mouse2", InputDeviceMouse::Button::Right.GetName() },
            { "mouse3", InputDeviceMouse::Button::Middle.GetName() },
            { "mouse4", InputDeviceMouse::Button::Other1.GetName() },
            { "mouse5", InputDeviceMouse::Button::Other2.GetName() },
            { "maxis_x", InputDeviceMouse::Movement::X.GetName() },
            { "maxis_y", InputDeviceMouse::Movement::Y.GetName() },
            { "maxis_z", InputDeviceMouse::Movement::Z.GetName() },
            { "mwheel_up", InputDeviceMouse::Movement::Z.GetName() },
            { "mwheel_down", InputDeviceMouse::Movement::Z.GetName() },
            { "mouse_pos", InputDeviceMouse::SystemCursorPosition.GetName() },

            { "escape", InputDeviceKeyboard::Key::Escape.GetName() },
            { "1", InputDeviceKeyboard::Key::Alphanumeric1.GetName() },
            { "2", InputDeviceKeyboard::Key::Alphanumeric2.GetName() },
            { "3", InputDeviceKeyboard::Key::Alphanumeric3.GetName() },
            { "4", InputDeviceKeyboard::Key::Alphanumeric4.GetName() },
            { "5", InputDeviceKeyboard::Key::Alphanumeric5.GetName() },
            { "6", InputDeviceKeyboard::Key::Alphanumeric6.GetName() },
            { "7", InputDeviceKeyboard::Key::Alphanumeric7.GetName() },
            { "8", InputDeviceKeyboard::Key::Alphanumeric8.GetName() },
            { "9", InputDeviceKeyboard::Key::Alphanumeric9.GetName() },
            { "0", InputDeviceKeyboard::Key::Alphanumeric0.GetName() },
            { "minus", InputDeviceKeyboard::Key::PunctuationHyphen.GetName() },
            { "equals", InputDeviceKeyboard::Key::PunctuationEquals.GetName() },
            { "backspace", InputDeviceKeyboard::Key::EditBackspace.GetName() },
            { "tab", InputDeviceKeyboard::Key::EditTab.GetName() },
            { "q", InputDeviceKeyboard::Key::AlphanumericQ.GetName() },
            { "w", InputDeviceKeyboard::Key::AlphanumericW.GetName() },
            { "e", InputDeviceKeyboard::Key::AlphanumericE.GetName() },
            { "r", InputDeviceKeyboard::Key::AlphanumericR.GetName() },
            { "t", InputDeviceKeyboard::Key::AlphanumericT.GetName() },
            { "y", InputDeviceKeyboard::Key::AlphanumericY.GetName() },
            { "u", InputDeviceKeyboard::Key::AlphanumericU.GetName() },
            { "i", InputDeviceKeyboard::Key::AlphanumericI.GetName() },
            { "o", InputDeviceKeyboard::Key::AlphanumericO.GetName() },
            { "p", InputDeviceKeyboard::Key::AlphanumericP.GetName() },
            { "lbracket", InputDeviceKeyboard::Key::PunctuationBracketL.GetName() },
            { "rbracket", InputDeviceKeyboard::Key::PunctuationBracketR.GetName() },
            { "enter", InputDeviceKeyboard::Key::EditEnter.GetName() },
            { "lctrl", InputDeviceKeyboard::Key::ModifierCtrlL.GetName() },
            { "a", InputDeviceKeyboard::Key::AlphanumericA.GetName() },
            { "s", InputDeviceKeyboard::Key::AlphanumericS.GetName() },
            { "d", InputDeviceKeyboard::Key::AlphanumericD.GetName() },
            { "f", InputDeviceKeyboard::Key::AlphanumericF.GetName() },
            { "g", InputDeviceKeyboard::Key::AlphanumericG.GetName() },
            { "h", InputDeviceKeyboard::Key::AlphanumericH.GetName() },
            { "j", InputDeviceKeyboard::Key::AlphanumericJ.GetName() },
            { "k", InputDeviceKeyboard::Key::AlphanumericK.GetName() },
            { "l", InputDeviceKeyboard::Key::AlphanumericL.GetName() },
            { "semicolon", InputDeviceKeyboard::Key::PunctuationSemicolon.GetName() },
            { "apostrophe", InputDeviceKeyboard::Key::PunctuationApostrophe.GetName() },
            { "tilde", InputDeviceKeyboard::Key::PunctuationTilde.GetName() },
            { "lshift", InputDeviceKeyboard::Key::ModifierShiftL.GetName() },
            { "backslash", InputDeviceKeyboard::Key::PunctuationBackslash.GetName() },
            { "z", InputDeviceKeyboard::Key::AlphanumericZ.GetName() },
            { "x", InputDeviceKeyboard::Key::AlphanumericX.GetName() },
            { "c", InputDeviceKeyboard::Key::AlphanumericC.GetName() },
            { "v", InputDeviceKeyboard::Key::AlphanumericV.GetName() },
            { "b", InputDeviceKeyboard::Key::AlphanumericB.GetName() },
            { "n", InputDeviceKeyboard::Key::AlphanumericN.GetName() },
            { "m", InputDeviceKeyboard::Key::AlphanumericM.GetName() },
            { "comma", InputDeviceKeyboard::Key::PunctuationComma.GetName() },
            { "period", InputDeviceKeyboard::Key::PunctuationPeriod.GetName() },
            { "slash", InputDeviceKeyboard::Key::PunctuationSlash.GetName() },
            { "rshift", InputDeviceKeyboard::Key::ModifierShiftR.GetName() },
            { "np_multiply", InputDeviceKeyboard::Key::NumPadMultiply.GetName() },
            { "lalt", InputDeviceKeyboard::Key::ModifierAltL.GetName() },
            { "space", InputDeviceKeyboard::Key::EditSpace.GetName() },
            { "capslock", InputDeviceKeyboard::Key::EditCapsLock.GetName() },
            { "f1", InputDeviceKeyboard::Key::Function01.GetName() },
            { "f2", InputDeviceKeyboard::Key::Function02.GetName() },
            { "f3", InputDeviceKeyboard::Key::Function03.GetName() },
            { "f4", InputDeviceKeyboard::Key::Function04.GetName() },
            { "f5", InputDeviceKeyboard::Key::Function05.GetName() },
            { "f6", InputDeviceKeyboard::Key::Function06.GetName() },
            { "f7", InputDeviceKeyboard::Key::Function07.GetName() },
            { "f8", InputDeviceKeyboard::Key::Function08.GetName() },
            { "f9", InputDeviceKeyboard::Key::Function09.GetName() },
            { "f10", InputDeviceKeyboard::Key::Function10.GetName() },
            { "numlock", InputDeviceKeyboard::Key::NumLock.GetName() },
            { "scrolllock", InputDeviceKeyboard::Key::WindowsSystemScrollLock.GetName() },
            { "np_7", InputDeviceKeyboard::Key::NumPad7.GetName() },
            { "np_8", InputDeviceKeyboard::Key::NumPad8.GetName() },
            { "np_9", InputDeviceKeyboard::Key::NumPad9.GetName() },
            { "np_subtract", InputDeviceKeyboard::Key::NumPadSubtract.GetName() },
            { "np_4", InputDeviceKeyboard::Key::NumPad4.GetName() },
            { "np_5", InputDeviceKeyboard::Key::NumPad5.GetName() },
            { "np_6", InputDeviceKeyboard::Key::NumPad6.GetName() },
            { "np_add", InputDeviceKeyboard::Key::NumPadAdd.GetName() },
            { "np_1", InputDeviceKeyboard::Key::NumPad1.GetName() },
            { "np_2", InputDeviceKeyboard::Key::NumPad2.GetName() },
            { "np_3", InputDeviceKeyboard::Key::NumPad3.GetName() },
            { "np_0", InputDeviceKeyboard::Key::NumPad0.GetName() },
            { "np_period", InputDeviceKeyboard::Key::NumPadDecimal.GetName() },
            { "f11", InputDeviceKeyboard::Key::Function11.GetName() },
            { "f12", InputDeviceKeyboard::Key::Function12.GetName() },
            { "f13", InputDeviceKeyboard::Key::Function13.GetName() },
            { "f14", InputDeviceKeyboard::Key::Function14.GetName() },
            { "f15", InputDeviceKeyboard::Key::Function15.GetName() },
            { "np_enter", InputDeviceKeyboard::Key::NumPadEnter.GetName() },
            { "rctrl", InputDeviceKeyboard::Key::ModifierCtrlR.GetName() },
            { "np_divide", InputDeviceKeyboard::Key::NumPadDivide.GetName() },
            { "print", InputDeviceKeyboard::Key::WindowsSystemPrint.GetName() },
            { "ralt", InputDeviceKeyboard::Key::ModifierAltR.GetName() },
            { "pause", InputDeviceKeyboard::Key::WindowsSystemPause.GetName() },
            { "home", InputDeviceKeyboard::Key::NavigationHome.GetName() },
            { "up", InputDeviceKeyboard::Key::NavigationArrowUp.GetName() },
            { "pgup", InputDeviceKeyboard::Key::NavigationPageUp.GetName() },
            { "left", InputDeviceKeyboard::Key::NavigationArrowLeft.GetName() },
            { "right", InputDeviceKeyboard::Key::NavigationArrowRight.GetName() },
            { "end", InputDeviceKeyboard::Key::NavigationEnd.GetName() },
            { "down", InputDeviceKeyboard::Key::NavigationArrowDown.GetName() },
            { "pgdn", InputDeviceKeyboard::Key::NavigationPageDown.GetName() },
            { "insert", InputDeviceKeyboard::Key::NavigationInsert.GetName() },
            { "delete", InputDeviceKeyboard::Key::NavigationDelete.GetName() },
            { "oem_102", InputDeviceKeyboard::Key::SupplementaryISO.GetName() },

            { "gamepad_a", InputDeviceGamepad::Button::A.GetName() },
            { "gamepad_b", InputDeviceGamepad::Button::B.GetName() },
            { "gamepad_x", InputDeviceGamepad::Button::X.GetName() },
            { "gamepad_y", InputDeviceGamepad::Button::Y.GetName() },
            { "gamepad_l1", InputDeviceGamepad::Button::L1.GetName() },
            { "gamepad_r1", InputDeviceGamepad::Button::R1.GetName() },
            { "gamepad_l2", InputDeviceGamepad::Trigger::L2.GetName() },
            { "gamepad_r2", InputDeviceGamepad::Trigger::R2.GetName() },
            { "gamepad_l3", InputDeviceGamepad::Button::L3.GetName() },
            { "gamepad_r3", InputDeviceGamepad::Button::R3.GetName() },
            { "gamepad_up", InputDeviceGamepad::Button::DU.GetName() },
            { "gamepad_down", InputDeviceGamepad::Button::DD.GetName() },
            { "gamepad_left", InputDeviceGamepad::Button::DL.GetName() },
            { "gamepad_right", InputDeviceGamepad::Button::DR.GetName() },
            { "gamepad_start", InputDeviceGamepad::Button::Start.GetName() },
            { "gamepad_select", InputDeviceGamepad::Button::Select.GetName() },
            { "gamepad_sticklx", InputDeviceGamepad::ThumbStickAxis1D::LX.GetName() },
            { "gamepad_stickly", InputDeviceGamepad::ThumbStickAxis1D::LY.GetName() },
            { "gamepad_stickrx", InputDeviceGamepad::ThumbStickAxis1D::RX.GetName() },
            { "gamepad_stickry", InputDeviceGamepad::ThumbStickAxis1D::RY.GetName() },

            // Additional platform device configurations may be added here

            { "OculusTouch_A", "oculus_button_a" },
            { "OculusTouch_B", "oculus_button_b" },
            { "OculusTouch_X", "oculus_button_x" },
            { "OculusTouch_Y", "oculus_button_y" },
            { "OculusTouch_LeftThumbstickButton", "oculus_button_l3" },
            { "OculusTouch_RightThumbstickButton", "oculus_button_r3" },
            { "OculusTouch_LeftTrigger", "oculus_trigger_l1" },
            { "OculusTouch_RightTrigger", "oculus_trigger_r1" },
            { "OculusTouch_LeftHandTrigger", "oculus_trigger_l2" },
            { "OculusTouch_RightHandTrigger", "oculus_trigger_r2" },
            { "OculusTouch_LeftThumbstickX", "oculus_thumbstick_l_x" },
            { "OculusTouch_LeftThumbstickY", "oculus_thumbstick_l_y" },
            { "OculusTouch_RightThumbstickX", "oculus_thumbstick_r_x" },
            { "OculusTouch_RightThumbstickY", "oculus_thumbstick_r_y" },

            { "OpenVR_A_0", "openvr_button_a_l" },
            { "OpenVR_A_1", "openvr_button_a_r" },
            { "OpenVR_DPadUp_0", "openvr_button_d_up_l" },
            { "OpenVR_DPadDown_0", "openvr_button_d_down_l" },
            { "OpenVR_DPadLeft_0", "openvr_button_d_left_l" },
            { "OpenVR_DPadRight_0", "openvr_button_d_right_l" },
            { "OpenVR_DPadUp_1", "openvr_button_d_up_r" },
            { "OpenVR_DPadDown_1", "openvr_button_d_down_r" },
            { "OpenVR_DPadLeft_1", "openvr_button_d_left_r" },
            { "OpenVR_DPadRight_1", "openvr_button_d_right_r" },
            { "OpenVR_Grip_0", "openvr_button_grip_l" },
            { "OpenVR_Grip_1", "openvr_button_grip_r" },
            { "OpenVR_Application_0", "openvr_button_start_l" },
            { "OpenVR_Application_1", "openvr_button_start_r" },
            { "OpenVR_System_0", "openvr_button_select_l" },
            { "OpenVR_System_1", "openvr_button_select_r" },
            { "OpenVR_TriggerButton_0", "openvr_button_trigger_l" },
            { "OpenVR_TriggerButton_1", "openvr_button_trigger_r" },
            { "OpenVR_TouchpadButton_0", "openvr_button_touchpad_l" },
            { "OpenVR_TouchpadButton_1", "openvr_button_touchpad_r" },
            { "OpenVR_Trigger_0", "openvr_trigger_l1" },
            { "OpenVR_Trigger_1", "openvr_trigger_r1" },
            { "OpenVR_TouchpadX_0", "openvr_touchpad_l_x" },
            { "OpenVR_TouchpadY_0", "openvr_touchpad_l_y" },
            { "OpenVR_TouchpadX_1", "openvr_touchpad_r_x" },
            { "OpenVR_TouchpadY_1", "openvr_touchpad_r_y" }
        };

        const auto& it = map.find(inputEventName.c_str());
        return it != map.end() ? it->second.c_str() : inputEventName.c_str();
    }
} // namespace Input
