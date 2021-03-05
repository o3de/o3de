/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "LmbrCentral_precompiled.h"
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
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
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
            Audio::AudioSystemRequestBus::BroadcastResult(rtpcID, &Audio::AudioSystemRequestBus::Events::GetAudioRtpcID, rtpcName);
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
            Audio::AudioSystemRequestBus::BroadcastResult(m_defaultRtpcID, &Audio::AudioSystemRequestBus::Events::GetAudioRtpcID, m_defaultRtpcName.c_str());
        }
    }

} // namespace LmbrCentral
