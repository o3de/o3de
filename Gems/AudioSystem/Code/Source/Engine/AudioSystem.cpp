/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioSystem.h>
#include <AudioProxy.h>
#include <SoundCVars.h>
#include <AudioSystem_Traits_Platform.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace Audio
{
    extern CAudioLogger g_audioLogger;
    static constexpr const char AudioControlsBasePath[]{ "libs/gameaudio/" };

    // Save off the threadId of the "Main Thread" that was used to connect EBuses.
    AZStd::thread_id g_mainThreadId;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // CAudioThread
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioThread::~CAudioThread()
    {
        Deactivate();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioThread::Run()
    {
        AZ_Assert(m_audioSystem, "Audio Thread has no Audio System to run!\n");
        m_running = true;
        while (m_running)
        {
            m_audioSystem->InternalUpdate();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioThread::Activate(CAudioSystem* const audioSystem)
    {
        m_audioSystem = audioSystem;

        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "Audio Thread";
        threadDesc.m_cpuId = AZ_TRAIT_AUDIOSYSTEM_AUDIO_THREAD_AFFINITY;

        auto threadFunc = AZStd::bind(&CAudioThread::Run, this);
        m_thread = AZStd::thread(threadDesc, threadFunc);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioThread::Deactivate()
    {
        if (m_running)
        {
            m_running = false;
            m_thread.join();
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // CAudioSystem
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystem::CAudioSystem()
        : m_bSystemInitialized(false)
    {
        g_mainThreadId = AZStd::this_thread::get_id();

        m_apAudioProxies.reserve(Audio::CVars::s_AudioObjectPoolSize);
        m_apAudioProxiesToBeFreed.reserve(16);
        m_controlsPath.assign(Audio::AudioControlsBasePath);

        AudioSystemRequestBus::Handler::BusConnect();
        AudioSystemThreadSafeRequestBus::Handler::BusConnect();
        AudioSystemInternalRequestBus::Handler::BusConnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystem::~CAudioSystem()
    {
        AudioSystemRequestBus::Handler::BusDisconnect();
        AudioSystemThreadSafeRequestBus::Handler::BusDisconnect();
        AudioSystemInternalRequestBus::Handler::BusDisconnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::PushRequest(const SAudioRequest& audioRequestData)
    {
        CAudioRequestInternal request(audioRequestData);

        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::PushRequest - called from non-Main thread!");
        AZ_Assert(0 == (request.nFlags & eARF_THREAD_SAFE_PUSH), "AudioSystem::PushRequest - called with flag THREAD_SAFE_PUSH!");
        AZ_Assert(0 == (request.nFlags & eARF_EXECUTE_BLOCKING), "AudioSystem::PushRequest - called with flag EXECUTE_BLOCKING!");

        AudioSystemInternalRequestBus::QueueBroadcast(&AudioSystemInternalRequestBus::Events::ProcessRequestByPriority, request);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::PushRequestBlocking(const SAudioRequest& audioRequestData)
    {
        // Main Thread!
        AZ_PROFILE_FUNCTION(Audio);

        CAudioRequestInternal request(audioRequestData);

        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::PushRequestBlocking - called from non-Main thread!");
        AZ_Assert(0 != (request.nFlags & eARF_EXECUTE_BLOCKING), "AudioSystem::PushRequestBlocking - called without EXECUTE_BLOCKING flag!");
        AZ_Assert(0 == (request.nFlags & eARF_THREAD_SAFE_PUSH), "AudioSystem::PushRequestBlocking - called with THREAD_SAFE_PUSH flag!");

        ProcessRequestBlocking(request);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::PushRequestThreadSafe(const SAudioRequest& audioRequestData)
    {
        CAudioRequestInternal request(audioRequestData);

        AZ_Assert(0 != (request.nFlags & eARF_THREAD_SAFE_PUSH), "AudioSystem::PushRequestThreadSafe - called without THREAD_SAFE_PUSH flag!");
        AZ_Assert(0 == (request.nFlags & eARF_EXECUTE_BLOCKING), "AudioSystem::PushRequestThreadSafe - called with flag EXECUTE_BLOCKING!");

        AudioSystemThreadSafeRequestBus::QueueFunction(AZStd::bind(&CAudioSystem::ProcessRequestThreadSafe, this, request));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::AddRequestListener(
        AudioRequestCallbackType func,
        void* const callbackOwner,
        const EAudioRequestType requestType,
        const TATLEnumFlagsType specificRequestMask)
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::AddRequestListener - called from a non-Main thread!");

        if (func)
        {
            SAudioEventListener listener;
            listener.m_callbackOwner = callbackOwner;
            listener.m_fnOnEvent = func;
            listener.m_requestType = requestType;
            listener.m_specificRequestMask = specificRequestMask;
            m_oATL.AddRequestListener(listener);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::RemoveRequestListener(AudioRequestCallbackType func, void* const callbackOwner)
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::RemoveRequestListener - called from a non-Main thread!");

        SAudioEventListener listener;
        listener.m_callbackOwner = callbackOwner;
        listener.m_fnOnEvent = func;
        m_oATL.RemoveRequestListener(listener);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::ExternalUpdate()
    {
        // Main Thread!
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::ExternalUpdate - called from non-Main thread!");

        // Notify callbacks on the pending callbacks queue...
        // These are requests that were completed then queued for callback processing to happen here.
        ExecuteRequestCompletionCallbacks(m_pendingCallbacksQueue, m_pendingCallbacksMutex);

        // Notify callbacks from the "thread safe" queue...
        ExecuteRequestCompletionCallbacks(m_threadSafeCallbacksQueue, m_threadSafeCallbacksMutex, true);

        // Other notifications to be sent out...
        AudioTriggerNotificationBus::ExecuteQueuedEvents();

        // Free any Audio Proxies that are queued up for deletion...
        for (auto audioProxy : m_apAudioProxiesToBeFreed)
        {
            azdestroy(audioProxy, Audio::AudioSystemAllocator);
        }

        m_apAudioProxiesToBeFreed.clear();

    #if !defined(AUDIO_RELEASE)
        DrawAudioDebugData();
    #endif // !AUDIO_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::InternalUpdate()
    {
        // Audio Thread!
        AZ_PROFILE_FUNCTION(Audio);

        auto startUpdateTime = AZStd::chrono::system_clock::now();        // stamp the start time

        bool handledBlockingRequests = false;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_blockingRequestsMutex);
            handledBlockingRequests = ProcessRequests(m_blockingRequestsQueue);
        }

        if (!handledBlockingRequests)
        {
            // Call the ProcessRequestByPriority events queued up...
            AudioSystemInternalRequestBus::ExecuteQueuedEvents();
        }

        // Call the ProcessRequestThreadSafe events queued up...
        AudioSystemThreadSafeRequestBus::ExecuteQueuedEvents();

        m_oATL.Update();

    #if !defined(AUDIO_RELEASE)
        #if defined(PROVIDE_GETNAME_SUPPORT)
        {
            AZ_PROFILE_SCOPE(Audio, "Sync Debug Name Changes");
            AZStd::lock_guard<AZStd::mutex> lock(m_debugNameStoreMutex);
            m_debugNameStore.SyncChanges(m_oATL.GetDebugStore());
        }
        #endif // PROVIDE_GETNAME_SUPPORT
    #endif // !AUDIO_RELEASE

        if (!handledBlockingRequests)
        {
            auto endUpdateTime = AZStd::chrono::system_clock::now();      // stamp the end time
            auto elapsedUpdateTime = AZStd::chrono::duration_cast<duration_ms>(endUpdateTime - startUpdateTime);
            if (elapsedUpdateTime < m_targetUpdatePeriod)
            {
                AZ_PROFILE_SCOPE(Audio, "Wait Remaining Time in Update Period");
                m_processingEvent.try_acquire_for(m_targetUpdatePeriod - elapsedUpdateTime);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystem::Initialize()
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::Initialize - called from a non-Main thread!");

        if (!m_bSystemInitialized)
        {
            m_audioSystemThread.Deactivate();
            m_oATL.Initialize();
            m_audioSystemThread.Activate(this);

            for (AZ::u64 i = 0; i < Audio::CVars::s_AudioObjectPoolSize; ++i)
            {
                auto audioProxy = azcreate(CAudioProxy, (), Audio::AudioSystemAllocator, "AudioProxy");
                m_apAudioProxies.push_back(audioProxy);
            }

            m_bSystemInitialized = true;
        }

        return m_bSystemInitialized;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::Release()
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::Release - called from a non-Main thread!");

        for (auto audioProxy : m_apAudioProxies)
        {
            azdestroy(audioProxy, Audio::AudioSystemAllocator);
        }

        for (auto audioProxy : m_apAudioProxiesToBeFreed)
        {
            azdestroy(audioProxy, Audio::AudioSystemAllocator);
        }

        m_apAudioProxies.clear();
        m_apAudioProxiesToBeFreed.clear();

        // Release the audio implementation...
        SAudioRequest request;
        SAudioManagerRequestData<eAMRT_RELEASE_AUDIO_IMPL> requestData;
        request.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);
        request.pData = &requestData;
        PushRequestBlocking(request);

        m_audioSystemThread.Deactivate();
        m_oATL.ShutDown();
        m_bSystemInitialized = false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioSystem::GetAudioTriggerID(const char* const sAudioTriggerName) const
    {
        return m_oATL.GetAudioTriggerID(sAudioTriggerName);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioSystem::GetAudioRtpcID(const char* const sAudioRtpcName) const
    {
        return m_oATL.GetAudioRtpcID(sAudioRtpcName);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioSystem::GetAudioSwitchID(const char* const sAudioStateName) const
    {
        return m_oATL.GetAudioSwitchID(sAudioStateName);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioSwitchStateID CAudioSystem::GetAudioSwitchStateID(const TAudioControlID nSwitchID, const char* const sAudioSwitchStateName) const
    {
        return m_oATL.GetAudioSwitchStateID(nSwitchID, sAudioSwitchStateName);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioPreloadRequestID CAudioSystem::GetAudioPreloadRequestID(const char* const sAudioPreloadRequestName) const
    {
        return m_oATL.GetAudioPreloadRequestID(sAudioPreloadRequestName);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioEnvironmentID CAudioSystem::GetAudioEnvironmentID(const char* const sAudioEnvironmentName) const
    {
        return m_oATL.GetAudioEnvironmentID(sAudioEnvironmentName);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystem::ReserveAudioListenerID(TAudioObjectID& rAudioObjectID)
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::ReserveAudioListenerID - called from a non-Main thread!");
        return m_oATL.ReserveAudioListenerID(rAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystem::ReleaseAudioListenerID(TAudioObjectID const nAudioObjectID)
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::ReleaseAudioListenerID - called from a non-Main thread!");
        return m_oATL.ReleaseAudioListenerID(nAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystem::SetAudioListenerOverrideID(const TAudioObjectID nAudioObjectID)
    {
        return m_oATL.SetAudioListenerOverrideID(nAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::GetInfo([[maybe_unused]] SAudioSystemInfo& rAudioSystemInfo)
    {
        //TODO: 
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CAudioSystem::GetControlsPath() const
    {
        return m_controlsPath.c_str();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::UpdateControlsPath()
    {
        AZStd::string controlsPath{ Audio::AudioControlsBasePath };
        const AZStd::string& subPath = m_oATL.GetControlsImplSubPath();
        if (!subPath.empty())
        {
            controlsPath.append(subPath);
        }

        if (AZ::StringFunc::RelativePath::Normalize(controlsPath))
        {
            m_controlsPath = controlsPath;
        }
        else
        {
            g_audioLogger.Log(
                eALT_ERROR, "AudioSystem::UpdateControlsPath - failed to normalize the controls path '%s'!", controlsPath.c_str());
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::RefreshAudioSystem([[maybe_unused]] const char* const levelName)
    {
    #if !defined(AUDIO_RELEASE)
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::RefreshAudioSystem - called from a non-Main thread!");

        // Get the controls path and a level-specific preload Id first.
        // This will be passed with the request so that it doesn't have to lookup this data
        // and punch the AudioSystemRequestBus from the Audio Thread.
        const char* audioControlsPath = GetControlsPath();

        Audio::TAudioPreloadRequestID levelPreloadId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
        if (levelName && levelName[0] != '\0')
        {
            levelPreloadId = GetAudioPreloadRequestID(levelName);
        }

        Audio::SAudioManagerRequestData<Audio::eAMRT_REFRESH_AUDIO_SYSTEM> requestData(audioControlsPath, levelName, levelPreloadId);
        Audio::SAudioRequest request;
        request.nFlags = (Audio::eARF_PRIORITY_HIGH | Audio::eARF_EXECUTE_BLOCKING);
        request.pData = &requestData;
        PushRequestBlocking(request);
    #endif // !AUDIO_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IAudioProxy* CAudioSystem::GetFreeAudioProxy()
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::GetFreeAudioProxy - called from a non-Main thread!");
        CAudioProxy* audioProxy = nullptr;

        if (!m_apAudioProxies.empty())
        {
            audioProxy = m_apAudioProxies.back();
            m_apAudioProxies.pop_back();
        }
        else
        {
            audioProxy = azcreate(CAudioProxy, (), Audio::AudioSystemAllocator, "AudioProxyEx");

        #if !defined(AUDIO_RELEASE)
            if (!audioProxy)
            {
                g_audioLogger.Log(eALT_ASSERT, "AudioSystem::GetFreeAudioProxy - failed to create new AudioProxy instance!");
            }
        #endif // !AUDIO_RELEASE
        }

        return static_cast<IAudioProxy*>(audioProxy);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::FreeAudioProxy(IAudioProxy* const audioProxyI)
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::FreeAudioProxy - called from a non-Main thread!");
        auto const audioProxy = static_cast<CAudioProxy*>(audioProxyI);

        if (AZStd::find(m_apAudioProxiesToBeFreed.begin(), m_apAudioProxiesToBeFreed.end(), audioProxy) != m_apAudioProxiesToBeFreed.end() || AZStd::find(m_apAudioProxies.begin(), m_apAudioProxies.end(), audioProxy) != m_apAudioProxies.end())
        {
            AZ_Warning("AudioSystem", false, "Attempting to free an already freed audio proxy");
            return;
        }

        if (m_apAudioProxies.size() < Audio::CVars::s_AudioObjectPoolSize)
        {
            m_apAudioProxies.push_back(audioProxy);
        }
        else
        {
            m_apAudioProxiesToBeFreed.push_back(audioProxy);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioSourceId CAudioSystem::CreateAudioSource(const SAudioInputConfig& sourceConfig)
    {
        return m_oATL.CreateAudioSource(sourceConfig);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::DestroyAudioSource(TAudioSourceId sourceId)
    {
        m_oATL.DestroyAudioSource(sourceId);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CAudioSystem::GetAudioControlName([[maybe_unused]] const EAudioControlType controlType, [[maybe_unused]] const TATLIDType atlID) const
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::GetAudioControlName - called from non-Main thread!");
        const char* sResult = nullptr;

    #if !defined(AUDIO_RELEASE)
        #if defined(PROVIDE_GETNAME_SUPPORT)
        AZStd::lock_guard<AZStd::mutex> lock(m_debugNameStoreMutex);
        switch (controlType)
        {
            case eACT_AUDIO_OBJECT:
            {
                sResult = m_debugNameStore.LookupAudioObjectName(atlID);
                break;
            }
            case eACT_TRIGGER:
            {
                sResult = m_debugNameStore.LookupAudioTriggerName(atlID);
                break;
            }
            case eACT_RTPC:
            {
                sResult = m_debugNameStore.LookupAudioRtpcName(atlID);
                break;
            }
            case eACT_SWITCH:
            {
                sResult = m_debugNameStore.LookupAudioSwitchName(atlID);
                break;
            }
            case eACT_PRELOAD:
            {
                sResult = m_debugNameStore.LookupAudioPreloadRequestName(atlID);
                break;
            }
            case eACT_ENVIRONMENT:
            {
                sResult = m_debugNameStore.LookupAudioEnvironmentName(atlID);
                break;
            }
            case eACT_SWITCH_STATE: // not handled here, use GetAudioSwitchStateName!
            case eACT_NONE:
            default: // fall-through!
            {
                g_audioLogger.Log(eALT_WARNING, "AudioSystem::GetAudioControlName - called with invalid EAudioControlType!");
                break;
            }
        }
        #endif // PROVIDE_GETNAME_SUPPORT
    #endif // !AUDIO_RELEASE

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CAudioSystem::GetAudioSwitchStateName([[maybe_unused]] const TAudioControlID switchID, [[maybe_unused]] const TAudioSwitchStateID stateID) const
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::GetAudioSwitchStateName - called from non-Main thread!");
        const char* sResult = nullptr;

    #if !defined(AUDIO_RELEASE)
        #if defined(PROVIDE_GETNAME_SUPPORT)
        AZStd::lock_guard<AZStd::mutex> lock(m_debugNameStoreMutex);
        sResult = m_debugNameStore.LookupAudioSwitchStateName(switchID, stateID);
        #endif // PROVIDE_GETNAME_SUPPORT
    #endif // !AUDIO_RELEASE

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::ExtractCompletedRequests(TAudioRequests& requestQueue, TAudioRequests& extractedCallbacks)
    {
        auto iter(requestQueue.begin());
        auto iterEnd(requestQueue.end());

        while (iter != iterEnd)
        {
            const CAudioRequestInternal& refRequest = (*iter);
            if (refRequest.IsComplete())
            {
                // the request has completed, eligible for notification callback.
                // move the request to the extraction queue.
                extractedCallbacks.push_back(refRequest);
                iter = requestQueue.erase(iter);
                iterEnd = requestQueue.end();
                continue;
            }

            ++iter;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::ExecuteRequestCompletionCallbacks(TAudioRequests& requestQueue, AZStd::mutex& requestQueueMutex, bool tryLock)
    {
        TAudioRequests extractedCallbacks;

        if (tryLock)
        {
            if (requestQueueMutex.try_lock())
            {
                ExtractCompletedRequests(requestQueue, extractedCallbacks);
                requestQueueMutex.unlock();
            }
        }
        else
        {
            AZStd::lock_guard<AZStd::mutex> lock(requestQueueMutex);
            ExtractCompletedRequests(requestQueue, extractedCallbacks);
        }

        // Notify listeners
        for (const auto& callback : extractedCallbacks)
        {
            m_oATL.NotifyListener(callback);
        }

        extractedCallbacks.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::ProcessRequestBlocking(CAudioRequestInternal& request)
    {
        AZ_PROFILE_FUNCTION(Audio);

        if (m_oATL.CanProcessRequests())
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_blockingRequestsMutex);
                m_blockingRequestsQueue.push_back(request);
            }

            m_processingEvent.release();
            m_mainEvent.acquire();

            ExecuteRequestCompletionCallbacks(m_blockingRequestsQueue, m_blockingRequestsMutex);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::ProcessRequestThreadSafe(CAudioRequestInternal request)
    {
        // Audio Thread!
        AZ_PROFILE_SCOPE(Audio, "Process Thread-Safe Request");

        if (m_oATL.CanProcessRequests())
        {
            if (request.eStatus == eARS_NONE)
            {
                request.eStatus = eARS_PENDING;
                m_oATL.ProcessRequest(request);
            }

            AZ_Assert(request.eStatus != eARS_PENDING, "AudioSystem::ProcessRequestThreadSafe - ATL finished processing request, but request is still in pending state!");
            if (request.eStatus != eARS_PENDING)
            {
                // push the request onto a callbacks queue for main thread to process later...
                AZStd::lock_guard<AZStd::mutex> lock(m_threadSafeCallbacksMutex);
                m_threadSafeCallbacksQueue.push_back(request);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::ProcessRequestByPriority(CAudioRequestInternal request)
    {
        // Todo: This should handle request priority, use request priority as bus Address and process in priority order.

        AZ_PROFILE_SCOPE(Audio, "Process Normal Request");

        AZ_Assert(g_mainThreadId != AZStd::this_thread::get_id(), "AudioSystem::ProcessRequestByPriority - called from Main thread!");

        if (m_oATL.CanProcessRequests())
        {
            if (request.eStatus == eARS_NONE)
            {
                request.eStatus = eARS_PENDING;
                m_oATL.ProcessRequest(request);
            }

            AZ_Assert(request.eStatus != eARS_PENDING, "AudioSystem::ProcessRequestByPriority - ATL finished processing request, but request is still in pending state!");
            if (request.eStatus != eARS_PENDING)
            {
                // push the request onto a callbacks queue for main thread to process later...
                AZStd::lock_guard<AZStd::mutex> lock(m_pendingCallbacksMutex);
                m_pendingCallbacksQueue.push_back(request);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystem::ProcessRequests(TAudioRequests& requestQueue)
    {
        bool success = false;

        for (auto& request : requestQueue)
        {
            if (!(request.nInternalInfoFlags & eARIF_WAITING_FOR_REMOVAL))
            {
                AZ_PROFILE_SCOPE(Audio, "Process Blocking Request");

                if (request.eStatus == eARS_NONE)
                {
                    request.eStatus = eARS_PENDING;
                    m_oATL.ProcessRequest(request);
                    success = true;
                }

                if (request.eStatus != eARS_PENDING)
                {
                    if (request.nFlags & eARF_EXECUTE_BLOCKING)
                    {
                        request.nInternalInfoFlags |= eARIF_WAITING_FOR_REMOVAL;
                        m_mainEvent.release();
                    }
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "AudioSystem::ProcessRequests - request still in Pending state after being processed by ATL!");
                }
            }
        }

        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(AUDIO_RELEASE)
    void CAudioSystem::DrawAudioDebugData()
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::DrawAudioDebugData - called from non-Main thread!");

        if (CVars::s_debugDrawOptions.GetRawFlags() != 0)
        {
            SAudioRequest oRequest;
            oRequest.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);
            SAudioManagerRequestData<eAMRT_DRAW_DEBUG_INFO> oRequestData;
            oRequest.pData = &oRequestData;

            PushRequestBlocking(oRequest);
        }
    }
#endif // !AUDIO_RELEASE

} // namespace Audio
