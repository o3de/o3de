/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Audio/AudioSystemComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <LmbrCentral/Audio/AudioTriggerComponentBus.h>

#include <AzCore/IO/Path/Path.h>

#include <IAudioSystem.h>
using namespace Audio;

namespace LmbrCentral
{
    ////////////////////////////////////////////////////////////////////////
    class BehaviorAudioSystemComponentNotificationBusHandler
        : public AudioSystemComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorAudioSystemComponentNotificationBusHandler, "{2644951B-AB87-4D4D-BBB6-310E0ED2A3C9}", AZ::SystemAllocator,
            OnGamePaused,
            OnGameUnpaused
        );

        void OnGamePaused() override
        {
            Call(FN_OnGamePaused);
        }

        void OnGameUnpaused() override
        {
            Call(FN_OnGameUnpaused);
        }
    };

    ////////////////////////////////////////////////////////////////////////
    // static
    void AudioSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AudioSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AudioSystemComponent>("Audio System", "Provides access to audio system features without the need for an Entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AudioSystemComponentRequestBus>("AudioSystemComponentRequestBus")
                ->Event("GlobalStopAllSounds", &AudioSystemComponentRequestBus::Events::GlobalStopAllSounds)
                ->Event("GlobalMuteAudio", &AudioSystemComponentRequestBus::Events::GlobalMuteAudio)
                ->Event("GlobalUnmuteAudio", &AudioSystemComponentRequestBus::Events::GlobalUnmuteAudio)
                ->Event("GlobalRefreshAudio", &AudioSystemComponentRequestBus::Events::GlobalRefreshAudio)
                ->Event("GlobalExecuteAudioTrigger", &AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger)
                ->Event("GlobalKillAudioTrigger", &AudioSystemComponentRequestBus::Events::GlobalKillAudioTrigger)
                ->Event("GlobalSetAudioRtpc", &AudioSystemComponentRequestBus::Events::GlobalSetAudioRtpc)
                ->Event("GlobalResetAudioRtpcs", &AudioSystemComponentRequestBus::Events::GlobalResetAudioRtpcs)
                ->Event("GlobalSetAudioSwitchState", &AudioSystemComponentRequestBus::Events::GlobalSetAudioSwitchState)
                ->Event("LevelLoadAudio", &AudioSystemComponentRequestBus::Events::LevelLoadAudio)
                ->Event("LevelUnloadAudio", &AudioSystemComponentRequestBus::Events::LevelUnloadAudio)
                ;

            behaviorContext->EBus<AudioSystemComponentNotificationBus>("Audio System Component Notifications", "AudioSystemComponentNotificationBus")
                ->Handler<BehaviorAudioSystemComponentNotificationBusHandler>()
                ;
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // static
    void AudioSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AudioSystemService"));
    }

    ////////////////////////////////////////////////////////////////////////
    // static
    void AudioSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AudioSystemService"));
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::Activate()
    {
        AzFramework::LevelSystemLifecycleNotificationBus::Handler::BusConnect();
        if (IsAudioSystemInitialized())
        {
            AudioSystemComponentRequestBus::Handler::BusConnect();
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::Deactivate()
    {
        AudioSystemComponentRequestBus::Handler::BusDisconnect();
        AzFramework::LevelSystemLifecycleNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////
    bool AudioSystemComponent::IsAudioSystemInitialized()
    {
        return (AZ::Interface<Audio::IAudioSystem>::Get() != nullptr);
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalStopAllSounds()
    {
        Audio::SystemRequest::StopAllAudio stopAll;
        AZ::Interface<Audio::IAudioSystem>::Get()->PushRequest(AZStd::move(stopAll));
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalMuteAudio()
    {
        Audio::SystemRequest::MuteAll muteAll;
        AZ::Interface<Audio::IAudioSystem>::Get()->PushRequest(AZStd::move(muteAll));
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalUnmuteAudio()
    {
        Audio::SystemRequest::UnmuteAll unmuteAll;
        AZ::Interface<Audio::IAudioSystem>::Get()->PushRequest(AZStd::move(unmuteAll));
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalRefreshAudio(AZStd::string_view levelName)
    {
        AZ::Interface<Audio::IAudioSystem>::Get()->RefreshAudioSystem(levelName.data());
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalExecuteAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            TAudioControlID triggerId = AZ::Interface<Audio::IAudioSystem>::Get()->GetAudioTriggerID(triggerName);
            if (triggerId != INVALID_AUDIO_CONTROL_ID)
            {
                Audio::ObjectRequest::ExecuteTrigger execTrigger;
                execTrigger.m_triggerId = triggerId;
                execTrigger.m_owner = (callbackOwnerEntityId.IsValid()
                    ? reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<AZ::u64>(callbackOwnerEntityId)))
                    : this);

                AZ::Interface<Audio::IAudioSystem>::Get()->PushRequest(AZStd::move(execTrigger));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalKillAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            TAudioControlID triggerId = AZ::Interface<Audio::IAudioSystem>::Get()->GetAudioTriggerID(triggerName);
            if (triggerId != INVALID_AUDIO_CONTROL_ID)
            {
                Audio::ObjectRequest::StopTrigger stopTrigger;
                stopTrigger.m_triggerId = triggerId;
                stopTrigger.m_owner = (callbackOwnerEntityId.IsValid()
                    ? reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<AZ::u64>(callbackOwnerEntityId)))
                    : this);

                AZ::Interface<Audio::IAudioSystem>::Get()->PushRequest(AZStd::move(stopTrigger));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalSetAudioRtpc(const char* rtpcName, float value)
    {
        if (rtpcName && rtpcName[0] != '\0')
        {
            TAudioControlID rtpcId = AZ::Interface<Audio::IAudioSystem>::Get()->GetAudioRtpcID(rtpcName);
            if (rtpcId != INVALID_AUDIO_CONTROL_ID)
            {
                Audio::ObjectRequest::SetParameterValue setParameter;
                setParameter.m_parameterId = rtpcId;
                setParameter.m_value = value;
                AZ::Interface<Audio::IAudioSystem>::Get()->PushRequest(AZStd::move(setParameter));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalResetAudioRtpcs()
    {
        Audio::ObjectRequest::ResetParameters resetParameters;
        AZ::Interface<Audio::IAudioSystem>::Get()->PushRequest(AZStd::move(resetParameters));
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalSetAudioSwitchState(const char* switchName, const char* stateName)
    {
        if (switchName && switchName[0] != '\0' && stateName && stateName[0] != '\0')
        {
            TAudioControlID switchId = AZ::Interface<Audio::IAudioSystem>::Get()->GetAudioSwitchID(switchName);
            TAudioSwitchStateID stateId = INVALID_AUDIO_SWITCH_STATE_ID;
            if (switchId != INVALID_AUDIO_CONTROL_ID)
            {
                AZ::Interface<Audio::IAudioSystem>::Get()->GetAudioSwitchStateID(switchId, stateName);
            }

            if (stateId != INVALID_AUDIO_SWITCH_STATE_ID)
            {
                Audio::ObjectRequest::SetSwitchValue setSwitch;
                setSwitch.m_switchId = switchId;
                setSwitch.m_stateId = stateId;
                AZ::Interface<Audio::IAudioSystem>::Get()->PushRequest(AZStd::move(setSwitch));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::LevelLoadAudio(AZStd::string_view levelName)
    {
        auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
        if (!audioSystem || levelName.empty())
        {
            return;
        }

        AZ::IO::FixedMaxPath levelControlsPath{ audioSystem->GetControlsPath() };
        levelControlsPath /= "levels";
        levelControlsPath /= levelName;

        Audio::SystemRequest::LoadControls loadControls;
        loadControls.m_controlsPath = levelControlsPath.c_str();
        loadControls.m_scope = eADS_LEVEL_SPECIFIC;
        audioSystem->PushRequestBlocking(AZStd::move(loadControls));

        TAudioPreloadRequestID preloadRequestId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
        audioSystem->GetAudioPreloadRequestID(levelName.data());
        if (preloadRequestId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
        {
            Audio::SystemRequest::LoadBank loadBank;
            loadBank.m_preloadRequestId = preloadRequestId;
            loadBank.m_asyncLoad = false;
            audioSystem->PushRequestBlocking(AZStd::move(loadBank));
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::LevelUnloadAudio()
    {
        if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
            audioSystem != nullptr)
        {
            // Unload level-specific banks...
            Audio::SystemRequest::UnloadBanksByScope unloadBanks;
            unloadBanks.m_scope = eADS_LEVEL_SPECIFIC;
            audioSystem->PushRequestBlocking(AZStd::move(unloadBanks));

            // Now unload level-specific audio config data (controls then preloads)...
            Audio::SystemRequest::UnloadControls unloadControls;
            unloadControls.m_scope = eADS_LEVEL_SPECIFIC;
            // same flags as above
            audioSystem->PushRequestBlocking(AZStd::move(unloadControls));
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::OnLoadingStart(const char* levelName)
    {
        if (levelName && levelName[0] != '\0')
        {
            LevelLoadAudio(AZStd::string_view{ levelName });
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::OnUnloadComplete([[maybe_unused]] const char* levelName)
    {
        LevelUnloadAudio();
    }

} // namespace LmbrCentral
