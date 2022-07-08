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
        void Initialize(const char* sObjectName, void* ownerOverride = nullptr, const bool bInitAsync = true) override;
        void Release() override;
        void ExecuteTrigger(TAudioControlID nTriggerID) override;
        void ExecuteSourceTrigger(TAudioControlID nTriggerID, const SAudioSourceInfo& rSourceInfo) override;
        void StopAllTriggers() override;
        void StopTrigger(TAudioControlID nTriggerID) override;
        void SetSwitchState(TAudioControlID nSwitchID, TAudioSwitchStateID nStateID) override;
        void SetRtpcValue(TAudioControlID nRtpcID, float fValue) override;
        void SetObstructionCalcType(ObstructionType eObstructionType) override;
        void SetPosition(const SATLWorldPosition& refPosition) override;
        void SetPosition(const AZ::Vector3& refPosition) override;
        void SetMultiplePositions(const MultiPositionParams& params) override;
        void SetEnvironmentAmount(TAudioEnvironmentID nEnvironmentID, float fValue) override;
        void ResetEnvironments() override;
        void ResetParameters() override;
        TAudioObjectID GetAudioObjectID() const override
        {
            return m_nAudioObjectID;
        }
        // ~IAudioProxy

    private:
        void TryEnqueueRequest(AudioRequestVariant&& request);
        void ExecuteQueuedRequests();
        bool HasId() const;
        void Reset();

        AudioRequestsQueue m_queuedAudioRequests;

        SATLWorldPosition m_oPosition;
        TAudioObjectID m_nAudioObjectID{ INVALID_AUDIO_OBJECT_ID };
        void* m_ownerOverride{ nullptr };

        bool m_waitingForId{ false };
        bool m_releaseAtEndOfQueue{ false };
    };

} // namespace Audio
