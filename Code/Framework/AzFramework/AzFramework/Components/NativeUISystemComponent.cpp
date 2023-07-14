/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    NativeUISystemComponent::NativeUISystemComponent()
    {
        if (m_applicationImplFactory = CreateApplicationImplementationFactory())
        {
            AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory);
        }

        if (m_deviceGamepadImplFactory = GetDeviceGamepadImplentationFactory())
        {
            AZ::Interface<InputDeviceGamepad::ImplementationFactory>::Register(m_deviceGamepadImplFactory);
        }
        
        if (m_deviceKeyboardImplFactory = GetDeviceKeyboardImplementationFactory())
        {
            AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Register(m_deviceKeyboardImplFactory);
        }

        if (m_deviceMotionImplFactory = GetDeviceMotionImplentationFactory())
        {
            AZ::Interface<InputDeviceMotion::ImplementationFactory>::Register(m_deviceMotionImplFactory);
        }

        if (m_deviceMouseImplFactory = GetDeviceMouseImplentationFactory())
        {
            AZ::Interface<InputDeviceMouse::ImplementationFactory>::Register(m_deviceMouseImplFactory);
        }

        if (m_deviceTouchImplFactory = GetDeviceTouchImplentationFactory())
        {
            AZ::Interface<InputDeviceTouch::ImplementationFactory>::Register(m_deviceTouchImplFactory);
        }

        if (m_deviceVirtualKeyboardImplFactory = GetDeviceVirtualKeyboardImplentationFactory())
        {
            AZ::Interface<InputDeviceVirtualKeyboard::ImplementationFactory>::Register(m_deviceVirtualKeyboardImplFactory);
        }

        if (m_nativeWindowImplFactory = GetNativeWindowImplementationFactory())
        {
            AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory);
        }
    }

    NativeUISystemComponent::~NativeUISystemComponent()
    {
        if (m_applicationImplFactory != nullptr)
        {
            AZ::Interface<Application::ImplementationFactory>::Unregister(m_applicationImplFactory);
            delete m_applicationImplFactory;
        }

        if (m_deviceGamepadImplFactory != nullptr)
        {
            AZ::Interface<InputDeviceGamepad::ImplementationFactory>::Unregister(m_deviceGamepadImplFactory);
            delete m_deviceGamepadImplFactory;
        }
        
        if (m_deviceKeyboardImplFactory != nullptr)
        {
            AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Unregister(m_deviceKeyboardImplFactory);
            delete m_deviceKeyboardImplFactory;
        }

        if (m_deviceMotionImplFactory != nullptr)
        {
            AZ::Interface<InputDeviceMotion::ImplementationFactory>::Unregister(m_deviceMotionImplFactory);
            delete m_deviceMotionImplFactory;
        }

        if (m_deviceMouseImplFactory != nullptr)
        {
            AZ::Interface<InputDeviceMouse::ImplementationFactory>::Unregister(m_deviceMouseImplFactory);
            delete m_deviceMouseImplFactory;
        }

        if (m_deviceTouchImplFactory != nullptr)
        {
            AZ::Interface<InputDeviceTouch::ImplementationFactory>::Unregister(m_deviceTouchImplFactory);
            delete m_deviceTouchImplFactory;
        }

        if (m_deviceVirtualKeyboardImplFactory != nullptr)
        {
            AZ::Interface<InputDeviceVirtualKeyboard::ImplementationFactory>::Unregister(m_deviceVirtualKeyboardImplFactory);
            delete m_deviceVirtualKeyboardImplFactory;
        }

        if (m_nativeWindowImplFactory != nullptr)
        {
            AZ::Interface<NativeWindow::ImplementationFactory>::Unregister(m_nativeWindowImplFactory);
            delete m_deviceVirtualKeyboardImplFactory;
        }
    }

    void NativeUISystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AzFramework::NativeUISystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void NativeUISystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void NativeUISystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void NativeUISystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
    }

    void NativeUISystemComponent::Activate()
    {
    }

    void NativeUISystemComponent::Deactivate()
    {
    }

} // namespace AzFramework
