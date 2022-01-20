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
#include <AudioInternalInterfaces.h>

#include <AzCore/Debug/Budget.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/thread.h>

#define PROVIDE_GETNAME_SUPPORT

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
        : public IAudioSystem
    {
        friend class CAudioThread;

    public:
        AZ_RTTI(CAudioSystem, "{96254647-000D-4896-93C4-92E0F258F21D}", IAudioSystem);
        AZ_CLASS_ALLOCATOR(CAudioSystem, AZ::SystemAllocator, 0);

        CAudioSystem();
        ~CAudioSystem() override;

        CAudioSystem(const CAudioSystem&) = delete;
        CAudioSystem& operator=(const CAudioSystem&) = delete;

        bool Initialize() override;
        void Release() override;

        //! NEW AUDIO REQUESTS
        void PushRequestNew(AudioRequestVariant&& request) override;
        void PushRequestBlockingNew(AudioRequestVariant&& request) override;
        void PushCallbackNew(AudioRequestVariant&& callback) override;
        //~ NEW AUDIO REQUESTS

        void ExternalUpdate() override;

        void AddRequestListener(AudioRequestCallbackType func, void* const callbackOwner,
            EAudioRequestType const requestType = eART_AUDIO_ALL_REQUESTS,
            TATLEnumFlagsType const specificRequestMask = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS) override;
        void RemoveRequestListener(AudioRequestCallbackType func, void* const callbackOwner) override;

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

        // When AUDIO_RELEASE is defined, these two functions always return nullptr
        const char* GetAudioControlName(const EAudioControlType controlType, const TATLIDType atlID) const override;
        const char* GetAudioSwitchStateName(const TAudioControlID switchID, const TAudioSwitchStateID stateID) const override;

    private:
        using TAudioProxies = AZStd::vector<CAudioProxy*, Audio::AudioSystemStdAllocator>;

        void InternalUpdate();

        bool m_bSystemInitialized;

        using duration_ms = AZStd::chrono::duration<float, AZStd::milli>;
        const duration_ms m_targetUpdatePeriod = AZStd::chrono::milliseconds(8);

        CAudioTranslationLayer m_oATL;
        CAudioThread m_audioSystemThread;


        //! NEW AUDIO REQUESTS
        using TAudioRequestQueue = AZStd::deque<AudioRequestVariant, Audio::AudioSystemStdAllocator>;
        TAudioRequestQueue m_pendingRequestsQueue;
        TAudioRequestQueue m_blockingRequestsQueue;
        TAudioRequestQueue m_pendingCallbacksQueue;
        AZStd::mutex m_pendingRequestsMutex;
        AZStd::mutex m_blockingRequestsMutex;
        AZStd::mutex m_pendingCallbacksMutex;
        //~ NEW AUDIO REQUESTS

        // Synchronization objects
        AZStd::binary_semaphore m_mainEvent;
        AZStd::binary_semaphore m_processingEvent;

        // Audio Proxy containers
        TAudioProxies m_apAudioProxies;
        TAudioProxies m_apAudioProxiesToBeFreed;

        AZStd::string m_controlsPath;

    #if !defined(AUDIO_RELEASE)
        #if defined(PROVIDE_GETNAME_SUPPORT)
        mutable AZStd::mutex m_debugNameStoreMutex;
        CATLDebugNameStore m_debugNameStore;
        #endif // PROVIDE_GETNAME_SUPPORT

        void DrawAudioDebugData();
    #endif // !AUDIO_RELEASE
    };

} // namespace Audio
