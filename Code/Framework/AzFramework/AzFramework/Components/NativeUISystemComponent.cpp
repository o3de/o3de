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
        // Initialize any supported native control / window implementation factories for this platform
    
        InitializeApplicationImplementationFactory();

        InitializeDeviceGamepadImplentationFactory();
        
        InitializeDeviceKeyboardImplementationFactory();

        InitializeDeviceMotionImplentationFactory();

        InitializeDeviceMouseImplentationFactory();

        InitializeDeviceTouchImplentationFactory();

        InitializeDeviceVirtualKeyboardImplentationFactory();

        InitializeNativeWindowImplementationFactory();
    }

    NativeUISystemComponent::~NativeUISystemComponent()
    {
        if (m_applicationImplFactory)
        {
            AZ::Interface<Application::ImplementationFactory>::Unregister(m_applicationImplFactory.get());
        }

        if (m_deviceGamepadImplFactory)
        {
            AZ::Interface<InputDeviceGamepad::ImplementationFactory>::Unregister(m_deviceGamepadImplFactory.get());
        }
        
        if (m_deviceKeyboardImplFactory)
        {
            AZ::Interface<InputDeviceKeyboard::ImplementationFactory>::Unregister(m_deviceKeyboardImplFactory.get());
        }

        if (m_deviceMotionImplFactory)
        {
            AZ::Interface<InputDeviceMotion::ImplementationFactory>::Unregister(m_deviceMotionImplFactory.get());
        }

        if (m_deviceMouseImplFactory)
        {
            AZ::Interface<InputDeviceMouse::ImplementationFactory>::Unregister(m_deviceMouseImplFactory.get());
        }

        if (m_deviceTouchImplFactory)
        {
            AZ::Interface<InputDeviceTouch::ImplementationFactory>::Unregister(m_deviceTouchImplFactory.get());
        }

        if (m_deviceVirtualKeyboardImplFactory)
        {
            AZ::Interface<InputDeviceVirtualKeyboard::ImplementationFactory>::Unregister(m_deviceVirtualKeyboardImplFactory.get());
        }

        if (m_nativeWindowImplFactory)
        {
            AZ::Interface<NativeWindow::ImplementationFactory>::Unregister(m_nativeWindowImplFactory.get());
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
        incompatible.push_back(AZ_CRC_CE("NativeUIInputSystemService"));
    }

    void NativeUISystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void NativeUISystemComponent::GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NativeUIInputSystemService"));
    }

    void NativeUISystemComponent::Activate()
    {
    }

    void NativeUISystemComponent::Deactivate()
    {
    }

} // namespace AzFramework
