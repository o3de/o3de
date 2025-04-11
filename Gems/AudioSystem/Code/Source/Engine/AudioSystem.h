/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ATL.h>
#include <AudioAllocators.h>

#include <AzCore/Debug/Budget.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/thread.h>

#if !defined(AUDIO_RELEASE)
    #include <AzFramework/Entity/EntityDebugDisplayBus.h>
#endif // !AUDIO_RELEASE

AZ_DECLARE_BUDGET(Audio);

namespace Audio
{
    // Forward declarations.
    class CAudioSystem;
    class CAudioProxy;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioThread
    {
    public:
        CAudioThread()
            : m_running(false)
        {}
        ~CAudioThread();

        void Activate(CAudioSystem* const audioSystem);
        void Run();
        void Deactivate();

    private:
        CAudioSystem* m_audioSystem = nullptr;
        AZStd::atomic_bool m_running;
        AZStd::thread m_thread;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioSystem
        : public AZ::Interface<Audio::IAudioSystem>::Registrar
        #if !defined(AUDIO_RELEASE)
        , AzFramework::DebugDisplayEventBus::Handler
        #endif
    {
        friend class CAudioThread;

    public:
        AZ_RTTI(CAudioSystem, "{96254647-000D-4896-93C4-92E0F258F21D}", IAudioSystem);
        AZ_CLASS_ALLOCATOR(CAudioSystem, AZ::SystemAllocator);

        CAudioSystem();
        ~CAudioSystem() override;

        CAudioSystem(const CAudioSystem&) = delete;
        CAudioSystem& operator=(const CAudioSystem&) = delete;

        bool Initialize() override;
        void Release() override;
        void ExternalUpdate() override;

        void PushRequest(AudioRequestVariant&& request) override;
        void PushRequests(AudioRequestsQueue& requests) override;
        void PushRequestBlocking(AudioRequestVariant&& request) override;
        void PushCallback(AudioRequestVariant&& callback) override;

        TAudioControlID GetAudioTriggerID(const char* const sAudioTriggerName) const override;
        TAudioControlID GetAudioRtpcID(const char* const sAudioRtpcName) const override;
        TAudioControlID GetAudioSwitchID(const char* const sAudioSwitchName) const override;
        TAudioSwitchStateID GetAudioSwitchStateID(const TAudioControlID nSwitchID, const char* const sAudioSwitchStateName) const override;
        TAudioPreloadRequestID GetAudioPreloadRequestID(const char* const sAudioPreloadRequestName) const override;
        TAudioEnvironmentID GetAudioEnvironmentID(const char* const sAudioEnvironmentName) const override;

        bool ReserveAudioListenerID(TAudioObjectID& rAudioObjectID) override;
        bool ReleaseAudioListenerID(const TAudioObjectID nAudioObjectID) override;
        bool SetAudioListenerOverrideID(const TAudioObjectID nAudioObjectID) override;

        const char* GetControlsPath() const override;
        void UpdateControlsPath() override;
        void RefreshAudioSystem(const char* const levelName) override;

        IAudioProxy* GetAudioProxy() override;
        void RecycleAudioProxy(IAudioProxy* const pIAudioProxy) override;

        TAudioSourceId CreateAudioSource(const SAudioInputConfig& sourceConfig) override;
        void DestroyAudioSource(TAudioSourceId sourceId) override;

    private:
        using TAudioProxies = AZStd::vector<CAudioProxy*, Audio::AudioSystemStdAllocator>;

        void InternalUpdate();

        bool m_bSystemInitialized;

        // Using microseconds to allow sub-millisecond sleeping. 4000us is 4ms.
        // chrono::microseconds is stored in 64-bit so can safely be added to epoch time (now) to produce absolute time.
        const AZStd::chrono::microseconds m_targetUpdatePeriod = AZStd::chrono::microseconds(4000);

        CAudioTranslationLayer m_oATL;
        CAudioThread m_audioSystemThread;

        AudioRequestsQueue m_blockingRequestsQueue;
        AudioRequestsQueue m_pendingRequestsQueue;
        AudioRequestsQueue m_pendingCallbacksQueue;
        AZStd::mutex m_blockingRequestsMutex;
        AZStd::mutex m_pendingRequestsMutex;
        AZStd::mutex m_pendingCallbacksMutex;

        // Synchronization objects
        AZStd::binary_semaphore m_mainEvent;
        AZStd::binary_semaphore m_processingEvent;

        // Audio Proxy containers
        TAudioProxies m_apAudioProxies;
        TAudioProxies m_apAudioProxiesToBeFreed;

        AZStd::string m_controlsPath;

    #if !defined(AUDIO_RELEASE)
        void DrawGlobalDebugInfo() override;
    #endif // !AUDIO_RELEASE
    };

} // namespace Audio
