/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Audio/AudioTriggerComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ISystem.h>
#include <LmbrCentral/Audio/AudioProxyComponentBus.h>


namespace LmbrCentral
{
    //=========================================================================
    // Behavior Context AudioTriggerComponentNotificationBus forwarder
    class BehaviorAudioTriggerNotificationBusHandler
        : public Audio::AudioTriggerNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            BehaviorAudioTriggerNotificationBusHandler, "{ACCB0C42-3752-496B-9B1F-19276925EBB0}", AZ::SystemAllocator,
            ReportTriggerStarted, ReportTriggerFinished);

        void ReportTriggerStarted(Audio::TAudioControlID triggerId) override
        {
            Call(FN_ReportTriggerStarted, triggerId);
        }

        void ReportTriggerFinished(Audio::TAudioControlID triggerId) override
        {
            Call(FN_ReportTriggerFinished, triggerId);
        }
    };

    //=========================================================================
    void AudioTriggerComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioTriggerComponent, AZ::Component>()
                ->Version(2)
                ->Field("Play Trigger", &AudioTriggerComponent::m_defaultPlayTriggerName)
                ->Field("Stop Trigger", &AudioTriggerComponent::m_defaultStopTriggerName)
                ->Field("Obstruction Type", &AudioTriggerComponent::m_obstructionType)
                ->Field("Plays Immediately", &AudioTriggerComponent::m_playsImmediately)
                ;

            serializeContext->Class<Audio::TriggerNotificationIdType>()
                ->Version(1)
                ->Field("Owner", &Audio::TriggerNotificationIdType::m_owner)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Enum<static_cast<AZ::u32>(Audio::ObstructionType::Ignore)>("AudioObstructionType_Ignore")
                ->Enum<static_cast<AZ::u32>(Audio::ObstructionType::SingleRay)>("AudioObstructionType_SingleRay")
                ->Enum<static_cast<AZ::u32>(Audio::ObstructionType::MultiRay)>("AudioObstructionType_MultiRay")
                ;

            behaviorContext->EBus<AudioTriggerComponentRequestBus>("AudioTriggerComponentRequestBus")
                ->Event("Play", &AudioTriggerComponentRequestBus::Events::Play)
                ->Event("Stop", &AudioTriggerComponentRequestBus::Events::Stop)
                ->Event("ExecuteTrigger", &AudioTriggerComponentRequestBus::Events::ExecuteTrigger)
                ->Event("KillTrigger", &AudioTriggerComponentRequestBus::Events::KillTrigger)
                ->Event("KillAllTriggers", &AudioTriggerComponentRequestBus::Events::KillAllTriggers)
                ->Event("SetMovesWithEntity", &AudioTriggerComponentRequestBus::Events::SetMovesWithEntity)
                ->Event("SetObstructionType", &AudioTriggerComponentRequestBus::Events::SetObstructionType)
                ;

            behaviorContext->Class<Audio::TriggerNotificationIdType>("AudioTriggerNotificationIdType")
                ->Constructor<AZ::EntityId>()
                    ->Attribute(AZ::Script::Attributes::DefaultConstructorOverrideIndex, 0)
                ;

            behaviorContext->EBus<Audio::AudioTriggerNotificationBus>("AudioTriggerNotificationBus")
                ->Handler<BehaviorAudioTriggerNotificationBusHandler>()
                ;
        }

    }

    //=========================================================================
    void AudioTriggerComponent::Activate()
    {
        OnPlayTriggerChanged();
        OnStopTriggerChanged();
        OnObstructionTypeChanged();

        AudioTriggerComponentRequestBus::Handler::BusConnect(GetEntityId());

        if (m_playsImmediately)
        {
            // if requested, play the set trigger at Activation time...
            Play();
        }
    }

    //=========================================================================
    void AudioTriggerComponent::Deactivate()
    {
        AudioTriggerComponentRequestBus::Handler::BusDisconnect(GetEntityId());

        KillAllTriggers();
    }

    //=========================================================================
    AudioTriggerComponent::AudioTriggerComponent(const AZStd::string& playTriggerName, const AZStd::string& stopTriggerName, Audio::ObstructionType obstructionType, bool playsImmediately)
        : m_defaultPlayTriggerName(playTriggerName)
        , m_defaultStopTriggerName(stopTriggerName)
        , m_obstructionType(obstructionType)
        , m_playsImmediately(playsImmediately)
    {
    }

    //=========================================================================
    void AudioTriggerComponent::Play()
    {
        if (m_defaultPlayTriggerID != INVALID_AUDIO_CONTROL_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, m_defaultPlayTriggerID);
        }
    }

    //=========================================================================
    void AudioTriggerComponent::Stop()
    {
        if (m_defaultStopTriggerID == INVALID_AUDIO_CONTROL_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::KillTrigger, m_defaultPlayTriggerID);
        }
        else
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, m_defaultStopTriggerID);
        }
    }

    //=========================================================================
    void AudioTriggerComponent::ExecuteTrigger(const char* triggerName)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            Audio::TAudioControlID triggerID = INVALID_AUDIO_CONTROL_ID;
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                triggerID = audioSystem->GetAudioTriggerID(triggerName);
            }

            if (triggerID != INVALID_AUDIO_CONTROL_ID)
            {
                AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, triggerID);
            }
        }
    }

    //=========================================================================
    void AudioTriggerComponent::KillTrigger(const char* triggerName)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            Audio::TAudioControlID triggerID = INVALID_AUDIO_CONTROL_ID;
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                triggerID = audioSystem->GetAudioTriggerID(triggerName);
            }

            if (triggerID != INVALID_AUDIO_CONTROL_ID)
            {
                AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::KillTrigger, triggerID);
            }
        }
    }

    //=========================================================================
    void AudioTriggerComponent::KillAllTriggers()
    {
        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::KillAllTriggers);
    }

    //=========================================================================
    void AudioTriggerComponent::SetMovesWithEntity(bool shouldTrackEntity)
    {
        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetMovesWithEntity, shouldTrackEntity);
    }

    //=========================================================================
    void AudioTriggerComponent::SetObstructionType(Audio::ObstructionType obstructionType)
    {
        if (m_obstructionType != obstructionType && obstructionType != Audio::ObstructionType::Count)
        {
            m_obstructionType = obstructionType;
            OnObstructionTypeChanged();
        }
    }

    //=========================================================================
    void AudioTriggerComponent::OnPlayTriggerChanged()
    {
        m_defaultPlayTriggerID = INVALID_AUDIO_CONTROL_ID;
        if (!m_defaultPlayTriggerName.empty())
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                m_defaultPlayTriggerID = audioSystem->GetAudioTriggerID(m_defaultPlayTriggerName.c_str());
            }
        }

        // "ChangeNotify" sends callbacks on every key press for a text field!
        // This results in a lot of failed lookups.
    }

    //=========================================================================
    void AudioTriggerComponent::OnStopTriggerChanged()
    {
        m_defaultStopTriggerID = INVALID_AUDIO_CONTROL_ID;
        if (!m_defaultStopTriggerName.empty())
        {
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                m_defaultStopTriggerID = audioSystem->GetAudioTriggerID(m_defaultStopTriggerName.c_str());
            }
        }
    }

    //=========================================================================
    void AudioTriggerComponent::OnObstructionTypeChanged()
    {
        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetObstructionCalcType, m_obstructionType);
    }

} // namespace LmbrCentral
