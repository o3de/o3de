/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioInterfacesCommonData.h>

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/parallel/mutex.h>

// External forward declarations.
struct IVisArea;
struct ICVar;
struct SSystemInitParams;

namespace Audio
{
    // Internal forward declarations.
    struct SAudioRequest;


    enum EATLDataScope : TATLEnumFlagsType
    {
        eADS_NONE           = 0,
        eADS_GLOBAL         = 1,
        eADS_LEVEL_SPECIFIC = 2,
        eADS_ALL            = 3,
    };

    enum EAudioManagerRequestType : TATLEnumFlagsType
    {
        eAMRT_NONE                      = 0,
        eAMRT_INIT_AUDIO_IMPL           = AUDIO_BIT(0),
        eAMRT_RELEASE_AUDIO_IMPL        = AUDIO_BIT(1),
        eAMRT_RESERVE_AUDIO_OBJECT_ID   = AUDIO_BIT(2),
        eAMRT_CREATE_SOURCE             = AUDIO_BIT(5),
        eAMRT_DESTROY_SOURCE            = AUDIO_BIT(6),
        eAMRT_PARSE_CONTROLS_DATA       = AUDIO_BIT(7),
        eAMRT_PARSE_PRELOADS_DATA       = AUDIO_BIT(8),
        eAMRT_CLEAR_CONTROLS_DATA       = AUDIO_BIT(9),
        eAMRT_CLEAR_PRELOADS_DATA       = AUDIO_BIT(10),
        eAMRT_PRELOAD_SINGLE_REQUEST    = AUDIO_BIT(11),
        eAMRT_UNLOAD_SINGLE_REQUEST     = AUDIO_BIT(12),
        eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE = AUDIO_BIT(13),
        eAMRT_REFRESH_AUDIO_SYSTEM      = AUDIO_BIT(14),
        eAMRT_LOSE_FOCUS                = AUDIO_BIT(15),
        eAMRT_GET_FOCUS                 = AUDIO_BIT(16),
        eAMRT_MUTE_ALL                  = AUDIO_BIT(17),
        eAMRT_UNMUTE_ALL                = AUDIO_BIT(18),
        eAMRT_STOP_ALL_SOUNDS           = AUDIO_BIT(19),
        eAMRT_DRAW_DEBUG_INFO           = AUDIO_BIT(20),
        eAMRT_CHANGE_LANGUAGE           = AUDIO_BIT(21),
        eAMRT_SET_AUDIO_PANNING_MODE    = AUDIO_BIT(22),
    };

    enum EAudioCallbackManagerRequestType : TATLEnumFlagsType
    {
        eACMRT_NONE                             = 0,
        eACMRT_REPORT_STARTED_EVENT             = AUDIO_BIT(0),
        eACMRT_REPORT_FINISHED_EVENT            = AUDIO_BIT(1),
        eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE = AUDIO_BIT(2),
    };

    enum EAudioListenerRequestType : TATLEnumFlagsType
    {
        eALRT_NONE = 0,
        eALRT_SET_POSITION = AUDIO_BIT(0),
    };

    enum EAudioObjectRequestType : TATLEnumFlagsType
    {
        eAORT_NONE                      = 0,
        eAORT_PREPARE_TRIGGER           = AUDIO_BIT(0),
        eAORT_UNPREPARE_TRIGGER         = AUDIO_BIT(1),
        eAORT_EXECUTE_TRIGGER           = AUDIO_BIT(2),
        eAORT_STOP_TRIGGER              = AUDIO_BIT(3),
        eAORT_STOP_ALL_TRIGGERS         = AUDIO_BIT(4),
        eAORT_SET_POSITION              = AUDIO_BIT(5),
        eAORT_SET_RTPC_VALUE            = AUDIO_BIT(6),
        eAORT_SET_SWITCH_STATE          = AUDIO_BIT(7),
        eAORT_SET_ENVIRONMENT_AMOUNT    = AUDIO_BIT(8),
        eAORT_RESET_ENVIRONMENTS        = AUDIO_BIT(9),
        eAORT_RESET_RTPCS               = AUDIO_BIT(10),
        eAORT_RELEASE_OBJECT            = AUDIO_BIT(11),
        eAORT_EXECUTE_SOURCE_TRIGGER    = AUDIO_BIT(12),
        eAORT_SET_MULTI_POSITIONS       = AUDIO_BIT(13),
    };

    enum EAudioObjectObstructionCalcType : TATLEnumFlagsType
    {
        eAOOCT_IGNORE       = 0,
        eAOOCT_SINGLE_RAY   = 1,
        eAOOCT_MULTI_RAY    = 2,
    };

    enum EAudioControlType : TATLEnumFlagsType
    {
        eACT_NONE           = 0,
        eACT_AUDIO_OBJECT   = 1,
        eACT_TRIGGER        = 2,
        eACT_RTPC           = 3,
        eACT_SWITCH         = 4,
        eACT_SWITCH_STATE   = 5,
        eACT_PRELOAD        = 6,
        eACT_ENVIRONMENT    = 7,
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    AZ_FORCE_INLINE T AudioStringToID(const char* const source)
    {
        return static_cast<T>(AZ::Crc32(source));
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Function Callback Typedefs
    using AudioRequestCallbackType = void(*)(const SAudioRequestInfo* const);
    using TriggerFinishedCallbackType = void(*)(const TAudioObjectID, const TAudioControlID, void* const);


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Manager Requests
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioManagerRequestDataBase
        : public SAudioRequestDataBase
    {
        explicit SAudioManagerRequestDataBase(const EAudioManagerRequestType ePassedType = eAMRT_NONE)
            : SAudioRequestDataBase(eART_AUDIO_MANAGER_REQUEST)
            , eType(ePassedType)
        {}

        ~SAudioManagerRequestDataBase() override {}

        const EAudioManagerRequestType eType;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <EAudioManagerRequestType T>
    struct SAudioManagerRequestData
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData()
            : SAudioManagerRequestDataBase(T)
        {}

        ~SAudioManagerRequestData() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_INIT_AUDIO_IMPL>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData()
            : SAudioManagerRequestDataBase(eAMRT_INIT_AUDIO_IMPL)
        {}

        ~SAudioManagerRequestData<eAMRT_INIT_AUDIO_IMPL>() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_RELEASE_AUDIO_IMPL> : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData()
            : SAudioManagerRequestDataBase(eAMRT_RELEASE_AUDIO_IMPL)
        {}

        ~SAudioManagerRequestData<eAMRT_RELEASE_AUDIO_IMPL>() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_RESERVE_AUDIO_OBJECT_ID>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData(TAudioObjectID* const pPassedObjectID, const char* const sPassedObjectName = nullptr)
            : SAudioManagerRequestDataBase(eAMRT_RESERVE_AUDIO_OBJECT_ID)
            , pObjectID(pPassedObjectID)
            , sObjectName(sPassedObjectName)
        {}

        ~SAudioManagerRequestData<eAMRT_RESERVE_AUDIO_OBJECT_ID>()override {}

        TAudioObjectID* const pObjectID;
        const char* const sObjectName;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_CREATE_SOURCE>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData(const SAudioInputConfig& sourceConfig)
            : SAudioManagerRequestDataBase(eAMRT_CREATE_SOURCE)
            , m_sourceConfig(sourceConfig)
        {}

        ~SAudioManagerRequestData<eAMRT_CREATE_SOURCE>() override {}

        SAudioInputConfig m_sourceConfig;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_DESTROY_SOURCE>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData(TAudioSourceId sourceId)
            : SAudioManagerRequestDataBase(eAMRT_DESTROY_SOURCE)
            , m_sourceId(sourceId)
        {}

        ~SAudioManagerRequestData<eAMRT_DESTROY_SOURCE>() override {}

        const TAudioSourceId m_sourceId;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData(const char* const sControlsFolderPath, const EATLDataScope ePassedDataScope)
            : SAudioManagerRequestDataBase(eAMRT_PARSE_CONTROLS_DATA)
            , sFolderPath(sControlsFolderPath)
            , eDataScope(ePassedDataScope)
        {}

        ~SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA>()override {}

        const char* const sFolderPath;
        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_PARSE_PRELOADS_DATA>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData(const char* const sControlsFolderPath, const EATLDataScope ePassedDataScope)
            : SAudioManagerRequestDataBase(eAMRT_PARSE_PRELOADS_DATA)
            , sFolderPath(sControlsFolderPath)
            , eDataScope(ePassedDataScope)
        {}

        ~SAudioManagerRequestData<eAMRT_PARSE_PRELOADS_DATA>()override {}

        const char* const sFolderPath;
        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_CLEAR_CONTROLS_DATA>
        : public SAudioManagerRequestDataBase
    {
        explicit SAudioManagerRequestData(const EATLDataScope ePassedDataScope = eADS_NONE)
            : SAudioManagerRequestDataBase(eAMRT_CLEAR_CONTROLS_DATA)
            , eDataScope(ePassedDataScope)
        {}

        ~SAudioManagerRequestData<eAMRT_CLEAR_CONTROLS_DATA>()override {}

        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_CLEAR_PRELOADS_DATA>
        : public SAudioManagerRequestDataBase
    {
        explicit SAudioManagerRequestData(const EATLDataScope ePassedDataScope = eADS_NONE)
            : SAudioManagerRequestDataBase(eAMRT_CLEAR_PRELOADS_DATA)
            , eDataScope(ePassedDataScope)
        {}

        ~SAudioManagerRequestData<eAMRT_CLEAR_PRELOADS_DATA>()override {}

        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST>
        : public SAudioManagerRequestDataBase
    {
        explicit SAudioManagerRequestData(const TAudioPreloadRequestID nRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID, const bool bPassedAutoLoadOnly = false)
            : SAudioManagerRequestDataBase(eAMRT_PRELOAD_SINGLE_REQUEST)
            , nPreloadRequestID(nRequestID)
            , bAutoLoadOnly(bPassedAutoLoadOnly)
        {}

        ~SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST>()override {}

        const TAudioPreloadRequestID nPreloadRequestID;
        const bool bAutoLoadOnly;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_UNLOAD_SINGLE_REQUEST>
        : public SAudioManagerRequestDataBase
    {
        explicit SAudioManagerRequestData(const TAudioPreloadRequestID nRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID)
            : SAudioManagerRequestDataBase(eAMRT_UNLOAD_SINGLE_REQUEST)
            , nPreloadRequestID(nRequestID)
        {}

        ~SAudioManagerRequestData<eAMRT_UNLOAD_SINGLE_REQUEST>()override {}

        const TAudioPreloadRequestID nPreloadRequestID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData(const EATLDataScope eScope = eADS_NONE)
            : SAudioManagerRequestDataBase(eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE)
            , eDataScope(eScope)
        {}

        ~SAudioManagerRequestData<eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE>()override {}

        const EATLDataScope eDataScope;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_REFRESH_AUDIO_SYSTEM>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData(const char* const controlsPath, const char* const levelName, TAudioPreloadRequestID preloadId)
            : SAudioManagerRequestDataBase(eAMRT_REFRESH_AUDIO_SYSTEM)
            , m_controlsPath(controlsPath)
            , m_levelName(levelName)
            , m_levelPreloadId(preloadId)
        {}

        ~SAudioManagerRequestData<eAMRT_REFRESH_AUDIO_SYSTEM>()override {}

        const char* const m_controlsPath;
        const char* const m_levelName;
        const TAudioPreloadRequestID m_levelPreloadId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_CHANGE_LANGUAGE>
        : public SAudioManagerRequestDataBase
    {
        SAudioManagerRequestData()
            : SAudioManagerRequestDataBase(eAMRT_CHANGE_LANGUAGE)
        {}

        ~SAudioManagerRequestData<eAMRT_CHANGE_LANGUAGE>()override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioManagerRequestData<eAMRT_SET_AUDIO_PANNING_MODE>
        : public SAudioManagerRequestDataBase
    {
        explicit SAudioManagerRequestData(PanningMode panningMode)
            : SAudioManagerRequestDataBase(eAMRT_SET_AUDIO_PANNING_MODE)
            , m_panningMode(panningMode)
        {}

        ~SAudioManagerRequestData<eAMRT_SET_AUDIO_PANNING_MODE>() override {}

        const PanningMode m_panningMode;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Callback Manager Requests
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioCallbackManagerRequestDataBase
        : public SAudioRequestDataBase
    {
        explicit SAudioCallbackManagerRequestDataBase(const EAudioCallbackManagerRequestType ePassedType = eACMRT_NONE)
            : SAudioRequestDataBase(eART_AUDIO_CALLBACK_MANAGER_REQUEST)
            , eType(ePassedType)
        {}

        ~SAudioCallbackManagerRequestDataBase() override {}

        const EAudioCallbackManagerRequestType eType;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <EAudioCallbackManagerRequestType T>
    struct SAudioCallbackManagerRequestData
        : public SAudioCallbackManagerRequestDataBase
    {
        SAudioCallbackManagerRequestData()
            : SAudioCallbackManagerRequestDataBase(T)
        {}

        ~SAudioCallbackManagerRequestData() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioCallbackManagerRequestData<eACMRT_REPORT_STARTED_EVENT>
        : public SAudioCallbackManagerRequestDataBase
    {
        SAudioCallbackManagerRequestData(TAudioEventID const nPassedEventID)
            : SAudioCallbackManagerRequestDataBase(eACMRT_REPORT_STARTED_EVENT)
            , nEventID(nPassedEventID)
        {}

        ~SAudioCallbackManagerRequestData<eACMRT_REPORT_STARTED_EVENT>() override {}

        const TAudioEventID nEventID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT>
        : public SAudioCallbackManagerRequestDataBase
    {
        SAudioCallbackManagerRequestData(const TAudioEventID nPassedEventID, const bool bPassedSuccess)
            : SAudioCallbackManagerRequestDataBase(eACMRT_REPORT_FINISHED_EVENT)
            , nEventID(nPassedEventID)
            , bSuccess(bPassedSuccess)
        {}

        ~SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT>()override {}

        const TAudioEventID nEventID;
        const bool bSuccess;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE>
        : public SAudioCallbackManagerRequestDataBase
    {
        SAudioCallbackManagerRequestData(TAudioControlID const nPassedControlID)
            : SAudioCallbackManagerRequestDataBase(eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE)
            , nAudioTriggerID(nPassedControlID)
        {}

        ~SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE>()override {}

        const TAudioControlID nAudioTriggerID;
    };



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Object Requests
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioObjectRequestDataBase
        : public SAudioRequestDataBase
    {
        explicit SAudioObjectRequestDataBase(const EAudioObjectRequestType ePassedType = eAORT_NONE)
            : SAudioRequestDataBase(eART_AUDIO_OBJECT_REQUEST)
            , eType(ePassedType)
        {}

        ~SAudioObjectRequestDataBase() override {}

        const EAudioObjectRequestType eType;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <EAudioObjectRequestType T>
    struct SAudioObjectRequestData
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(T)
        {}

        ~SAudioObjectRequestData() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_EXECUTE_TRIGGER)
            , nTriggerID(INVALID_AUDIO_CONTROL_ID)
            , fTimeUntilRemovalInMS(0.0f)
        {}

        SAudioObjectRequestData(const TAudioControlID nPassedTriggerID, const float fPassedTimeUntilRemovalInMS)
            : SAudioObjectRequestDataBase(eAORT_EXECUTE_TRIGGER)
            , nTriggerID(nPassedTriggerID)
            , fTimeUntilRemovalInMS(fPassedTimeUntilRemovalInMS)
        {}

        ~SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER>()override {}

        TAudioControlID nTriggerID;
        float fTimeUntilRemovalInMS;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_PREPARE_TRIGGER>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_PREPARE_TRIGGER)
            , nTriggerID(INVALID_AUDIO_CONTROL_ID)
        {}

        explicit SAudioObjectRequestData(const TAudioControlID nPassedTriggerID)
            : SAudioObjectRequestDataBase(eAORT_PREPARE_TRIGGER)
            , nTriggerID(nPassedTriggerID)
        {}

        ~SAudioObjectRequestData<eAORT_PREPARE_TRIGGER>()override {}

        TAudioControlID nTriggerID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_UNPREPARE_TRIGGER>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_UNPREPARE_TRIGGER)
            , nTriggerID(INVALID_AUDIO_CONTROL_ID)
        {}

        explicit SAudioObjectRequestData(const TAudioControlID nPassedTriggerID)
            : SAudioObjectRequestDataBase(eAORT_UNPREPARE_TRIGGER)
            , nTriggerID(nPassedTriggerID)
        {}

        ~SAudioObjectRequestData<eAORT_UNPREPARE_TRIGGER>()override {}

        TAudioControlID nTriggerID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_STOP_TRIGGER>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_STOP_TRIGGER)
            , nTriggerID(INVALID_AUDIO_CONTROL_ID)
        {}

        explicit SAudioObjectRequestData(const TAudioControlID nPassedTriggerID)
            : SAudioObjectRequestDataBase(eAORT_STOP_TRIGGER)
            , nTriggerID(nPassedTriggerID)
        {}

        ~SAudioObjectRequestData<eAORT_STOP_TRIGGER>()override {}

        TAudioControlID nTriggerID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_STOP_ALL_TRIGGERS>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_STOP_ALL_TRIGGERS)
            , m_filterByOwner(false)
        {}

        SAudioObjectRequestData(bool filterByOwner)
            : SAudioObjectRequestDataBase(eAORT_STOP_ALL_TRIGGERS)
            , m_filterByOwner(filterByOwner)
        {}

        ~SAudioObjectRequestData<eAORT_STOP_ALL_TRIGGERS>()override {}

        const bool m_filterByOwner;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_SET_POSITION>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_SET_POSITION)
            , oPosition()
        {}

        explicit SAudioObjectRequestData(const SATLWorldPosition& oPassedPosition)
            : SAudioObjectRequestDataBase(eAORT_SET_POSITION)
            , oPosition(oPassedPosition)
        {}

        ~SAudioObjectRequestData<eAORT_SET_POSITION>()override {}

        SATLWorldPosition oPosition;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_SET_RTPC_VALUE>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_SET_RTPC_VALUE)
            , nControlID(INVALID_AUDIO_CONTROL_ID)
            , fValue(0.0f)
        {}

        SAudioObjectRequestData(const TAudioControlID nPassedControlID, const float fPassedValue)
            : SAudioObjectRequestDataBase(eAORT_SET_RTPC_VALUE)
            , nControlID(nPassedControlID)
            , fValue(fPassedValue)
        {}

        ~SAudioObjectRequestData<eAORT_SET_RTPC_VALUE>()override {}

        TAudioControlID nControlID;
        float fValue;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_SET_SWITCH_STATE>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_SET_SWITCH_STATE)
            , nSwitchID(INVALID_AUDIO_CONTROL_ID)
            , nStateID(INVALID_AUDIO_SWITCH_STATE_ID)
        {}

        SAudioObjectRequestData(const TAudioControlID nPassedControlID, const TAudioSwitchStateID nPassedStateID)
            : SAudioObjectRequestDataBase(eAORT_SET_SWITCH_STATE)
            , nSwitchID(nPassedControlID)
            , nStateID(nPassedStateID)
        {}

        ~SAudioObjectRequestData<eAORT_SET_SWITCH_STATE>()override {}

        TAudioControlID nSwitchID;
        TAudioSwitchStateID nStateID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_SET_ENVIRONMENT_AMOUNT>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_SET_ENVIRONMENT_AMOUNT)
            , nEnvironmentID(INVALID_AUDIO_ENVIRONMENT_ID)
            , fAmount(1.0f)
        {}

        SAudioObjectRequestData(const TAudioEnvironmentID nPassedEnvironmentID, const float fPassedAmount)
            : SAudioObjectRequestDataBase(eAORT_SET_ENVIRONMENT_AMOUNT)
            , nEnvironmentID(nPassedEnvironmentID)
            , fAmount(fPassedAmount)
        {}

        ~SAudioObjectRequestData<eAORT_SET_ENVIRONMENT_AMOUNT>()override {}

        TAudioEnvironmentID nEnvironmentID;
        float fAmount;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_RESET_ENVIRONMENTS>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_RESET_ENVIRONMENTS)
        {}

        ~SAudioObjectRequestData<eAORT_RESET_ENVIRONMENTS>()override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_RESET_RTPCS>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_RESET_RTPCS)
        {}

        ~SAudioObjectRequestData<eAORT_RESET_RTPCS>() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_RELEASE_OBJECT>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_RELEASE_OBJECT)
        {}

        ~SAudioObjectRequestData<eAORT_RELEASE_OBJECT>()override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_EXECUTE_SOURCE_TRIGGER)
            , m_triggerId(INVALID_AUDIO_CONTROL_ID)
        {}

        SAudioObjectRequestData(
            TAudioControlID triggerId,
            const SAudioSourceInfo& sourceInfo)
            : SAudioObjectRequestDataBase(eAORT_EXECUTE_SOURCE_TRIGGER)
            , m_triggerId(triggerId)
            , m_sourceInfo(sourceInfo)
        {}

        ~SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER>()override {}

        TAudioControlID m_triggerId;
        SAudioSourceInfo m_sourceInfo;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <>
    struct SAudioObjectRequestData<eAORT_SET_MULTI_POSITIONS>
        : public SAudioObjectRequestDataBase
    {
        SAudioObjectRequestData()
            : SAudioObjectRequestDataBase(eAORT_SET_MULTI_POSITIONS)
        {}

        SAudioObjectRequestData(const MultiPositionParams& params)
            : SAudioObjectRequestDataBase(eAORT_SET_MULTI_POSITIONS)
            , m_params(params)
        {}

        ~SAudioObjectRequestData<eAORT_SET_MULTI_POSITIONS>() override {}

        MultiPositionParams m_params;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Listener Requests
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioListenerRequestDataBase
        : public SAudioRequestDataBase
    {
        explicit SAudioListenerRequestDataBase(const EAudioListenerRequestType ePassedType = eALRT_NONE)
            : SAudioRequestDataBase(eART_AUDIO_LISTENER_REQUEST)
            , eType(ePassedType)
        {}

        ~SAudioListenerRequestDataBase() override {}

        const EAudioListenerRequestType eType;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<EAudioListenerRequestType T>
    struct SAudioListenerRequestData
        : public SAudioListenerRequestDataBase
    {
        SAudioListenerRequestData()
            : SAudioListenerRequestDataBase(T)
        {}

        ~SAudioListenerRequestData() override {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<>
    struct SAudioListenerRequestData<eALRT_SET_POSITION>
        : public SAudioListenerRequestDataBase
    {
        SAudioListenerRequestData()
            : SAudioListenerRequestDataBase(eALRT_SET_POSITION)
        {}

        explicit SAudioListenerRequestData(const SATLWorldPosition& oWorldPosition)
            : SAudioListenerRequestDataBase(eALRT_SET_POSITION)
            , oNewPosition(oWorldPosition)
        {}

        ~SAudioListenerRequestData<eALRT_SET_POSITION>()override {}

        SATLWorldPosition oNewPosition;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioSystemInfo
    {
        SAudioSystemInfo()
            : nCountUsedAudioTriggers(0)
            , nCountUnusedAudioTriggers(0)
            , nCountUsedAudioEvents(0)
            , nCountUnusedAudioEvents(0)
        {}

        AZStd::size_t nCountUsedAudioTriggers;
        AZStd::size_t nCountUnusedAudioTriggers;
        AZStd::size_t nCountUsedAudioEvents;
        AZStd::size_t nCountUnusedAudioEvents;

        AZ::Vector3 oListenerPos;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IAudioProxy
    {
        virtual ~IAudioProxy() = default;

        virtual void Initialize(const char* sObjectName, bool bInitAsync = true) = 0;
        virtual void Release() = 0;
        virtual void Reset() = 0;

        virtual void ExecuteSourceTrigger(TAudioControlID nTriggerID, const SAudioSourceInfo& rSourceInfo, const SAudioCallBackInfos& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) = 0;
        virtual void ExecuteTrigger(TAudioControlID nTriggerID, const SAudioCallBackInfos& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) = 0;
        virtual void StopAllTriggers() = 0;
        virtual void StopTrigger(TAudioControlID nTriggerID) = 0;
        virtual void SetSwitchState(TAudioControlID nSwitchID, TAudioSwitchStateID nStateID) = 0;
        virtual void SetRtpcValue(TAudioControlID nRtpcID, float fValue) = 0;
        virtual void SetObstructionCalcType(EAudioObjectObstructionCalcType eObstructionType) = 0;
        virtual void SetPosition(const SATLWorldPosition& rPosition) = 0;
        virtual void SetPosition(const AZ::Vector3& rPosition) = 0;
        virtual void SetMultiplePositions(const MultiPositionParams& params) = 0;
        virtual void SetEnvironmentAmount(TAudioEnvironmentID nEnvironmentID, float fAmount) = 0;
        virtual void SetCurrentEnvironments() = 0;
        virtual void ResetRtpcValues() = 0;
        virtual TAudioObjectID GetAudioObjectID() const = 0;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioPreloadNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioPreloadNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - PreloadID Address, Multiple Handler
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = TAudioPreloadRequestID;
        using MutexType = AZStd::recursive_mutex;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        virtual void OnAudioPreloadCached() = 0;
        virtual void OnAudioPreloadUncached() = 0;
    };

    using AudioPreloadNotificationBus = AZ::EBus<AudioPreloadNotifications>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioSystemRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioSystemRequests() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - Single Bus Address, Single Handler
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        virtual bool Initialize() = 0;
        virtual void Release() = 0;
        virtual void ExternalUpdate() = 0;

        virtual void PushRequest(const SAudioRequest& rAudioRequestData) = 0;
        virtual void PushRequestBlocking(const SAudioRequest& audioRequestData) = 0;

        virtual void AddRequestListener(
            AudioRequestCallbackType callBack,
            void* objectToListenTo,
            EAudioRequestType requestType = eART_AUDIO_ALL_REQUESTS,
            TATLEnumFlagsType specificRequestMask = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS) = 0;
        virtual void RemoveRequestListener(
            AudioRequestCallbackType callBack,
            void* requestOwner) = 0;

        virtual TAudioControlID GetAudioTriggerID(const char* sAudioTriggerName) const = 0;
        virtual TAudioControlID GetAudioRtpcID(const char* sAudioRtpcName) const = 0;
        virtual TAudioControlID GetAudioSwitchID(const char* sAudioSwitchName) const = 0;
        virtual TAudioSwitchStateID GetAudioSwitchStateID(TAudioControlID nSwitchID, const char* sAudioSwitchStateName) const = 0;
        virtual TAudioPreloadRequestID GetAudioPreloadRequestID(const char* sAudioPreloadRequestName) const = 0;
        virtual TAudioEnvironmentID GetAudioEnvironmentID(const char* sAudioEnvironmentName) const = 0;

        virtual bool ReserveAudioListenerID(TAudioObjectID& rAudioObjectID) = 0;
        virtual bool ReleaseAudioListenerID(TAudioObjectID nAudioObjectID) = 0;
        virtual bool SetAudioListenerOverrideID(TAudioObjectID nAudioObjectID) = 0;

        virtual void GetInfo(SAudioSystemInfo& rAudioSystemInfo) = 0;
        virtual const char* GetControlsPath() const = 0;
        virtual void UpdateControlsPath() = 0;
        virtual void RefreshAudioSystem(const char* levelName) = 0;

        virtual IAudioProxy* GetFreeAudioProxy() = 0;
        virtual void FreeAudioProxy(IAudioProxy* pIAudioProxy) = 0;

        virtual TAudioSourceId CreateAudioSource(const SAudioInputConfig& sourceConfig) = 0;
        virtual void DestroyAudioSource(TAudioSourceId sourceId) = 0;

        virtual const char* GetAudioControlName(EAudioControlType controlType, TATLIDType atlID) const = 0;
        virtual const char* GetAudioSwitchStateName(TAudioControlID switchID, TAudioSwitchStateID stateID) const = 0;
    };

    using AudioSystemRequestBus = AZ::EBus<AudioSystemRequests>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioTriggerNotifications
        : public AZ::EBusTraits
    {
        virtual ~AudioTriggerNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - Address by ID, Multiple Handler, Mutex, Queued
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const bool EnableEventQueue = true;
        using MutexType = AZStd::recursive_mutex;
        using BusIdType = Audio::TAudioControlID;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        virtual void ReportDurationInfo(TAudioEventID, float /*duration*/, float /*estimatedDuration*/) = 0;
    };

    using AudioTriggerNotificationBus = AZ::EBus<AudioTriggerNotifications>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioSystemThreadSafeRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioSystemThreadSafeRequests() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - Single Bus Address, Single Handler, Mutex, Queued
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const bool EnableEventQueue = true;
        using MutexType = AZStd::recursive_mutex;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        virtual void PushRequestThreadSafe(const SAudioRequest& audioRequestData) = 0;
    };

    using AudioSystemThreadSafeRequestBus = AZ::EBus<AudioSystemThreadSafeRequests>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IAudioSystem
        : public AudioSystemRequestBus::Handler
        , public AudioSystemThreadSafeRequestBus::Handler
    {
        ~IAudioSystem() override = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioStreamingRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioStreamingRequests() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Single Bus, Single Handler
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = TAudioSourceId;
        using MutexType = AZStd::recursive_mutex;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ReadStreamingInput>
        // Summary:
        //      Load streaming input into the ATL-specific audio input
        // Arguments:
        //      data - Buffer of data to load into the input device's internal buffer
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual AZStd::size_t ReadStreamingInput(const AudioStreamData& data) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ReadStreamingMultiTrackInput>
        // Summary:
        //      Load streaming multi-track input into the ATL-specific audio input
        // Arguments:
        //      data - Buffers of multi-track data to load into the input device's internal buffer
        // Return value:
        //      The number of frames loaded into the internal buffer
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual AZStd::size_t ReadStreamingMultiTrackInput(AudioStreamMultiTrackData& data) = 0;
    };

    using AudioStreamingRequestBus = AZ::EBus<AudioStreamingRequests>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    namespace Gem
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Used for initializing and releasing the audio system (AudioSystem/ATL) code.
        class AudioSystemGemRequests
            : public AZ::EBusTraits
        {
        public:
            ///////////////////////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            ///////////////////////////////////////////////////////////////////////////////////////////

            // Interface methods
            virtual bool Initialize(const SSystemInitParams* initParams) = 0;
            virtual void Release() = 0;
        };

        using AudioSystemGemRequestBus = AZ::EBus<AudioSystemGemRequests>;


        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Used for initializing and releasing the audio engine (middleware layer) code.
        class AudioEngineGemRequests
            : public AZ::EBusTraits
        {
        public:
            ///////////////////////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            ///////////////////////////////////////////////////////////////////////////////////////////

            // Interface methods
            virtual bool Initialize() = 0;
            virtual void Release() = 0;
        };

        using AudioEngineGemRequestBus = AZ::EBus<AudioEngineGemRequests>;

    } // namespace Gem

} // namespace Audio
