/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Application/Application.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>
#include <AzFramework/Windowing/NativeWindow.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class AndroidApplicationImplFactory 
        : public Application::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<Application::Implementation> Create() override;
    };

    class AndroidDeviceKeyboardImplFactory
        : public InputDeviceKeyboard::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceKeyboard::Implementation> Create(InputDeviceKeyboard& inputDevice) override;
    };

    class AndroidDeviceMotionImplFactory
        : public InputDeviceMotion::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMotion::Implementation> Create(InputDeviceMotion& inputDevice) override;
    };

    class AndroidDeviceMouseImplFactory
        : public InputDeviceMouse::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMouse::Implementation> Create(InputDeviceMouse& inputDevice) override;
    };

    class AndroidDeviceTouchImplFactory
        : public InputDeviceTouch::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceTouch::Implementation> Create(InputDeviceTouch& inputDevice) override;
    };

    class AndroidDeviceVirtualKeyboardImplFactory
        : public InputDeviceVirtualKeyboard::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceVirtualKeyboard::Implementation> Create(InputDeviceVirtualKeyboard& inputDevice) override;
    };

    class AndroidNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<NativeWindow::Implementation> Create() override;
    };

} // namespace AzFramework
