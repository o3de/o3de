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

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioProxy::~CAudioProxy()
    {
        Release();  // this should be okay to do if we no longer queue Release requests.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Initialize(const char* sObjectName, void* ownerOverride, bool bInitAsync /* = true */)
    {
        if (HasId())
        {
            // Already has an ID assigned, nothing needed
            return;
        }

        m_ownerOverride = ownerOverride;

        Audio::SystemRequest::ReserveObject reserveObject;
        reserveObject.m_objectName = (sObjectName ? sObjectName : "");
        reserveObject.m_callback = [this](const Audio::SystemRequest::ReserveObject& request)
        {
            // Assign the new audio object ID...
            m_nAudioObjectID = request.m_objectId;
            m_waitingForId = false;
            // Now execute any requests queued while this was waiting for an ID assignment
            ExecuteQueuedRequests();
        };

        // 0: instance-specific initialization (default).  So the bAsync flag is used to determine init type.
        // 1: All initialize sync
        // 2: All initialize async
        m_waitingForId = true;
        auto audioProxiesInitType = static_cast<AZ::u32>(Audio::CVars::s_AudioProxiesInitType);
        if ((bInitAsync && audioProxiesInitType == 0) || audioProxiesInitType == 2)
        {
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(reserveObject));
        }
        else
        {
            reserveObject.m_flags = eARF_SYNC_CALLBACK;
            AZ::Interface<IAudioSystem>::Get()->PushRequestBlocking(AZStd::move(reserveObject));
            AZ_Assert(m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID, "Failed to reserve audio object ID on AudioProxy '%s'", sObjectName);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteTrigger(TAudioControlID nTriggerID)
    {
        Audio::ObjectRequest::ExecuteTrigger execTrigger;
        execTrigger.m_triggerId = nTriggerID;
        execTrigger.m_owner = (m_ownerOverride ? m_ownerOverride : this);

        if (HasId())
        {
            execTrigger.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(execTrigger));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(execTrigger));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteSourceTrigger(TAudioControlID nTriggerID, const SAudioSourceInfo& rSourceInfo)
    {
        Audio::ObjectRequest::ExecuteSourceTrigger execSourceTrigger;
        execSourceTrigger.m_triggerId = nTriggerID;
        execSourceTrigger.m_sourceInfo = rSourceInfo;
        execSourceTrigger.m_owner = (m_ownerOverride ? m_ownerOverride : this);

        if (HasId())
        {
            execSourceTrigger.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(execSourceTrigger));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(execSourceTrigger));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::StopAllTriggers()
    {
        Audio::ObjectRequest::StopAllTriggers stopAll;
        stopAll.m_owner = (m_ownerOverride ? m_ownerOverride : this);
        stopAll.m_filterByOwner = true;

        if (HasId())
        {
            stopAll.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(stopAll));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(stopAll));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::StopTrigger(TAudioControlID nTriggerID)
    {
        Audio::ObjectRequest::StopTrigger stopTrigger;
        stopTrigger.m_triggerId = nTriggerID;
        stopTrigger.m_owner = (m_ownerOverride ? m_ownerOverride : this);

        if (HasId())
        {
            stopTrigger.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(stopTrigger));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(stopTrigger));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetSwitchState(TAudioControlID nSwitchID, TAudioSwitchStateID nStateID)
    {
        Audio::ObjectRequest::SetSwitchValue setSwitch;
        setSwitch.m_switchId = nSwitchID;
        setSwitch.m_stateId = nStateID;

        if (HasId())
        {
            setSwitch.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(setSwitch));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(setSwitch));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetRtpcValue(TAudioControlID nRtpcID, float fValue)
    {
        Audio::ObjectRequest::SetParameterValue setParameter;
        setParameter.m_parameterId = nRtpcID;
        setParameter.m_value = fValue;

        if (HasId())
        {
            setParameter.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(setParameter));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(setParameter));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetObstructionCalcType(ObstructionType eObstructionType)
    {
        const size_t obstructionType = static_cast<size_t>(eObstructionType);

        if (obstructionType < AZ_ARRAY_SIZE(ATLInternalControlIDs::OOCStateIDs))
        {
            SetSwitchState(ATLInternalControlIDs::ObstructionOcclusionCalcSwitchID, ATLInternalControlIDs::OOCStateIDs[obstructionType]);
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
                AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(setPosition));
            }
        }
        else
        {
            // If a SetPosition is queued during init, don't need to check threshold.
            m_oPosition = refPosition;

            // Make sure the forward/up directions are normalized
            m_oPosition.NormalizeForwardVec();
            m_oPosition.NormalizeUpVec();

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
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(setMultiPosition));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(setMultiPosition));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetEnvironmentAmount(TAudioEnvironmentID nEnvironmentID, float fValue)
    {
        Audio::ObjectRequest::SetEnvironmentValue setEnvironment;
        setEnvironment.m_environmentId = nEnvironmentID;
        setEnvironment.m_value = fValue;

        if (HasId())
        {
            setEnvironment.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(setEnvironment));
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
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(resetEnvironments));
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
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(resetParameters));
        }
        else
        {
            TryEnqueueRequest(AZStd::move(resetParameters));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Release()
    {
        // If it has ID, push a Release request.
        // When the proxy is still waiting for an ID to be assigned,
        // set a flag which indicates that after executing all the queued
        // events it should be released.
        // After calling Release(), the pointer should not be used
        // anymore since it's been recycled back to the AudioSystem's pool.
        if (HasId())
        {
            Audio::ObjectRequest::Release releaseObject;
            releaseObject.m_audioObjectId = m_nAudioObjectID;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(releaseObject));
        }
        else if (m_waitingForId)
        {
            m_releaseAtEndOfQueue = true;
            return;
        }

        Reset();
        AZ::Interface<IAudioSystem>::Get()->RecycleAudioProxy(this);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteQueuedRequests()
    {
        for (auto& requestVariant : m_queuedAudioRequests)
        {
            // Inject the audio object ID that was assigned, then kick off the requests.
            AZStd::visit(
                [this](auto&& request)
                {
                    request.m_audioObjectId = m_nAudioObjectID;
                },
                requestVariant);
        }
        AZ::Interface<IAudioSystem>::Get()->PushRequests(m_queuedAudioRequests);
        m_queuedAudioRequests.clear();

        if (m_releaseAtEndOfQueue)
        {
            Release();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioProxy::HasId() const
    {
        return (m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Reset()
    {
        m_nAudioObjectID = INVALID_AUDIO_OBJECT_ID;
        m_oPosition = {};
        m_ownerOverride = nullptr;
        m_releaseAtEndOfQueue = false;
        m_waitingForId = false;
        m_queuedAudioRequests.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Find Functors - Helpers for async queueing
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct FindSetSwitchValue
    {
        explicit FindSetSwitchValue(const ObjectRequest::SetSwitchValue& request)
            : m_audioSwitchId(request.m_switchId)
            , m_audioStateId(request.m_stateId)
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
        explicit FindSetParameterValue(const Audio::ObjectRequest::SetParameterValue& request)
            : m_audioParameterId(request.m_parameterId)
            , m_audioParameterValue(request.m_value)
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
        explicit FindSetPosition(const Audio::ObjectRequest::SetPosition& request)
            : m_position(request.m_position)
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
            if (auto setPositionRequest = AZStd::get_if<Audio::ObjectRequest::SetMultiplePositions>(&refRequest);
                setPositionRequest != nullptr)
            {
                // A multi-position request exists, setting a single position can't overwrite it.
                found = true;
            }

            return found;
        }

    private:
        const SATLWorldPosition& m_position;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct FindSetMultiplePositions
    {
        explicit FindSetMultiplePositions(const Audio::ObjectRequest::SetMultiplePositions& request)
            : m_params(request.m_params)
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
        const MultiPositionParams& m_params;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct FindSetEnvironmentValue
    {
        explicit FindSetEnvironmentValue(const Audio::ObjectRequest::SetEnvironmentValue& request)
            : m_audioEnvironmentId(request.m_environmentId)
            , m_audioEnvironmentValue(request.m_value)
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
        bool operator()(const AudioRequestVariant& refRequest)
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
        bool addFront = false;
        bool shouldAdd = AZStd::visit(
            [this, &addFront](auto&& request) -> bool
            {
                using RequestType = AZStd::decay_t<decltype(request)>;
                if constexpr (AZStd::is_same_v<RequestType, Audio::ObjectRequest::ExecuteTrigger>
                    || AZStd::is_same_v<RequestType, Audio::ObjectRequest::StopTrigger>
                    || AZStd::is_same_v<RequestType, Audio::ObjectRequest::ExecuteSourceTrigger>)
                {
                    // Always add these types of requests!
                    return true;
                }
                else if constexpr (AZStd::is_same_v<RequestType, Audio::ObjectRequest::SetPosition>)
                {
                    // Position should be set in front of queue, before other things happen.
                    addFront = true;
                    auto findIter = AZStd::find_if(m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindSetPosition(request));
                    return (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<RequestType, Audio::ObjectRequest::SetParameterValue>)
                {
                    auto findIter =
                        AZStd::find_if(m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindSetParameterValue(request));
                    return (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<RequestType, Audio::ObjectRequest::SetSwitchValue>)
                {
                    auto findIter = AZStd::find_if(m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindSetSwitchValue(request));
                    return (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<RequestType, Audio::ObjectRequest::SetEnvironmentValue>)
                {
                    auto findIter =
                        AZStd::find_if(m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindSetEnvironmentValue(request));
                    return (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<RequestType, Audio::ObjectRequest::StopAllTriggers>
                    || AZStd::is_same_v<RequestType, Audio::ObjectRequest::ResetParameters>
                    || AZStd::is_same_v<RequestType, Audio::ObjectRequest::ResetEnvironments>
                    || AZStd::is_same_v<RequestType, Audio::ObjectRequest::Release>)
                {
                    // Generic find
                    auto findIter = AZStd::find_if(m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindRequestType<RequestType>());
                    return (findIter == m_queuedAudioRequests.end());
                }
                else if constexpr (AZStd::is_same_v<RequestType, Audio::ObjectRequest::SetMultiplePositions>)
                {
                    // Position should be set in front of queue, before other things happen.
                    addFront = true;
                    auto findIter = AZStd::find_if(
                        m_queuedAudioRequests.begin(), m_queuedAudioRequests.end(), FindSetMultiplePositions(request));
                    return (findIter == m_queuedAudioRequests.end());
                }
                else
                {
                    // Not implemented yet!
                    // e.g. Audio::ObjectRequest::PrepareTrigger, Audio::ObjectRequest::UnprepareTrigger
                    return false;
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
