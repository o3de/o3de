/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/NativeUISystemComponent.h>

#include <AzFramework/Application/Application_Windows.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse_Windows.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_Windows.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Windows.h>
#include <AzFramework/Windowing/NativeWindow_Windows.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class WindowsApplicationImplFactory 
        : public Application::ImplementationFactory
    {
    public:
        Application::Implementation* Create() override
        {
            return aznew ApplicationWindows();
        }
    };

    struct LinuxDeviceKeyboardImplFactory
        : public InputDeviceKeyboard::ImplementationFactory
    {
        InputDeviceKeyboard::Implementation* Create(InputDeviceKeyboard& inputDevice) override
        {
            return aznew InputDeviceKeyboardWindows(inputDevice);
        }
    };

    class WindowsDeviceMouseImplFactory
        : public InputDeviceMouse::ImplementationFactory
    {
    public:
        InputDeviceMouse::Implementation* Create(InputDeviceMouse& inputDevice) override
        {
            return aznew InputDeviceMouseWindows(inputDevice);
        }
    };

    class WindowsDeviceGamepadImplFactory
        : public InputDeviceGamepad::ImplementationFactory
    {
    public:
        InputDeviceGamepad::Implementation* Create(InputDeviceGamepad& inputDevice) override
        {
            // Before creating any instances of InputDeviceGamepadWindows, ensure that XInput is loaded
            AZStd::shared_ptr<AZ::DynamicModuleHandle> xinputModuleHandle = XInput::LoadDynamicModule();
            if (!xinputModuleHandle)
            {
                // Could not load XInput9_1_0, this is most likely a Windows Server 2012 machine,
                // in which case we don't care about game-pad support
                return nullptr;
            }
            AZ_Assert(
                inputDevice.GetInputDeviceId().GetIndex() < GetMaxSupportedGamepads(),
                "Creating InputDeviceGamepadWindows with index %d that is greater than the max supported by xinput: %d",
                inputDevice.GetInputDeviceId().GetIndex(),
                GetMaxSupportedGamepads());

            return aznew InputDeviceGamepadWindows(inputDevice, xinputModuleHandle);
        }
        AZ::u32 GetMaxSupportedGamepads() override
        {
            return XUSER_MAX_COUNT;
        }
    };

    struct WindowsNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
        NativeWindow::Implementation* Create() override
        {
            return aznew NativeWindowImpl_Win32();
        }
    };

    Application::ImplementationFactory* NativeUISystemComponent::CreateApplicationImplementationFactory() const
    {
        return aznew WindowsApplicationImplFactory();
    }

    InputDeviceKeyboard::ImplementationFactory* NativeUISystemComponent::GetDeviceKeyboardImplementationFactory() const
    {
        return aznew LinuxDeviceKeyboardImplFactory();
    }

    InputDeviceMouse::ImplementationFactory* NativeUISystemComponent::GetDeviceMouseImplentationFactory() const
    {
        return aznew WindowsDeviceMouseImplFactory();
    }

    InputDeviceGamepad::ImplementationFactory* NativeUISystemComponent::GetDeviceGamepadImplentationFactory() const
    {
        return aznew WindowsDeviceGamepadImplFactory();
    }

    InputDeviceMotion::ImplementationFactory* NativeUISystemComponent::GetDeviceMotionImplentationFactory() const
    {
        return nullptr;
    }

    InputDeviceTouch::ImplementationFactory* NativeUISystemComponent::GetDeviceTouchImplentationFactory() const
    {
        return nullptr;
    }
    InputDeviceVirtualKeyboard::ImplementationFactory* NativeUISystemComponent::GetDeviceVirtualKeyboardImplentationFactory() const
    {
        return nullptr;
    }
    NativeWindow::ImplementationFactory* NativeUISystemComponent::GetNativeWindowImplementationFactory() const
    {
        return aznew WindowsNativeWindowFactory();
    }



} // namespace AzFramework
