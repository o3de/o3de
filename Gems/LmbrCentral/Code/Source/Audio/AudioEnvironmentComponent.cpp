/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AudioEnvironmentComponent.h"

#include <ISystem.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    void AudioEnvironmentComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioEnvironmentComponent, AZ::Component>()
                ->Version(1)
                ->Field("Environment name", &AudioEnvironmentComponent::m_defaultEnvironmentName)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AudioEnvironmentComponentRequestBus>("AudioEnvironmentComponentRequestBus")
                ->Event("SetAmount", &AudioEnvironmentComponentRequestBus::Events::SetAmount)
                ->Event("SetEnvironmentAmount", &AudioEnvironmentComponentRequestBus::Events::SetEnvironmentAmount);
        }
    }

    //=========================================================================
    void AudioEnvironmentComponent::Activate()
    {
        OnDefaultEnvironmentChanged();

        AudioEnvironmentComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    //=========================================================================
    void AudioEnvironmentComponent::Deactivate()
    {
        AudioEnvironmentComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    //=========================================================================
    AudioEnvironmentComponent::AudioEnvironmentComponent(const AZStd::string& environmentName)
        : m_defaultEnvironmentName(environmentName)
    {
    }

    //=========================================================================
    void AudioEnvironmentComponent::SetAmount(float amount)
    {
        // set amount on the default Environment (if set)
        if (m_defaultEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetEnvironmentAmount, m_defaultEnvironmentID, amount);
        }
    }

    //=========================================================================
    void AudioEnvironmentComponent::SetEnvironmentAmount(const char* environmentName, float amount)
    {
        Audio::TAudioEnvironmentID environmentID = INVALID_AUDIO_ENVIRONMENT_ID;
        if (environmentName && environmentName[0] != '\0')
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                environmentID = audioSystem->GetAudioEnvironmentID(environmentName);
            }
        }

        if (environmentID != INVALID_AUDIO_ENVIRONMENT_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetEnvironmentAmount, environmentID, amount);
        }
    }

    //=========================================================================
    void AudioEnvironmentComponent::OnDefaultEnvironmentChanged()
    {
        m_defaultEnvironmentID = INVALID_AUDIO_ENVIRONMENT_ID;
        if (!m_defaultEnvironmentName.empty())
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                m_defaultEnvironmentID = audioSystem->GetAudioEnvironmentID(m_defaultEnvironmentName.c_str());
            }
        }
    }

} // namespace LmbrCentral
