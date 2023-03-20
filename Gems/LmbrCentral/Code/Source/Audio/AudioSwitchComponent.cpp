/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AudioSwitchComponent.h"

#include <ISystem.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    void AudioSwitchComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioSwitchComponent, AZ::Component>()
                ->Version(1)
                ->Field("Switch name", &AudioSwitchComponent::m_defaultSwitchName)
                ->Field("State name", &AudioSwitchComponent::m_defaultStateName)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AudioSwitchComponentRequestBus>("AudioSwitchComponentRequestBus")
                ->Event("SetState", &AudioSwitchComponentRequestBus::Events::SetState)
                ->Event("SetSwitchState", &AudioSwitchComponentRequestBus::Events::SetSwitchState)
                ;
        }
    }

    //=========================================================================
    void AudioSwitchComponent::Activate()
    {
        OnDefaultSwitchChanged();
        OnDefaultStateChanged();

        // set the default switch state, if valid IDs were found.
        if (m_defaultSwitchID != INVALID_AUDIO_CONTROL_ID && m_defaultStateID != INVALID_AUDIO_SWITCH_STATE_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetSwitchState, m_defaultSwitchID, m_defaultStateID);
        }

        AudioSwitchComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    //=========================================================================
    void AudioSwitchComponent::Deactivate()
    {
        AudioSwitchComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    //=========================================================================
    AudioSwitchComponent::AudioSwitchComponent(const AZStd::string& switchName, const AZStd::string& stateName)
        : m_defaultSwitchName(switchName)
        , m_defaultStateName(stateName)
    {
    }

    //=========================================================================
    void AudioSwitchComponent::SetState(const char* stateName)
    {
        if (m_defaultSwitchID != INVALID_AUDIO_CONTROL_ID)
        {
            // only allowed if there's a default switch that is known.
            if (stateName && stateName[0] != '\0')
            {
                Audio::TAudioSwitchStateID stateID = INVALID_AUDIO_SWITCH_STATE_ID;
                if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
                {
                    stateID = audioSystem->GetAudioSwitchStateID(m_defaultSwitchID, stateName);
                }

                if (stateID != INVALID_AUDIO_SWITCH_STATE_ID)
                {
                    AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetSwitchState, m_defaultSwitchID, stateID);
                }
            }
        }
    }

    //=========================================================================
    void AudioSwitchComponent::SetSwitchState(const char* switchName, const char* stateName)
    {
        Audio::TAudioControlID switchID = INVALID_AUDIO_CONTROL_ID;
        Audio::TAudioSwitchStateID stateID = INVALID_AUDIO_SWITCH_STATE_ID;

        // lookup switch...
        if (switchName && switchName[0] != '\0')
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                switchID = audioSystem->GetAudioSwitchID(switchName);
            }
        }

        // using the switchID (if found), lookup the state...
        if (switchID != INVALID_AUDIO_CONTROL_ID && stateName && stateName[0] != '\0')
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                stateID = audioSystem->GetAudioSwitchStateID(switchID, stateName);
            }
        }

        // if both IDs found, make the call...
        if (switchID != INVALID_AUDIO_CONTROL_ID && stateID != INVALID_AUDIO_SWITCH_STATE_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetSwitchState, switchID, stateID);
        }
    }

    //=========================================================================
    void AudioSwitchComponent::OnDefaultSwitchChanged()
    {
        m_defaultSwitchID = INVALID_AUDIO_CONTROL_ID;
        if (!m_defaultSwitchName.empty())
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                m_defaultSwitchID = audioSystem->GetAudioSwitchID(m_defaultSwitchName.c_str());
            }
        }
    }

    //=========================================================================
    void AudioSwitchComponent::OnDefaultStateChanged()
    {
        m_defaultStateID = INVALID_AUDIO_SWITCH_STATE_ID;
        if (!m_defaultStateName.empty() && m_defaultSwitchID != INVALID_AUDIO_CONTROL_ID)
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                m_defaultStateID = audioSystem->GetAudioSwitchStateID(m_defaultSwitchID, m_defaultStateName.c_str());
            }
        }
    }

} // namespace LmbrCentral
