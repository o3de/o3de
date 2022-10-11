/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AudioRtpcComponent.h"

#include <ISystem.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    void AudioRtpcComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioRtpcComponent, AZ::Component>()
                ->Version(1)
                ->Field("Rtpc Name", &AudioRtpcComponent::m_defaultRtpcName)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AudioRtpcComponentRequestBus>("AudioRtpcComponentRequestBus")
                ->Event("SetValue", &AudioRtpcComponentRequestBus::Events::SetValue)
                ->Event("SetRtpcValue", &AudioRtpcComponentRequestBus::Events::SetRtpcValue)
                ;
        }
    }

    //=========================================================================
    void AudioRtpcComponent::Activate()
    {
        OnRtpcNameChanged();

        AudioRtpcComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    //=========================================================================
    void AudioRtpcComponent::Deactivate()
    {
        AudioRtpcComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    //=========================================================================
    void AudioRtpcComponent::SetValue(float value)
    {
        if (m_defaultRtpcID != INVALID_AUDIO_CONTROL_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetRtpcValue, m_defaultRtpcID, value);
        }
    }

    //=========================================================================
    AudioRtpcComponent::AudioRtpcComponent(const AZStd::string& rtpcName)
        : m_defaultRtpcName(rtpcName)
    {
    }

    //=========================================================================
    void AudioRtpcComponent::SetRtpcValue(const char* rtpcName, float value)
    {
        if (rtpcName && rtpcName[0] != '\0')
        {
            Audio::TAudioControlID rtpcID = INVALID_AUDIO_CONTROL_ID;
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                rtpcID = audioSystem->GetAudioRtpcID(rtpcName);
            }

            if (rtpcID != INVALID_AUDIO_CONTROL_ID)
            {
                AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetRtpcValue, rtpcID, value);
            }
        }
    }

    //=========================================================================
    void AudioRtpcComponent::OnRtpcNameChanged()
    {
        m_defaultRtpcID = INVALID_AUDIO_CONTROL_ID;
        if (!m_defaultRtpcName.empty())
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                m_defaultRtpcID = audioSystem->GetAudioRtpcID(m_defaultRtpcName.c_str());
            }
        }
    }

} // namespace LmbrCentral
