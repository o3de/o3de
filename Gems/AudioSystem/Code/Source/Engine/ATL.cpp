/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATL.h>

#if !defined(AUDIO_RELEASE)
    #include <AzCore/Math/Color.h>
#endif // !AUDIO_RELEASE

#include <AzCore/Debug/Profiler.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <SoundCVars.h>
#include <AudioProxy.h>
#include <ATLAudioObject.h>
#include <IAudioSystemImplementation.h>

#include <IRenderAuxGeom.h>

namespace Audio
{
    extern CAudioLogger g_audioLogger;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline EAudioRequestResult ConvertToRequestResult(const EAudioRequestStatus eAudioRequestStatus)
    {
        EAudioRequestResult eResult;

        switch (eAudioRequestStatus)
        {
            case EAudioRequestStatus::Success:
            {
                eResult = EAudioRequestResult::Success;
                break;
            }
            case EAudioRequestStatus::Failure:
                [[fallthrough]];
            case EAudioRequestStatus::FailureInvalidObjectId:
                [[fallthrough]];
            case EAudioRequestStatus::FailureInvalidControlId:
                [[fallthrough]];
            case EAudioRequestStatus::FailureInvalidRequest:
                [[fallthrough]];
            case EAudioRequestStatus::PartialSuccess:
            {
                eResult = EAudioRequestResult::Failure;
                break;
            }
            default:
            {
                g_audioLogger.Log(LogType::Assert, "Invalid AudioRequestStatus '%u'. Cannot be converted to an AudioRequestResult. ", eAudioRequestStatus);
                eResult = EAudioRequestResult::Failure;
                break;
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioTranslationLayer::CAudioTranslationLayer()
        : m_pGlobalAudioObject(nullptr)
        , m_nGlobalAudioObjectID(GLOBAL_AUDIO_OBJECT_ID)
        , m_nTriggerInstanceIDCounter(1)
        , m_nextSourceId(INVALID_AUDIO_SOURCE_ID)
        , m_oAudioEventMgr()
        , m_oAudioObjectMgr(m_oAudioEventMgr)
        , m_oAudioListenerMgr()
        , m_oFileCacheMgr(m_cPreloadRequests)
        , m_oAudioEventListenerMgr()
        , m_oXmlProcessor(m_cTriggers, m_cRtpcs, m_cSwitches, m_cEnvironments, m_cPreloadRequests, m_oFileCacheMgr)
        , m_nFlags(eAIS_NONE)
    {
        GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

#if !defined(AUDIO_RELEASE)
        m_oAudioEventMgr.SetDebugNameStore(&m_oDebugNameStore);
        m_oAudioObjectMgr.SetDebugNameStore(&m_oDebugNameStore);
        m_oXmlProcessor.SetDebugNameStore(&m_oDebugNameStore);
#endif // !AUDIO_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioTranslationLayer::~CAudioTranslationLayer()
    {
        // By the time ATL is being destroyed, ReleaseImplComponent should have been called already
        // to release the implementation object.  See CAudioSystem::Release().
        GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::Initialize()
    {
        m_lastUpdateTime = AZStd::chrono::system_clock::now();
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ShutDown()
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::Update()
    {
        AZ_PROFILE_FUNCTION(Audio);

        auto current = AZStd::chrono::system_clock::now();
        m_elapsedTime = AZStd::chrono::duration_cast<duration_ms>(current - m_lastUpdateTime);
        m_lastUpdateTime = current;
        float elapsedMs = m_elapsedTime.count();

        UpdateSharedData();

        m_oAudioEventMgr.Update(elapsedMs);
        m_oAudioObjectMgr.Update(elapsedMs, m_oSharedData.m_oActiveListenerPosition);
        m_oAudioListenerMgr.Update(elapsedMs);
        m_oFileCacheMgr.Update();

        AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::Update, elapsedMs);
    }

    //! NEW AUDIO REQUESTS
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::ProcessRequestNew(AudioRequestVariant&& requestVariant)
    {
        // Version A - AZStd::visit w/ a processor object
        // Pros: No wonky syntax, the processor class can be implemented elsewhere
        // Cons: No easy way to connect to the ATL
        //
        // Version B - AZStd::visit w/ a lambda and if constexpr on type
        // Pros: Syntax is a little more messy and it looks like a big switch statement, but
        //   we can pass the ATL in the lambda capture
        // Cons: Seems like it can only be an inline lambda because of the 'auto&&' parameter

        // Version A...
        //struct RequestProcessor
        //{
        //    void operator()([[maybe_unused]] Audio::SystemRequest::Initialize& request)
        //    {
        //        AZ_Printf("ATL Request Processor", "Initialize Audio System\n");
        //        request.SetStatus(EAudioRequestStatus::Pending);
        //        // do stuff ...
        //    }
        //    void operator()([[maybe_unused]] Audio::SystemRequest::Shutdown& request)
        //    {
        //        AZ_Printf("ATL Request Processor", "Shutdown Audio System\n");
        //    }
        //
        // ... ...
        //
        //    void operator()([[maybe_unused]] Audio::ObjectRequest::SetMultiplePositions& request)
        //    {
        //    }
        //    void operator()([[maybe_unused]] Audio::ListenerRequest::SetWorldTransform& request)
        //    {
        //    }
        //};
        //AZStd::visit(RequestProcessor{}, request);


        bool hasCallback = false;
        // VERSION B
        EAudioRequestStatus status = AZStd::visit(
            [this, &hasCallback]([[maybe_unused]] auto&& request) -> EAudioRequestStatus
            {
                EAudioRequestStatus result = EAudioRequestStatus::None;

                request.SetStatus(EAudioRequestStatus::Pending);

                using T = AZStd::decay_t<decltype(request)>;

                // ... System ...

                if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::Initialize>)
                {
                    AZ_Printf("ATL Request Lambda", "Initialize Audio System\n");

                    result = InitializeImplComponent();
                    // Immediately release the impl if it failed to init...
                    if (result != EAudioRequestStatus::Success)
                    {
                        ReleaseImplComponent();
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::Shutdown>)
                {
                    AZ_Printf("ATL Request Lambda", "Shutdown Audio System\n");
                    ReleaseImplComponent();
                    result = EAudioRequestStatus::Success;
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::ReserveObject>)
                {
                #if !defined(AUDIO_RELEASE)
                    result = BoolToARS(ReserveAudioObjectID(request.m_objectId, request.m_objectName.c_str()));
                #else
                    result = BoolToARS(ReserveAudioObjectID(request.m_objectId));
                #endif // !AUDIO_RELEASE
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::CreateSource>)
                {
                    bool created = false;
                    AudioSystemImplementationRequestBus::BroadcastResult(
                        created, &AudioSystemImplementationRequestBus::Events::CreateAudioSource, request.m_sourceConfig);
                    result = BoolToARS(created);
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::DestroySource>)
                {
                    AudioSystemImplementationRequestBus::Broadcast(
                        &AudioSystemImplementationRequestBus::Events::DestroyAudioSource, request.m_sourceId);
                    result = EAudioRequestStatus::Success;
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::LoadControls>)
                {
                    result = ParseControlsData(request.m_controlsPath.c_str(), request.m_scope);
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::UnloadControls>)
                {
                    result = ClearControlsData(request.m_scope);
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::LoadBank>)
                {
                    //!  NEED A BLOCKING FLAG TO INDICATE WHETHER IT SHOULD LOAD ASYNC OR NOT  !//
                    result = m_oFileCacheMgr.TryLoadRequest(request.m_preloadRequestId, true, request.m_autoLoadOnly);
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::UnloadBank>)
                {
                    result = m_oFileCacheMgr.TryUnloadRequest(request.m_preloadRequestId);
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::UnloadBanksByScope>)
                {
                    result = m_oFileCacheMgr.UnloadDataByScope(request.m_scope);
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::ReloadAll>)
                {
                    result = RefreshAudioSystem(request.m_controlsPath.c_str(), request.m_levelName.c_str(), request.m_levelPreloadId);
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::LoseFocus>)
                {
                    result = LoseFocus();
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::GetFocus>)
                {
                    result = GetFocus();
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::MuteAll>)
                {
                    result = MuteAll();
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::UnmuteAll>)
                {
                    result = UnmuteAll();
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::StopAllAudio>)
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(
                        result, &AudioSystemImplementationRequestBus::Events::StopAllSounds);
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::DrawDebug>)
                {
                #if !defined(AUDIO_RELEASE)
                    DrawAudioSystemDebugInfo();
                    result = EAudioRequestStatus::Success;
                #endif // !AUDIO_RELEASE
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::ChangeLanguage>)
                {
                    SetImplLanguage();
                    m_oFileCacheMgr.UpdateLocalizedFileCacheEntries();
                    result = EAudioRequestStatus::Success;
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::SetPanningMode>)
                {
                    AudioSystemImplementationRequestBus::Broadcast(
                        &AudioSystemImplementationRequestBus::Events::SetPanningMode, request.m_panningMode);
                    result = EAudioRequestStatus::Success;
                }

                // ... Object ...

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::ExecuteTrigger>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (auto it = m_cTriggers.find(request.m_triggerId); it != m_cTriggers.end())
                    {
                        result = ActivateTrigger(
                            audioObject, it->second
                            //... request.pOwner
                            //... request.pUserData
                            //... request.pUserDataOwner
                            //... request.nFlags
                        );
                    }
                    else
                    {
                        result = EAudioRequestStatus::FailureInvalidControlId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::PrepareTrigger>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (auto it = m_cTriggers.find(request.m_triggerId); it != m_cTriggers.end())
                    {
                        result = PrepUnprepTriggerAsync(audioObject, it->second, true);
                    }
                    else
                    {
                        result = EAudioRequestStatus::FailureInvalidControlId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::UnprepareTrigger>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (auto it = m_cTriggers.find(request.m_triggerId); it != m_cTriggers.end())
                    {
                        result = PrepUnprepTriggerAsync(audioObject, it->second, false);
                    }
                    else
                    {
                        result = EAudioRequestStatus::FailureInvalidControlId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::StopTrigger>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (auto it = m_cTriggers.find(request.m_triggerId); it != m_cTriggers.end())
                    {
                        result = StopTrigger(audioObject, it->second);
                    }
                    else
                    {
                        result = EAudioRequestStatus::FailureInvalidControlId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::StopAllTriggers>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else
                    {
                        // TODO: Filter by Owner - there is no owner in the request yet
                        //if (request.m_filterByOwner)
                        //{
                        //    StopAllTriggers(audioObject, request.m_owner);
                        //}
                        //else
                        //{
                        StopAllTriggers(audioObject);
                        //}

                        // Should we return the result of StopAllTriggers call instead?
                        result = EAudioRequestStatus::Success;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetPosition>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (audioObject->HasPosition())
                    {
                        auto const positionalObject = static_cast<CATLAudioObject*>(audioObject);
                        AudioSystemImplementationRequestBus::BroadcastResult(
                            result, &AudioSystemImplementationRequestBus::Events::SetPosition, positionalObject->GetImplDataPtr(),
                            request.m_position);

                        if (result == EAudioRequestStatus::Success)
                        {
                            positionalObject->SetPosition(request.m_position);
                        }
                    }
                    else
                    {
                        g_audioLogger.Log(LogType::Comment, "ATL received request to set position on the global audio object!");
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetParameterValue>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (auto it = m_cRtpcs.find(request.m_parameterId); it != m_cRtpcs.end())
                    {
                        result = SetRtpc(audioObject, it->second, request.m_value);
                    }
                    else
                    {
                        result = EAudioRequestStatus::FailureInvalidControlId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetSwitchValue>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (auto itSwitch = m_cSwitches.find(request.m_switchId); itSwitch != m_cSwitches.end())
                    {
                        if (auto itState = itSwitch->second->cStates.find(request.m_stateId);
                            itState != itSwitch->second->cStates.end())
                        {
                            result = SetSwitchState(audioObject, itState->second);
                        }
                        else
                        {
                            result = EAudioRequestStatus::FailureInvalidControlId;
                        }
                    }
                    else
                    {
                        result = EAudioRequestStatus::FailureInvalidControlId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetEnvironmentValue>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (audioObject->HasPosition())
                    {
                        if (auto it = m_cEnvironments.find(request.m_environmentId);
                            it != m_cEnvironments.end())
                        {
                            result = SetEnvironment(audioObject, it->second, request.m_value);
                        }
                        else
                        {
                            result = EAudioRequestStatus::FailureInvalidControlId;
                        }
                    }
                    else
                    {
                        g_audioLogger.Log(LogType::Comment, "ATL received request to set environment value on the global audio object!");
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::ResetParameters>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else
                    {
                        result = ResetRtpcs(audioObject);
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::ResetEnvironments>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else
                    {
                        result = ResetEnvironments(audioObject);
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::Release>)
                {
                    if (request.m_audioObjectId != m_nGlobalAudioObjectID)
                    {
                        result = BoolToARS(ReleaseAudioObjectID(request.m_audioObjectId));
                    }
                    else
                    {
                        g_audioLogger.Log(LogType::Comment, "ATL received request to release the global audio object!");
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::ExecuteSourceTrigger>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (auto it = m_cTriggers.find(request.m_triggerId); it != m_cTriggers.end())
                    {
                        SATLSourceData sourceData(request.m_sourceInfo);
                        result = ActivateTrigger(audioObject, it->second,
                            nullptr, // request.pOwner
                            nullptr, // request.pUserData
                            0,       // request.nFlags
                            &sourceData
                        );
                    }
                    else
                    {
                        result = EAudioRequestStatus::FailureInvalidControlId;
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::ObjectRequest::SetMultiplePositions>)
                {
                    auto audioObject = GetRequestObject(request.m_audioObjectId);
                    if (!audioObject)
                    {
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                    else if (audioObject->HasPosition())
                    {
                        auto const positionalObject = static_cast<CATLAudioObject*>(audioObject);
                        AudioSystemImplementationRequestBus::BroadcastResult(
                            result, &AudioSystemImplementationRequestBus::Events::SetMultiplePositions, positionalObject->GetImplDataPtr(),
                            request.m_params);

                        if (result == EAudioRequestStatus::Success)
                        {
                            // This is odd, why is this being done?  Because the source of positional information is elsewhere?
                            positionalObject->SetPosition(SATLWorldPosition());
                        }
                    }
                    else
                    {
                        g_audioLogger.Log(LogType::Comment, "ATL received request to set multiple positions on the global audio object!");
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                }

                // ... Listener ...

                else if constexpr (AZStd::is_same_v<T, Audio::ListenerRequest::SetWorldTransform>)
                {
                    TAudioObjectID listenerId = INVALID_AUDIO_OBJECT_ID;
                    // Check for an audio listener override
                    TAudioObjectID overrideListenerId = m_oAudioListenerMgr.GetOverrideListenerID();
                    if (overrideListenerId != INVALID_AUDIO_OBJECT_ID)
                    {
                        if (request.m_audioObjectId == overrideListenerId)
                        {
                            // Have an override set, and the ID in the request matches the override.
                            // Reroute to the default listener.
                            listenerId = m_oAudioListenerMgr.GetDefaultListenerID();
                        }
                        else if (request.m_audioObjectId != INVALID_AUDIO_OBJECT_ID)
                        {
                            // Override is set, but the request specified a different listener ID, allow it.
                            listenerId = request.m_audioObjectId;
                        }
                        else
                        {
                            // Override is set, but no listener ID specified.  Typically this would go
                            // to the default listener, but with overrides we explicitly ignore this.
                            return EAudioRequestStatus::FailureInvalidObjectId;
                        }
                    }
                    else if (request.m_audioObjectId == INVALID_AUDIO_OBJECT_ID)
                    {
                        listenerId = m_oAudioListenerMgr.GetDefaultListenerID();
                    }
                    else
                    {
                        listenerId = request.m_audioObjectId;
                    }

                    if (CATLListenerObject* listener = m_oAudioListenerMgr.LookupID(listenerId); listener != nullptr)
                    {
                        AudioSystemImplementationRequestBus::BroadcastResult(
                            result, &AudioSystemImplementationRequestBus::Events::SetListenerPosition, listener->m_pImplData,
                            request.m_transform);

                        if (result == EAudioRequestStatus::Success)
                        {
                            listener->oPosition = request.m_transform;
                        }
                    }
                    else
                    {
                        g_audioLogger.Log(LogType::Comment, "ATL could not find listener with ID %u", listenerId);
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                }

                else
                {
                    static_assert(false, "AudioRequestVariant visitor is non-exhaustive across all variant types!");
                    result = EAudioRequestStatus::FailureInvalidRequest;
                }

                request.SetStatus(result);
                if (request.m_callback)
                {
                    hasCallback = true;
                }

                return result;
            }
            , requestVariant);

        if (hasCallback)
        {
            AZ::Interface<IAudioSystem>::Get()->PushCallbackNew(AZStd::move(requestVariant));
        }

        if (status != EAudioRequestStatus::Success)
        {
            g_audioLogger.Log(LogType::Error, "Audio Request did not succeed!\n");
        }
    }
    //~ NEW AUDIO REQUESTS

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioTranslationLayer::GetAudioTriggerID(const char* const audioTriggerName) const
    {
        auto triggerId = AudioStringToID<TAudioControlID>(audioTriggerName);
        auto it = m_cTriggers.find(triggerId);
        if (it == m_cTriggers.end())
        {
            return INVALID_AUDIO_CONTROL_ID;
        }

        return triggerId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioTranslationLayer::GetAudioRtpcID(const char* const audioRtpcName) const
    {
        auto rtpcId = AudioStringToID<TAudioControlID>(audioRtpcName);
        auto it = m_cRtpcs.find(rtpcId);
        if (it == m_cRtpcs.end())
        {
            return INVALID_AUDIO_CONTROL_ID;
        }

        return rtpcId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioTranslationLayer::GetAudioSwitchID(const char* const audioStateName) const
    {
        auto switchId = AudioStringToID<TAudioControlID>(audioStateName);
        auto it = m_cSwitches.find(switchId);
        if (it == m_cSwitches.end())
        {
            return INVALID_AUDIO_CONTROL_ID;
        }

        return switchId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioSwitchStateID CAudioTranslationLayer::GetAudioSwitchStateID(const TAudioControlID switchId, const char* const audioSwitchStateName) const
    {
        auto stateId = AudioStringToID<TAudioSwitchStateID>(audioSwitchStateName);
        auto itSwitch = m_cSwitches.find(switchId);
        if (itSwitch != m_cSwitches.end())
        {
            auto itState = itSwitch->second->cStates.find(stateId);
            if (itState == itSwitch->second->cStates.end())
            {
                return INVALID_AUDIO_SWITCH_STATE_ID;
            }
        }

        return stateId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioPreloadRequestID CAudioTranslationLayer::GetAudioPreloadRequestID(const char* const audioPreloadRequestName) const
    {
        auto preloadRequestId = AudioStringToID<TAudioPreloadRequestID>(audioPreloadRequestName);
        auto it = m_cPreloadRequests.find(preloadRequestId);
        if (it == m_cPreloadRequests.end())
        {
            return INVALID_AUDIO_PRELOAD_REQUEST_ID;
        }

        return preloadRequestId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioEnvironmentID CAudioTranslationLayer::GetAudioEnvironmentID(const char* const audioEnvironmentName) const
    {
        auto environmentId = AudioStringToID<TAudioEnvironmentID>(audioEnvironmentName);
        auto it = m_cEnvironments.find(environmentId);
        if (it == m_cEnvironments.end())
        {
            return INVALID_AUDIO_ENVIRONMENT_ID;
        }

        return environmentId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReserveAudioObjectID(TAudioObjectID& rAudioObjectID)
    {
        return m_oAudioObjectMgr.ReserveID(rAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReleaseAudioObjectID(const TAudioObjectID nAudioObjectID)
    {
        const bool bSuccess = m_oAudioObjectMgr.ReleaseID(nAudioObjectID);

#if !defined(AUDIO_RELEASE)
        if (bSuccess)
        {
            m_oDebugNameStore.RemoveAudioObject(nAudioObjectID);
        }
#endif // !AUDIO_RELEASE

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReserveAudioListenerID(TAudioObjectID& rAudioObjectID)
    {
        return m_oAudioListenerMgr.ReserveID(rAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReleaseAudioListenerID(const TAudioObjectID nAudioObjectID)
    {
        return m_oAudioListenerMgr.ReleaseID(nAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::SetAudioListenerOverrideID(const TAudioObjectID nAudioObjectID)
    {
        return m_oAudioListenerMgr.SetOverrideListenerID(nAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::AddRequestListener(const SAudioEventListener& listener)
    {
        m_oAudioEventListenerMgr.AddRequestListener(listener);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::RemoveRequestListener(const SAudioEventListener& listener)
    {
        m_oAudioEventListenerMgr.RemoveRequestListener(listener);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ParseControlsData(const char* const pConfigFolderPath, const EATLDataScope eDataScope)
    {
        m_oXmlProcessor.ParseControlsData(pConfigFolderPath, eDataScope);
        m_oXmlProcessor.ParsePreloadsData(pConfigFolderPath, eDataScope);
        return EAudioRequestStatus::Success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ClearControlsData(const EATLDataScope eDataScope)
    {
        m_oXmlProcessor.ClearControlsData(eDataScope);
        m_oXmlProcessor.ClearPreloadsData(eDataScope);
        return EAudioRequestStatus::Success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::string& CAudioTranslationLayer::GetControlsImplSubPath() const
    {
        return m_implSubPath;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioSourceId CAudioTranslationLayer::CreateAudioSource(const SAudioInputConfig& sourceConfig)
    {
        AZ_Assert(sourceConfig.m_sourceId == INVALID_AUDIO_SOURCE_ID, "ATL - Request to CreateAudioSource already contains a valid source Id.\n");

        TAudioSourceId sourceId = ++m_nextSourceId;

        Audio::SystemRequest::CreateSource createSourceRequest(sourceConfig);
        createSourceRequest.m_sourceConfig.m_sourceId = sourceId;
        AZ::Interface<IAudioSystem>::Get()->PushRequestBlockingNew(AZStd::move(createSourceRequest));

        return sourceId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DestroyAudioSource([[maybe_unused]] TAudioSourceId sourceId)
    {
        Audio::SystemRequest::DestroySource destroySourceRequest(sourceId);
        AZ::Interface<IAudioSystem>::Get()->PushRequestNew(AZStd::move(destroySourceRequest));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //void CAudioTranslationLayer::NotifyListener(const CAudioRequestInternal& rRequest)
    //{
    //    // This should always be the main thread!
    //    TATLEnumFlagsType nSpecificAudioRequest = INVALID_AUDIO_ENUM_FLAG_TYPE;
    //    TAudioControlID nAudioControlID = INVALID_AUDIO_CONTROL_ID;
    //    TAudioEventID audioEventID = INVALID_AUDIO_EVENT_ID;

    //    switch (rRequest.pData->eRequestType)
    //    {
    //        case eART_AUDIO_MANAGER_REQUEST:
    //        {
    //            auto const pRequestDataBase = static_cast<const SAudioManagerRequestDataInternalBase*>(rRequest.pData.get());
    //            nSpecificAudioRequest = static_cast<TATLEnumFlagsType>(pRequestDataBase->eType);
    //            break;
    //        }
    //        case eART_AUDIO_CALLBACK_MANAGER_REQUEST:
    //        {
    //            auto const pRequestDataBase = static_cast<const SAudioCallbackManagerRequestDataInternalBase*>(rRequest.pData.get());
    //            nSpecificAudioRequest = static_cast<TATLEnumFlagsType>(pRequestDataBase->eType);
    //            switch (pRequestDataBase->eType)
    //            {
    //                case eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE:
    //                {
    //                    auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE>*>(pRequestDataBase);
    //                    nAudioControlID = pRequestData->nAudioTriggerID;
    //                    break;
    //                }
    //                case eACMRT_REPORT_STARTED_EVENT:
    //                {
    //                    auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_STARTED_EVENT>*>(pRequestDataBase);
    //                    audioEventID = pRequestData->nEventID;
    //                    break;
    //                }
    //            }
    //            break;
    //        }
    //        case eART_AUDIO_OBJECT_REQUEST:
    //        {
    //            auto const pRequestDataBase = static_cast<const SAudioObjectRequestDataInternalBase*>(rRequest.pData.get());
    //            nSpecificAudioRequest = static_cast<TATLEnumFlagsType>(pRequestDataBase->eType);

    //            switch (pRequestDataBase->eType)
    //            {
    //                case eAORT_EXECUTE_TRIGGER:
    //                {
    //                    auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_EXECUTE_TRIGGER>*>(pRequestDataBase);
    //                    nAudioControlID = pRequestData->nTriggerID;
    //                    break;
    //                }
    //            }
    //            break;
    //        }
    //        case eART_AUDIO_LISTENER_REQUEST:
    //        {
    //            auto const pRequestDataBase = static_cast<const SAudioListenerRequestDataInternalBase*>(rRequest.pData.get());
    //            nSpecificAudioRequest = static_cast<TATLEnumFlagsType>(pRequestDataBase->eType);
    //            break;
    //        }
    //        default:
    //        {
    //            g_audioLogger.Log(LogType::Assert, "Unknown request type during CAudioTranslationLayer::NotifyListener!");
    //            break;
    //        }
    //    }

    //    const SAudioRequestInfo oResult(
    //        ConvertToRequestResult(rRequest.eStatus),
    //        rRequest.pOwner,
    //        rRequest.pUserData,
    //        rRequest.pUserDataOwner,
    //        rRequest.pData->eRequestType,
    //        nSpecificAudioRequest,
    //        nAudioControlID,
    //        rRequest.nAudioObjectID,
    //        audioEventID);

    //    m_oAudioEventListenerMgr.NotifyListener(&oResult);
    //}

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //EAudioRequestStatus CAudioTranslationLayer::ProcessAudioCallbackManagerRequest(const SAudioRequestDataInternal* pPassedRequestData)
    //{
    //    EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can we switch this to None?

    //    if (pPassedRequestData)
    //    {
    //        auto const pRequestDataBase = static_cast<const SAudioCallbackManagerRequestDataInternalBase*>(pPassedRequestData);

    //        switch (pRequestDataBase->eType)
    //        {
    //            case eACMRT_REPORT_STARTED_EVENT:
    //            {
    //                auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_STARTED_EVENT>*>(pPassedRequestData);
    //                CATLEvent* const pEvent = m_oAudioEventMgr.LookupID(pRequestData->nEventID);

    //                if (pEvent)
    //                {
    //                    pEvent->m_audioEventState = eAES_PLAYING_DELAYED;

    //                    if (pEvent->m_nObjectID != m_nGlobalAudioObjectID)
    //                    {
    //                        m_oAudioObjectMgr.ReportStartedEvent(pEvent);
    //                    }
    //                    else
    //                    {
    //                        m_pGlobalAudioObject->ReportStartedEvent(pEvent);
    //                    }
    //                }

    //                eResult = EAudioRequestStatus::Success;
    //                break;
    //            }
    //            case eACMRT_REPORT_FINISHED_EVENT:
    //            {
    //                auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_FINISHED_EVENT>*>(pPassedRequestData);
    //                CATLEvent* const pEvent = m_oAudioEventMgr.LookupID(pRequestData->nEventID);

    //                if (pEvent)
    //                {
    //                    if (pEvent->m_nObjectID != m_nGlobalAudioObjectID)
    //                    {
    //                        m_oAudioObjectMgr.ReportFinishedEvent(pEvent, pRequestData->bSuccess);
    //                    }
    //                    else if (m_pGlobalAudioObject)
    //                    {   // safety-net check to prevent crash if a finished callback is received very late
    //                        m_pGlobalAudioObject->ReportFinishedEvent(pEvent, pRequestData->bSuccess);
    //                    }

    //                    m_oAudioEventMgr.ReleaseEvent(pEvent);
    //                }

    //                eResult = EAudioRequestStatus::Success;
    //                break;
    //            }
    //            case eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE:
    //                [[fallthrough]];
    //            case eACMRT_NONE:
    //            {
    //                eResult = EAudioRequestStatus::Success;
    //                break;
    //            }
    //            default:
    //            {
    //                eResult = EAudioRequestStatus::FailureInvalidRequest;
    //                g_audioLogger.Log(LogType::Warning, "ATL received an unknown AudioCallbackManager request: %u", pRequestDataBase->eType);
    //                break;
    //            }
    //        }
    //    }

    //    return eResult;
    //}

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::InitializeImplComponent()
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can this be switched to None?

        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::Initialize);
        if (eResult == EAudioRequestStatus::Success)
        {
            IATLAudioObjectData* pGlobalObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pGlobalObjectData, &AudioSystemImplementationRequestBus::Events::NewGlobalAudioObjectData, m_nGlobalAudioObjectID);

            m_pGlobalAudioObject = azcreate(CATLGlobalAudioObject, (m_nGlobalAudioObjectID, pGlobalObjectData), Audio::AudioSystemAllocator, "ATLGlobalAudioObject");

            m_oAudioObjectMgr.Initialize();
            m_oAudioEventMgr.Initialize();
            m_oAudioListenerMgr.Initialize();
            m_oXmlProcessor.Initialize();
            m_oFileCacheMgr.Initialize();

            SetImplLanguage();

            // Update the implementation subpath for locating ATL controls...
            AudioSystemImplementationRequestBus::BroadcastResult(m_implSubPath, &AudioSystemImplementationRequestBus::Events::GetImplSubPath);
        }

#if !defined(AUDIO_RELEASE)
        else
        {
            const char* implementationName = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(implementationName, &AudioSystemImplementationRequestBus::Events::GetImplementationNameString);
            g_audioLogger.Log(LogType::Error, "Failed to Initialize the AudioSystemImplementationComponent '%s'\n", implementationName);
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::ReleaseImplComponent()
    {
        // During audio middleware shutdown we do not allow for any new requests originating from the "dying" audio middleware!
        m_nFlags |= eAIS_AUDIO_MIDDLEWARE_SHUTTING_DOWN;

        m_oXmlProcessor.ClearControlsData(eADS_ALL);
        m_oXmlProcessor.ClearPreloadsData(eADS_ALL);

        if (m_pGlobalAudioObject)
        {
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, m_pGlobalAudioObject->GetImplDataPtr());
            azdestroy(m_pGlobalAudioObject, Audio::AudioSystemAllocator);
            m_pGlobalAudioObject = nullptr;
        }

        m_oAudioObjectMgr.Release();
        m_oAudioListenerMgr.Release();
        m_oAudioEventMgr.Release();
        m_oFileCacheMgr.Release();
        m_oXmlProcessor.Release();

        m_implSubPath.clear();

        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can this be switched to None?
        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::ShutDown);

        // If we allow developers to change the audio implementation module at run-time, these should be at Warning level.
        // If we ever revoke that functionality, these should be promoted to Asserts.
        AZ_Warning("ATL", eResult == EAudioRequestStatus::Success,
            "ATL ReleaseImplComponent - Shutting down the audio implementation failed!");

        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::Release);
        AZ_Warning("ATL", eResult == EAudioRequestStatus::Success,
            "ATL ReleaseImplComponent - Releasing the audio implementation failed!");

        m_nFlags &= ~eAIS_AUDIO_MIDDLEWARE_SHUTTING_DOWN;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::PrepUnprepTriggerAsync(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger,
        const bool bPrepare)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can this be switched to None?

        const TAudioObjectID nATLObjectID = pAudioObject->GetID();
        const TObjectTriggerImplStates& rTriggerImplStates = pAudioObject->GetTriggerImpls();

        for (auto const triggerImpl : pTrigger->m_cImplPtrs)
        {
            TATLEnumFlagsType nTriggerImplFlags = INVALID_AUDIO_ENUM_FLAG_TYPE;
            TObjectTriggerImplStates::const_iterator iPlace = rTriggerImplStates.end();
            if (FindPlaceConst(rTriggerImplStates, triggerImpl->m_nATLID, iPlace))
            {
                nTriggerImplFlags = iPlace->second.nFlags;
            }

            const EATLSubsystem eReceiver = triggerImpl->GetReceiver();
            CATLEvent* pEvent = m_oAudioEventMgr.GetEvent(eReceiver);

            EAudioRequestStatus ePrepUnprepResult = EAudioRequestStatus::Failure;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    if (bPrepare)
                    {
                        if (((nTriggerImplFlags & eATS_PREPARED) == 0) && ((nTriggerImplFlags & eATS_LOADING) == 0))
                        {
                            AudioSystemImplementationRequestBus::BroadcastResult(ePrepUnprepResult, &AudioSystemImplementationRequestBus::Events::PrepareTriggerAsync,
                                pAudioObject->GetImplDataPtr(),
                                triggerImpl->m_pImplData,
                                pEvent->m_pImplData);
                        }
                    }
                    else
                    {
                        if (((nTriggerImplFlags & eATS_PREPARED) != 0) && ((nTriggerImplFlags & eATS_UNLOADING) == 0))
                        {
                            AudioSystemImplementationRequestBus::BroadcastResult(ePrepUnprepResult, &AudioSystemImplementationRequestBus::Events::UnprepareTriggerAsync,
                                pAudioObject->GetImplDataPtr(),
                                triggerImpl->m_pImplData,
                                pEvent->m_pImplData);
                        }
                    }

                    if (ePrepUnprepResult == EAudioRequestStatus::Success)
                    {
                        pEvent->m_nObjectID = nATLObjectID;
                        pEvent->m_nTriggerID = triggerImpl->m_nATLID;
                        pEvent->m_nTriggerImplID = triggerImpl->m_nATLID;

                        pEvent->m_audioEventState = bPrepare ? eAES_LOADING : eAES_UNLOADING;
                    }
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    //TODO: handle this
                    break;
                }
                default:
                {
                    g_audioLogger.Log(LogType::Error, "Unknown ATL Recipient");
                    break;
                }
            }

            if (ePrepUnprepResult == EAudioRequestStatus::Success)
            {
                pEvent->SetDataScope(pTrigger->GetDataScope());
                pAudioObject->ReportStartedEvent(pEvent);
                pAudioObject->IncrementRefCount();
                eResult = EAudioRequestStatus::Success; // if at least one event fires, it is a success
            }
            else
            {
                m_oAudioEventMgr.ReleaseEvent(pEvent);
            }
        }

#if !defined(AUDIO_RELEASE)
        if (eResult != EAudioRequestStatus::Success)
        {
            // No TriggerImpl produced an active event.
            g_audioLogger.Log(LogType::Warning, "PrepUnprepTriggerAsync failed on AudioObject \"%s\" (ID: %u)", m_oDebugNameStore.LookupAudioObjectName(nATLObjectID), nATLObjectID);
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ActivateTrigger(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger,
        void* const pOwner /* = nullptr */,
        void* const pUserData /* = nullptr */,
        const TATLEnumFlagsType nFlags /* = INVALID_AUDIO_ENUM_FLAG_TYPE */,
        const SATLSourceData* pSourceData /* = nullptr */)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can this be switched to None?

        if (pAudioObject->HasPosition())
        {
            // If the AudioObject uses Obstruction/Occlusion then set the values before activating the trigger.
            auto const pPositionedAudioObject = static_cast<CATLAudioObject*>(pAudioObject);

            if (pPositionedAudioObject->CanRunRaycasts() && !pPositionedAudioObject->HasActiveEvents())
            {
                pPositionedAudioObject->RunRaycasts(m_oSharedData.m_oActiveListenerPosition);
            }
        }

        const TAudioControlID nATLTriggerID = pTrigger->GetID();

        // Sets eATS_STARTING on this TriggerInstance to avoid
        // reporting TriggerFinished while the events are being started.
        pAudioObject->ReportStartingTriggerInstance(m_nTriggerInstanceIDCounter, nATLTriggerID);

        for (auto const triggerImpl : pTrigger->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = triggerImpl->GetReceiver();
            CATLEvent* const pEvent = m_oAudioEventMgr.GetEvent(eReceiver);
            pEvent->m_pImplData->m_triggerId = nATLTriggerID;

            EAudioRequestStatus eActivateResult = EAudioRequestStatus::Failure;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eActivateResult, &AudioSystemImplementationRequestBus::Events::ActivateTrigger,
                        pAudioObject->GetImplDataPtr(),
                        triggerImpl->m_pImplData,
                        pEvent->m_pImplData,
                        pSourceData);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    eActivateResult = ActivateInternalTrigger(
                            pAudioObject,
                            triggerImpl->m_pImplData,
                            pEvent->m_pImplData);
                    break;
                }
                default:
                {
                    g_audioLogger.Log(LogType::Error, "Unknown ATL Recipient");
                    break;
                }
            }

            if (eActivateResult == EAudioRequestStatus::Success || eActivateResult == EAudioRequestStatus::Pending)
            {
                pEvent->m_nObjectID = pAudioObject->GetID();
                pEvent->m_nTriggerID = nATLTriggerID;
                pEvent->m_nTriggerImplID = triggerImpl->m_nATLID;
                pEvent->m_nTriggerInstanceID = m_nTriggerInstanceIDCounter;
                pEvent->SetDataScope(pTrigger->GetDataScope());

                if (eActivateResult == EAudioRequestStatus::Success)
                {
                    pEvent->m_audioEventState = eAES_PLAYING;
                }
                else if (eActivateResult == EAudioRequestStatus::Pending)
                {
                    pEvent->m_audioEventState = eAES_LOADING;
                }

                pAudioObject->ReportStartedEvent(pEvent);
                pAudioObject->IncrementRefCount();

                // If at least one event fires, it is a success: the trigger has been activated.
                eResult = EAudioRequestStatus::Success;
            }
            else
            {
                m_oAudioEventMgr.ReleaseEvent(pEvent);
            }
        }

        // Either removes the eATS_STARTING flag on this trigger instance or removes it if no event was started.
        pAudioObject->ReportStartedTriggerInstance(m_nTriggerInstanceIDCounter++, pOwner, pUserData, nFlags);

#if !defined(AUDIO_RELEASE)
        if (eResult != EAudioRequestStatus::Success)
        {
            // No TriggerImpl generated an active event.
            g_audioLogger.Log(LogType::Warning, "Trigger \"%s\" failed on AudioObject \"%s\" (ID: %u)", m_oDebugNameStore.LookupAudioTriggerName(nATLTriggerID), m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID()), pAudioObject->GetID());
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopTrigger(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can this be switched to None?

        const TAudioControlID nATLTriggerID = pTrigger->GetID();

        TObjectEventSet rEvents = pAudioObject->GetActiveEvents();

        for (auto eventId : rEvents)
        {
            const CATLEvent* const pEvent = m_oAudioEventMgr.LookupID(eventId);

            if (pEvent && pEvent->IsPlaying() && (pEvent->m_nTriggerID == nATLTriggerID))
            {
                switch (pEvent->m_eSender)
                {
                    case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                    {
                        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::StopEvent,
                            pAudioObject->GetImplDataPtr(),
                            pEvent->m_pImplData);
                        break;
                    }
                    case eAS_ATL_INTERNAL:
                    {
                        eResult = StopInternalEvent(pAudioObject, pEvent->m_pImplData);
                        break;
                    }
                    default:
                    {
                        g_audioLogger.Log(LogType::Error, "ATL - StopTrigger: Unknown ATL Recipient");
                        break;
                    }
                }
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopAllTriggers(CATLAudioObjectBase* const pAudioObject, void* const pOwner /* = nullptr*/)
    {
        if (!pOwner)
        {
            EAudioRequestStatus eResult = EAudioRequestStatus::Failure;
            AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::StopAllEvents, pAudioObject->GetImplDataPtr());
            return eResult;
        }
        else
        {
            EAudioRequestStatus eResult = EAudioRequestStatus::Success;

            auto triggerInstances = pAudioObject->GetTriggerInstancesByOwner(pOwner);

            auto activeEvents = pAudioObject->GetActiveEvents();

            for (auto eventId : activeEvents)
            {
                const CATLEvent* const atlEvent = m_oAudioEventMgr.LookupID(eventId);

                if (atlEvent && triggerInstances.find(atlEvent->m_nTriggerInstanceID) != triggerInstances.end())
                {
                    EAudioRequestStatus eSingleResult = EAudioRequestStatus::Failure;

                    switch (atlEvent->m_eSender)
                    {
                        case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                        {
                            AudioSystemImplementationRequestBus::BroadcastResult(eSingleResult, &AudioSystemImplementationRequestBus::Events::StopEvent,
                                pAudioObject->GetImplDataPtr(),
                                atlEvent->m_pImplData);
                            break;
                        }

                        case eAS_ATL_INTERNAL:
                        {
                            eSingleResult = StopInternalEvent(pAudioObject, atlEvent->m_pImplData);
                            break;
                        }

                        default:
                        {
                            g_audioLogger.Log(LogType::Error, "ATL - StopAllTriggersFiltered: Unknown ATL Recipient");
                            break;
                        }
                    }

                    if (eSingleResult != EAudioRequestStatus::Success)
                    {
                        eResult = EAudioRequestStatus::Failure;     // should we report partial failure?
                    }
                }
            }

            return eResult;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetSwitchState(
        CATLAudioObjectBase* const pAudioObject,
        const CATLSwitchState* const pState)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can this be switched to None?

        for (auto switchStateImpl : pState->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = switchStateImpl->GetReceiver();
            EAudioRequestStatus eSetStateResult = EAudioRequestStatus::Failure;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eSetStateResult, &AudioSystemImplementationRequestBus::Events::SetSwitchState,
                        pAudioObject->GetImplDataPtr(),
                        switchStateImpl->m_pImplData);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    eSetStateResult = SetInternalSwitchState(pAudioObject, switchStateImpl->m_pImplData);
                    break;
                }
                default:
                {
                    g_audioLogger.Log(LogType::Error, "Unknown ATL Recipient");
                    break;
                }
            }

            if (eSetStateResult == EAudioRequestStatus::Success)
            {
                eResult = EAudioRequestStatus::Success; // if at least one of the implementations is set successfully, it is a success
            }
        }

        if (eResult == EAudioRequestStatus::Success)
        {
            pAudioObject->SetSwitchState(pState->GetParentID(), pState->GetID());
        }
#if !defined(AUDIO_RELEASE)
        else
        {
            const char* const sSwitchName = m_oDebugNameStore.LookupAudioSwitchName(pState->GetParentID());
            const char* const sSwitchStateName = m_oDebugNameStore.LookupAudioSwitchStateName(pState->GetParentID(), pState->GetID());
            const char* const sAudioObjectName = m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID());
            g_audioLogger.Log(LogType::Warning, "Failed to set the ATLSwitch \"%s\" to ATLSwitchState \"%s\" on AudioObject \"%s\" (ID: %u)", sSwitchName, sSwitchStateName, sAudioObjectName, pAudioObject->GetID());
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetRtpc(
        CATLAudioObjectBase* const pAudioObject,
        const CATLRtpc* const pRtpc,
        const float fValue)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can this be switched to None?

        for (auto rtpcImpl : pRtpc->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = rtpcImpl->GetReceiver();
            EAudioRequestStatus eSetRtpcResult = EAudioRequestStatus::Failure;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eSetRtpcResult, &AudioSystemImplementationRequestBus::Events::SetRtpc,
                        pAudioObject->GetImplDataPtr(),
                        rtpcImpl->m_pImplData,
                        fValue);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    eSetRtpcResult = SetInternalRtpc(pAudioObject, rtpcImpl->m_pImplData, fValue);
                    break;
                }
                default:
                {
                    g_audioLogger.Log(LogType::Error, "Unknown ATL Recipient");
                    break;
                }
            }

            if (eSetRtpcResult == EAudioRequestStatus::Success)
            {
                eResult = EAudioRequestStatus::Success; // if at least one of the implementations is set successfully, it is a success
            }
        }

        if (eResult == EAudioRequestStatus::Success)
        {
            pAudioObject->SetRtpc(pRtpc->GetID(), fValue);
        }
#if !defined(AUDIO_RELEASE)
        else
        {
            const char* const sRtpcName = m_oDebugNameStore.LookupAudioRtpcName(pRtpc->GetID());
            const char* const sAudioObjectName = m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID());
            g_audioLogger.Log(LogType::Warning, "Failed to set the ATLRtpc \"%s\" to %f on AudioObject \"%s\" (ID: %u)", sRtpcName, fValue, sAudioObjectName, pAudioObject->GetID());
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ResetRtpcs(CATLAudioObjectBase* const pAudioObject)
    {
        const TObjectRtpcMap rRtpcs = pAudioObject->GetRtpcs();
        EAudioRequestStatus eResult = EAudioRequestStatus::Success;     // why is this one Success?

        for (auto& rtpcPair : rRtpcs)
        {
            auto it = m_cRtpcs.find(rtpcPair.first);
            if (it != m_cRtpcs.end())
            {
                for (auto rtpcImpl : it->second->m_cImplPtrs)
                {
                    EAudioRequestStatus eResetRtpcResult = EAudioRequestStatus::Failure;
                    switch (rtpcImpl->GetReceiver())
                    {
                        case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                        {
                            AudioSystemImplementationRequestBus::BroadcastResult(eResetRtpcResult, &AudioSystemImplementationRequestBus::Events::ResetRtpc,
                                pAudioObject->GetImplDataPtr(),
                                rtpcImpl->m_pImplData);
                            break;
                        }

                        case eAS_ATL_INTERNAL:
                        {
                            // Implement internal Rtpcs later
                            eResetRtpcResult = EAudioRequestStatus::Success;
                            break;
                        }

                        default:
                        {
                            g_audioLogger.Log(LogType::Error, "ATL - ResetRtpc: Unknown ATL Recipient");
                            break;
                        }
                    }

                    // If any reset failed, consider it an overall failure
                    if (eResetRtpcResult != EAudioRequestStatus::Success)
                    {
                        eResult = EAudioRequestStatus::Failure;
                    }
                }
            }
        }

        if (eResult == EAudioRequestStatus::Success)
        {
            pAudioObject->ClearRtpcs();
        }
#if !defined(AUDIO_RELEASE)
        else
        {
            const TAudioObjectID objectId = pAudioObject->GetID();

            g_audioLogger.Log(LogType::Warning,
                "Failed to Reset Rtpcs on AudioObject \"%s\" (ID: %u)",
                m_oDebugNameStore.LookupAudioObjectName(objectId),
                objectId);
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetEnvironment(
        CATLAudioObjectBase* const pAudioObject,
        const CATLAudioEnvironment* const pEnvironment,
        const float fAmount)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;     // can this be switched to None?

        for (auto environmentImpl : pEnvironment->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = environmentImpl->GetReceiver();
            EAudioRequestStatus eSetEnvResult = EAudioRequestStatus::Failure;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eSetEnvResult, &AudioSystemImplementationRequestBus::Events::SetEnvironment,
                        pAudioObject->GetImplDataPtr(),
                        environmentImpl->m_pImplData,
                        fAmount);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    eSetEnvResult = SetInternalEnvironment(pAudioObject, environmentImpl->m_pImplData, fAmount);
                    break;
                }
                default:
                {
                    g_audioLogger.Log(LogType::Error, "Unknown ATL Recipient");
                    break;
                }
            }

            if (eSetEnvResult == EAudioRequestStatus::Success)
            {
                eResult = EAudioRequestStatus::Success; // if at least one of the implementations is set successfully, it is a success
            }
        }

        if (eResult == EAudioRequestStatus::Success)
        {
            pAudioObject->SetEnvironmentAmount(pEnvironment->GetID(), fAmount);
        }
#if !defined(AUDIO_RELEASE)
        else
        {
            const char* const sEnvironmentName = m_oDebugNameStore.LookupAudioEnvironmentName(pEnvironment->GetID());
            const char* const sAudioObjectName = m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID());
            g_audioLogger.Log(LogType::Warning, "Failed to set the ATLAudioEnvironment \"%s\" to %f on AudioObject \"%s\" (ID: %u)", sEnvironmentName, fAmount, sAudioObjectName, pAudioObject->GetID());
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ResetEnvironments(CATLAudioObjectBase* const pAudioObject)
    {
        const TObjectEnvironmentMap rEnvironments = pAudioObject->GetEnvironments();

        EAudioRequestStatus eResult = EAudioRequestStatus::Success;

        for (auto& environmentAmountPair : rEnvironments)
        {
            auto it = m_cEnvironments.find(environmentAmountPair.first);
            if (it != m_cEnvironments.end())
            {
                const EAudioRequestStatus eSetEnvResult = SetEnvironment(pAudioObject, it->second, 0.0f);

                if (eSetEnvResult != EAudioRequestStatus::Success)
                {
                    // If setting at least one Environment fails, we consider this a failure.
                    eResult = EAudioRequestStatus::Failure;
                }
            }
        }

        if (eResult == EAudioRequestStatus::Success)
        {
            pAudioObject->ClearEnvironments();
        }
#if !defined(AUDIO_RELEASE)
        else
        {
            const TAudioObjectID nObjectID = pAudioObject->GetID();

            g_audioLogger.Log(
                LogType::Warning,
                "Failed to Reset AudioEnvironments on AudioObject \"%s\" (ID: %u)",
                m_oDebugNameStore.LookupAudioObjectName(nObjectID),
                nObjectID);
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ActivateInternalTrigger(
        [[maybe_unused]] CATLAudioObjectBase* const pAudioObject,
        [[maybe_unused]] const IATLTriggerImplData* const pTriggerData,
        [[maybe_unused]] IATLEventData* const pEventData)
    {
        //TODO implement
        return EAudioRequestStatus::Failure;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopInternalEvent(
        [[maybe_unused]] CATLAudioObjectBase* const pAudioObject,
        [[maybe_unused]] const IATLEventData* const pEventData)
    {
        //TODO implement
        return EAudioRequestStatus::Failure;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopAllInternalEvents([[maybe_unused]] CATLAudioObjectBase* const pAudioObject)
    {
        //TODO implement
        return EAudioRequestStatus::Failure;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetInternalRtpc(
        [[maybe_unused]] CATLAudioObjectBase* const pAudioObject,
        [[maybe_unused]] const IATLRtpcImplData* const pRtpcData,
        [[maybe_unused]] const float fValue)
    {
        //TODO implement
        return EAudioRequestStatus::Failure;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetInternalSwitchState(
        CATLAudioObjectBase* const pAudioObject,
        const IATLSwitchStateImplData* const pSwitchStateData)
    {
        auto const pInternalStateData = static_cast<const SATLSwitchStateImplData_internal*>(pSwitchStateData);

        //TODO: once there is more than one internal switch, a more sensible approach needs to be developed
        if (pInternalStateData->nATLInternalSwitchID == ATLInternalControlIDs::ObstructionOcclusionCalcSwitchID)
        {
            if (pAudioObject->HasPosition())
            {
                auto const pPositionedAudioObject = static_cast<CATLAudioObject*>(pAudioObject);

                if (pInternalStateData->nATLInternalStateID == ATLInternalControlIDs::OOCStateIDs[eAOOCT_IGNORE])
                {
                    SATLSoundPropagationData oPropagationData;
                    pPositionedAudioObject->SetRaycastCalcType(eAOOCT_IGNORE);
                    pPositionedAudioObject->GetObstOccData(oPropagationData);

                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::SetObstructionOcclusion,
                        pPositionedAudioObject->GetImplDataPtr(),
                        oPropagationData.fObstruction,
                        oPropagationData.fOcclusion);
                }
                else if (pInternalStateData->nATLInternalStateID == ATLInternalControlIDs::OOCStateIDs[eAOOCT_SINGLE_RAY])
                {
                    pPositionedAudioObject->SetRaycastCalcType(eAOOCT_SINGLE_RAY);
                }
                else if (pInternalStateData->nATLInternalStateID == ATLInternalControlIDs::OOCStateIDs[eAOOCT_MULTI_RAY])
                {
                    pPositionedAudioObject->SetRaycastCalcType(eAOOCT_MULTI_RAY);
                }
                else
                {
                    g_audioLogger.Log(LogType::Warning, "SetInternalSwitchState - Unknown value specified for SetObstructionOcclusionCalc");
                }
            }
        }
        else if (pInternalStateData->nATLInternalSwitchID == ATLInternalControlIDs::ObjectVelocityTrackingSwitchID)
        {
            if (pAudioObject->HasPosition())
            {
                auto const pPositionedAudioObject = static_cast<CATLAudioObject*>(pAudioObject);
                if (pInternalStateData->nATLInternalStateID == ATLInternalControlIDs::OVTOnStateID)
                {
                    pPositionedAudioObject->SetVelocityTracking(true);
                }
                else if (pInternalStateData->nATLInternalStateID == ATLInternalControlIDs::OVTOffStateID)
                {
                    pPositionedAudioObject->SetVelocityTracking(false);
                }
                else
                {
                    g_audioLogger.Log(LogType::Warning, "SetInternalSwitchState - Unknown value specified for SetVelocityTracking (ly-fixit update this name!)");
                }
            }
        }

        return EAudioRequestStatus::Success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetInternalEnvironment(
        [[maybe_unused]] CATLAudioObjectBase* const pAudioObject,
        [[maybe_unused]] const IATLEnvironmentImplData* const pEnvironmentImplData,
        [[maybe_unused]] const float fAmount)
    {
        // TODO: implement
        return EAudioRequestStatus::Failure;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::MuteAll()
    {
        EAudioRequestStatus result = EAudioRequestStatus::Failure;
        const CATLTrigger* trigger = nullptr;
        if (auto it = m_cTriggers.find(ATLInternalControlIDs::MuteAllTriggerID);
            it != m_cTriggers.end())
        {
            trigger = it->second;
        }

        if (trigger)
        {
            result = ActivateTrigger(m_pGlobalAudioObject, trigger);
        }
        else
        {
            g_audioLogger.Log(LogType::Warning, "ATL - Trigger not found for: ATLInternalControlIDs::MuteAllTriggerID");
        }

        if (result == EAudioRequestStatus::Success)
        {
            m_nFlags |= eAIS_IS_MUTED;
        }

        AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemMuteAll);
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::UnmuteAll()
    {
        EAudioRequestStatus result = EAudioRequestStatus::Failure;
        const CATLTrigger* trigger = nullptr;
        if (auto it = m_cTriggers.find(ATLInternalControlIDs::UnmuteAllTriggerID);
            it != m_cTriggers.end())
        {
            trigger = it->second;
        }

        if (trigger)
        {
            result = ActivateTrigger(m_pGlobalAudioObject, trigger);
        }
        else
        {
            g_audioLogger.Log(LogType::Warning, "ATL - Trigger not found for: ATLInternalControlIDs::UnmuteAllTriggerID");
        }

        if (result == EAudioRequestStatus::Success)
        {
            m_nFlags &= ~eAIS_IS_MUTED;
        }

        AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemUnmuteAll);
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::LoseFocus()
    {
        EAudioRequestStatus result = EAudioRequestStatus::Success;      // default is success when nothing needs to happen
    #if !defined(AUDIO_RELEASE)
        if (!Audio::CVars::s_IgnoreWindowFocus && 0 == (m_nFlags & eAIS_IS_MUTED))
    #endif // !AUDIO_RELEASE
        {
            if (auto it = m_cTriggers.find(ATLInternalControlIDs::LoseFocusTriggerID); it != m_cTriggers.end())
            {
                result = ActivateTrigger(m_pGlobalAudioObject, it->second);
            }
            else
            {
                g_audioLogger.Log(LogType::Warning, "ATL - Trigger not found for: 'lose_focus'");
                result = EAudioRequestStatus::FailureInvalidControlId;
            }

            AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemLoseFocus);
        }
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::GetFocus()
    {
        EAudioRequestStatus result = EAudioRequestStatus::Success;      // default is success when nothing needs to happen
    #if !defined(AUDIO_RELEASE)
        if (!Audio::CVars::s_IgnoreWindowFocus && 0 == (m_nFlags & eAIS_IS_MUTED))
    #endif // !AUDIO_RELEASE
        {
            AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemGetFocus);

            if (auto it = m_cTriggers.find(ATLInternalControlIDs::GetFocusTriggerID); it != m_cTriggers.end())
            {
                result = ActivateTrigger(m_pGlobalAudioObject, it->second);
            }
            else
            {
                g_audioLogger.Log(LogType::Warning, "ATL - Trigger not found for: 'get_focus'");
                result = EAudioRequestStatus::FailureInvalidControlId;
            }
        }
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::UpdateSharedData()
    {
        m_oAudioListenerMgr.GetDefaultListenerPosition(m_oSharedData.m_oActiveListenerPosition);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::SetImplLanguage()
    {
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            AZ::CVarFixedString languageAudio;
            if (auto result = console->GetCvarValue("g_languageAudio", languageAudio); result == AZ::GetValueResult::Success)
            {
                AudioSystemImplementationRequestBus::Broadcast(
                    &AudioSystemImplementationRequestBus::Events::SetLanguage, languageAudio.data());
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObjectBase* CAudioTranslationLayer::GetRequestObject(TAudioObjectID objectId)
    {
        if (objectId == INVALID_AUDIO_OBJECT_ID)
        {
            return static_cast<CATLAudioObjectBase*>(m_pGlobalAudioObject);
        }
        else
        {
            return static_cast<CATLAudioObjectBase*>(m_oAudioObjectMgr.LookupID(objectId));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_UNLOAD:
            RaycastProcessor::s_raycastsEnabled = false;
            break;
        case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
        case ESYSTEM_EVENT_LEVEL_PRECACHE_START:
            RaycastProcessor::s_raycastsEnabled = true;
            break;
        case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
            RaycastProcessor::s_raycastsEnabled = (wparam != 0);
            break;
        default:
            break;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::RefreshAudioSystem(const char* const controlsPath, const char* const levelName, TAudioPreloadRequestID levelPreloadId)
    {
        g_audioLogger.Log(LogType::Always, "$8Beginning to refresh the AudioSystem!");

        if (!controlsPath || controlsPath[0] == '\0')
        {
            g_audioLogger.Log(LogType::Error, "ATL RefreshAudioSystem - Controls path is null, can't complete the refresh!");
            return EAudioRequestStatus::Failure;
        }

        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;
        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::StopAllSounds);
        AZ_Error("AudioTranslationLayer", eResult == EAudioRequestStatus::Success,
            "ATL RefreshAudioSystem - Failed to StopAllSounds!");

        eResult = m_oFileCacheMgr.UnloadDataByScope(eADS_LEVEL_SPECIFIC);
        AZ_Error("AudioTranslationLayer", eResult == EAudioRequestStatus::Success,
            "ATL RefreshAudioSystem - Failed to unload old level banks!");

        eResult = m_oFileCacheMgr.UnloadDataByScope(eADS_GLOBAL);
        AZ_Error("AudioTranslationLayer", eResult == EAudioRequestStatus::Success,
            "ATL RefreshAudioSystem - Failed to unload old global banks!");

        eResult = ClearControlsData(eADS_ALL);
        AZ_Error("AudioTranslationLayer", eResult == EAudioRequestStatus::Success,
            "ATL RefreshAudioSystem - Failed to clear old controls data!");

        AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemRefresh);

        SetImplLanguage();

        eResult = ParseControlsData(controlsPath, eADS_GLOBAL);
        AZ_Error("AudioTranslationLayer", eResult == EAudioRequestStatus::Success,
            "ATL RefreshAudioSystem - Failed to load fresh global controls data!");

        eResult = m_oFileCacheMgr.TryLoadRequest(ATLInternalControlIDs::GlobalPreloadRequestID, true, true);
        AZ_Error("AudioTranslationLayer", eResult == EAudioRequestStatus::Success,
            "ATL RefreshAudioSystem - Failed to load fresh global banks!");

        if (levelName && levelName[0] != '\0')
        {
            AZStd::string levelControlsPath(controlsPath);
            levelControlsPath.append("levels/");
            levelControlsPath.append(levelName);
            AZ::StringFunc::RelativePath::Normalize(levelControlsPath);

            eResult = ParseControlsData(levelControlsPath.c_str(), eADS_LEVEL_SPECIFIC);
            AZ_Error("AudioTranslationLayer", eResult == EAudioRequestStatus::Success,
                "ATL RefreshAudioSystem - Failed to parse fresh level controls data!");

            if (levelPreloadId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
            {
                eResult = m_oFileCacheMgr.TryLoadRequest(levelPreloadId, true, true);
                AZ_Error("AudioTranslationLayer", eResult == EAudioRequestStatus::Success,
                    "ATL RefreshAudioSystem - Failed to load fresh level banks!");
            }
        }

        if (m_nFlags & eAIS_IS_MUTED)
        {
            // restore the muted state...
            MuteAll();
        }

        g_audioLogger.Log(LogType::Always, "$3Done refreshing the AudioSystem!");

        return EAudioRequestStatus::Success;
    }

#if !defined(AUDIO_RELEASE)

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReserveAudioObjectID(TAudioObjectID& rAudioObjectID, const char* const sAudioObjectName)
    {
        const bool bSuccess = m_oAudioObjectMgr.ReserveID(rAudioObjectID, sAudioObjectName);

        if (bSuccess)
        {
            m_oDebugNameStore.AddAudioObject(rAudioObjectID, sAudioObjectName);
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawAudioSystemDebugInfo()
    {
        AZ_PROFILE_FUNCTION(Audio);

        // ToDo: Update to work with Atom? LYN-3677
        /*if (CVars::s_debugDrawOptions.GetRawFlags() != 0)
        {
            DrawAudioObjectDebugInfo(*pAuxGeom); // needs to be called first so that the rest of the labels are printed
            // on top (Draw2dLabel doesn't provide a way set which labels are printed on top)

            const size_t nPrimaryPoolSize = AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().Capacity();
            const size_t nPrimaryPoolUsedSize = nPrimaryPoolSize - AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GetUnAllocatedMemory();

            float fPosX = 0.0f;
            float fPosY = 4.0f;

            const float fColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
            const float fColorRed[4] = { 1.0f, 0.0f, 0.0f, 0.7f };
            const float fColorGreen[4] = { 0.0f, 1.0f, 0.0f, 0.7f };
            const float fColorBlue[4] = { 0.4f, 0.4f, 1.0f, 1.0f };

            const char* implementationName = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(implementationName, &AudioSystemImplementationRequestBus::Events::GetImplementationNameString);
            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.6f, fColorBlue, false, "AudioTranslationLayer with %s", implementationName);

            fPosX += 20.0f;
            fPosY += 17.0f;

            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.35f, fColor, false,
                "AudioSystem Memory: %.2f / %.2f MiB",
                (nPrimaryPoolUsedSize / 1024) / 1024.f,
                (nPrimaryPoolSize / 1024) / 1024.f
                );


            float const fLineHeight = 13.0f;

            SAudioImplMemoryInfo oMemoryInfo;
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::GetMemoryInfo, oMemoryInfo);

            fPosY += fLineHeight;
            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.35f, fColor, false,
                "AudioImpl Memory: %.2f / %.2f MiB",
                (oMemoryInfo.nPrimaryPoolUsedSize / 1024) / 1024.f,
                (oMemoryInfo.nPrimaryPoolSize / 1024) / 1024.f
                );

            static const float SMOOTHING_ALPHA = 0.2f;

            const AZ::Vector3 vPos = m_oSharedData.m_oActiveListenerPosition.GetPositionVec();
            const AZ::Vector3 vFwd = m_oSharedData.m_oActiveListenerPosition.GetForwardVec();
            const size_t nNumAudioObjects = m_oAudioObjectMgr.GetNumAudioObjects();
            const size_t nNumActiveAudioObjects = m_oAudioObjectMgr.GetNumActiveAudioObjects();
            const size_t nEvents = m_oAudioEventMgr.GetNumActive();
            const size_t nListeners = m_oAudioListenerMgr.GetNumActive();
            const size_t nNumEventListeners = m_oAudioEventListenerMgr.GetNumEventListeners();


            const bool bActive = true;
            const float fColorListener[4] =
            {
                bActive ? fColorGreen[0] : fColorRed[0],
                bActive ? fColorGreen[1] : fColorRed[1],
                bActive ? fColorGreen[2] : fColorRed[2],
                1.0f
            };

            const float* fColorNumbers = fColorBlue;

            TAudioObjectID activeListenerID = INVALID_AUDIO_OBJECT_ID;
            if (CATLListenerObject* overrideListener = m_oAudioListenerMgr.LookupID(m_oAudioListenerMgr.GetOverrideListenerID()))
            {
                activeListenerID = overrideListener->GetID();
            }
            else if (CATLListenerObject* defaultListener = m_oAudioListenerMgr.LookupID(m_oAudioListenerMgr.GetDefaultListenerID()))
            {
                activeListenerID = defaultListener->GetID();
            }

            fPosY += fLineHeight;
            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.35f, fColorListener, false,
                "Listener <%llu> PosXYZ: %.2f %.2f %.2f FwdXYZ: %.2f %.2f %.2f",
                activeListenerID,
                static_cast<float>(vPos.GetX()), static_cast<float>(vPos.GetY()), static_cast<float>(vPos.GetZ()),
                static_cast<float>(vFwd.GetX()), static_cast<float>(vFwd.GetY()), static_cast<float>(vFwd.GetZ()));

            fPosY += fLineHeight;
            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.35f, fColorNumbers, false,
                "Objects: %3zu/%3zu | Events: %3zu  EventListeners %3zu | Listeners: %zu",
                nNumActiveAudioObjects, nNumAudioObjects, nEvents, nNumEventListeners, nListeners);

            fPosY += fLineHeight;
            DrawATLComponentDebugInfo(*pAuxGeom, fPosX, fPosY);

            pAuxGeom->Commit(7);
        }*/
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawATLComponentDebugInfo(IRenderAuxGeom& auxGeom, float fPosX, const float fPosY)
    {
        m_oFileCacheMgr.DrawDebugInfo(auxGeom, fPosX, fPosY);

        if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::MemoryInfo))
        {
            DrawImplMemoryPoolDebugInfo(auxGeom, fPosX, fPosY);
        }

        if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::ActiveObjects))
        {
            m_oAudioObjectMgr.DrawDebugInfo(auxGeom, fPosX, fPosY);
            fPosX += 800.0f;
        }

        if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::ActiveEvents))
        {
            m_oAudioEventMgr.DrawDebugInfo(auxGeom, fPosX, fPosY);
        }

        if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::DrawListener))
        {
            m_oAudioListenerMgr.DrawDebugInfo(auxGeom);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void BytesToString(AZ::u64 bytes, char* buffer, size_t bufLength)
    {
        if (bytes < (1 << 10))
        {
            azsnprintf(buffer, bufLength, "%llu B", bytes);
        }
        else if (bytes < (1 << 20))
        {
            azsnprintf(buffer, bufLength, "%.2f KB", static_cast<double>(bytes) / static_cast<double>(1 << 10));
        }
        else
        {
            azsnprintf(buffer, bufLength, "%.2f MB", static_cast<double>(bytes) / static_cast<double>(1 << 20));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawImplMemoryPoolDebugInfo(IRenderAuxGeom& auxGeom, float fPosX, float fPosY)
    {
        const float colorMax = 0.9f;
        const float colorMin = 0.1f;
        const float textSize = 1.5f;
        const float lineHeight = 15.f;
        float color[4] = { colorMax, colorMax, colorMax, 0.9f };

        const AZ::Color whiteColor(colorMax, colorMax, colorMax, 0.9f);
        const AZ::Color greenColor(colorMin, colorMax, colorMin, 0.9f);
        const AZ::Color yellowColor(colorMax, colorMax, colorMin, 0.9f);
        const AZ::Color redColor(colorMax, colorMin, colorMin, 0.9f);

        float posY = fPosY;
        constexpr float xTablePositions[7] = { 0.f, 40.f, 300.f, 400.f, 500.f, 600.f, 700.f };
        auxGeom.Draw2dLabel(fPosX + xTablePositions[0], posY, textSize, color, false, "ID");
        auxGeom.Draw2dLabel(fPosX + xTablePositions[1], posY, textSize, color, false, "Name");
        auxGeom.Draw2dLabel(fPosX + xTablePositions[2], posY, textSize, color, false, "Curr Used");
        auxGeom.Draw2dLabel(fPosX + xTablePositions[3], posY, textSize, color, false, "Peak Used");
        auxGeom.Draw2dLabel(fPosX + xTablePositions[4], posY, textSize, color, false, "%% of Used");
        auxGeom.Draw2dLabel(fPosX + xTablePositions[5], posY, textSize, color, false, "Allocs");
        auxGeom.Draw2dLabel(fPosX + xTablePositions[6], posY, textSize, color, false, "Frees");

        // Get the memory pool information...
        AZStd::vector<AudioImplMemoryPoolInfo> poolInfos;
        AudioSystemImplementationRequestBus::BroadcastResult(poolInfos, &AudioSystemImplementationRequestBus::Events::GetMemoryPoolInfo);

        if (!poolInfos.empty())
        {
            const AudioImplMemoryPoolInfo& globalInfo = poolInfos.back();
            AZ_Assert(globalInfo.m_poolId == -1, "Global memory info doesn't have the expected ID.\n");
            poolInfos.pop_back();

            AZ::u64 totalPeak = 0;
            AZ::u64 totalAllocs = 0;
            AZ::u64 totalFrees = 0;
            constexpr size_t bufferSize = 32;
            char buffer[bufferSize] = { 0 };

            for (auto& poolInfo : poolInfos)
            {
                posY += lineHeight;

                totalPeak += poolInfo.m_peakUsed;
                totalAllocs += poolInfo.m_numAllocs;
                totalFrees += poolInfo.m_numFrees;
                float percentUsed = static_cast<float>(poolInfo.m_memoryUsed) / static_cast<float>(globalInfo.m_memoryUsed);

                // Calculate a color (green->-yellow->-red) based on percentage.
                AZ::Color percentColor;
                if (percentUsed < 0.5f)
                {
                    percentColor = greenColor.Lerp(yellowColor, percentUsed * 2.f);
                }
                else
                {
                    percentColor = yellowColor.Lerp(redColor, (percentUsed * 2.f) - 1.f);
                }
                percentColor.StoreToFloat4(color);
                percentUsed *= 100.f;

                // ID
                auxGeom.Draw2dLabel(fPosX + xTablePositions[0], posY, textSize, color, false, "%d", poolInfo.m_poolId);

                // Name
                auxGeom.Draw2dLabel(fPosX + xTablePositions[1], posY, textSize, color, false, "%s", poolInfo.m_poolName);

                // Current Used (bytes)
                BytesToString(poolInfo.m_memoryUsed, buffer, bufferSize);
                auxGeom.Draw2dLabel(fPosX + xTablePositions[2], posY, textSize, color, false, "%s", buffer);

                // Peak Used (bytes)
                BytesToString(poolInfo.m_peakUsed, buffer, bufferSize);
                auxGeom.Draw2dLabel(fPosX + xTablePositions[3], posY, textSize, color, false, "%s", buffer);

                // % of Used (percent)
                auxGeom.Draw2dLabel(fPosX + xTablePositions[4], posY, textSize, color, false, "%.1f %%", percentUsed);

                // Allocs
                auxGeom.Draw2dLabel(fPosX + xTablePositions[5], posY, textSize, color, false, "%u", poolInfo.m_numAllocs);

                // Frees
                auxGeom.Draw2dLabel(fPosX + xTablePositions[6], posY, textSize, color, false, "%u", poolInfo.m_numFrees);
            }

            whiteColor.StoreToFloat4(color);
            posY += (2.f * lineHeight);

            auxGeom.Draw2dLabel(fPosX + xTablePositions[1], posY, textSize, color, false, "Name");
            auxGeom.Draw2dLabel(fPosX + xTablePositions[1], posY + lineHeight, textSize, color, false, "%s", globalInfo.m_poolName);

            auxGeom.Draw2dLabel(fPosX + xTablePositions[2], posY, textSize, color, false, "Total Used");
            BytesToString(globalInfo.m_memoryUsed, buffer, bufferSize);
            auxGeom.Draw2dLabel(fPosX + xTablePositions[2], posY + lineHeight, textSize, color, false, "%s", buffer);

            auxGeom.Draw2dLabel(fPosX + xTablePositions[3], posY, textSize, color, false, "Total Peak");
            BytesToString(totalPeak, buffer, bufferSize);
            auxGeom.Draw2dLabel(fPosX + xTablePositions[3], posY + lineHeight, textSize, color, false, "%s", buffer);

            auxGeom.Draw2dLabel(fPosX + xTablePositions[4], posY, textSize, color, false, "Total Size");
            BytesToString(globalInfo.m_memoryReserved, buffer, bufferSize);
            auxGeom.Draw2dLabel(fPosX + xTablePositions[4], posY + lineHeight, textSize, color, false, "%s", buffer);

            auxGeom.Draw2dLabel(fPosX + xTablePositions[5], posY, textSize, color, false, "Total Allocs");
            auxGeom.Draw2dLabel(fPosX + xTablePositions[5], posY + lineHeight, textSize, color, false, "%llu", totalAllocs);

            auxGeom.Draw2dLabel(fPosX + xTablePositions[6], posY, textSize, color, false, "Total Frees");
            auxGeom.Draw2dLabel(fPosX + xTablePositions[6], posY + lineHeight, textSize, color, false, "%llu", totalFrees);
        }
        else
        {
            auxGeom.Draw2dLabel(
                fPosX, posY + lineHeight, textSize, color, false, "No memory pool information is available for display!");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawAudioObjectDebugInfo(IRenderAuxGeom& auxGeom)
    {
        SATLWorldPosition oListenerPosition;
        m_oAudioListenerMgr.GetDefaultListenerPosition(oListenerPosition);
        m_oAudioObjectMgr.DrawPerObjectDebugInfo(auxGeom, oListenerPosition.GetPositionVec());
    }
#endif // !AUDIO_RELEASE
} // namespace Audio
