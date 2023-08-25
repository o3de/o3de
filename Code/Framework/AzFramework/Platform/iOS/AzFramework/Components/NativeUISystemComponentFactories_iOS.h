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
#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>
#include <AzFramework/Windowing/NativeWindow.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class IosApplicationImplFactory 
        : public Application::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<Application::Implementation> Create() override;
    };

    class IosDeviceGamepadImplFactory
        : public InputDeviceGamepad::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceGamepad::Implementation> Create(InputDeviceGamepad& inputDevice) override;
        AZ::u32 GetMaxSupportedGamepads() const override;
    };

    class IosDeviceMotionImplFactory
        : public InputDeviceMotion::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMotion::Implementation> Create(InputDeviceMotion& inputDevice) override;
    };

    class IosDeviceTouchImplFactory
        : public InputDeviceTouch::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceTouch::Implementation> Create(InputDeviceTouch& inputDevice) override;
    };

    class IosDeviceVirtualKeyboardImplFactory
        : public InputDeviceVirtualKeyboard::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceVirtualKeyboard::Implementation> Create(InputDeviceVirtualKeyboard& inputDevice) override;
    };

    class IosNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<NativeWindow::Implementation> Create() override;
    };

} // namespace AzFramework
