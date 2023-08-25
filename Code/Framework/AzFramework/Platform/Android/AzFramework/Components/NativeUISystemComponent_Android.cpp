/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Components/NativeUISystemComponentFactories_Android.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    {
        m_applicationImplFactory = AZStd::make_unique<AndroidApplicationImplFactory>();
        AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceGamepadImplentationFactory()
    {
        // Gamepad input not supported on Android
    }

    void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    {
        m_deviceKeyboardImplFactory = AZStd::make_unique<AndroidDeviceKeyboardImplFactory>();
        AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Register(m_deviceKeyboardImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    {
        m_deviceMotionImplFactory = AZStd::make_unique<AndroidDeviceMotionImplFactory>();
        AZ::Interface<InputDeviceMotion::ImplementationFactory>::Register(m_deviceMotionImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    {
        m_deviceMouseImplFactory = AZStd::make_unique<AndroidDeviceMouseImplFactory>();
        AZ::Interface<InputDeviceMouse::ImplementationFactory>::Register(m_deviceMouseImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    {
        m_deviceTouchImplFactory = AZStd::make_unique<AndroidDeviceTouchImplFactory>();
        AZ::Interface<InputDeviceTouch::ImplementationFactory>::Register(m_deviceTouchImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    {
        m_deviceVirtualKeyboardImplFactory = AZStd::make_unique<AndroidDeviceVirtualKeyboardImplFactory>();
        AZ::Interface<InputDeviceVirtualKeyboard::ImplementationFactory>::Register(m_deviceVirtualKeyboardImplFactory.get());
    }

    void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    {
        m_nativeWindowImplFactory = AZStd::make_unique<AndroidNativeWindowFactory>();
        AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory.get());
    }
} // namespace AzFramework
