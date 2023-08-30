/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>
#include <AzFramework/Windowing/NativeWindow.h>

namespace AzFramework
{
    //! This component manages the lifecycle of the UI-related factories and interfaces and initializes the ones that are 
    //! supported by the current platform.
    class NativeUISystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AzFramework::NativeUISystemComponent, "{F2BA607F-08A9-42DD-9C0D-F324BFDD4DFD}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        NativeUISystemComponent();
        ~NativeUISystemComponent();

        NativeUISystemComponent(const NativeUISystemComponent&) = delete;
        NativeUISystemComponent& operator=(const NativeUISystemComponent&) = delete;

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;

    private:
        void InitializeApplicationImplementationFactory();
        void InitializeDeviceGamepadImplentationFactory();
        void InitializeDeviceKeyboardImplementationFactory();
        void InitializeDeviceMotionImplentationFactory(); 
        void InitializeDeviceMouseImplentationFactory();
        void InitializeDeviceTouchImplentationFactory();
        void InitializeDeviceVirtualKeyboardImplentationFactory();
        void InitializeNativeWindowImplementationFactory();

        AZStd::unique_ptr<Application::ImplementationFactory> m_applicationImplFactory;
        AZStd::unique_ptr<InputDeviceGamepad::ImplementationFactory> m_deviceGamepadImplFactory;
        AZStd::unique_ptr<InputDeviceKeyboard::ImplementationFactory> m_deviceKeyboardImplFactory;
        AZStd::unique_ptr<InputDeviceMotion::ImplementationFactory> m_deviceMotionImplFactory;
        AZStd::unique_ptr<InputDeviceMouse::ImplementationFactory> m_deviceMouseImplFactory;
        AZStd::unique_ptr<InputDeviceTouch::ImplementationFactory> m_deviceTouchImplFactory;
        AZStd::unique_ptr<InputDeviceVirtualKeyboard::ImplementationFactory> m_deviceVirtualKeyboardImplFactory;
        AZStd::unique_ptr<NativeWindow::ImplementationFactory> m_nativeWindowImplFactory;
    };
} // namespace AzFramework
