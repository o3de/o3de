/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AudioProxy.h"

#include <ATLCommon.h>
#include <SoundCVars.h>
#include <AudioLogger.h>
#include <MathConversion.h>

#include <ISystem.h>

namespace Audio
{
    extern CAudioLogger g_audioLogger;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioProxy::CAudioProxy()
        : m_nAudioObjectID(INVALID_AUDIO_OBJECT_ID)
        , m_nFlags(eAPF_NONE)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioProxy::~CAudioProxy()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) != 0)
        {
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::RemoveRequestListener, &CAudioProxy::OnAudioEvent, this);
        }

        AZ_Assert(m_nAudioObjectID == INVALID_AUDIO_OBJECT_ID, "Expected AudioObjectID [%d] to be invalid when the audio proxy is destructed.", m_nAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Initialize(const char* const sObjectName, const bool bInitAsync /* = true */)
    {
        auto audioProxiesInitType = static_cast<AZ::u32>(Audio::CVars::s_AudioProxiesInitType);
        if ((bInitAsync && audioProxiesInitType == 0) || audioProxiesInitType == 2)
        {
            if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
            {
                // Add the request listener to receive callback when the audio object ID has been registered with middleware...
                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::AddRequestListener, &CAudioProxy::OnAudioEvent, this, eART_AUDIO_MANAGER_REQUEST, eAMRT_RESERVE_AUDIO_OBJECT_ID);

                m_nFlags |= eAPF_WAITING_FOR_ID;

                SAudioRequest oRequest;
                SAudioManagerRequestData<eAMRT_RESERVE_AUDIO_OBJECT_ID> oRequestData(&m_nAudioObjectID, sObjectName);
                oRequest.nFlags = (eARF_PRIORITY_HIGH | eARF_SYNC_CALLBACK);
                oRequest.pOwner = this;
                oRequest.pData = &oRequestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
            }
            else
            {
                SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_INITIALIZE);
                oQueuedCommand.sValue = sObjectName;
                TryAddQueuedCommand(oQueuedCommand);
            }
        }
        else
        {
            SAudioRequest oRequest;
            SAudioManagerRequestData<eAMRT_RESERVE_AUDIO_OBJECT_ID> oRequestData(&m_nAudioObjectID, sObjectName);
            oRequest.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);
            oRequest.pData = &oRequestData;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oRequest);

    #if !defined(AUDIO_RELEASE)
            if (m_nAudioObjectID == INVALID_AUDIO_OBJECT_ID)
            {
                g_audioLogger.Log(eALT_ASSERT, "Failed to reserve audio object ID on AudioProxy (%s)!", sObjectName);
            }
    #endif // !AUDIO_RELEASE
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteSourceTrigger(
        TAudioControlID nTriggerID,
        const SAudioSourceInfo& rSourceInfo,
        const SAudioCallBackInfos& rCallbackInfos /* = SAudioCallBackInfos::GetEmptyObject() */)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            AZ_Assert(m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID, "Invalid AudioObjectID found when an audio proxy is executing a source trigger!");

            SAudioRequest oRequest;
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = rCallbackInfos.nRequestFlags;

            SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER> oRequestData(nTriggerID, rSourceInfo);
            oRequest.pOwner = (rCallbackInfos.pObjectToNotify != nullptr) ? rCallbackInfos.pObjectToNotify : this;
            oRequest.pUserData = rCallbackInfos.pUserData;
            oRequest.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            oRequest.pData = &oRequestData;
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_EXECUTE_SOURCE_TRIGGER);
            oQueuedCommand.nTriggerID = nTriggerID;
            oQueuedCommand.pOwnerOverride = rCallbackInfos.pObjectToNotify;
            oQueuedCommand.pUserData = rCallbackInfos.pUserData;
            oQueuedCommand.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            oQueuedCommand.nRequestFlags = rCallbackInfos.nRequestFlags;
            oQueuedCommand.rSourceInfo = rSourceInfo;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteTrigger(
        const TAudioControlID nTriggerID,
        const SAudioCallBackInfos& rCallbackInfos /* = SAudioCallBackInfos::GetEmptyObject() */)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            AZ_Assert(m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID, "Invalid AudioObjectID found when an audio proxy is executing a trigger!");

            SAudioRequest oRequest;
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = rCallbackInfos.nRequestFlags;

            SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> oRequestData(nTriggerID, 0.0f);
            oRequest.pOwner = (rCallbackInfos.pObjectToNotify != nullptr) ? rCallbackInfos.pObjectToNotify : this;
            oRequest.pUserData = rCallbackInfos.pUserData;
            oRequest.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            oRequest.pData = &oRequestData;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_EXECUTE_TRIGGER);
            oQueuedCommand.nTriggerID = nTriggerID;
            oQueuedCommand.pOwnerOverride = rCallbackInfos.pObjectToNotify;
            oQueuedCommand.pUserData = rCallbackInfos.pUserData;
            oQueuedCommand.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            oQueuedCommand.nRequestFlags = rCallbackInfos.nRequestFlags;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::StopAllTriggers()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_STOP_ALL_TRIGGERS> oRequestData;
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_STOP_ALL_TRIGGERS);
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::StopTrigger(const TAudioControlID nTriggerID)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_STOP_TRIGGER> oRequestData(nTriggerID);
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_STOP_TRIGGER);
            oQueuedCommand.nTriggerID = nTriggerID;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_SWITCH_STATE> oRequestData(nSwitchID, nStateID);
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_SWITCH_STATE);
            oQueuedCommand.nSwitchID = nSwitchID;
            oQueuedCommand.nStateID = nStateID;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetRtpcValue(const TAudioControlID nRtpcID, const float fValue)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> oRequestData(nRtpcID, fValue);
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_RTPC_VALUE);
            oQueuedCommand.nRtpcID = nRtpcID;
            oQueuedCommand.fValue = fValue;
            TryAddQueuedCommand(oQueuedCommand);
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
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            // Update position only if the delta exceeds a given value.
            if (Audio::CVars::s_PositionUpdateThreshold <= 0.f      // <-- no gating
                || !refPosition.GetPositionVec().IsClose(m_oPosition.GetPositionVec(), Audio::CVars::s_PositionUpdateThreshold))
            {
                m_oPosition = refPosition;

                // Make sure the forward/up directions are normalized
                m_oPosition.NormalizeForwardVec();
                m_oPosition.NormalizeUpVec();

                SAudioRequest oRequest;
                SAudioObjectRequestData<eAORT_SET_POSITION> oRequestData(m_oPosition);
                oRequest.nAudioObjectID = m_nAudioObjectID;
                oRequest.nFlags = eARF_PRIORITY_NORMAL;
                oRequest.pData = &oRequestData;
                oRequest.pOwner = this;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
            }
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_POSITION);
            oQueuedCommand.oPosition = refPosition;
            TryAddQueuedCommand(oQueuedCommand);
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
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest request;
            SAudioObjectRequestData<eAORT_SET_MULTI_POSITIONS> requestData(params);
            request.nAudioObjectID = m_nAudioObjectID;
            request.nFlags = eARF_PRIORITY_NORMAL;
            request.pData = &requestData;
            request.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_MULTI_POSITIONS);
            oQueuedCommand.oMultiPosParams= params;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fValue)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_ENVIRONMENT_AMOUNT> oRequestData(nEnvironmentID, fValue);
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_ENVIRONMENT_AMOUNT);
            oQueuedCommand.nEnvironmentID = nEnvironmentID;
            oQueuedCommand.fValue = fValue;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetCurrentEnvironments()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) != 0)
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_CURRENT_ENVIRONMENTS);
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ClearEnvironments()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_RESET_ENVIRONMENTS> oRequestData;
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            TryAddQueuedCommand(SQueuedAudioCommand(eQACT_CLEAR_ENVIRONMENTS));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ResetRtpcValues()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest request;
            SAudioObjectRequestData<eAORT_RESET_RTPCS> requestData;
            request.nAudioObjectID = m_nAudioObjectID;
            request.nFlags = eARF_PRIORITY_NORMAL;
            request.pData = &requestData;
            request.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
        }
        else
        {
            TryAddQueuedCommand(SQueuedAudioCommand(eQACT_CLEAR_RTPCS));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Release()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            Reset();
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::FreeAudioProxy, this);
        }
        else
        {
            TryAddQueuedCommand(SQueuedAudioCommand(eQACT_RELEASE));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Reset()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            if (m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID)
            {
                // Request must be asynchronous and lowest priority!
                SAudioRequest oRequest;
                SAudioObjectRequestData<eAORT_RELEASE_OBJECT> oRequestData;
                oRequest.nAudioObjectID = m_nAudioObjectID;
                oRequest.pData = &oRequestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);

                m_nAudioObjectID = INVALID_AUDIO_OBJECT_ID;
            }

            m_oPosition = SATLWorldPosition();
        }
        else
        {
            TryAddQueuedCommand(SQueuedAudioCommand(eQACT_RESET));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteQueuedCommands()
    {
        // Remove the request listener once the audio system has properly reserved the audio object ID for this proxy.
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::RemoveRequestListener, &CAudioProxy::OnAudioEvent, this);

        m_nFlags &= ~eAPF_WAITING_FOR_ID;

        if (!m_aQueuedAudioCommands.empty())
        {
            TQueuedAudioCommands::iterator Iter(m_aQueuedAudioCommands.begin());
            TQueuedAudioCommands::const_iterator IterEnd(m_aQueuedAudioCommands.end());
            for (; Iter != IterEnd; ++Iter)
            {
                const SQueuedAudioCommand& refCommand = (*Iter);
                // TODO: pass the refCommand.pUserData to all methods (maybe pass refCommand as a parameter to all functions)

                switch (refCommand.eType)
                {
                    case eQACT_EXECUTE_TRIGGER:
                    {
                        const SAudioCallBackInfos callbackInfos(refCommand.pOwnerOverride, refCommand.pUserData, refCommand.pUserDataOwner, refCommand.nRequestFlags);
                        ExecuteTrigger(refCommand.nTriggerID, callbackInfos);
                        break;
                    }
                    case eQACT_EXECUTE_SOURCE_TRIGGER:
                    {
                        const SAudioCallBackInfos callbackInfos(refCommand.pOwnerOverride, refCommand.pUserData, refCommand.pUserDataOwner, refCommand.nRequestFlags);
                        ExecuteSourceTrigger(refCommand.nTriggerID, refCommand.rSourceInfo, callbackInfos);
                        break;
                    }
                    case eQACT_STOP_TRIGGER:
                    {
                        StopTrigger(refCommand.nTriggerID);
                        break;
                    }
                    case eQACT_SET_SWITCH_STATE:
                    {
                        SetSwitchState(refCommand.nSwitchID, refCommand.nStateID);
                        break;
                    }
                    case eQACT_SET_RTPC_VALUE:
                    {
                        SetRtpcValue(refCommand.nRtpcID, refCommand.fValue);
                        break;
                    }
                    case eQACT_SET_POSITION:
                    {
                        SetPosition(refCommand.oPosition);
                        break;
                    }
                    case eQACT_SET_ENVIRONMENT_AMOUNT:
                    {
                        SetEnvironmentAmount(refCommand.nEnvironmentID, refCommand.fValue);
                        break;
                    }
                    case eQACT_SET_CURRENT_ENVIRONMENTS:
                    {
                        SetCurrentEnvironments();
                        break;
                    }
                    case eQACT_CLEAR_ENVIRONMENTS:
                    {
                        ClearEnvironments();
                        break;
                    }
                    case eQACT_CLEAR_RTPCS:
                    {
                        ResetRtpcValues();
                        break;
                    }
                    case eQACT_RESET:
                    {
                        Reset();
                        break;
                    }
                    case eQACT_RELEASE:
                    {
                        Release();
                        break;
                    }
                    case eQACT_INITIALIZE:
                    {
                        Initialize(refCommand.sValue.c_str(), true);
                        break;
                    }
                    case eQACT_STOP_ALL_TRIGGERS:
                    {
                        StopAllTriggers();
                        break;
                    }
                    case eQACT_SET_MULTI_POSITIONS:
                    {
                        SetMultiplePositions(refCommand.oMultiPosParams);
                        break;
                    }
                    default:
                    {
                        g_audioLogger.Log(eALT_ASSERT, "Unknown command type in CAudioProxy::ExecuteQueuedCommands!");
                        break;
                    }
                }

                if ((m_nFlags & eAPF_WAITING_FOR_ID) != 0)
                {
                    // An Initialize command was queued up.
                    // Here we need to keep all commands after the Initialize.
                    break;
                }
            }

            if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
            {
                m_aQueuedAudioCommands.clear();
            }
            else
            {
                // An Initialize command was queued up.
                // Here we need to keep queued commands except for Reset and Initialize.
                Iter = m_aQueuedAudioCommands.begin();

                while (Iter != IterEnd)
                {
                    const SQueuedAudioCommand& refCommand = (*Iter);

                    if (refCommand.eType == eQACT_RESET || refCommand.eType == eQACT_INITIALIZE)
                    {
                        Iter = m_aQueuedAudioCommands.erase(Iter);
                        IterEnd = m_aQueuedAudioCommands.end();
                        continue;
                    }

                    ++Iter;
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::TryAddQueuedCommand(const SQueuedAudioCommand& refCommand)
    {
        bool bAdd = true;

        switch (refCommand.eType)
        {
            case eQACT_EXECUTE_TRIGGER:
            case eQACT_EXECUTE_SOURCE_TRIGGER:
            case eQACT_STOP_TRIGGER:
            {
                // These type of commands get always pushed back!
                break;
            }
            case eQACT_SET_SWITCH_STATE:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetSwitchState(refCommand.nSwitchID, refCommand.nStateID)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_RTPC_VALUE:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetRtpcValue(refCommand.nRtpcID, refCommand.fValue)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_POSITION:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetPosition(refCommand.oPosition)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_ENVIRONMENT_AMOUNT:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetEnvironmentAmount(refCommand.nEnvironmentID, refCommand.fValue)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_CURRENT_ENVIRONMENTS:
            case eQACT_CLEAR_ENVIRONMENTS:
            case eQACT_CLEAR_RTPCS:
            case eQACT_RELEASE:
            {
                // These type of commands don't need another instance!
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindCommand(refCommand.eType)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_RESET:
            {
                for (const SQueuedAudioCommand& rLocalCommand : m_aQueuedAudioCommands)
                {
                    if (rLocalCommand.eType == eQACT_RELEASE)
                    {
                        // If eQACT_RELEASE is already queued up then there is no need for adding a eQACT_RESET command.
                        bAdd = false;
                        break;
                    }
                }

                if (!bAdd)
                {
                    // If this proxy is resetting then there is no need for any pending commands.
                    m_aQueuedAudioCommands.clear();
                }
                break;
            }
            case eQACT_INITIALIZE:
            {
                // There must be only 1 Initialize command be queued up.
                m_aQueuedAudioCommands.clear();

                // Precede the Initialization with a Reset command to release the pending audio object.
                m_aQueuedAudioCommands.push_back(SQueuedAudioCommand(eQACT_RESET));
                break;
            }
            case eQACT_STOP_ALL_TRIGGERS:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    // only add if the last request is different...
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.end() - 1, m_aQueuedAudioCommands.end(), SFindCommand(refCommand.eType)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_MULTI_POSITIONS:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    // Find+Update or Add.
                    // Can morph a SetPosition command into a Multi-Position command.
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetMultiplePositions(refCommand.oMultiPosParams)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            default:
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown queued command type [%d] in CAudioProxy::TryAddQueuedCommand!", refCommand.eType);
                bAdd = false;
                break;
            }
        }

        if (bAdd)
        {
            if (refCommand.eType == eQACT_SET_POSITION || refCommand.eType == eQACT_SET_MULTI_POSITIONS)
            {
                // Make sure we set position first!
                m_aQueuedAudioCommands.push_front(refCommand);
            }
            else
            {
                m_aQueuedAudioCommands.push_back(refCommand);
            }
        }
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::OnAudioEvent(const SAudioRequestInfo* const pAudioRequestInfo)
    {
        if (pAudioRequestInfo->eResult == eARR_SUCCESS && pAudioRequestInfo->eAudioRequestType == eART_AUDIO_MANAGER_REQUEST)
        {
            const auto eAudioManagerRequestType = static_cast<const EAudioManagerRequestType>(pAudioRequestInfo->nSpecificAudioRequest);

            if (eAudioManagerRequestType == eAMRT_RESERVE_AUDIO_OBJECT_ID)
            {
                auto const pAudioProxy = static_cast<CAudioProxy*>(pAudioRequestInfo->pOwner);

                if (pAudioProxy)
                {
                    pAudioProxy->ExecuteQueuedCommands();
                }
            }
        }
    }

} // namespace Audio
