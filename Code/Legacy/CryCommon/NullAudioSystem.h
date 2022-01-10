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

        void Initialize(const char*, const bool = true) override {}
        void Release() override {}
        void Reset() override {}
        void ExecuteSourceTrigger(TAudioControlID, const SAudioSourceInfo&, const SAudioCallBackInfos& = SAudioCallBackInfos::GetEmptyObject()) override {}
        void ExecuteTrigger(TAudioControlID, const SAudioCallBackInfos& = SAudioCallBackInfos::GetEmptyObject()) override {}
        void StopAllTriggers() override {}
        void StopTrigger(TAudioControlID) override {}
        void SetSwitchState(TAudioControlID, TAudioSwitchStateID) override {}
        void SetRtpcValue(TAudioControlID, float) override {}
        void SetObstructionCalcType(EAudioObjectObstructionCalcType) override {}
        void SetPosition(const SATLWorldPosition&) override {}
        void SetPosition(const AZ::Vector3&) override {}
        void SetMultiplePositions(const MultiPositionParams&) override {}
        void SetEnvironmentAmount(TAudioEnvironmentID, float) override {}
        void SetCurrentEnvironments() override {}
        void ResetRtpcValues() override {}
        TAudioObjectID GetAudioObjectID() const override { return INVALID_AUDIO_OBJECT_ID; }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class NullAudioSystem
        : public IAudioSystem
    {
    public:
        AZ_CLASS_ALLOCATOR(Audio::NullAudioSystem, AZ::SystemAllocator, 0)

        NullAudioSystem()
        {
            AudioSystemRequestBus::Handler::BusConnect();
            AZ_TracePrintf(AZ::Debug::Trace::GetDefaultSystemWindow(), "<Audio>: Running with Null Audio System!\n");
        }
        ~NullAudioSystem() override
        {
            AudioSystemRequestBus::Handler::BusDisconnect();
        }

        bool Initialize() override { return true; }
        void Release() override {}
        void ExternalUpdate() override {}

        //! NEW AUDIO REQUESTS
        void PushRequestNew(AudioRequestVariant&&) override {}
        void PushRequestBlockingNew(AudioRequestVariant&&) override {}
        //~ NEW AUDIO REQUESTS

        void AddRequestListener(AudioRequestCallbackType, void*, EAudioRequestType, TATLEnumFlagsType) override {}
        void RemoveRequestListener(AudioRequestCallbackType, void*) override {}

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

        IAudioProxy* GetFreeAudioProxy() override { return static_cast<IAudioProxy*>(&m_nullAudioProxy); }
        void FreeAudioProxy(IAudioProxy*) override {}

        TAudioSourceId CreateAudioSource(const SAudioInputConfig&) override { return INVALID_AUDIO_SOURCE_ID; }
        void DestroyAudioSource(TAudioSourceId) override {}

        const char* GetAudioControlName(EAudioControlType, TATLIDType) const override { return nullptr; }
        const char* GetAudioSwitchStateName(TAudioControlID, TAudioSwitchStateID) const override { return nullptr; }

    private:
        NullAudioProxy m_nullAudioProxy;
    };

} // namespace Audio
