/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BarrierInputSystemComponent.h>
#include <BarrierInputKeyboard.h>
#include <BarrierInputMouse.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzCore/Console/IConsole.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace BarrierInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class T>
    void OnBarrierConnectionCVarChanged(const T&)
    {
        BarrierInputConnectionNotificationBus::Broadcast(&BarrierInputConnectionNotifications::OnBarrierConnectionCVarChanged);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ_CVAR(AZ::CVarFixedString,
            barrier_clientScreenName,
            "",
            OnBarrierConnectionCVarChanged<AZ::CVarFixedString>,
            AZ::ConsoleFunctorFlags::DontReplicate,
            "The Barrier screen name assigned to this client.");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ_CVAR(AZ::CVarFixedString,
            barrier_serverHostName,
            "",
            OnBarrierConnectionCVarChanged<AZ::CVarFixedString>,
            AZ::ConsoleFunctorFlags::DontReplicate,
            "The IP or hostname of the Barrier server to connect to.");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ_CVAR(AZ::u32,
            barrier_connectionPort,
            BarrierClient::DEFAULT_BARRIER_CONNECTION_PORT_NUMBER,
            OnBarrierConnectionCVarChanged<AZ::u32>,
            AZ::ConsoleFunctorFlags::DontReplicate,
            "The port number over which to connect to the Barrier server.");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierInputSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BarrierInputSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<BarrierInputSystemComponent>("BarrierInput", "Provides functionality related to Barrier input.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierInputSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("BarrierInputService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierInputSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("BarrierInputService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierInputSystemComponent::Activate()
    {
        TryCreateBarrierClientAndInputDeviceImplementations();
        BarrierInputConnectionNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierInputSystemComponent::Deactivate()
    {
        BarrierInputConnectionNotificationBus::Handler::BusDisconnect();
        DestroyBarrierClientAndInputDeviceImplementations();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierInputSystemComponent::OnBarrierConnectionCVarChanged()
    {
        TryCreateBarrierClientAndInputDeviceImplementations();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierInputSystemComponent::TryCreateBarrierClientAndInputDeviceImplementations()
    {
        // Destroy any existing Barrier client and input device implementations.
        DestroyBarrierClientAndInputDeviceImplementations();
        
        const AZ::CVarFixedString barrierClientScreenNameCVar = static_cast<AZ::CVarFixedString>(barrier_clientScreenName);
        const AZ::CVarFixedString barrierServerHostNameCVar = static_cast<AZ::CVarFixedString>(barrier_serverHostName);
        const AZ::u32 barrierConnectionPort = static_cast<AZ::u32>(barrier_connectionPort);
        if (!barrierClientScreenNameCVar.empty() && !barrierServerHostNameCVar.empty() && barrierConnectionPort)
        {
            // Enable the Barrier keyboard/mouse input device implementations.
            InputDeviceKeyboardBarrierImplFactory   keyboardBarrierImplFactory;
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::Bus::Event(
                AzFramework::InputDeviceKeyboard::Id,
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::SetCustomImplementation,
                &keyboardBarrierImplFactory);

            InputDeviceMouseBarrierImplFactory mouseBarrierImplFactory;
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::Bus::Event(
                AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::SetCustomImplementation,
                &mouseBarrierImplFactory);

            // Create the Barrier client instance.
            m_barrierClient = AZStd::make_unique<BarrierClient>(barrierClientScreenNameCVar.c_str(), barrierServerHostNameCVar.c_str(), barrierConnectionPort);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierInputSystemComponent::DestroyBarrierClientAndInputDeviceImplementations()
    {
        if (m_barrierClient)
        {
            // Destroy the Barrier client instance.
            m_barrierClient.reset();

            // Reset to the default keyboard/mouse input device implementations.
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::Bus::Event(
                AzFramework::InputDeviceKeyboard::Id,
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::SetCustomImplementation,
                AZ::Interface<AzFramework::InputDeviceKeyboard::ImplementationFactory>::Get());
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::Bus::Event(
                AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::SetCustomImplementation,
                AZ::Interface<AzFramework::InputDeviceMouse::ImplementationFactory>::Get());
        }
    }
} // namespace BarrierInput
