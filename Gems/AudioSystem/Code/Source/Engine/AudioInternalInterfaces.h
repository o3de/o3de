/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioSystem.h>
#include <IAudioSystemImplementation.h>

#include <platform.h>
#include <smartptr.h>

#if !defined(AUDIO_RELEASE)
    #include <sstream>
#endif // !AUDIO_RELEASE

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioEventResult : TATLEnumFlagsType
    {
        eAER_NONE           = 0,
        eAER_SUCCESS        = 1,
        eAER_FAILED         = 2,
        eAER_STILL_LOADING  = 3,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioRequestInfoFlags : TATLEnumFlagsType
    {
        eARIF_NONE                  = 0,
        eARIF_WAITING_FOR_REMOVAL   = AUDIO_BIT(0),
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioEventListener
    {
        SAudioEventListener()
            : m_callbackOwner(nullptr)
            , m_fnOnEvent(nullptr)
            , m_requestType(eART_AUDIO_ALL_REQUESTS)
            , m_specificRequestMask(ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS)
        {}

        SAudioEventListener(const SAudioEventListener& other)
            : m_callbackOwner(other.m_callbackOwner)
            , m_fnOnEvent(other.m_fnOnEvent)
            , m_requestType(other.m_requestType)
            , m_specificRequestMask(other.m_specificRequestMask)
        {}

        SAudioEventListener& operator=(const SAudioEventListener& other)
        {
            m_callbackOwner = other.m_callbackOwner;
            m_fnOnEvent = other.m_fnOnEvent;
            m_requestType = other.m_requestType;
            m_specificRequestMask = other.m_specificRequestMask;
            return *this;
        }

        bool operator==(const SAudioEventListener& rhs) const
        {
            return (m_callbackOwner == rhs.m_callbackOwner
                && m_fnOnEvent == rhs.m_fnOnEvent
                && m_requestType == rhs.m_requestType
                && m_specificRequestMask == rhs.m_specificRequestMask);
        }

        const void* m_callbackOwner;
        AudioRequestCallbackType m_fnOnEvent;
        EAudioRequestType m_requestType;
        TATLEnumFlagsType m_specificRequestMask;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioRequestDataInternal
        : public _i_multithread_reference_target_t
    {
        explicit SAudioRequestDataInternal(EAudioRequestType eType = eART_NONE)
            : eRequestType(eType)
        {}

        ~SAudioRequestDataInternal() override {}

        void Release() override;

        const EAudioRequestType eRequestType;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Manager Requests (Internal)
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioManagerRequestDataInternalBase
        : public SAudioRequestDataInternal
    {
        explicit SAudioManagerRequestDataInternalBase(const EAudioManagerRequestType eAMRType)
            : SAudioRequestDataInternal(eART_AUDIO_MANAGER_REQUEST)
            , eType(eAMRType)
        {}

        ~SAudioManagerRequestDataInternalBase() override {}

        const EAudioManagerRequestType eType;

    private:
        SAudioManagerRequestDataInternalBase();
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <EAudioManagerRequestType T>
    struct SAudioManagerRequestDataInternal
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<T>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
        {}

        ~SAudioManagerRequestDataInternal() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_INIT_AUDIO_IMPL>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_INIT_AUDIO_IMPL>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_INIT_AUDIO_IMPL>() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_RELEASE_AUDIO_IMPL>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_RELEASE_AUDIO_IMPL>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_RELEASE_AUDIO_IMPL>() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_RESERVE_AUDIO_OBJECT_ID>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_RESERVE_AUDIO_OBJECT_ID>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , pObjectID(pAMRData->pObjectID)
#if !defined(AUDIO_RELEASE)
            , sObjectName(pAMRData->sObjectName)
#endif // !AUDIO_RELEASE
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_RESERVE_AUDIO_OBJECT_ID>()override {}

        TAudioObjectID* const pObjectID;

#if !defined(AUDIO_RELEASE)
        AZStd::string sObjectName;
#endif // !AUDIO_RELEASE
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_CREATE_SOURCE>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_CREATE_SOURCE>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , m_sourceConfig(pAMRData->m_sourceConfig)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_CREATE_SOURCE>() override {}

        SAudioInputConfig m_sourceConfig;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestDataInternal<eAMRT_DESTROY_SOURCE>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_DESTROY_SOURCE>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , m_sourceId(pAMRData->m_sourceId)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_DESTROY_SOURCE>() override {}

        TAudioSourceId m_sourceId;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_PARSE_CONTROLS_DATA>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , sControlsPath(pAMRData->sFolderPath)
            , eDataScope(pAMRData->eDataScope)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_PARSE_CONTROLS_DATA>()override {}

        const AZStd::string sControlsPath;
        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_PARSE_PRELOADS_DATA>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_PARSE_PRELOADS_DATA>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , sControlsPath(pAMRData->sFolderPath)
            , eDataScope(pAMRData->eDataScope)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_PARSE_PRELOADS_DATA>()override {}

        const AZStd::string sControlsPath;
        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_CLEAR_CONTROLS_DATA>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_CLEAR_CONTROLS_DATA>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , eDataScope(pAMRData->eDataScope)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_CLEAR_CONTROLS_DATA>()override {}

        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_CLEAR_PRELOADS_DATA>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_CLEAR_PRELOADS_DATA>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , eDataScope(pAMRData->eDataScope)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_CLEAR_PRELOADS_DATA>()override {}

        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_PRELOAD_SINGLE_REQUEST>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , nPreloadRequest(pAMRData->nPreloadRequestID)
            , bAutoLoadOnly(pAMRData->bAutoLoadOnly)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_PRELOAD_SINGLE_REQUEST>()override {}

        const TAudioPreloadRequestID nPreloadRequest;
        const bool bAutoLoadOnly;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_UNLOAD_SINGLE_REQUEST>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_UNLOAD_SINGLE_REQUEST>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , nPreloadRequest(pAMRData->nPreloadRequestID)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_UNLOAD_SINGLE_REQUEST>()override {}

        const TAudioPreloadRequestID nPreloadRequest;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , eDataScope(pAMRData->eDataScope)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE>()override {}

        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_REFRESH_AUDIO_SYSTEM>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_REFRESH_AUDIO_SYSTEM>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , m_controlsPath(pAMRData->m_controlsPath)
            , m_levelName(pAMRData->m_levelName)
            , m_levelPreloadId(pAMRData->m_levelPreloadId)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_REFRESH_AUDIO_SYSTEM>()override {}

        const AZStd::string m_controlsPath;
        const AZStd::string m_levelName;
        const TAudioPreloadRequestID m_levelPreloadId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioManagerRequestDataInternal<eAMRT_CHANGE_LANGUAGE>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_CHANGE_LANGUAGE>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_CHANGE_LANGUAGE>()override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestDataInternal<eAMRT_SET_AUDIO_PANNING_MODE>
        : public SAudioManagerRequestDataInternalBase
    {
        SAudioManagerRequestDataInternal(const SAudioManagerRequestData<eAMRT_SET_AUDIO_PANNING_MODE>* const pAMRData)
            : SAudioManagerRequestDataInternalBase(pAMRData->eType)
            , m_panningMode(pAMRData->m_panningMode)
        {}

        ~SAudioManagerRequestDataInternal<eAMRT_SET_AUDIO_PANNING_MODE>() override {}

        PanningMode m_panningMode;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Callback Manager Requests (Internal)
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioCallbackManagerRequestDataInternalBase
        : public SAudioRequestDataInternal
    {
        explicit SAudioCallbackManagerRequestDataInternalBase(const EAudioCallbackManagerRequestType eAMRType)
            : SAudioRequestDataInternal(eART_AUDIO_CALLBACK_MANAGER_REQUEST)
            , eType(eAMRType)
        {}

        ~SAudioCallbackManagerRequestDataInternalBase() override {}

        const EAudioCallbackManagerRequestType eType;

    private:
        SAudioCallbackManagerRequestDataInternalBase();
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <EAudioCallbackManagerRequestType T>
    struct SAudioCallbackManagerRequestDataInternal
        : public SAudioCallbackManagerRequestDataInternalBase
    {
        SAudioCallbackManagerRequestDataInternal(const SAudioCallbackManagerRequestData<T>* const pAMRData)
            : SAudioCallbackManagerRequestDataInternalBase(pAMRData->eType)
        {}

        ~SAudioCallbackManagerRequestDataInternal() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_STARTED_EVENT>
        : public SAudioCallbackManagerRequestDataInternalBase
    {
        SAudioCallbackManagerRequestDataInternal(const SAudioCallbackManagerRequestData<eACMRT_REPORT_STARTED_EVENT>* const pACMRData)
            : SAudioCallbackManagerRequestDataInternalBase(pACMRData->eType)
            , nEventID(pACMRData->nEventID)
        {}

        ~SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_STARTED_EVENT>()override {}

        const TAudioEventID nEventID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_FINISHED_EVENT>
        : public SAudioCallbackManagerRequestDataInternalBase
    {
        SAudioCallbackManagerRequestDataInternal(const SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT>* const pACMRData)
            : SAudioCallbackManagerRequestDataInternalBase(pACMRData->eType)
            , nEventID(pACMRData->nEventID)
            , bSuccess(pACMRData->bSuccess)
        {}

        ~SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_FINISHED_EVENT>()override {}

        const TAudioEventID nEventID;
        const bool bSuccess;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE>
        : public SAudioCallbackManagerRequestDataInternalBase
    {
        SAudioCallbackManagerRequestDataInternal(const SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE>* const pACMRData)
            : SAudioCallbackManagerRequestDataInternalBase(pACMRData->eType)
            , nAudioTriggerID(pACMRData->nAudioTriggerID)
        {}

        ~SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE>()override {}

        const TAudioControlID nAudioTriggerID;
    };



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Object Requests (Internal)
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioObjectRequestDataInternalBase
        : public SAudioRequestDataInternal
    {
        explicit SAudioObjectRequestDataInternalBase(const EAudioObjectRequestType eAORType)
            : SAudioRequestDataInternal(eART_AUDIO_OBJECT_REQUEST)
            , eType(eAORType)
        {}

        ~SAudioObjectRequestDataInternalBase() override {}

        const EAudioObjectRequestType eType;

    private:
        SAudioObjectRequestDataInternalBase();
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <EAudioObjectRequestType T>
    struct SAudioObjectRequestDataInternal
        : public SAudioObjectRequestDataInternalBase
    {
        SAudioObjectRequestDataInternal()
            : SAudioObjectRequestDataInternalBase(T)
        {}

        ~SAudioObjectRequestDataInternal() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_EXECUTE_TRIGGER>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , nTriggerID(pAORData->nTriggerID)
            , fTimeUntilRemovalInMS(pAORData->fTimeUntilRemovalInMS)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_EXECUTE_TRIGGER>()override {}

        const TAudioControlID nTriggerID;
        const float fTimeUntilRemovalInMS;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_PREPARE_TRIGGER>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_PREPARE_TRIGGER>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , nTriggerID(pAORData->nTriggerID)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_PREPARE_TRIGGER>()override {}

        const TAudioControlID nTriggerID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_UNPREPARE_TRIGGER>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_UNPREPARE_TRIGGER>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , nTriggerID(pAORData->nTriggerID)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_UNPREPARE_TRIGGER>()override {}

        const TAudioControlID nTriggerID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_STOP_TRIGGER>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_STOP_TRIGGER>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , nTriggerID(pAORData->nTriggerID)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_STOP_TRIGGER>()override {}

        const TAudioControlID nTriggerID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_STOP_ALL_TRIGGERS>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_STOP_ALL_TRIGGERS>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , m_filterByOwner(pAORData->m_filterByOwner)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_STOP_ALL_TRIGGERS>()override {}

        const bool m_filterByOwner;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_SET_POSITION>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_SET_POSITION>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , oPosition(pAORData->oPosition)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_SET_POSITION>()override {}

        const SATLWorldPosition oPosition;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_SET_RTPC_VALUE>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_SET_RTPC_VALUE>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , nControlID(pAORData->nControlID)
            , fValue(pAORData->fValue)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_SET_RTPC_VALUE>()override {}

        const TAudioControlID nControlID;
        const float fValue;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_SET_SWITCH_STATE>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_SET_SWITCH_STATE>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , nSwitchID(pAORData->nSwitchID)
            , nStateID(pAORData->nStateID)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_SET_SWITCH_STATE>()override {}

        const TAudioControlID nSwitchID;
        const TAudioSwitchStateID nStateID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_SET_ENVIRONMENT_AMOUNT>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_SET_ENVIRONMENT_AMOUNT>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , nEnvironmentID(pAORData->nEnvironmentID)
            , fAmount(pAORData->fAmount)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_SET_ENVIRONMENT_AMOUNT>()override {}

        const TAudioEnvironmentID nEnvironmentID;
        const float fAmount;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_RESET_ENVIRONMENTS>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_RESET_ENVIRONMENTS>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_RESET_ENVIRONMENTS>()override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_RESET_RTPCS>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_RESET_RTPCS>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_RESET_RTPCS>() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_RELEASE_OBJECT>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_RELEASE_OBJECT>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_RELEASE_OBJECT>()override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_EXECUTE_SOURCE_TRIGGER>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , m_triggerId(pAORData->m_triggerId)
            , m_sourceInfo(pAORData->m_sourceInfo)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_EXECUTE_SOURCE_TRIGGER>() override {}

        const TAudioControlID m_triggerId;
        const SAudioSourceInfo m_sourceInfo;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestDataInternal<eAORT_SET_MULTI_POSITIONS>
        : public SAudioObjectRequestDataInternalBase
    {
        explicit SAudioObjectRequestDataInternal(const SAudioObjectRequestData<eAORT_SET_MULTI_POSITIONS>* const pAORData)
            : SAudioObjectRequestDataInternalBase(pAORData->eType)
            , m_params(pAORData->m_params)
        {}

        ~SAudioObjectRequestDataInternal<eAORT_SET_MULTI_POSITIONS>() override {}

        MultiPositionParams m_params;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Listener Requests (Internal)
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioListenerRequestDataInternalBase
        : public SAudioRequestDataInternal
    {
        explicit SAudioListenerRequestDataInternalBase(const EAudioListenerRequestType eALRType)
            : SAudioRequestDataInternal(eART_AUDIO_LISTENER_REQUEST)
            , eType(eALRType)
        {}

        ~SAudioListenerRequestDataInternalBase() override {}

        const EAudioListenerRequestType eType;

    private:
        SAudioListenerRequestDataInternalBase();
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<EAudioListenerRequestType T>
    struct SAudioListenerRequestDataInternal
        : public SAudioListenerRequestDataInternalBase
    {
        SAudioListenerRequestDataInternal()
            : SAudioListenerRequestDataInternalBase(T)
        {}

        ~SAudioListenerRequestDataInternal() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioListenerRequestDataInternal<eALRT_SET_POSITION>
        : public SAudioListenerRequestDataInternalBase
    {
        explicit SAudioListenerRequestDataInternal(const SAudioListenerRequestData<eALRT_SET_POSITION>* const pALRData)
            : SAudioListenerRequestDataInternalBase(pALRData->eType)
            , oNewPosition(pALRData->oNewPosition)
        {}

        ~SAudioListenerRequestDataInternal<eALRT_SET_POSITION>()override {}

        const SATLWorldPosition oNewPosition;
    };

    SAudioRequestDataInternal* ConvertToInternal(const SAudioRequestDataBase* const pExternalData);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioRequestInternal
    {
    public:
        CAudioRequestInternal()
            : nFlags(eARF_NONE)
            , nAudioObjectID(INVALID_AUDIO_OBJECT_ID)
            , pOwner(nullptr)
            , pUserData(nullptr)
            , pUserDataOwner(nullptr)
            , eStatus(eARS_NONE)
            , nInternalInfoFlags(eARIF_NONE)
            , pData(nullptr)
        {}

        CAudioRequestInternal(const SAudioRequest& rExternalRequest)
            : nFlags(rExternalRequest.nFlags)
            , nAudioObjectID(rExternalRequest.nAudioObjectID)
            , pOwner(rExternalRequest.pOwner)
            , pUserData(rExternalRequest.pUserData)
            , pUserDataOwner(rExternalRequest.pUserDataOwner)
            , eStatus(eARS_NONE)
            , nInternalInfoFlags(eARIF_NONE)
            , pData(ConvertToInternal(rExternalRequest.pData))
        {}

        ~CAudioRequestInternal() = default;

        bool IsComplete() const
        {
            return (eStatus == eARS_SUCCESS || eStatus == eARS_FAILURE);
        }

#if !defined(AUDIO_RELEASE)
        // Debug Logging Helper
        AZStd::string ToString()
        {
            static const AZStd::unordered_map<const EAudioManagerRequestType, const AZStd::string> managerRequests
            {
                { eAMRT_INIT_AUDIO_IMPL, "INIT IMPL" },
                { eAMRT_RELEASE_AUDIO_IMPL, "RELEASE IMPL" },
                { eAMRT_RESERVE_AUDIO_OBJECT_ID, "RESERVE OBJECT ID" },
                { eAMRT_CREATE_SOURCE, "CREATE SOURCE" },
                { eAMRT_DESTROY_SOURCE, "DESTROY SOURCE" },
                { eAMRT_PARSE_CONTROLS_DATA, "PARSE CONTROLS" },
                { eAMRT_PARSE_PRELOADS_DATA, "PARSE PRELOADS" },
                { eAMRT_CLEAR_CONTROLS_DATA, "CLEAR CONTROLS" },
                { eAMRT_CLEAR_PRELOADS_DATA, "CLEAR PRELOADS" },
                { eAMRT_PRELOAD_SINGLE_REQUEST, "PRELOAD SINGLE" },
                { eAMRT_UNLOAD_SINGLE_REQUEST, "UNLOAD SINGLE" },
                { eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE, "UNLOAD SCOPE" },
                { eAMRT_REFRESH_AUDIO_SYSTEM, "REFRESH AUDIO SYSTEM" },
                { eAMRT_LOSE_FOCUS, "LOSE FOCUS" },
                { eAMRT_GET_FOCUS, "GET FOCUS" },
                { eAMRT_MUTE_ALL, "MUTE" },
                { eAMRT_UNMUTE_ALL, "UNMUTE" },
                { eAMRT_STOP_ALL_SOUNDS, "STOP ALL" },
                { eAMRT_DRAW_DEBUG_INFO, "DRAW DEBUG" },
                { eAMRT_CHANGE_LANGUAGE, "CHANGE LANGUAGE" },
                { eAMRT_SET_AUDIO_PANNING_MODE, "SET PANNING MODE" },
            };
            static const AZStd::unordered_map<const EAudioCallbackManagerRequestType, const AZStd::string> callbackRequests
            {
                { eACMRT_REPORT_STARTED_EVENT, "STARTED EVENT" },
                { eACMRT_REPORT_FINISHED_EVENT, "FINISHED EVENT" },
                { eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE, "FINISHED TRIGGER INSTANCE" },
            };
            static const AZStd::unordered_map<const EAudioListenerRequestType, const AZStd::string> listenerRequests
            {
                { eALRT_SET_POSITION, "SET POSITION" },
            };
            static const AZStd::unordered_map<const EAudioObjectRequestType, const AZStd::string> objectRequests
            {
                { eAORT_PREPARE_TRIGGER, "PREPARE TRIGGER" },
                { eAORT_UNPREPARE_TRIGGER, "UNPREPARE TRIGGER" },
                { eAORT_EXECUTE_TRIGGER, "EXECUTE TRIGGER" },
                { eAORT_STOP_TRIGGER, "STOP TRIGGER" },
                { eAORT_STOP_ALL_TRIGGERS, "STOP ALL" },
                { eAORT_SET_POSITION, "SET POSITION" },
                { eAORT_SET_RTPC_VALUE, "SET RTPC" },
                { eAORT_SET_SWITCH_STATE, "SET SWITCH" },
                { eAORT_SET_ENVIRONMENT_AMOUNT, "SET ENV AMOUNT" },
                { eAORT_RESET_ENVIRONMENTS, "RESET ENVS" },
                { eAORT_RESET_RTPCS, "RESET RTPCS" },
                { eAORT_RELEASE_OBJECT, "RELEASE OBJECT" },
                { eAORT_EXECUTE_SOURCE_TRIGGER, "EXECUTE SOURCE TRIGGER" },
                { eAORT_SET_MULTI_POSITIONS, "SET MULTI POSITIONS" },
            };

            std::stringstream ss;

            ss << "AudioRequest(";

            if (pData->eRequestType == eART_AUDIO_MANAGER_REQUEST)
            {
                ss << "AUDIO MANAGER : ";
                auto requestStr = managerRequests.at(static_cast<const SAudioManagerRequestDataInternalBase*>(pData.get())->eType);
                ss << requestStr.c_str();
            }

            if (pData->eRequestType == eART_AUDIO_CALLBACK_MANAGER_REQUEST)
            {
                ss << "AUDIO CALLBACK MGR : ";
                auto requestStr = callbackRequests.at(static_cast<const SAudioCallbackManagerRequestDataInternalBase*>(pData.get())->eType);
                ss << requestStr.c_str();
            }

            if (pData->eRequestType == eART_AUDIO_LISTENER_REQUEST)
            {
                ss << "AUDIO LISTENER : ";
                auto requestStr = listenerRequests.at(static_cast<const SAudioListenerRequestDataInternalBase*>(pData.get())->eType);
                ss << requestStr.c_str();
            }
            if (pData->eRequestType == eART_AUDIO_OBJECT_REQUEST)
            {
                ss << "AUDIO OBJECT : ";
                auto requestStr = objectRequests.at(static_cast<const SAudioObjectRequestDataInternalBase*>(pData.get())->eType);
                ss << requestStr.c_str();
            }

            ss << "): [";
            if (nFlags & eARF_PRIORITY_NORMAL)
            {
                ss << "PRIORITY NORMAL, ";
            }
            if (nFlags & eARF_PRIORITY_HIGH)
            {
                ss << "PRIORITY HIGH, ";
            }
            if (nFlags & eARF_EXECUTE_BLOCKING)
            {
                ss << "EXECUTE BLOCKING, ";
            }
            if (nFlags & eARF_SYNC_CALLBACK)
            {
                ss << "SYNC CALLBACK, ";
            }
            if (nFlags & eARF_SYNC_FINISHED_CALLBACK)
            {
                ss << "SYNC FINISHED CALLBACK, ";
            }
            if (nFlags & eARF_THREAD_SAFE_PUSH)
            {
                ss << "THREAD SAFE PUSH, ";
            }
            ss << "]";

            return AZStd::string(ss.str().c_str());
        }
#endif // !AUDIO_RELEASE

        TATLEnumFlagsType nFlags;
        TAudioObjectID nAudioObjectID;
        void* pOwner;
        void* pUserData;
        void* pUserDataOwner;
        EAudioRequestStatus eStatus;
        TATLEnumFlagsType nInternalInfoFlags;
        _smart_ptr<SAudioRequestDataInternal> pData;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(AUDIO_RELEASE)
    // Filter for drawing debug info to the screen
    enum EAudioDebugDrawFilter : TATLEnumFlagsType
    {
        eADDF_NONE                          = 0,
        eADDF_DRAW_SPHERES                  = AUDIO_BIT(6),// a
        eADDF_SHOW_OBJECT_LABEL             = AUDIO_BIT(7),// b
        eADDF_SHOW_OBJECT_TRIGGERS          = AUDIO_BIT(8),// c
        eADDF_SHOW_OBJECT_STATES            = AUDIO_BIT(9),// d
        eADDF_SHOW_OBJECT_RTPCS             = AUDIO_BIT(10),// e
        eADDF_SHOW_OBJECT_ENVIRONMENTS      = AUDIO_BIT(11),// f
        eADDF_DRAW_OBSTRUCTION_RAYS         = AUDIO_BIT(12),// g
        eADDF_SHOW_OBSTRUCTION_RAY_LABELS   = AUDIO_BIT(13),// h
        eADDF_DRAW_LISTENER_SPHERE          = AUDIO_BIT(14),// i

        eADDF_SHOW_ACTIVE_EVENTS            = AUDIO_BIT(27),// v
        eADDF_SHOW_ACTIVE_OBJECTS           = AUDIO_BIT(28),// w
        eADDF_SHOW_FILECACHE_MANAGER_INFO   = AUDIO_BIT(29),// x

        eADDF_SHOW_IMPL_MEMORY_POOL_USAGE   = AUDIO_BIT(30),// y
    };
#endif // !AUDIO_RELEASE

} // namespace Audio
