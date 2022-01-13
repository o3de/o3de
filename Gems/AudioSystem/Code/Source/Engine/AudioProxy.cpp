/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioProxy.h>

#include <ATLCommon.h>
#include <SoundCVars.h>
#include <AudioLogger.h>
#include <MathConversion.h>

namespace Audio
{
    extern CAudioLogger g_audioLogger;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioProxy::~CAudioProxy()
    {
        //if ((m_nFlags & eAPF_WAITING_FOR_ID) != 0)
        //{
        //    AZ::Interface<IAudioSystem>::Get()->RemoveRequestListener(&CAudioProxy::OnAudioEvent, this);
        //}

        AZ_Assert(m_nAudioObjectID == INVALID_AUDIO_OBJECT_ID, "Expected AudioObjectID [%d] to be invalid when the audio proxy is destructed.", m_nAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Initialize(const char* const sObjectName, const bool bInitAsync /* = true */)
    {
        auto audioProxiesInitType = static_cast<AZ::u32>(Audio::CVars::s_AudioProxiesInitType);

        Audio::SystemRequest::ReserveObject reserveObject;
        reserveObject.m_objectName = (sObjectName ? sObjectName : "");
        reserveObject.m_callback = [this](const Audio::SystemRequest::ReserveObject& request)
        {
            // Assign the new audio object ID and clear the 'waiting on id' flag...
            m_nAudioObjectID = request.m_objectId;
            // Now execute any requests queued while this was waiting
            ExecuteQueuedRequests();
        };

        // TODO: sort out the init types.  Figure out if we're going to support queued initialize or not.
        // Seems like we should just do async init all the time.
        // 0: instance-specific initialization (default).  So the bAsync flag is used to determine init type.
        // 1: All initialize sync
        // 2: All initialize async
        if ((bInitAsync && audioProxiesInitType == 0) || audioProxiesInitType == 2)
        {
            if (!HasId())
            {
                // Add the request listener to receive callback when the audio object ID has been registered with middleware...
                //AZ::Interface<IAudioSystem>::Get()->AddRequestListener(
                //    &CAudioProxy::OnAudioEvent, this, eART_AUDIO_MANAGER_REQUEST, eAMRT_RESERVE_AUDIO_OBJECT_ID);

                // TODO:
                // This request used flags eARF_PRIORITY_HIGH and eARF_SYNC_CALLBACK and request.pOwner = this !!
                AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(reserveObject));
            }
            // Don't support this...
            //else
            //{
            //    SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_INITIALIZE);
            //    oQueuedCommand.sValue = sObjectName;
            //    TryAddQueuedCommand(oQueuedCommand);
            //}
        }
        else
        {
            // TODO:
            // This request used flags eARF_PRIORITY_HIGH and eARF_EXECUTE_BLOCKING !!
            AZ::Interface<IAudioSystem>::Get()->PushRequestBlockingNew(AZStd::move(reserveObject));

            // Problem is that even with the blocking request, the callback is still async, so this assert will hit.
            // Either remove this or get sync callbacks working again.
    #if !defined(AUDIO_RELEASE)
            if (m_nAudioObjectID == INVALID_AUDIO_OBJECT_ID)
            {
                g_audioLogger.Log(LogType::Assert, "Failed to reserve audio object ID on AudioProxy (%s)!", sObjectName);
            }
    #endif // !AUDIO_RELEASE
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteSourceTrigger(
        TAudioControlID nTriggerID,
        const SAudioSourceInfo& rSourceInfo,
        [[maybe_unused]] const SAudioCallBackInfos& rCallbackInfos /* = SAudioCallBackInfos::GetEmptyObject() */)
    {
        Audio::ObjectRequest::ExecuteSourceTrigger execSourceTrigger;
        execSourceTrigger.m_triggerId = nTriggerID;
        execSourceTrigger.m_sourceInfo = rSourceInfo;

        if (HasId())
        {
            AZ_Assert(m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID, "Invalid AudioObjectID found when an audio proxy is executing a source trigger!");

            execSourceTrigger.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            // This request copied flags: request.nFlags = rCallbackInfos.nRequestFlags;
            // Also:
            // request.pOwner = (rCallbackInfos.pObjectToNotify != nullptr) ? rCallbackInfos.pObjectToNotify : this;
            // request.pUserData = rCallbackInfos.pUserData;
            // request.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(execSourceTrigger));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(execSourceTrigger));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteTrigger(
        const TAudioControlID nTriggerID,
        [[maybe_unused]] const SAudioCallBackInfos& rCallbackInfos /* = SAudioCallBackInfos::GetEmptyObject() */)
    {
        Audio::ObjectRequest::ExecuteTrigger execTrigger;
        execTrigger.m_triggerId = nTriggerID;

        if (HasId())
        {
            AZ_Assert(m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID, "Invalid AudioObjectID found when an audio proxy is executing a trigger!");

            execTrigger.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            // This request copied flags: request.nFlags = rCallbackInfos.nRequestFlags;
            // Also:
            // request.pOwner = (rCallbackInfos.pObjectToNotify != nullptr) ? rCallbackInfos.pObjectToNotify : this;
            // request.pUserData = rCallbackInfos.pUserData;
            // request.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(execTrigger));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(execTrigger));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::StopAllTriggers()
    {
        Audio::ObjectRequest::StopAllTriggers stopAll;

        if (HasId())
        {
            stopAll.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            // request.pOwner = this;
            // How does this work with m_filterByOwner ??
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(stopAll));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(stopAll));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::StopTrigger(const TAudioControlID nTriggerID)
    {
        Audio::ObjectRequest::StopTrigger stopTrigger;
        stopTrigger.m_triggerId = nTriggerID;

        if (HasId())
        {
            stopTrigger.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            //oRequest.pOwner = this;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(stopTrigger));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(stopTrigger));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID)
    {
        Audio::ObjectRequest::SetSwitchValue setSwitch;
        setSwitch.m_switchId = nSwitchID;
        setSwitch.m_stateId = nStateID;

        if (HasId())
        {
            setSwitch.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            //oRequest.pOwner = this;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(setSwitch));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(setSwitch));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetRtpcValue(const TAudioControlID nRtpcID, const float fValue)
    {
        Audio::ObjectRequest::SetParameterValue setParameter;
        setParameter.m_parameterId = nRtpcID;
        setParameter.m_value = fValue;

        if (HasId())
        {
            setParameter.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            //oRequest.pOwner = this;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(setParameter));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(setParameter));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetObstructionCalcType(const EAudioObjectObstructionCalcType eObstructionType)
    {
        const size_t nObstructionCalcIndex = static_cast<size_t>(eObstructionType);

        if (nObstructionCalcIndex < AZ_ARRAY_SIZE(ATLInternalControlIDs::OOCStateIDs))
        {
            SetSwitchState(ATLInternalControlIDs::ObstructionOcclusionCalcSwitchID, ATLInternalControlIDs::OOCStateIDs[nObstructionCalcIndex]);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetPosition(const SATLWorldPosition& refPosition)
    {
        Audio::ObjectRequest::SetPosition setPosition;

        if (HasId())
        {
            // Update position only if the delta exceeds a given value.
            if (Audio::CVars::s_PositionUpdateThreshold <= 0.f      // <-- no gating
                || !refPosition.GetPositionVec().IsClose(m_oPosition.GetPositionVec(), Audio::CVars::s_PositionUpdateThreshold))
            {
                m_oPosition = refPosition;

                // Make sure the forward/up directions are normalized
                m_oPosition.NormalizeForwardVec();
                m_oPosition.NormalizeUpVec();

                setPosition.m_audioObjectId = m_nAudioObjectID;
                setPosition.m_position = m_oPosition;
                // TODO:
                //oRequest.pOwner = this;
                AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(setPosition));
            }
        }
        else
        {
            // If a SetPosition is queued during init, we don't worry about the position update threshold gating.
            setPosition.m_position = refPosition;
            TryEnqueueRequest(AZStd::move(setPosition));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetPosition(const AZ::Vector3& refPosition)
    {
        SetPosition(SATLWorldPosition(refPosition));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetMultiplePositions(const MultiPositionParams& params)
    {
        Audio::ObjectRequest::SetMultiplePositions setMultiPosition;
        setMultiPosition.m_params = params;

        if (HasId())
        {
            setMultiPosition.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            //request.pOwner = this;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(setMultiPosition));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(setMultiPosition));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fValue)
    {
        Audio::ObjectRequest::SetEnvironmentValue setEnvironment;
        setEnvironment.m_environmentId = nEnvironmentID;
        setEnvironment.m_value = fValue;

        if (HasId())
        {
            setEnvironment.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            //oRequest.pOwner = this;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(setEnvironment));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(setEnvironment));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ResetEnvironments()
    {
        Audio::ObjectRequest::ResetEnvironments resetEnvironments;

        if (HasId())
        {
            resetEnvironments.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            //oRequest.pOwner = this;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(resetEnvironments));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(resetEnvironments));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ResetParameters()
    {
        Audio::ObjectRequest::ResetParameters resetParameters;

        if (HasId())
        {
            resetParameters.m_audioObjectId = m_nAudioObjectID;
            // TODO:
            //request.pOwner = this;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(resetParameters));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(resetParameters));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Release()
    {
        // Should just have a Release function, no reset.
        // If it has ID, push a Release request and reset info
        // Should we send a StopAll?
        // No Queueing of this type of call/request, it should be considered an immediate reset/recycle
        if (HasId())
        {
            //Reset();
            Audio::ObjectRequest::Release releaseObject;
            releaseObject.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(releaseObject));

            m_nAudioObjectID = INVALID_AUDIO_OBJECT_ID;
            m_oPosition = {};
            m_queuedAudioRequests.clear();
        }
        else
        {
            // TODO
            //TryAddQueuedCommand(SQueuedAudioCommand(eQACT_RELEASE));
        }

        AZ::Interface<IAudioSystem>::Get()->RecycleAudioProxy(this);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteQueuedRequests()
    {
        for (auto& requestVariant : m_queuedAudioRequests)
        {
            // Need to plug in the audio object ID that was assigned, then kick off the requests.
            AZStd::visit(
                [this](auto&& request)
                {
                    request.m_audioObjectId = m_nAudioObjectID;
                },
                requestVariant);
            AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(requestVariant));
        }
        m_queuedAudioRequests.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioProxy::HasId() const
    {
        return (m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID);
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct FindSetSwitchValue
    {
        FindSetSwitchValue(TAudioControlID audioSwitchId, TAudioSwitchStateID audioStateId)
            : m_audioSwitchId(audioSwitchId)
            , m_audioStateId(audioStateId)
        {
        }

        bool operator()(AudioRequestVariant& refRequest)
        {
            bool found = false;
            if (auto setSwitchRequest = AZStd::get_if<Audio::ObjectRequest::SetSwitchValue>(&refRequest);
                setSwitchRequest != nullptr)
            {
                if (setSwitchRequest->m_switchId == m_audioSwitchId)
                {
                    // A set command for this switch exists, update the value being set.
                    found = true;
                    setSwitchRequest->m_stateId = m_audioStateId;
                }
            }
            return found;
        }

    private:
        TAudioControlID m_audioSwitchId;
        TAudioSwitchStateID m_audioStateId;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct FindSetParameterValue
    {
        FindSetParameterValue(TAudioControlID audioParameterId, float audioParameterValue)
            : m_audioParameterId(audioParameterId)
            , m_audioParameterValue(audioParameterValue)
        {
        }

        bool operator()(AudioRequestVariant& refRequest)
        {
            bool found = false;
            if (auto setParameterRequest = AZStd::get_if<Audio::ObjectRequest::SetParameterValue>(&refRequest);
                setParameterRequest != nullptr)
            {
                if (setParameterRequest->m_parameterId == m_audioParameterId)
                {
                    // A set command for this parameter exists, update the value being set.
                    found = true;
                    setParameterRequest->m_value = m_audioParameterValue;
                }
            }
            return found;
        }

    private:
        TAudioControlID m_audioParameterId;
        float m_audioParameterValue;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct FindSetPosition
    {
        FindSetPosition(SATLWorldPosition& refPosition)
            : m_position(refPosition)
        {
        }

        bool operator()(AudioRequestVariant& refRequest)
        {
            bool found = false;
            if (auto setPositionRequest = AZStd::get_if<Audio::ObjectRequest::SetPosition>(&refRequest);
                setPositionRequest != nullptr)
            {
                // A set position request already exists, update the position.
                found = true;
                setPositionRequest->m_position = m_position;
            }
            return found;
        }

    private:
        SATLWorldPosition& m_position;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct FindSetMultiplePositions
    {
        FindSetMultiplePositions(MultiPositionParams& refParams)
            : m_params(refParams)
        {
        }

        bool operator()(AudioRequestVariant& refRequest)
        {
            bool found = false;
            if (auto setPositionRequest = AZStd::get_if<Audio::ObjectRequest::SetPosition>(&refRequest);
                setPositionRequest != nullptr)
            {
                // A Multi-Position request will replace an existing single position request.
                Audio::ObjectRequest::SetMultiplePositions setMultiRequest;
                setMultiRequest.m_audioObjectId = setPositionRequest->m_audioObjectId;
                setMultiRequest.m_status = setPositionRequest->m_status;
                refRequest = AZStd::move(setMultiRequest);
            }
            if (auto setPositionRequest = AZStd::get_if<Audio::ObjectRequest::SetMultiplePositions>(&refRequest);
                setPositionRequest != nullptr)
            {
                // A MultiPosition request exists already (or was transformed into one), set the position data.
                setPositionRequest->m_params = AZStd::move(m_params);
                found = true;
            }
            return found;
        }

    private:
        MultiPositionParams& m_params;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct FindSetEnvironmentValue
    {
        FindSetEnvironmentValue(TAudioEnvironmentID audioEnvironmentId, float audioEnvironmentValue)
            : m_audioEnvironmentId(audioEnvironmentId)
            , m_audioEnvironmentValue(audioEnvironmentValue)
        {
        }

        bool operator()(AudioRequestVariant& refRequest)
        {
            bool found = false;
            if (auto setEnvironmentRequest = AZStd::get_if<Audio::ObjectRequest::SetEnvironmentValue>(&refRequest);
                setEnvironmentRequest != nullptr)
            {
                if (setEnvironmentRequest->m_environmentId == m_audioEnvironmentId)
                {
                    // Update the value
                    setEnvironmentRequest->m_value = m_audioEnvironmentValue;
                    found = true;
                }
            }
            return found;
        }

    private:
        TAudioEnvironmentID m_audioEnvironmentId;
        float m_audioEnvironmentValue;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T>
    struct FindRequestType
    {
        FindRequestType() = default;
        bool operator()(AudioRequestVariant& refRequest)
        {
            if (auto request = AZStd::get_if<T>(&refRequest); request != nullptr)
            {
                return true;
            }
            return false;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::TryEnqueueRequest(AudioRequestVariant&& requestVariant)
    {
        bool shouldAdd = true;
        bool addFront = false;
        AZStd::visit(
            [this, &shouldAdd, &addFront](auto&& request)
            {
                using T = AZStd::decay_t<decltype(request)>;
                if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::ExecuteTrigger>
                    || AZStd::is_same_v<T, Audio::ObjectRequest::StopTrigger>
                    || AZStd::is_same_v<T, Audio::ObjectRequest::ExecuteSourceTrigger>)
                {
                    // always push these types of requests!
                }
                // else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::PrepareTrigger>
                //     || AZStd::is_same_v<T, Audio::ObjectRequest::UnprepareTrigger>)
                //{
                //     // Not implemented yet!
                //}
                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetPosition>)
                {
                    // Position should be set in front of queue, before other things happen.
                    addFront = true;
                    auto findIter =
                        AZStd::find_if(m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindSetPosition(request.m_position));
                    shouldAdd = (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetParameterValue>)
                {
                    auto findIter = AZStd::find_if(
                        m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(),
                        FindSetParameterValue(request.m_parameterId, request.m_value));
                    shouldAdd = (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetSwitchValue>)
                {
                    auto findIter = AZStd::find_if(
                        m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(),
                        FindSetSwitchValue(request.m_switchId, request.m_stateId));
                    shouldAdd = (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetEnvironmentValue>)
                {
                    auto findIter = AZStd::find_if(
                        m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(),
                        FindSetEnvironmentValue(request.m_environmentId, request.m_value));
                    shouldAdd = (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::StopAllTriggers>
                    || AZStd::is_same_v<T, Audio::ObjectRequest::ResetParameters>
                    || AZStd::is_same_v<T, Audio::ObjectRequest::ResetEnvironments>
                    || AZStd::is_same_v<T, Audio::ObjectRequest::Release>)
                {
                    // Generic find
                    auto findIter = AZStd::find_if(
                        m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindRequestType<T>());
                    shouldAdd = (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetMultiplePositions>)
                {
                    addFront = true;
                    auto findIter = AZStd::find_if(
                        m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindSetMultiplePositions(request.m_params));
                    shouldAdd = (findIter == m_queuedAudioRequests.end());
                }
                else
                {
                    shouldAdd = false;
                }
            },
            requestVariant);

        if (shouldAdd)
        {
            if (addFront)
            {
                m_queuedAudioRequests.push_front(AZStd::move(requestVariant));
            }
            else
            {
                m_queuedAudioRequests.push_back(AZStd::move(requestVariant));
            }
        }
    }

} // namespace Audio
