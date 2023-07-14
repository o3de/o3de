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
    //! This system will take care of reflecting physics material classes and
    //! registering the physics material asset.
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
        Application::ImplementationFactory* CreateApplicationImplementationFactory() const;
        InputDeviceGamepad::ImplementationFactory* GetDeviceGamepadImplentationFactory() const;
        InputDeviceKeyboard::ImplementationFactory* GetDeviceKeyboardImplementationFactory() const;
        InputDeviceMotion::ImplementationFactory* GetDeviceMotionImplentationFactory() const; 
        InputDeviceMouse::ImplementationFactory* GetDeviceMouseImplentationFactory() const;
        InputDeviceTouch::ImplementationFactory* GetDeviceTouchImplentationFactory() const;
        InputDeviceVirtualKeyboard::ImplementationFactory* GetDeviceVirtualKeyboardImplentationFactory() const;
        NativeWindow::ImplementationFactory* GetNativeWindowImplementationFactory() const;

        Application::ImplementationFactory* m_applicationImplFactory = nullptr;
        InputDeviceGamepad::ImplementationFactory* m_deviceGamepadImplFactory = nullptr;
        InputDeviceKeyboard::ImplementationFactory* m_deviceKeyboardImplFactory = nullptr;
        InputDeviceMotion::ImplementationFactory* m_deviceMotionImplFactory = nullptr;
        InputDeviceMouse::ImplementationFactory* m_deviceMouseImplFactory = nullptr;
        InputDeviceTouch::ImplementationFactory* m_deviceTouchImplFactory = nullptr;
        InputDeviceVirtualKeyboard::ImplementationFactory* m_deviceVirtualKeyboardImplFactory = nullptr;
        NativeWindow::ImplementationFactory* m_nativeWindowImplFactory = nullptr;
    };
} // namespace AzFramework
