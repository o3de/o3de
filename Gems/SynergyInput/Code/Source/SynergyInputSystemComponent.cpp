/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SynergyInputSystemComponent.h>
#include <SynergyInputKeyboard.h>
#include <SynergyInputMouse.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzCore/Console/IConsole.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SynergyInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void OnSynergyConnectionCVarChanged(const AZ::CVarFixedString&)
    {
        SynergyInputConnectionNotificationBus::Broadcast(&SynergyInputConnectionNotifications::OnSynergyConnectionCVarChanged);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ_CVAR(AZ::CVarFixedString,
            synergy_clientScreenName,
            "",
            OnSynergyConnectionCVarChanged,
            AZ::ConsoleFunctorFlags::DontReplicate,
            "The Synergy screen name assigned to this client.");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ_CVAR(AZ::CVarFixedString,
            synergy_serverHostName,
            "",
            OnSynergyConnectionCVarChanged,
            AZ::ConsoleFunctorFlags::DontReplicate,
            "The IP or hostname of the Synergy server to connect to.");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SynergyInputSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SynergyInputSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SynergyInputSystemComponent>("SynergyInput", "Provides functionality related to Synergy input.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SynergyInputSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SynergyInputService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SynergyInputSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SynergyInputService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SynergyInputSystemComponent::Activate()
    {
        TryCreateSynergyClientAndInputDeviceImplementations();
        SynergyInputConnectionNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SynergyInputSystemComponent::Deactivate()
    {
        SynergyInputConnectionNotificationBus::Handler::BusDisconnect();
        DestroySynergyClientAndInputDeviceImplementations();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SynergyInputSystemComponent::OnSynergyConnectionCVarChanged()
    {
        TryCreateSynergyClientAndInputDeviceImplementations();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SynergyInputSystemComponent::TryCreateSynergyClientAndInputDeviceImplementations()
    {
        // Destroy any existing Synergy client and input device implementations.
        DestroySynergyClientAndInputDeviceImplementations();
        
        const AZ::CVarFixedString synergyClientScreenNameCVar = static_cast<AZ::CVarFixedString>(synergy_clientScreenName);
        const AZ::CVarFixedString synergyServerHostNameCVar = static_cast<AZ::CVarFixedString>(synergy_serverHostName);
        if (!synergyClientScreenNameCVar.empty() && !synergyServerHostNameCVar.empty())
        {
            // Enable the Synergy keyboard/mouse input device implementations.
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::Bus::Event(
                AzFramework::InputDeviceKeyboard::Id,
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::SetCustomImplementation,
                SynergyInput::InputDeviceKeyboardSynergy::Create);
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::Bus::Event(
                AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::SetCustomImplementation,
                SynergyInput::InputDeviceMouseSynergy::Create);

            // Create the Synergy client instance.
            m_synergyClient = AZStd::make_unique<SynergyClient>(synergyClientScreenNameCVar.c_str(), synergyServerHostNameCVar.c_str());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SynergyInputSystemComponent::DestroySynergyClientAndInputDeviceImplementations()
    {
        if (m_synergyClient)
        {
            // Destroy the Synergy client instance.
            m_synergyClient.reset();

            // Reset to the default keyboard/mouse input device implementations.
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::Bus::Event(
                AzFramework::InputDeviceKeyboard::Id,
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::SetCustomImplementation,
                AzFramework::InputDeviceKeyboard::Implementation::Create);
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::Bus::Event(
                AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::SetCustomImplementation,
                AzFramework::InputDeviceMouse::Implementation::Create);
        }
    }
} // namespace SynergyInput
