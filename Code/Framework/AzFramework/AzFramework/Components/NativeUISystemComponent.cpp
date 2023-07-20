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
        if ((m_applicationImplFactory = CreateApplicationImplementationFactory()) != nullptr)
        {
            AZ::Interface<Application::ImplementationFactory>::Register(m_applicationImplFactory);
        }

        if ((m_deviceGamepadImplFactory = GetDeviceGamepadImplentationFactory()) != nullptr)
        {
            AZ::Interface<InputDeviceGamepad::ImplementationFactory>::Register(m_deviceGamepadImplFactory);
        }
        
        if ((m_deviceKeyboardImplFactory = GetDeviceKeyboardImplementationFactory()) != nullptr)
        {
            AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Register(m_deviceKeyboardImplFactory);
        }

        if ((m_deviceMotionImplFactory = GetDeviceMotionImplentationFactory()) != nullptr)
        {
            AZ::Interface<InputDeviceMotion::ImplementationFactory>::Register(m_deviceMotionImplFactory);
        }

        if ((m_deviceMouseImplFactory = GetDeviceMouseImplentationFactory()) != nullptr)
        {
            AZ::Interface<InputDeviceMouse::ImplementationFactory>::Register(m_deviceMouseImplFactory);
        }

        if ((m_deviceTouchImplFactory = GetDeviceTouchImplentationFactory()) != nullptr)
        {
            AZ::Interface<InputDeviceTouch::ImplementationFactory>::Register(m_deviceTouchImplFactory);
        }

        if ((m_deviceVirtualKeyboardImplFactory = GetDeviceVirtualKeyboardImplentationFactory()) != nullptr)
        {
            AZ::Interface<InputDeviceVirtualKeyboard::ImplementationFactory>::Register(m_deviceVirtualKeyboardImplFactory);
        }

        if ((m_nativeWindowImplFactory = GetNativeWindowImplementationFactory()) != nullptr)
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

    void NativeUISystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void NativeUISystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("NativeUIInputSystemService", 0x67675d29));
    }

    void NativeUISystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void NativeUISystemComponent::GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("NativeUIInputSystemService", 0x67675d29));
    }

    void NativeUISystemComponent::Activate()
    {
    }

    void NativeUISystemComponent::Deactivate()
    {
    }

} // namespace AzFramework
