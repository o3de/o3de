/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Components/NativeUISystemComponentFactories_Mac.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    void NativeUISystemComponent::InitializeApplicationImplementationFactory()
    {
        m_applicationImplFactory = AZStd::make_unique<MacApplicationImplFactory>();
        AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceGamepadImplentationFactory()
    {
        m_deviceGamepadImplFactory = AZStd::make_unique<MacDeviceGamepadImplFactory>();
        AZ::Interface<InputDeviceGamepad::ImplementationFactory>::Register(m_deviceGamepadImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceKeyboardImplementationFactory()
    {
        m_deviceKeyboardImplFactory = AZStd::make_unique<MacDeviceKeyboardImplFactory>();
        AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Register(m_deviceKeyboardImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceMotionImplentationFactory()
    {
        // Motion Input not supported on Mac
    }

    void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    {
        m_deviceMouseImplFactory = AZStd::make_unique<MacDeviceMouseImplFactory>();
        AZ::Interface<InputDeviceMouse::ImplementationFactory>::Register(m_deviceMouseImplFactory.get());
    }

    void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    {
        // Touch Input not supported on Mac
    }

    void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    {
        // Virtual Keyboard not supported on Mac
    }

    void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    {
        m_nativeWindowImplFactory = AZStd::make_unique<MacNativeWindowFactory>();
        AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory.get());
    }
} // namespace AzFramework
