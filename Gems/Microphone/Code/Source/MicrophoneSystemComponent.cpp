/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "MicrophoneSystemComponent.h"

namespace Audio
{
    void MicrophoneSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MicrophoneSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MicrophoneSystemComponent>("Microphone", "Provides access to a connected Microphone Device to capture and read the data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MicrophoneSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MicrophoneService"));
    }

    void MicrophoneSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MicrophoneService"));
    }

    void MicrophoneSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void MicrophoneSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    MicrophoneSystemComponent::MicrophoneSystemComponent()
        : m_impl(Implementation::Create())
    {
    }

    MicrophoneSystemComponent::~MicrophoneSystemComponent()
    {
        delete m_impl;
        m_impl = nullptr;
    }

    void MicrophoneSystemComponent::Init()
    {
    }

    void MicrophoneSystemComponent::Activate()
    {
        InitializeDevice();

        MicrophoneRequestBus::Handler::BusConnect();
    }

    void MicrophoneSystemComponent::Deactivate()
    {
        MicrophoneRequestBus::Handler::BusDisconnect();

        EndSession();
        ShutdownDevice();
    }

    bool MicrophoneSystemComponent::InitializeDevice()
    {
        if (m_impl)
        {
            m_initialized = m_impl->InitializeDevice();

            if (!m_initialized)
            {
                AZ_TracePrintf("MicrophoneSystemComponent", "Failed to initialize a Microphone device, check your OS audio device settings.\n");
                ShutdownDevice();
            }

            return m_initialized;
        }
        return false;
    }

    void MicrophoneSystemComponent::ShutdownDevice()
    {
        if (m_impl)
        {
            m_impl->ShutdownDevice();
            m_initialized = false;
        }
    }

    bool MicrophoneSystemComponent::StartSession()
    {
        if (m_impl && m_initialized)
        {
            return m_impl->StartSession();
        }
        return false;
    }

    void MicrophoneSystemComponent::EndSession()
    {
        if (m_impl && m_initialized)
        {
            m_impl->EndSession();
        }
    }

    bool MicrophoneSystemComponent::IsCapturing()
    {
        if (m_impl && m_initialized)
        {
            return m_impl->IsCapturing();
        }
        return false;
    }

    SAudioInputConfig MicrophoneSystemComponent::GetFormatConfig() const
    {
        if (m_impl)
        {
            return m_impl->GetFormatConfig();
        }
        return SAudioInputConfig();
    }

    AZStd::size_t MicrophoneSystemComponent::GetData(void** outputData, AZStd::size_t numFrames, const SAudioInputConfig& targetConfig, bool shouldDeinterleave)
    {
        if (m_impl && IsCapturing())
        {
            return m_impl->GetData(outputData, numFrames, targetConfig, shouldDeinterleave);
        }
        return 0;
    }

} // namespace Audio
