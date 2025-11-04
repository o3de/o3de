/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/KeyTrackLinearDynamic.h>

namespace EMotionFX::MotionMatching
{
    struct EMFX_API Sample
    {
        AZ_TYPE_INFO(Sample, "{6B67C064-08AF-431A-B236-82D3565D46A2}");
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Vector3 m_facingDirection = AZ::Vector3::CreateZero();
    };

    //! Used to store the trajectory history for the root motion (motion extraction node).
    //! The trajectory history is independent of the trajectory feature and captures a sample with every engine tick.
    //! The recorded history needs to record and track at least the time the trajectory feature/query requires.
    class EMFX_API TrajectoryHistory
    {
    public:
        void Init(const Pose& pose, size_t jointIndex, const AZ::Vector3& facingAxisDir, float numSecondsToTrack);
        void Clear();

        void Update(float timeDelta);
        void AddSample(const Pose& pose);

        using Sample = EMotionFX::MotionMatching::Sample;

        //! time in range [0, m_numSecondsToTrack]
        Sample Evaluate(float time) const;

        //! time in range [0, 1] where 0 is the current character position and 1 the oldest keyframe in the trajectory history
        Sample EvaluateNormalized(float normalizedTime) const;

        float GetNumSecondsToTrack() const { return m_numSecondsToTrack; }
        float GetCurrentTime() const { return m_currentTime; }
        size_t GetJointIndex() const { return m_jointIndex; }

        void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color, float timeStart = 0.0f) const;
        void DebugDrawSampled(AzFramework::DebugDisplayRequests& debugDisplay, size_t numSamples, const AZ::Color& color) const;

    private:
        void PrefillSamples(const Pose& pose, float timeDelta);

        KeyTrackLinearDynamic<Sample> m_keytrack;
        float m_numSecondsToTrack = 0.0f;
        size_t m_jointIndex = 0;
        float m_currentTime = 0.0f;
        AZ::Vector3 m_facingAxisDir; //! Facing direction of the character asset. (e.g. 0,1,0 when it is looking towards Y-axis)

        static constexpr float m_debugMarkerSize = 0.02f;
    };
} // namespace EMotionFX::MotionMatching
