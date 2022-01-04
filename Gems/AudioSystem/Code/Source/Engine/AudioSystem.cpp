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
    AZStd::thread_id g_audioThreadId;

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
        g_audioThreadId = AZStd::this_thread::get_id();
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
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystem::~CAudioSystem()
    {
    }



    //! NEW AUDIO REQUESTS
    void CAudioSystem::PushRequestNew(AudioRequestType&& request)
    {
        AZStd::scoped_lock lock(m_pendingRequestsMutex);
        m_pendingRequestsQueue.push_back(AZStd::move(request));
    }

    void CAudioSystem::PushRequestBlockingNew([[maybe_unused]] AudioRequestType&& request)
    {
        // pass this request to be processed immediately and block this calling thread.
        {
            AZStd::scoped_lock lock(m_blockingRequestsMutex);
            m_blockingRequestsQueue.push_back(AZStd::move(request));
        }

        m_processingEvent.release();
        m_mainEvent.acquire();
    }
    //~ NEW AUDIO REQUESTS



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
        AZ_Assert(g_audioThreadId == AZStd::this_thread::get_id(), "AudioSystem::InternalUpdate - called from non-Audio thread!");
        AZ_PROFILE_FUNCTION(Audio);

        auto startUpdateTime = AZStd::chrono::system_clock::now();        // stamp the start time

        bool handledBlockingRequests = false;

        {   // Process a single blocking request, if any, and release the semaphore on the main thread.
            // This ensures that main thread will become unblocked quickly.
            // If blocking requests were processed, can skip processing of normal requests and skip having
            // the audio thread block through the rest of its update period.
            AZStd::scoped_lock lock(m_blockingRequestsMutex);
            if (!m_blockingRequestsQueue.empty())
            {
                // Blocking request...
                AudioRequestType& request(m_blockingRequestsQueue.front());
                m_oATL.ProcessRequestNew(AZStd::move(request));
                handledBlockingRequests = true;

                m_mainEvent.release();

                m_blockingRequestsQueue.pop_front();
            }
        }

        if (!handledBlockingRequests)
        {
            TAudioRequestQueue requestsToProcess{};
            {
                AZStd::scoped_lock lock(m_pendingRequestsMutex);
                m_pendingRequestsQueue.swap(AZStd::move(requestsToProcess));
            }

            while (!requestsToProcess.empty())
            {
                // Normal request...
                AudioRequestType& request(requestsToProcess.front());
                m_oATL.ProcessRequestNew(AZStd::move(request));
                requestsToProcess.pop_front();
            }
        }

        m_oATL.Update();

    #if !defined(AUDIO_RELEASE)
        #if defined(PROVIDE_GETNAME_SUPPORT)
        {
            AZ_PROFILE_SCOPE(Audio, "Sync Debug Name Changes");
            AZStd::scoped_lock lock(m_debugNameStoreMutex);
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
        Audio::System::Shutdown shutdownRequest;
        AZ::Interface<IAudioSystem>::Get()->PushRequestBlockingNew(AZStd::move(shutdownRequest));

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
        [[maybe_unused]]
        const char* audioControlsPath = GetControlsPath();

        Audio::TAudioPreloadRequestID levelPreloadId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
        if (levelName && levelName[0] != '\0')
        {
            levelPreloadId = GetAudioPreloadRequestID(levelName);
        }

        Audio::System::ReloadAll reloadRequest;
        reloadRequest.m_controlsPath = audioControlsPath;
        reloadRequest.m_levelName = levelName;
        reloadRequest.m_levelPreloadId = levelPreloadId;
        AZ::Interface<IAudioSystem>::Get()->PushRequestBlockingNew(AZStd::move(reloadRequest));
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
                [[fallthrough]];
            case eACT_NONE:
                [[fallthrough]];
            default:
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
#if !defined(AUDIO_RELEASE)
    void CAudioSystem::DrawAudioDebugData()
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::DrawAudioDebugData - called from non-Main thread!");

        if (CVars::s_debugDrawOptions.GetRawFlags() != 0)
        {
            // TODO:
            //SAudioRequest oRequest;
            //oRequest.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);
            //SAudioManagerRequestData<eAMRT_DRAW_DEBUG_INFO> oRequestData;
            //oRequest.pData = &oRequestData;
            //PushRequestBlocking(oRequest);

            //Audio::System::DrawDebug drawDebugRequest;
            //AZ::Interface<IAudioSystem>::Get()->PushRequestBlockingNew(AZStd::move(drawDebugRequest));
        }
    }
#endif // !AUDIO_RELEASE

} // namespace Audio
