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
#include <AzCore/Console/ILogger.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace Audio
{
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

        #if !defined(AUDIO_RELEASE)
        AzFramework::DebugDisplayEventBus::Handler::BusConnect();
        #endif // !AUDIO_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystem::~CAudioSystem()
    {
        #if !defined(AUDIO_RELEASE)
        AzFramework::DebugDisplayEventBus::Handler::BusDisconnect();
        #endif // !AUDIO_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::PushRequest(AudioRequestVariant&& request)
    {
        AZStd::scoped_lock lock(m_pendingRequestsMutex);
        m_pendingRequestsQueue.push_back(AZStd::move(request));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::PushRequests(AudioRequestsQueue& requests)
    {
        AZStd::scoped_lock lock(m_pendingRequestsMutex);
        for (auto& request : requests)
        {
            m_pendingRequestsQueue.push_back(AZStd::move(request));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::PushRequestBlocking(AudioRequestVariant&& request)
    {
        // Add this request to be processed immediately.
        // Release the m_processingEvent so that when the request is finished the audio thread doesn't
        // block through it's normal time slice and can immediately re-enter the run loop to process more.
        // Acquire the m_mainEvent semaphore to block the main thread.
        // This helps when there's a longer queue of blocking requests so that the back-and-forth between
        // threads is minimized.
        {
            AZStd::scoped_lock lock(m_blockingRequestsMutex);
            m_blockingRequestsQueue.push_back(AZStd::move(request));
        }

        m_processingEvent.release();
        m_mainEvent.acquire();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::PushCallback(AudioRequestVariant&& callback)
    {
        AZStd::scoped_lock lock(m_pendingCallbacksMutex);
        m_pendingCallbacksQueue.push_back(AZStd::move(callback));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::ExternalUpdate()
    {
        // Main Thread!
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::ExternalUpdate - called from non-Main thread!");

        {
            AudioRequestsQueue callbacksToProcess{};
            {
                AZStd::scoped_lock lock(m_pendingCallbacksMutex);
                callbacksToProcess = AZStd::move(m_pendingCallbacksQueue);
            }

            while (!callbacksToProcess.empty())
            {
                AudioRequestVariant& callbackVariant(callbacksToProcess.front());
                AZStd::visit(
                    [](auto&& request)
                    {
                        if (request.m_callback)
                        {
                            request.m_callback(request);
                        }
                    },
                    callbackVariant);
                callbacksToProcess.pop_front();
            }
        }

        // Other notifications to be sent out...
        AudioTriggerNotificationBus::ExecuteQueuedEvents();

        // Free any Audio Proxies that are queued up for deletion...
        for (auto audioProxy : m_apAudioProxiesToBeFreed)
        {
            azdestroy(audioProxy, Audio::AudioSystemAllocator);
        }

        m_apAudioProxiesToBeFreed.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::InternalUpdate()
    {
        // Audio Thread!
        AZ_Assert(g_audioThreadId == AZStd::this_thread::get_id(), "AudioSystem::InternalUpdate - called from non-Audio thread!");
        AZ_PROFILE_FUNCTION(Audio);

        auto startUpdateTime = AZStd::chrono::steady_clock::now();        // stamp the start time

        // Process a single blocking request, if any, and release the semaphore the main thread is trying to acquire.
        // This ensures that main thread will become unblocked quickly.
        // If blocking requests were processed, can skip processing of normal requests and skip having
        // the audio thread block through the rest of its update period.
        bool handleBlockingRequest = false;
        AudioRequestVariant blockingRequest;

        {
            AZStd::scoped_lock lock(m_blockingRequestsMutex);
            handleBlockingRequest = !m_blockingRequestsQueue.empty();
            if (handleBlockingRequest)
            {
                blockingRequest = AZStd::move(m_blockingRequestsQueue.front());
                m_blockingRequestsQueue.pop_front();
            }
        }

        if (handleBlockingRequest)
        {
            m_oATL.ProcessRequest(AZStd::move(blockingRequest));
            m_mainEvent.release();
        }

        if (!handleBlockingRequest)
        {
            // Normal request processing: lock and swap the pending requests queue
            // so that the queue can be opened for new requests while the current set
            // of requests can be processed.
            AudioRequestsQueue requestsToProcess{};
            {
                AZStd::scoped_lock lock(m_pendingRequestsMutex);
                requestsToProcess = AZStd::move(m_pendingRequestsQueue);
            }

            while (!requestsToProcess.empty())
            {
                // Normal request...
                AudioRequestVariant& request(requestsToProcess.front());
                m_oATL.ProcessRequest(AZStd::move(request));
                requestsToProcess.pop_front();
            }
        }

        m_oATL.Update();

        if (!handleBlockingRequest)
        {
            auto endUpdateTime = AZStd::chrono::steady_clock::now();      // stamp the end time
            auto elapsedUpdateTime = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(endUpdateTime - startUpdateTime);
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
                auto audioProxy = azcreate(CAudioProxy, (), Audio::AudioSystemAllocator);
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

        // Mark the system as uninitialized before we destroy the audio proxies so that we can avoid
        // recycling them on system shutdown.
        m_bSystemInitialized = false;

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
        Audio::SystemRequest::Shutdown shutdownRequest;
        AZ::Interface<IAudioSystem>::Get()->PushRequestBlocking(AZStd::move(shutdownRequest));

        m_audioSystemThread.Deactivate();
        m_oATL.ShutDown();
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
            AZLOG_ERROR("AudioSystem::UpdateControlsPath - failed to normalize the controls path '%s'!", controlsPath.c_str());
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

        Audio::SystemRequest::ReloadAll reloadRequest;
        reloadRequest.m_controlsPath = audioControlsPath;
        reloadRequest.m_levelName = levelName;
        reloadRequest.m_levelPreloadId = levelPreloadId;
        AZ::Interface<IAudioSystem>::Get()->PushRequestBlocking(AZStd::move(reloadRequest));
    #endif // !AUDIO_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IAudioProxy* CAudioSystem::GetAudioProxy()
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::GetAudioProxy - called from a non-Main thread!");
        CAudioProxy* audioProxy = nullptr;

        if (!m_apAudioProxies.empty())
        {
            audioProxy = m_apAudioProxies.back();
            m_apAudioProxies.pop_back();
        }
        else
        {
            audioProxy = azcreate(CAudioProxy, (), Audio::AudioSystemAllocator);
            AZ_Assert(audioProxy != nullptr, "AudioSystem::GetAudioProxy - failed to create new AudioProxy instance!");
        }

        return static_cast<IAudioProxy*>(audioProxy);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystem::RecycleAudioProxy(IAudioProxy* const audioProxyI)
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::RecycleAudioProxy - called from a non-Main thread!");
        auto const audioProxy = static_cast<CAudioProxy*>(audioProxyI);

        // If the system is shutting down, don't recycle the audio proxies.
        if (!m_bSystemInitialized)
        {
            return;
        }

        if (AZStd::find(m_apAudioProxiesToBeFreed.begin(), m_apAudioProxiesToBeFreed.end(), audioProxy) != m_apAudioProxiesToBeFreed.end()
            || AZStd::find(m_apAudioProxies.begin(), m_apAudioProxies.end(), audioProxy) != m_apAudioProxies.end())
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
#if !defined(AUDIO_RELEASE)
    void CAudioSystem::DrawGlobalDebugInfo()
    {
        AZ_Assert(g_mainThreadId == AZStd::this_thread::get_id(), "AudioSystem::DrawGlobalDebugInfo - called from non-Main thread!");

        if (CVars::s_debugDrawOptions.GetRawFlags() != 0)
        {
            Audio::SystemRequest::DrawDebug drawDebug;
            AZ::Interface<IAudioSystem>::Get()->PushRequestBlocking(AZStd::move(drawDebug));
        }
    }
#endif // !AUDIO_RELEASE

} // namespace Audio
