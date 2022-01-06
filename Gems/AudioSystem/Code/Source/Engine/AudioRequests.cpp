/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioAllocators.h>
#include <AudioInternalInterfaces.h>
#include <AudioLogger.h>

#define REQUEST_CASE_BLOCK(CLASS, ENUM, P_SOURCE, P_RESULT)                                        \
case ENUM:                                                                                         \
{                                                                                                  \
    (P_RESULT) = azcreate(CLASS##Internal<ENUM>, (static_cast<const CLASS<ENUM>* const>(P_SOURCE)), Audio::AudioSystemAllocator);   \
    break;                                                                                         \
}


#define AM_REQUEST_BLOCK(ENUM) REQUEST_CASE_BLOCK(SAudioManagerRequestData, ENUM, pExternalData, pResult)
#define ACM_REQUEST_BLOCK(ENUM) REQUEST_CASE_BLOCK(SAudioCallbackManagerRequestData, ENUM, pExternalData, pResult)
#define AO_REQUEST_BLOCK(ENUM) REQUEST_CASE_BLOCK(SAudioObjectRequestData, ENUM, pExternalData, pResult)
#define AL_REQUEST_BLOCK(ENUM) REQUEST_CASE_BLOCK(SAudioListenerRequestData, ENUM, pExternalData, pResult)

namespace Audio
{
    extern CAudioLogger g_audioLogger;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SAudioRequestDataInternal* ConvertToInternal(const SAudioRequestDataBase* const pExternalData)
    {
        const EAudioRequestType eRequestType = pExternalData->eRequestType;
        SAudioRequestDataInternal* pResult = nullptr;

        switch (eRequestType)
        {
            case eART_AUDIO_MANAGER_REQUEST:
            {
                auto const pBase = static_cast<const SAudioManagerRequestDataBase*>(pExternalData);
                AZ_Assert(pBase, "AudioRequests ConvertToInternal - Cast to audio manager request base failed!");

                switch (pBase->eType)
                {
                    AM_REQUEST_BLOCK(eAMRT_INIT_AUDIO_IMPL)
                    AM_REQUEST_BLOCK(eAMRT_RELEASE_AUDIO_IMPL)
                    AM_REQUEST_BLOCK(eAMRT_RESERVE_AUDIO_OBJECT_ID)
                    AM_REQUEST_BLOCK(eAMRT_CREATE_SOURCE)
                    AM_REQUEST_BLOCK(eAMRT_DESTROY_SOURCE)
                    AM_REQUEST_BLOCK(eAMRT_PARSE_CONTROLS_DATA)
                    AM_REQUEST_BLOCK(eAMRT_PARSE_PRELOADS_DATA)
                    AM_REQUEST_BLOCK(eAMRT_CLEAR_CONTROLS_DATA)
                    AM_REQUEST_BLOCK(eAMRT_CLEAR_PRELOADS_DATA)
                    AM_REQUEST_BLOCK(eAMRT_PRELOAD_SINGLE_REQUEST)
                    AM_REQUEST_BLOCK(eAMRT_UNLOAD_SINGLE_REQUEST)
                    AM_REQUEST_BLOCK(eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE)
                    AM_REQUEST_BLOCK(eAMRT_REFRESH_AUDIO_SYSTEM)
                    AM_REQUEST_BLOCK(eAMRT_LOSE_FOCUS)
                    AM_REQUEST_BLOCK(eAMRT_GET_FOCUS)
                    AM_REQUEST_BLOCK(eAMRT_MUTE_ALL)
                    AM_REQUEST_BLOCK(eAMRT_UNMUTE_ALL)
                    AM_REQUEST_BLOCK(eAMRT_STOP_ALL_SOUNDS)
                    AM_REQUEST_BLOCK(eAMRT_DRAW_DEBUG_INFO)
                    AM_REQUEST_BLOCK(eAMRT_CHANGE_LANGUAGE)
                    AM_REQUEST_BLOCK(eAMRT_SET_AUDIO_PANNING_MODE)
                    default:
                    {
                        g_audioLogger.Log(eALT_ERROR, "Unknown audio manager request type (%d)", pBase->eType);
                        AZ_Assert(false, "Unknown audio manager request type (%d)", pBase->eType);
                        break;
                    }
                }
                break;
            }
            case eART_AUDIO_CALLBACK_MANAGER_REQUEST:
            {
                auto const pBase = static_cast<const SAudioCallbackManagerRequestDataBase*>(pExternalData);
                AZ_Assert(pBase, "AudioRequests ConvertToInternal - Cast to audio callback manager request base failed!");

                switch (pBase->eType)
                {
                    ACM_REQUEST_BLOCK(eACMRT_REPORT_STARTED_EVENT)
                    ACM_REQUEST_BLOCK(eACMRT_REPORT_FINISHED_EVENT)
                    ACM_REQUEST_BLOCK(eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE)
                    default:
                    {
                        g_audioLogger.Log(eALT_ERROR, "Unknown audio callback manager request type (%d)", pBase->eType);
                        AZ_Assert(false, "Unknown audio callback manager request type (%d)", pBase->eType);
                        break;
                    }
                }
                break;
            }
            case eART_AUDIO_OBJECT_REQUEST:
            {
                auto const pBase = static_cast<const SAudioObjectRequestDataBase*>(pExternalData);
                AZ_Assert(pBase, "AudioRequests ConvertToInternal - Cast to audio object request base failed!");

                switch (pBase->eType)
                {
                    AO_REQUEST_BLOCK(eAORT_EXECUTE_TRIGGER)
                    AO_REQUEST_BLOCK(eAORT_PREPARE_TRIGGER)
                    AO_REQUEST_BLOCK(eAORT_UNPREPARE_TRIGGER)
                    AO_REQUEST_BLOCK(eAORT_STOP_TRIGGER)
                    AO_REQUEST_BLOCK(eAORT_STOP_ALL_TRIGGERS)
                    AO_REQUEST_BLOCK(eAORT_SET_POSITION)
                    AO_REQUEST_BLOCK(eAORT_SET_SWITCH_STATE)
                    AO_REQUEST_BLOCK(eAORT_SET_RTPC_VALUE)
                    AO_REQUEST_BLOCK(eAORT_SET_ENVIRONMENT_AMOUNT)
                    AO_REQUEST_BLOCK(eAORT_RESET_ENVIRONMENTS)
                    AO_REQUEST_BLOCK(eAORT_RESET_RTPCS)
                    AO_REQUEST_BLOCK(eAORT_RELEASE_OBJECT)
                    AO_REQUEST_BLOCK(eAORT_EXECUTE_SOURCE_TRIGGER)
                    AO_REQUEST_BLOCK(eAORT_SET_MULTI_POSITIONS)
                    default:
                    {
                        g_audioLogger.Log(eALT_ERROR, "Unknown audio object request type (%d)", pBase->eType);
                        AZ_Assert(false, "Unknown audio object request type (%d)", pBase->eType);
                        break;
                    }
                }
                break;
            }
            case eART_AUDIO_LISTENER_REQUEST:
            {
                auto const pBase = static_cast<const SAudioListenerRequestDataBase*>(pExternalData);
                AZ_Assert(pBase, "AudioRequests ConvertToInternal - Cast to audio listener request base failed!");

                switch (pBase->eType)
                {
                    AL_REQUEST_BLOCK(eALRT_SET_POSITION)
                    default:
                    {
                        g_audioLogger.Log(eALT_ERROR, "Unknown audio listener request type (%d)", pBase->eType);
                        AZ_Assert(false, "Unknown audio listener request type (%d)", pBase->eType);
                        break;
                    }
                }
                break;
            }
            default:
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown audio request type (%d)", eRequestType);
                AZ_Assert(false, "Unknown audio request type (%d)", eRequestType);
                break;
            }
        }

        return pResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void SAudioRequestDataInternal::Release()
    {
        const int nCount = m_nRefCounter.fetch_sub(1, AZStd::memory_order_acq_rel) - 1; // because we get the original value back
        AZ_Assert(nCount >= 0, "AudioRequests Release - Decremented reference counter too many times!");

        if (nCount == 0)
        {
            azdestroy(this, Audio::AudioSystemAllocator);
        }
        else if (nCount < 0)
        {
            g_audioLogger.Log(eALT_ASSERT, "Deleting Reference Counted Object Twice!");
        }
    }
} // namespace Audio
