/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioSystem.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class NullAudioProxy
        : public IAudioProxy
    {
    public:
        NullAudioProxy() = default;
        ~NullAudioProxy() override = default;

        void Initialize(const char*, void*, const bool = true) override {}
        void Release() override {}
        void ExecuteTrigger(TAudioControlID) override {}
        void ExecuteSourceTrigger(TAudioControlID, const SAudioSourceInfo&) override {}
        void StopAllTriggers() override {}
        void StopTrigger(TAudioControlID) override {}
        void SetSwitchState(TAudioControlID, TAudioSwitchStateID) override {}
        void SetRtpcValue(TAudioControlID, float) override {}
        void SetObstructionCalcType(ObstructionType) override {}
        void SetPosition(const SATLWorldPosition&) override {}
        void SetPosition(const AZ::Vector3&) override {}
        void SetMultiplePositions(const MultiPositionParams&) override {}
        void SetEnvironmentAmount(TAudioEnvironmentID, float) override {}
        void ResetEnvironments() override {}
        void ResetParameters() override {}
        TAudioObjectID GetAudioObjectID() const override { return INVALID_AUDIO_OBJECT_ID; }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class NullAudioSystem
        : public AZ::Interface<Audio::IAudioSystem>::Registrar
    {
    public:
        AZ_RTTI(NullAudioSystem, "{27F054BF-B51C-472C-9ECF-BBBB710C5AC1}", IAudioSystem);
        AZ_CLASS_ALLOCATOR(Audio::NullAudioSystem, AZ::SystemAllocator)

        NullAudioSystem()
        {
            AZ_TracePrintf(AZ::Debug::Trace::GetDefaultSystemWindow(), "<Audio>: Running with Null Audio System!\n");
        }
        ~NullAudioSystem() override = default;

        bool Initialize() override { return true; }
        void Release() override {}
        void ExternalUpdate() override {}

        void PushRequest(AudioRequestVariant&&) override {}
        void PushRequests(AudioRequestsQueue&) override {}
        void PushRequestBlocking(AudioRequestVariant&&) override {}
        void PushCallback(AudioRequestVariant&&) override {}

        TAudioControlID GetAudioTriggerID(const char*) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioControlID GetAudioRtpcID(const char*) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioControlID GetAudioSwitchID(const char*) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioSwitchStateID GetAudioSwitchStateID(TAudioControlID, const char*) const override { return INVALID_AUDIO_SWITCH_STATE_ID; }
        TAudioPreloadRequestID GetAudioPreloadRequestID(const char*) const override { return INVALID_AUDIO_PRELOAD_REQUEST_ID; }
        TAudioEnvironmentID GetAudioEnvironmentID(const char*) const override { return INVALID_AUDIO_ENVIRONMENT_ID; }

        bool ReserveAudioListenerID(TAudioObjectID& rAudioObjectID) override { rAudioObjectID = INVALID_AUDIO_OBJECT_ID; return true; }
        bool ReleaseAudioListenerID(TAudioObjectID) override { return true; }
        bool SetAudioListenerOverrideID(TAudioObjectID) override { return true; }

        const char* GetControlsPath() const override { return ""; }
        void UpdateControlsPath() override {}
        void RefreshAudioSystem(const char*) override {}

        IAudioProxy* GetAudioProxy() override { return static_cast<IAudioProxy*>(&m_nullAudioProxy); }
        void RecycleAudioProxy(IAudioProxy*) override {}

        TAudioSourceId CreateAudioSource(const SAudioInputConfig&) override { return INVALID_AUDIO_SOURCE_ID; }
        void DestroyAudioSource(TAudioSourceId) override {}

    private:
        NullAudioProxy m_nullAudioProxy;
    };

} // namespace Audio
