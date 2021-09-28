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
#include <ISystem.h>

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
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
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
        provided.push_back(AZ_CRC("AudioSystemService", 0xfbb9b542));
    }

    ////////////////////////////////////////////////////////////////////////
    // static
    void AudioSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AudioSystemService", 0xfbb9b542));
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::Activate()
    {
        AudioSystemComponentRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::Deactivate()
    {
        AudioSystemComponentRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////
    bool AudioSystemComponent::IsAudioSystemInitialized()
    {
        return Audio::AudioSystemRequestBus::HasHandlers();
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalStopAllSounds()
    {
        SAudioRequest request;
        SAudioManagerRequestData<eAMRT_STOP_ALL_SOUNDS> requestData;
        request.pData = &requestData;

        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalMuteAudio()
    {
        SAudioRequest request;
        SAudioManagerRequestData<eAMRT_MUTE_ALL> requestData;
        request.pData = &requestData;

        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalUnmuteAudio()
    {
        SAudioRequest request;
        SAudioManagerRequestData<eAMRT_UNMUTE_ALL> requestData;
        request.pData = &requestData;

        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalRefreshAudio(AZStd::string_view levelName)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::RefreshAudioSystem, levelName.data());
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalExecuteAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
            if (triggerId != INVALID_AUDIO_CONTROL_ID)
            {
                SAudioRequest request;
                SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> requestData;
                requestData.nTriggerID = triggerId;
                request.pData = &requestData;

                if (callbackOwnerEntityId.IsValid())
                {
                    request.nFlags = (eARF_PRIORITY_NORMAL | eARF_SYNC_FINISHED_CALLBACK);
                    request.pOwner = this;
                    request.pUserData = SAudioCallBackInfos::UserData(AZ::u64(callbackOwnerEntityId));
                    request.pUserDataOwner = nullptr;
                }

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalKillAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
            if (triggerId != INVALID_AUDIO_CONTROL_ID)
            {
                SAudioRequest request;
                SAudioObjectRequestData<eAORT_STOP_TRIGGER> requestData;
                requestData.nTriggerID = triggerId;
                request.pData = &requestData;

                if (callbackOwnerEntityId.IsValid())
                {
                    request.nFlags = (eARF_PRIORITY_NORMAL | eARF_SYNC_FINISHED_CALLBACK);
                    request.pOwner = this;
                    request.pUserData = SAudioCallBackInfos::UserData(AZ::u64(callbackOwnerEntityId));
                    request.pUserDataOwner = nullptr;
                }

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalSetAudioRtpc(const char* rtpcName, float value)
    {
        if (rtpcName && rtpcName[0] != '\0')
        {
            TAudioControlID rtpcId = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(rtpcId, &AudioSystemRequestBus::Events::GetAudioRtpcID, rtpcName);
            if (rtpcId != INVALID_AUDIO_CONTROL_ID)
            {
                SAudioRequest request;
                SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> requestData(rtpcId, value);
                request.pData = &requestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalResetAudioRtpcs()
    {
        SAudioRequest request;
        SAudioObjectRequestData<eAORT_RESET_RTPCS> requestData;
        request.pData = &requestData;

        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::GlobalSetAudioSwitchState(const char* switchName, const char* stateName)
    {
        if (switchName && switchName[0] != '\0' && stateName && stateName[0] != '\0')
        {
            TAudioControlID switchId = INVALID_AUDIO_CONTROL_ID;
            TAudioSwitchStateID stateId = INVALID_AUDIO_SWITCH_STATE_ID;
            AudioSystemRequestBus::BroadcastResult(switchId, &AudioSystemRequestBus::Events::GetAudioSwitchID, switchName);
            if (switchId != INVALID_AUDIO_CONTROL_ID)
            {
                AudioSystemRequestBus::BroadcastResult(stateId, &AudioSystemRequestBus::Events::GetAudioSwitchStateID, switchId, stateName);
            }

            if (stateId != INVALID_AUDIO_SWITCH_STATE_ID)
            {
                SAudioRequest request;
                SAudioObjectRequestData<eAORT_SET_SWITCH_STATE> requestData(switchId, stateId);
                request.pData = &requestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::LevelLoadAudio(AZStd::string_view levelName)
    {
        const char* controlsPath = nullptr;
        AudioSystemRequestBus::BroadcastResult(controlsPath, &AudioSystemRequestBus::Events::GetControlsPath);
        if (!controlsPath)
        {
            return;
        }

        AZ::IO::FixedMaxPath levelControlsPath{ controlsPath };
        levelControlsPath /= "levels";
        levelControlsPath /= levelName;

        SAudioRequest request;
        SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA> requestData(levelControlsPath.Native().data(), eADS_LEVEL_SPECIFIC);
        request.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);
        request.pData = &requestData;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, request);

        SAudioManagerRequestData<eAMRT_PARSE_PRELOADS_DATA> requestData2(levelControlsPath.Native().data(), eADS_LEVEL_SPECIFIC);
        request.pData = &requestData2;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, request);

        TAudioPreloadRequestID preloadRequestId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
        AudioSystemRequestBus::BroadcastResult(
            preloadRequestId, &AudioSystemRequestBus::Events::GetAudioPreloadRequestID, levelName.data());
        if (preloadRequestId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
        {
            SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST> requestData3(preloadRequestId);
            request.pData = &requestData3;
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, request);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::LevelUnloadAudio()
    {
        SAudioRequest request;

        // Unload level-specific banks...
        SAudioManagerRequestData<eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE> requestData(eADS_LEVEL_SPECIFIC);
        request.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);
        request.pData = &requestData;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, request);

        // Now unload level-specific audio config data (controls then preloads)...
        SAudioManagerRequestData<eAMRT_CLEAR_CONTROLS_DATA> requestData2(eADS_LEVEL_SPECIFIC);
        request.pData = &requestData2;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, request);

        SAudioManagerRequestData<eAMRT_CLEAR_PRELOADS_DATA> requestData3(eADS_LEVEL_SPECIFIC);
        request.pData = &requestData3;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, request);
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::AddRequestListener,
            &AudioSystemComponent::OnAudioEvent,
            this,
            eART_AUDIO_CALLBACK_MANAGER_REQUEST,
            eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE
        );

        system.GetILevelSystem()->AddListener(this);
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::RemoveRequestListener,
            &AudioSystemComponent::OnAudioEvent,
            this
        );

        system.GetILevelSystem()->RemoveListener(this);
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::OnLoadingStart(const char* levelName)
    {
        LevelLoadAudio(AZStd::string_view{ levelName });
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::OnUnloadComplete([[maybe_unused]] const char* levelName)
    {
        LevelUnloadAudio();
    }

    ////////////////////////////////////////////////////////////////////////
    // static
    void AudioSystemComponent::OnAudioEvent(const SAudioRequestInfo* const requestInfo)
    {
        if (requestInfo->eAudioRequestType == eART_AUDIO_CALLBACK_MANAGER_REQUEST)
        {
            const auto notificationType = static_cast<EAudioCallbackManagerRequestType>(requestInfo->nSpecificAudioRequest);
            if (notificationType == eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE && requestInfo->eResult == eARR_SUCCESS)
            {
                AZ::EntityId callbackOwnerEntityId(reinterpret_cast<AZ::u64>(requestInfo->pUserData));
                AudioTriggerComponentNotificationBus::Event(callbackOwnerEntityId, &AudioTriggerComponentNotificationBus::Events::OnTriggerFinished, requestInfo->nAudioControlID);
            }
        }
    }

} // namespace LmbrCentral
