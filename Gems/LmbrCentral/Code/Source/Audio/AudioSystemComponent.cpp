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

#include <LmbrCentral_precompiled.h>

#include <Audio/AudioSystemComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <LmbrCentral/Audio/AudioTriggerComponentBus.h>

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

        void OnGamePaused()
        {
            Call(FN_OnGamePaused);
        }

        void OnGameUnpaused()
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
                ->Event("GlobalExecuteAudioTrigger", &AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger)
                ->Event("GlobalKillAudioTrigger", &AudioSystemComponentRequestBus::Events::GlobalKillAudioTrigger)
                ->Event("GlobalSetAudioRtpc", &AudioSystemComponentRequestBus::Events::GlobalSetAudioRtpc)
                ->Event("GlobalResetAudioRtpcs", &AudioSystemComponentRequestBus::Events::GlobalResetAudioRtpcs)
                ->Event("GlobalSetAudioSwitchState", &AudioSystemComponentRequestBus::Events::GlobalSetAudioSwitchState)
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
    void AudioSystemComponent::GlobalStopAllSounds()
    {
        SAudioRequest request;
        SAudioManagerRequestData<eAMRT_STOP_ALL_SOUNDS> requestData;
        request.pData = &requestData;

        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
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
    void AudioSystemComponent::OnCrySystemInitialized(ISystem&, const SSystemInitParams&)
    {
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
            &AudioSystemComponent::OnAudioEvent,
            this,
            Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST,
            Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE
        );
    }

    ////////////////////////////////////////////////////////////////////////
    void AudioSystemComponent::OnCrySystemShutdown(ISystem&)
    {
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener,
            &AudioSystemComponent::OnAudioEvent,
            this
        );
    }

    ////////////////////////////////////////////////////////////////////////
    // static
    void AudioSystemComponent::OnAudioEvent(const Audio::SAudioRequestInfo* const requestInfo)
    {
        if (requestInfo->eAudioRequestType == Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST)
        {
            const auto notificationType = static_cast<Audio::EAudioCallbackManagerRequestType>(requestInfo->nSpecificAudioRequest);
            if (notificationType == Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE && requestInfo->eResult == Audio::eARR_SUCCESS)
            {
                AZ::EntityId callbackOwnerEntityId(reinterpret_cast<AZ::u64>(requestInfo->pUserData));
                AudioTriggerComponentNotificationBus::Event(callbackOwnerEntityId, &AudioTriggerComponentNotificationBus::Events::OnTriggerFinished, requestInfo->nAudioControlID);
            }
        }
    }

} // namespace LmbrCentral
