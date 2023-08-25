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
        AZStd::unique_ptr<Application::Implementation> Create() override
        {
            return AZStd::make_unique<ApplicationWindows>();
        }
    };

    struct WindowsDeviceKeyboardImplFactory
        : public InputDeviceKeyboard::ImplementationFactory
    {
        AZStd::unique_ptr<InputDeviceKeyboard::Implementation> Create(InputDeviceKeyboard& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceKeyboardWindows>(inputDevice);
        }
    };

    class WindowsDeviceMouseImplFactory
        : public InputDeviceMouse::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMouse::Implementation> Create(InputDeviceMouse& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceMouseWindows>(inputDevice);
        }
    };

    class WindowsDeviceGamepadImplFactory
        : public InputDeviceGamepad::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceGamepad::Implementation> Create(InputDeviceGamepad& inputDevice) override
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

            return AZStd::make_unique<InputDeviceGamepadWindows>(inputDevice, xinputModuleHandle);
        }
        AZ::u32 GetMaxSupportedGamepads() const override
        {
            return XUSER_MAX_COUNT;
        }
    };

    struct WindowsNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
        AZStd::unique_ptr<NativeWindow::Implementation> Create() override
        {
            return AZStd::make_unique<NativeWindowImpl_Win32>();
        }
    };

    void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    {
        m_applicationImplFactory = AZStd::make_unique<WindowsApplicationImplFactory>();
        AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceGamepadImplentationFactory()
    {
        m_deviceGamepadImplFactory = AZStd::make_unique<WindowsDeviceGamepadImplFactory>();
        AZ::Interface<InputDeviceGamepad::ImplementationFactory>::Register(m_deviceGamepadImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    {
        m_deviceKeyboardImplFactory = AZStd::make_unique<WindowsDeviceKeyboardImplFactory>();
        AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Register(m_deviceKeyboardImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    {
        // Motion Input not supported on Windows
    }

    void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    {
        m_deviceMouseImplFactory = AZStd::make_unique<WindowsDeviceMouseImplFactory>();
        AZ::Interface<InputDeviceMouse::ImplementationFactory>::Register(m_deviceMouseImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    {
        // Touch Input not supported on Windows
    }

    void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    {
        // Virtual Keyboard not supported on Windows
    }

    void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    {
        m_nativeWindowImplFactory = AZStd::make_unique<WindowsNativeWindowFactory>();
        AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory.get());
    }
} // namespace AzFramework
