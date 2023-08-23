/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Components/NativeUISystemComponentFactories_iOS.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    {
        m_applicationImplFactory = AZStd::make_unique<IosApplicationImplFactory>();
        AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceGamepadImplentationFactory()
    {
        m_deviceGamepadImplFactory = AZStd::make_unique<IosDeviceGamepadImplFactory>();
        AZ::Interface<InputDeviceGamepad::ImplementationFactory>::Register(m_deviceGamepadImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    {
        // Keyboard Input not supported on iOS
    }

    void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    {
        m_deviceMotionImplFactory = AZStd::make_unique<IosDeviceMotionImplFactory>();
        AZ::Interface<InputDeviceMotion::ImplementationFactory>::Register(m_deviceMotionImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    {
        // Mouse Input not supported on iOS
    }

    void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    {
        m_deviceTouchImplFactory = AZStd::make_unique<IosDeviceTouchImplFactory>();
        AZ::Interface<InputDeviceTouch::ImplementationFactory>::Register(m_deviceTouchImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    {
        m_deviceVirtualKeyboardImplFactory = AZStd::make_unique<IosDeviceVirtualKeyboardImplFactory>();
        AZ::Interface<InputDeviceVirtualKeyboard::ImplementationFactory>::Register(m_deviceVirtualKeyboardImplFactory.get());
    }

    void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    {
        m_nativeWindowImplFactory = AZStd::make_unique<IosNativeWindowFactory>();
        AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory.get());
    }
} // namespace AzFramework
