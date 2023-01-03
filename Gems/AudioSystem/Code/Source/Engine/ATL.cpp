/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATL.h>

#include <AzCore/Console/ILogger.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <SoundCVars.h>
#include <AudioProxy.h>
#include <ATLAudioObject.h>
#include <IAudioSystemImplementation.h>

#if !defined(AUDIO_RELEASE)
    // Debug Draw
    #include <AzCore/Math/Color.h>
    #include <AzFramework/Entity/EntityDebugDisplayBus.h>
#endif // !AUDIO_RELEASE


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline EAudioRequestResult ConvertToRequestResult(const EAudioRequestStatus eAudioRequestStatus)
    {
        return (eAudioRequestStatus == EAudioRequestStatus::Success) ? EAudioRequestResult::Success : EAudioRequestResult::Failure;
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
        , m_oXmlProcessor(m_cTriggers, m_cRtpcs, m_cSwitches, m_cEnvironments, m_cPreloadRequests, m_oFileCacheMgr)
        , m_nFlags(eAIS_NONE)
    {
        if (GetISystem() && GetISystem()->GetISystemEventDispatcher())
        {
            GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
        }

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
        if (GetISystem() && GetISystem()->GetISystemEventDispatcher())
        {
            GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::Initialize()
    {
        m_lastUpdateTime = AZStd::chrono::steady_clock::now();
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

        auto current = AZStd::chrono::steady_clock::now();
        m_elapsedTime = AZStd::chrono::duration_cast<duration_ms>(current - m_lastUpdateTime);
        m_lastUpdateTime = current;
        float elapsedMs = m_elapsedTime.count();

        UpdateSharedData();

        m_oAudioObjectMgr.Update(elapsedMs, m_oSharedData.m_oActiveListenerPosition);
        m_oFileCacheMgr.Update();

        AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::Update, elapsedMs);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::ProcessRequest(AudioRequestVariant&& requestVariant)
    {
        bool hasCallback = false;
        EAudioRequestStatus status = AZStd::visit(
            [this, &hasCallback]([[maybe_unused]] auto&& request) -> EAudioRequestStatus
            {
                EAudioRequestStatus result = EAudioRequestStatus::None;

                request.m_status = EAudioRequestStatus::Pending;

                using T = AZStd::decay_t<decltype(request)>;

                // ... System ...

                if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::Initialize>)
                {
                    result = InitializeImplComponent();
                    // Immediately release the impl if it failed to init...
                    if (result != EAudioRequestStatus::Success)
                    {
                        ReleaseImplComponent();
                    }
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::Shutdown>)
                {
                    ReleaseImplComponent();
                    result = EAudioRequestStatus::Success;
                }

                else if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::ReserveObject>)
                {
                    result = BoolToARS(
                        ReserveAudioObjectID(request.m_objectId, request.m_objectName.empty() ? nullptr : request.m_objectName.c_str()));
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
                    result = m_oFileCacheMgr.TryLoadRequest(request.m_preloadRequestId, !request.m_asyncLoad, request.m_autoLoadOnly);
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
                        result = ActivateTrigger(audioObject, it->second, request.m_owner);
                    }
                    else
                    {
                        result = EAudioRequestStatus::FailureInvalidControlId;
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
                        result = ActivateTrigger(audioObject, it->second, request.m_owner, &sourceData);
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
                        result = StopAllTriggers(audioObject, request.m_filterByOwner ? request.m_owner : nullptr);
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
                        AZLOG_DEBUG("ATL received a request to set position on the Global Audio Object!");
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
                        AZLOG_DEBUG("ATL received a request to set environment value on the Global Audio Object!");
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
                        AZLOG_DEBUG("ATL received a request to release the Global Audio Object!");
                        result = EAudioRequestStatus::FailureInvalidObjectId;
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
                        AZLOG_DEBUG("ATL received a request to set multiple positions on the Global Audio Object!");
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                }

                // ... Callback ...

                else if constexpr (AZStd::is_same_v<T, Audio::CallbackRequest::ReportFinishedEvent>)
                {
                    if (CATLEvent* atlEvent = m_oAudioEventMgr.LookupID(request.m_eventId);
                        atlEvent != nullptr)
                    {
                        if (atlEvent->m_nObjectID != m_nGlobalAudioObjectID)
                        {
                            m_oAudioObjectMgr.ReportEventFinished(atlEvent);
                        }
                        else if (m_pGlobalAudioObject)
                        {
                            m_pGlobalAudioObject->EventFinished(atlEvent);
                        }
                        m_oAudioEventMgr.ReleaseEvent(atlEvent);
                    }
                    result = EAudioRequestStatus::Success;
                }

                // ... Listener ...

                else if constexpr (AZStd::is_same_v<T, Audio::ListenerRequest::SetWorldTransform>)
                {
                    TAudioObjectID listenerId = INVALID_AUDIO_OBJECT_ID;
                    TAudioObjectID overrideId = m_oAudioListenerMgr.GetOverrideListenerID();
                    if (request.m_audioObjectId == overrideId
                        || request.m_audioObjectId == INVALID_AUDIO_OBJECT_ID)
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
                        AZLOG_DEBUG("ATL could not find listener with ID %llu", listenerId);
                        result = EAudioRequestStatus::FailureInvalidObjectId;
                    }
                }

                else
                {
                    static_assert(AZStd::is_void_v<T>, "AudioRequestVariant visitor is non-exhaustive across all variant types!");
                    result = EAudioRequestStatus::FailureInvalidRequest;
                }

                request.m_status = result;

                if (request.m_callback)
                {
                    if (0 != (request.m_flags & eARF_SYNC_CALLBACK))
                    {
                        request.m_callback(request);
                    }
                    else
                    {
                        hasCallback = true;
                    }
                }

                return result;
            }
            , requestVariant);

        if (auto audioSystem = AZ::Interface<IAudioSystem>::Get();
            audioSystem != nullptr && hasCallback)
        {
            audioSystem->PushCallback(AZStd::move(requestVariant));
        }

        if (status != EAudioRequestStatus::Success)
        {
            AZLOG_DEBUG("Audio Request did not succeed!");
        }
    }

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
    bool CAudioTranslationLayer::ReserveAudioObjectID(TAudioObjectID& rAudioObjectID, const char* const sAudioObjectName)
    {
        const bool bSuccess = m_oAudioObjectMgr.ReserveID(rAudioObjectID, sAudioObjectName);

#if !defined(AUDIO_RELEASE)
        if (bSuccess && sAudioObjectName && sAudioObjectName[0] != '\0')
        {
            m_oDebugNameStore.AddAudioObject(rAudioObjectID, sAudioObjectName);
        }
#endif // !AUDIO_RELEASE

        return bSuccess;
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
        AZ::Interface<IAudioSystem>::Get()->PushRequestBlocking(AZStd::move(createSourceRequest));

        return sourceId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DestroyAudioSource([[maybe_unused]] TAudioSourceId sourceId)
    {
        Audio::SystemRequest::DestroySource destroySourceRequest(sourceId);
        AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(destroySourceRequest));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::InitializeImplComponent()
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;

        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::Initialize);
        if (eResult == EAudioRequestStatus::Success)
        {
            IATLAudioObjectData* pGlobalObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(
                pGlobalObjectData, &AudioSystemImplementationRequestBus::Events::NewGlobalAudioObjectData, m_nGlobalAudioObjectID);

            m_pGlobalAudioObject = azcreate(
                CATLGlobalAudioObject, (m_nGlobalAudioObjectID, pGlobalObjectData), Audio::AudioSystemAllocator);

            m_oAudioListenerMgr.Initialize();
            m_oAudioObjectMgr.Initialize();
            m_oAudioEventMgr.Initialize();
            m_oFileCacheMgr.Initialize();

            SetImplLanguage();

            // Update the implementation subpath for locating ATL controls...
            AudioSystemImplementationRequestBus::BroadcastResult(
                m_implSubPath, &AudioSystemImplementationRequestBus::Events::GetImplSubPath);
        }
#if !defined(AUDIO_RELEASE)
        else
        {
            const char* implementationName = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(
                implementationName, &AudioSystemImplementationRequestBus::Events::GetImplementationNameString);
            AZLOG_ERROR("Failed to initialize the AudioSystemImplementation '%s'", implementationName);
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
            AudioSystemImplementationRequestBus::Broadcast(
                &AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, m_pGlobalAudioObject->GetImplDataPtr());
            azdestroy(m_pGlobalAudioObject, Audio::AudioSystemAllocator);
            m_pGlobalAudioObject = nullptr;
        }

        m_oAudioListenerMgr.Release();
        m_oAudioObjectMgr.Release();
        m_oAudioEventMgr.Release();
        m_oFileCacheMgr.Release();

        m_implSubPath.clear();

        if (AudioSystemImplementationRequestBus::HasHandlers())
        {
            EAudioRequestStatus result = EAudioRequestStatus::Failure;
            AudioSystemImplementationRequestBus::BroadcastResult(result, &AudioSystemImplementationRequestBus::Events::ShutDown);
            AZ_Error("ATL", result == EAudioRequestStatus::Success,
                "ATL ReleaseImplComponent - Shutting down the audio implementation failed!");

            AudioSystemImplementationRequestBus::BroadcastResult(result, &AudioSystemImplementationRequestBus::Events::Release);
            AZ_Error("ATL", result == EAudioRequestStatus::Success,
                "ATL ReleaseImplComponent - Releasing the audio implementation failed!");
        }

        m_nFlags &= ~eAIS_AUDIO_MIDDLEWARE_SHUTTING_DOWN;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::PrepUnprepTriggerAsync(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger,
        const bool bPrepare)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;

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
                    break;
                }
            }

            if (ePrepUnprepResult == EAudioRequestStatus::Success)
            {
                pEvent->SetDataScope(pTrigger->GetDataScope());
                pAudioObject->EventStarted(pEvent);
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
            AZLOG_DEBUG(
                "PrepUnprepTriggerAsync failed on AudioObject '%s' (ID: %llu)", m_oDebugNameStore.LookupAudioObjectName(nATLObjectID),
                nATLObjectID);
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ActivateTrigger(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger,
        void* const pOwner /* = nullptr */,
        const SATLSourceData* pSourceData /* = nullptr */)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;

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
        pAudioObject->TriggerInstanceStarting(m_nTriggerInstanceIDCounter, nATLTriggerID);

        for (auto const triggerImpl : pTrigger->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = triggerImpl->GetReceiver();
            CATLEvent* const pEvent = m_oAudioEventMgr.GetEvent(eReceiver);
            pEvent->m_pImplData->m_triggerId = nATLTriggerID;
            pEvent->m_pImplData->m_owner = pOwner;

            EAudioRequestStatus eActivateResult = EAudioRequestStatus::Failure;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eActivateResult,
                        &AudioSystemImplementationRequestBus::Events::ActivateTrigger,
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
                    break;
                }
            }

            if (eActivateResult == EAudioRequestStatus::Success)
            {
                pEvent->m_nObjectID = pAudioObject->GetID();
                pEvent->m_nTriggerID = nATLTriggerID;
                pEvent->m_nTriggerImplID = triggerImpl->m_nATLID;
                pEvent->m_nTriggerInstanceID = m_nTriggerInstanceIDCounter;
                pEvent->SetDataScope(pTrigger->GetDataScope());
                pEvent->m_audioEventState = eAES_PLAYING;

                pAudioObject->EventStarted(pEvent);

                // If at least one event fires, it is a success: the trigger has been activated.
                eResult = EAudioRequestStatus::Success;
            }
            else
            {
                m_oAudioEventMgr.ReleaseEvent(pEvent);
            }
        }

        // Either removes the eATS_STARTING flag on this trigger instance or removes it if no event was started.
        pAudioObject->TriggerInstanceStarted(m_nTriggerInstanceIDCounter++, pOwner);

#if !defined(AUDIO_RELEASE)
        if (eResult != EAudioRequestStatus::Success)
        {
            // No TriggerImpl generated an active event.
            AZLOG_DEBUG(
                "ATL Trigger '%s' failed to execute on AudioObject '%s' (ID: %llu)",
                m_oDebugNameStore.LookupAudioTriggerName(nATLTriggerID), m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID()),
                pAudioObject->GetID());
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopTrigger(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger)
    {
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;

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
                            break;
                        }
                    }

                    if (eSingleResult != EAudioRequestStatus::Success)
                    {
                        eResult = EAudioRequestStatus::Failure;
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
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;

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
            AZLOG_DEBUG(
                "ATL failed to set Switch '%s' to State '%s' on AudioObject '%s' (ID: %llu)", sSwitchName, sSwitchStateName,
                sAudioObjectName, pAudioObject->GetID());
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
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;

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
            AZLOG_DEBUG(
                "ATL failed to set Parameter '%s' to %f on AudioObject '%s' (ID: %llu)", sRtpcName, fValue, sAudioObjectName,
                pAudioObject->GetID());
        }
#endif // !AUDIO_RELEASE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ResetRtpcs(CATLAudioObjectBase* const pAudioObject)
    {
        const TObjectRtpcMap rRtpcs = pAudioObject->GetRtpcs();
        EAudioRequestStatus eResult = EAudioRequestStatus::Success;

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

            AZLOG_DEBUG(
                "ATL failed to reset parameters on AudioObject '%s' (ID: %llu)", m_oDebugNameStore.LookupAudioObjectName(objectId),
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
        EAudioRequestStatus eResult = EAudioRequestStatus::Failure;

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
            AZLOG_DEBUG(
                "ATL failed to set environment '%s' to %f on AudioObject '%s' (ID: %llu)", sEnvironmentName, fAmount, sAudioObjectName,
                pAudioObject->GetID());
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

            AZLOG_DEBUG(
                "ATL failed to reset environments on AudioObject '%s' (ID: %llu)", m_oDebugNameStore.LookupAudioObjectName(nObjectID),
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

                if (pInternalStateData->nATLInternalStateID == ATLInternalControlIDs::OOCStateIDs[static_cast<size_t>(ObstructionType::Ignore)])
                {
                    SATLSoundPropagationData oPropagationData;
                    pPositionedAudioObject->SetRaycastCalcType(ObstructionType::Ignore);
                    pPositionedAudioObject->GetObstOccData(oPropagationData);

                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::SetObstructionOcclusion,
                        pPositionedAudioObject->GetImplDataPtr(),
                        oPropagationData.fObstruction,
                        oPropagationData.fOcclusion);
                }
                else if (pInternalStateData->nATLInternalStateID == ATLInternalControlIDs::OOCStateIDs[static_cast<size_t>(ObstructionType::SingleRay)])
                {
                    pPositionedAudioObject->SetRaycastCalcType(ObstructionType::SingleRay);
                }
                else if (pInternalStateData->nATLInternalStateID == ATLInternalControlIDs::OOCStateIDs[static_cast<size_t>(ObstructionType::MultiRay)])
                {
                    pPositionedAudioObject->SetRaycastCalcType(ObstructionType::MultiRay);
                }
                else
                {
                    AZLOG_DEBUG("SetInternalSwitchState - Unknown value for ObstructionType");
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
                    AZLOG_DEBUG("SetInternalSwitchState - Unknown value for ObjectVelocityTracking");
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
            AZLOG_DEBUG("ATL Trigger not found for MuteAllTriggerID");
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
            AZLOG_DEBUG("ATL Trigger not found for UnmuteAllTriggerID");
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
                AZLOG_DEBUG("ATL Trigger not found for LoseFocusTriggerID");
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
                AZLOG_DEBUG("ATL Trigger not found for GetFocusTriggerID");
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
        AZLOG_INFO("$8Beginning to refresh the AudioSystem...")

        if (!controlsPath || controlsPath[0] == '\0')
        {
            AZLOG_INFO("Controls path is null, cannot continue with refresh");
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

        AZLOG_INFO("$3Done refreshing the AudioSystem!");

        return EAudioRequestStatus::Success;
    }

#if !defined(AUDIO_RELEASE)

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawAudioSystemDebugInfo()
    {
        AZ_PROFILE_FUNCTION(Audio);

        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
        AzFramework::DebugDisplayRequests* debugDisplay{ AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus) };

        auto oldState = debugDisplay->GetState();

        if (debugDisplay && CVars::s_debugDrawOptions.GetRawFlags() != 0)
        {
            DrawAudioObjectDebugInfo(*debugDisplay);

            const float textSize = 1.f;

            const size_t nPrimaryPoolSize = AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().Capacity();
            const size_t nPrimaryPoolUsedSize = nPrimaryPoolSize - AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GetUnAllocatedMemory();

            float fPosX = 0.0f;
            float fPosY = 4.0f;

            const AZ::Color colorWhite{ 1.0f, 1.0f, 1.0f, 0.9f };
            const AZ::Color colorRed{ 1.0f, 0.0f, 0.0f, 0.7f };
            const AZ::Color colorGreen{ 0.0f, 1.0f, 0.0f, 0.7f };
            const AZ::Color colorBlue{ 0.4f, 0.4f, 1.0f, 1.0f };

            AZStd::string str;
            const char* implementationName = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(implementationName, &AudioSystemImplementationRequestBus::Events::GetImplementationNameString);

            str = AZStd::string::format("Audio Translation Layer with %s", implementationName);

            debugDisplay->SetColor(colorBlue);
            debugDisplay->Draw2dTextLabel(fPosX, fPosY, textSize, str.c_str());

            fPosX += 30.0f;
            fPosY += 30.0f;

            str = AZStd::string::format(
                "Audio System Mem:  %.2f / %.2f MiB", (nPrimaryPoolUsedSize / 1024) / 1024.f, (nPrimaryPoolSize / 1024) / 1024.f);
            debugDisplay->SetColor(colorWhite);
            debugDisplay->Draw2dTextLabel(fPosX, fPosY, textSize, str.c_str());

            float const fLineHeight = 20.0f;

            SAudioImplMemoryInfo oMemoryInfo;
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::GetMemoryInfo, oMemoryInfo);

            fPosY += fLineHeight;

            str = AZStd::string::format(
                "Audio Engine Mem:  %.2f / %.2f MiB", (oMemoryInfo.nPrimaryPoolUsedSize / 1024) / 1024.f,
                (oMemoryInfo.nPrimaryPoolSize / 1024) / 1024.f);
            debugDisplay->Draw2dTextLabel(fPosX, fPosY, textSize, str.c_str());

            const AZ::Vector3 vPos = m_oSharedData.m_oActiveListenerPosition.GetPositionVec();
            const AZ::Vector3 vFwd = m_oSharedData.m_oActiveListenerPosition.GetForwardVec();
            const size_t nNumAudioObjects = m_oAudioObjectMgr.GetNumAudioObjects();
            const size_t nNumActiveAudioObjects = m_oAudioObjectMgr.GetNumActiveAudioObjects();
            const size_t nEvents = m_oAudioEventMgr.GetNumActive();
            const size_t nListeners = m_oAudioListenerMgr.GetNumActive();

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

            str = AZStd::string::format(
                "Active Listener (ID: %llu) -> Position: ( %.2f,  %.2f,  %.2f )   Forward: ( %.2f,  %.2f,  %.2f )", activeListenerID,
                vPos.GetX(), vPos.GetY(), vPos.GetZ(), vFwd.GetX(), vFwd.GetY(), vFwd.GetZ());
            debugDisplay->SetColor(colorGreen);
            debugDisplay->Draw2dTextLabel(fPosX, fPosY, textSize, str.c_str());

            fPosY += fLineHeight;

            str = AZStd::string::format(
                "Audio Objects: %3zu / %3zu  |  Events: %3zu  |  Listeners: %zu", nNumActiveAudioObjects, nNumAudioObjects, nEvents,
                nListeners);
            debugDisplay->SetColor(colorWhite);
            debugDisplay->Draw2dTextLabel(fPosX, fPosY, textSize, str.c_str());

            fPosY += fLineHeight;

            DrawATLComponentDebugInfo(*debugDisplay, fPosX, fPosY);
        }

        debugDisplay->SetState(oldState);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawATLComponentDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, float fPosX, float fPosY)
    {
        m_oFileCacheMgr.DrawDebugInfo(debugDisplay, fPosX, fPosY);

        if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::MemoryInfo)))
        {
            DrawImplMemoryPoolDebugInfo(debugDisplay, fPosX, fPosY);
        }

        if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::ActiveObjects)))
        {
            m_oAudioObjectMgr.DrawDebugInfo(debugDisplay, fPosX, fPosY);
            fPosX += 1000.0f;
        }

        if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::ActiveEvents)))
        {
            m_oAudioEventMgr.DrawDebugInfo(debugDisplay, fPosX, fPosY);
        }

        if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::DrawListener)))
        {
            m_oAudioListenerMgr.DrawDebugInfo(debugDisplay);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void BytesToString(AZ::u64 bytes, char* buffer, size_t bufLength)
    {
        if (bytes < (1 << 10))
        {
            azsnprintf(buffer, bufLength, "  B: %llu", bytes);
        }
        else if (bytes < (1 << 20))
        {
            azsnprintf(buffer, bufLength, "KiB: %.2f", static_cast<double>(bytes) / static_cast<double>(1 << 10));
        }
        else
        {
            azsnprintf(buffer, bufLength, "MiB: %.2f", static_cast<double>(bytes) / static_cast<double>(1 << 20));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawImplMemoryPoolDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, float fPosX, float fPosY)
    {
        const float colorMax = 0.9f;
        const float colorMin = 0.1f;
        const float textSize = 0.8f;
        const float lineHeight = 15.f;

        const AZ::Color whiteColor(colorMax, colorMax, colorMax, 0.9f);
        const AZ::Color greenColor(colorMin, colorMax, colorMin, 0.9f);
        const AZ::Color yellowColor(colorMax, colorMax, colorMin, 0.9f);
        const AZ::Color redColor(colorMax, colorMin, colorMin, 0.9f);

        float posY = fPosY;
        constexpr float xTablePositions[8] = { 0.f, 40.f, 300.f, 500.f, 700.f, 900.f, 1100.f, 1200.f };
        debugDisplay.SetColor(whiteColor);
        debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[0], posY, textSize, "ID");
        debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[1], posY, textSize, "Name");
        debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[2], posY, textSize, "Curr Used");
        debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[3], posY, textSize, "Peak Used");
        debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[4], posY, textSize, "% of Total Used");
        debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[5], posY, textSize, "% of Total Size");
        debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[6], posY, textSize, "# Allocs");
        debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[7], posY, textSize, "# Frees");

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
            AZStd::string str;

            for (auto& poolInfo : poolInfos)
            {
                posY += lineHeight;

                totalPeak += poolInfo.m_peakUsed;
                totalAllocs += poolInfo.m_numAllocs;
                totalFrees += poolInfo.m_numFrees;
                float percentUsed = static_cast<float>(poolInfo.m_memoryUsed) / static_cast<float>(globalInfo.m_memoryUsed);
                float percentTotal = static_cast<float>(poolInfo.m_memoryUsed) / static_cast<float>(globalInfo.m_memoryReserved);

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
                percentUsed *= 100.f;
                percentTotal *= 100.f;
                debugDisplay.SetColor(percentColor);

                // ID
                str = AZStd::string::format("%d", poolInfo.m_poolId);
                debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[0], posY, textSize, str.c_str());

                // Name
                str = AZStd::string::format("%s", poolInfo.m_poolName);
                debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[1], posY, textSize, str.c_str());

                // Current Used (bytes)
                BytesToString(poolInfo.m_memoryUsed, buffer, bufferSize);
                str = AZStd::string::format("%s", buffer);
                debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[2], posY, textSize, str.c_str());

                // Peak Used (bytes)
                BytesToString(poolInfo.m_peakUsed, buffer, bufferSize);
                str = AZStd::string::format("%s", buffer);
                debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[3], posY, textSize, str.c_str());

                // % of Used (percentage)
                str = AZStd::string::format("%.2f %%", percentUsed);
                debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[4], posY, textSize, str.c_str());

                // % of Total (percentage)
                str = AZStd::string::format("%.2f %%", percentTotal);
                debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[5], posY, textSize, str.c_str());

                // # Allocs
                str = AZStd::string::format("%u", poolInfo.m_numAllocs);
                debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[6], posY, textSize, str.c_str());

                // # Frees
                str = AZStd::string::format("%u", poolInfo.m_numFrees);
                debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[7], posY, textSize, str.c_str());
            }

            debugDisplay.SetColor(whiteColor);
            posY += (2.f * lineHeight);

            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[1], posY, textSize, "Name");
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[2], posY, textSize, "Total Used");
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[3], posY, textSize, "Peak Used");
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[4], posY, textSize, "% Used");
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[5], posY, textSize, "Total Size");
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[6], posY, textSize, "# Allocs");
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[7], posY, textSize, "# Frees");
            posY += lineHeight;

            str = AZStd::string::format("%s", globalInfo.m_poolName);
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[1], posY, textSize, str.c_str());

            float totalUsagePct = static_cast<float>(globalInfo.m_memoryUsed) / static_cast<float>(globalInfo.m_memoryReserved);
            AZ::Color percentTotalColor;
            if (totalUsagePct < 0.5f)
            {
                percentTotalColor = greenColor.Lerp(yellowColor, totalUsagePct * 2.f);
            }
            else
            {
                percentTotalColor = yellowColor.Lerp(redColor, (totalUsagePct * 2.f) - 1.f);
            }
            totalUsagePct *= 100.f;
            debugDisplay.SetColor(percentTotalColor);

            BytesToString(globalInfo.m_memoryUsed, buffer, bufferSize);
            str = AZStd::string::format("%s", buffer);
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[2], posY, textSize, str.c_str());

            BytesToString(totalPeak, buffer, bufferSize);
            str = AZStd::string::format("%s", buffer);
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[3], posY, textSize, str.c_str());

            str = AZStd::string::format("%.2f %%", totalUsagePct);
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[4], posY, textSize, str.c_str());

            BytesToString(globalInfo.m_memoryReserved, buffer, bufferSize);
            str = AZStd::string::format("%s", buffer);
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[5], posY, textSize, str.c_str());

            debugDisplay.SetColor(whiteColor);

            str = AZStd::string::format("%llu", totalAllocs);
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[6], posY, textSize, str.c_str());

            str = AZStd::string::format("%llu", totalFrees);
            debugDisplay.Draw2dTextLabel(fPosX + xTablePositions[7], posY, textSize, str.c_str());
        }
        else
        {
            debugDisplay.SetColor(whiteColor);
            debugDisplay.Draw2dTextLabel(fPosX, posY + lineHeight, textSize, "** No memory pool information is available for display **");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawAudioObjectDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        debugDisplay.DepthTestOff();
        SATLWorldPosition oListenerPosition;
        m_oAudioListenerMgr.GetDefaultListenerPosition(oListenerPosition);
        m_oAudioObjectMgr.DrawPerObjectDebugInfo(debugDisplay, oListenerPosition.GetPositionVec());
        debugDisplay.DepthTestOn();
    }
#endif // !AUDIO_RELEASE
} // namespace Audio
