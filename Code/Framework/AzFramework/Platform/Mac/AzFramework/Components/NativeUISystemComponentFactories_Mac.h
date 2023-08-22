/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Application/Application.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Windowing/NativeWindow.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class MacApplicationImplFactory 
        : public Application::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<Application::Implementation> Create() override;
    };

    class MacDeviceGamepadImplFactory
        : public InputDeviceGamepad::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceGamepad::Implementation> Create(InputDeviceGamepad& inputDevice) override;
        AZ::u32 GetMaxSupportedGamepads() override;
    };

    class MacDeviceKeyboardImplFactory
        : public InputDeviceKeyboard::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceKeyboard::Implementation> Create(InputDeviceKeyboard& inputDevice) override;
    };

    class MacDeviceMouseImplFactory
        : public InputDeviceMouse::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMouse::Implementation> Create(InputDeviceMouse& inputDevice) override;
    };

    class MacNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<NativeWindow::Implementation> Create() override;
    };

} // namespace AzFramework
