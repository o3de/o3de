/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    namespace DebugDraw
    {
        enum Options : AZ::u32
        {
            None = 0,
            DrawObjects = (1 << 0),
            ObjectLabels = (1 << 1),
            ObjectTriggers = (1 << 2),
            ObjectStates = (1 << 3),
            ObjectRtpcs = (1 << 4),
            ObjectEnvironments = (1 << 5),
            DrawRays = (1 << 6),
            RayLabels = (1 << 7),
            DrawListener = (1 << 8),
            ActiveEvents = (1 << 9),
            ActiveObjects = (1 << 10),
            FileCacheInfo = (1 << 11),
            MemoryInfo = (1 << 12),
        };
    }

    namespace FileCacheManagerDebugDraw
    {
        enum Options : AZ::u8
        {
            All = 0,
            Global = (1 << 0),
            LevelSpecific = (1 << 1),
            UseCounted = (1 << 2),
            Loaded = (1 << 3),
        };
    }
#endif // !AUDIO_RELEASE

} // namespace Audio
