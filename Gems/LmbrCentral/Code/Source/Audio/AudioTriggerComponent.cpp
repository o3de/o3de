/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AudioTriggerComponent.h"

#include <ISystem.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    //=========================================================================
    // Behavior Context AudioTriggerComponentNotificationBus forwarder
    class BehaviorAudioTriggerComponentNotificationBusHandler
        : public AudioTriggerComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorAudioTriggerComponentNotificationBusHandler, "{ACCB0C42-3752-496B-9B1F-19276925EBB0}", AZ::SystemAllocator,
            OnTriggerFinished);

        void OnTriggerFinished(const Audio::TAudioControlID triggerID) override
        {
            Call(FN_OnTriggerFinished, triggerID);
        }
    };

    //=========================================================================
    void AudioTriggerComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioTriggerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Play Trigger", &AudioTriggerComponent::m_defaultPlayTriggerName)
                ->Field("Stop Trigger", &AudioTriggerComponent::m_defaultStopTriggerName)
                ->Field("Obstruction Type", &AudioTriggerComponent::m_obstructionType)
                ->Field("Plays Immediately", &AudioTriggerComponent::m_playsImmediately)
                ->Field("Send Finished Event", &AudioTriggerComponent::m_notifyWhenTriggerFinishes)
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

            behaviorContext->EBus<AudioTriggerComponentNotificationBus>("AudioTriggerComponentNotificationBus")
                ->Handler<BehaviorAudioTriggerComponentNotificationBusHandler>()
                ;
        }

    }

    //=========================================================================
    void AudioTriggerComponent::Activate()
    {
        OnPlayTriggerChanged();
        OnStopTriggerChanged();
        OnObstructionTypeChanged();

        if (m_notifyWhenTriggerFinishes)
        {
            m_callbackInfo.reset(new Audio::SAudioCallBackInfos(
                this,
                static_cast<AZ::u64>(GetEntityId()),
                nullptr,
                (Audio::eARF_PRIORITY_NORMAL | Audio::eARF_SYNC_FINISHED_CALLBACK)
            ));

            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
                &AudioTriggerComponent::OnAudioEvent,
                this,
                Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST,
                Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE);
        }
        else
        {
            m_callbackInfo.reset(new Audio::SAudioCallBackInfos(Audio::SAudioCallBackInfos::GetEmptyObject()));
        }

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

        if (m_notifyWhenTriggerFinishes)
        {
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener, &AudioTriggerComponent::OnAudioEvent, this);
        }

        KillAllTriggers();

        m_callbackInfo.reset();
    }

    //=========================================================================
    AudioTriggerComponent::AudioTriggerComponent(const AZStd::string& playTriggerName, const AZStd::string& stopTriggerName, Audio::ObstructionType obstructionType, bool playsImmediately, bool notifyFinished)
        : m_defaultPlayTriggerName(playTriggerName)
        , m_defaultStopTriggerName(stopTriggerName)
        , m_obstructionType(obstructionType)
        , m_playsImmediately(playsImmediately)
        , m_notifyWhenTriggerFinishes(notifyFinished)
    {
    }

    //=========================================================================
    void AudioTriggerComponent::Play()
    {
        if (m_defaultPlayTriggerID != INVALID_AUDIO_CONTROL_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, m_defaultPlayTriggerID, *m_callbackInfo);
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
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, m_defaultStopTriggerID, *m_callbackInfo);
        }
    }

    //=========================================================================
    void AudioTriggerComponent::ExecuteTrigger(const char* triggerName)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            Audio::TAudioControlID triggerID = INVALID_AUDIO_CONTROL_ID;
            Audio::AudioSystemRequestBus::BroadcastResult(triggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
            if (triggerID != INVALID_AUDIO_CONTROL_ID)
            {
                AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, triggerID, *m_callbackInfo);
            }
        }
    }

    //=========================================================================
    void AudioTriggerComponent::KillTrigger(const char* triggerName)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            Audio::TAudioControlID triggerID = INVALID_AUDIO_CONTROL_ID;
            Audio::AudioSystemRequestBus::BroadcastResult(triggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
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
    // static
    void AudioTriggerComponent::OnAudioEvent(const Audio::SAudioRequestInfo* const requestInfo)
    {
        // look for the 'finished trigger instance' event
        if (requestInfo->eAudioRequestType == Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST)
        {
            const auto notificationType = static_cast<Audio::EAudioCallbackManagerRequestType>(requestInfo->nSpecificAudioRequest);
            if (notificationType == Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE)
            {
                if (requestInfo->eResult == Audio::eARR_SUCCESS)
                {
                    AZ::EntityId entityId(reinterpret_cast<AZ::u64>(requestInfo->pUserData));
                    AudioTriggerComponentNotificationBus::Event(entityId, &AudioTriggerComponentNotificationBus::Events::OnTriggerFinished, requestInfo->nAudioControlID);
                }
            }
        }
    }

    //=========================================================================
    void AudioTriggerComponent::OnPlayTriggerChanged()
    {
        m_defaultPlayTriggerID = INVALID_AUDIO_CONTROL_ID;
        if (!m_defaultPlayTriggerName.empty())
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_defaultPlayTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, m_defaultPlayTriggerName.c_str());
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
            Audio::AudioSystemRequestBus::BroadcastResult(m_defaultStopTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, m_defaultStopTriggerName.c_str());
        }
    }

    //=========================================================================
    void AudioTriggerComponent::OnObstructionTypeChanged()
    {
        // This conversion to legacy enum will be removed eventually:
        auto legacyObstructionType = static_cast<Audio::EAudioObjectObstructionCalcType>(m_obstructionType);
        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetObstructionCalcType, legacyObstructionType);
    }

} // namespace LmbrCentral
