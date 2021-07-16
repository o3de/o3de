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
        NullAudioProxy() {}
        ~NullAudioProxy() override {}

        void Initialize(const char* const, const bool = true) override {}
        void Release() override {}
        void Reset() override {}
        void ExecuteSourceTrigger([[maybe_unused]] TAudioControlID nTriggerID, [[maybe_unused]] const SAudioSourceInfo& rSourceInfo, [[maybe_unused]] const SAudioCallBackInfos & rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override {}
        void ExecuteTrigger(const TAudioControlID, [[maybe_unused]] const SAudioCallBackInfos& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override {}
        void StopAllTriggers() override {}
        void StopTrigger(const TAudioControlID) override {}
        void SetSwitchState(const TAudioControlID, const TAudioSwitchStateID) override {}
        void SetRtpcValue(const TAudioControlID, const float) override {}
        void SetObstructionCalcType(const EAudioObjectObstructionCalcType) override {}
        void SetPosition(const SATLWorldPosition&) override {}
        void SetPosition(const AZ::Vector3&) override {}
        void SetMultiplePositions([[maybe_unused]] const MultiPositionParams& positions) override {}
        void SetEnvironmentAmount(const TAudioEnvironmentID, const float) override {}
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
            AudioSystemThreadSafeRequestBus::Handler::BusConnect();
            AZ_TracePrintf(AZ::Debug::Trace::GetDefaultSystemWindow(), "<Audio>: Running with Null Audio System!\n");
        }
        ~NullAudioSystem() override
        {
            AudioSystemRequestBus::Handler::BusDisconnect();
            AudioSystemThreadSafeRequestBus::Handler::BusDisconnect();
        }

        bool Initialize() override { return true; }
        void Release() override {}
        void ExternalUpdate() override {}

        void PushRequest(const SAudioRequest&) override {}
        void PushRequestBlocking(const SAudioRequest&) override {}
        void PushRequestThreadSafe(const SAudioRequest&) override {}
        void AddRequestListener(AudioRequestCallbackType, void* const, const EAudioRequestType, const TATLEnumFlagsType) override {}
        void RemoveRequestListener(AudioRequestCallbackType, void* const) override {}

        TAudioControlID GetAudioTriggerID(const char* const) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioControlID GetAudioRtpcID(const char* const) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioControlID GetAudioSwitchID(const char* const) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioSwitchStateID GetAudioSwitchStateID(const TAudioControlID, const char* const) const override { return INVALID_AUDIO_SWITCH_STATE_ID; }
        TAudioPreloadRequestID GetAudioPreloadRequestID(const char* const) const override { return INVALID_AUDIO_PRELOAD_REQUEST_ID; }
        TAudioEnvironmentID GetAudioEnvironmentID(const char* const) const override { return INVALID_AUDIO_ENVIRONMENT_ID; }

        bool ReserveAudioListenerID(TAudioObjectID& rAudioObjectID) override { rAudioObjectID = INVALID_AUDIO_OBJECT_ID; return true; }
        bool ReleaseAudioListenerID(const TAudioObjectID) override { return true; }
        bool SetAudioListenerOverrideID(const TAudioObjectID) override { return true; }

        void GetInfo(SAudioSystemInfo&) override {}
        const char* GetControlsPath() const override { return ""; }
        void UpdateControlsPath() override {}
        void RefreshAudioSystem(const char* const) override {}

        IAudioProxy* GetFreeAudioProxy() override { return static_cast<IAudioProxy*>(&m_nullAudioProxy); }
        void FreeAudioProxy(IAudioProxy* const) override {}

        TAudioSourceId CreateAudioSource([[maybe_unused]] const SAudioInputConfig& sourceConfig) override { return INVALID_AUDIO_SOURCE_ID; }
        void DestroyAudioSource([[maybe_unused]] TAudioSourceId sourceId) override {}

        const char* GetAudioControlName([[maybe_unused]] const EAudioControlType controlType, [[maybe_unused]] const TATLIDType atlID) const override { return nullptr; }
        const char* GetAudioSwitchStateName([[maybe_unused]] const TAudioControlID switchID, [[maybe_unused]] const TAudioSwitchStateID stateID) const override { return nullptr; }

    private:
        NullAudioProxy m_nullAudioProxy;
    };

} // namespace Audio
