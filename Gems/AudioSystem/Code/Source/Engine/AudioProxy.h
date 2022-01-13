/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AudioAllocators.h>
#include <IAudioSystem.h>

#include <AzCore/std/containers/deque.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioProxy
        : public IAudioProxy
    {
    public:
        CAudioProxy() = default;
        ~CAudioProxy() override;

        // IAudioProxy
        void Initialize(const char* const sObjectName, const bool bInitAsync = true) override;
        void Release() override;
        void ExecuteSourceTrigger(
            TAudioControlID nTriggerID,
            const SAudioSourceInfo& rSourceInfo,
            const SAudioCallBackInfos& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override;
        void ExecuteTrigger(const TAudioControlID nTriggerID, const SAudioCallBackInfos& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override;
        void StopAllTriggers() override;
        void StopTrigger(const TAudioControlID nTriggerID) override;
        void SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID) override;
        void SetRtpcValue(const TAudioControlID nRtpcID, const float fValue) override;
        void SetObstructionCalcType(const EAudioObjectObstructionCalcType eObstructionType) override;
        void SetPosition(const SATLWorldPosition& refPosition) override;
        void SetPosition(const AZ::Vector3& refPosition) override;
        void SetMultiplePositions(const MultiPositionParams& params) override;
        void SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fValue) override;
        void ResetEnvironments() override;
        void ResetParameters() override;
        TAudioObjectID GetAudioObjectID() const override
        {
            return m_nAudioObjectID;
        }
        // ~IAudioProxy

    private:
        TAudioObjectID m_nAudioObjectID{ INVALID_AUDIO_OBJECT_ID };
        SATLWorldPosition m_oPosition;

        // NEW!
        using QueuedAudioRequests = AZStd::deque<AudioRequestVariant, Audio::AudioSystemStdAllocator>;
        QueuedAudioRequests m_queuedAudioRequests;
        void TryEnqueueRequest(AudioRequestVariant&& request);
        void ExecuteQueuedRequests();
        bool HasId() const;
    };

} // namespace Audio
