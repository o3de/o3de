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
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
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
            Audio::AudioSystemRequestBus::BroadcastResult(environmentID, &Audio::AudioSystemRequestBus::Events::GetAudioEnvironmentID, environmentName);
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
            Audio::AudioSystemRequestBus::BroadcastResult(m_defaultEnvironmentID, &Audio::AudioSystemRequestBus::Events::GetAudioEnvironmentID, m_defaultEnvironmentName.c_str());
        }
    }

} // namespace LmbrCentral
