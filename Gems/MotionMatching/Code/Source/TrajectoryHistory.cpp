/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TrajectoryHistory.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/EMotionFXManager.h>

namespace EMotionFX::MotionMatching
{
    TrajectoryHistory::Sample operator*(TrajectoryHistory::Sample sample, float weight)
    {
        return {sample.m_position * weight, sample.m_facingDirection * weight};
    }

    TrajectoryHistory::Sample operator*(float weight, TrajectoryHistory::Sample sample)
    {
        return {weight * sample.m_position, weight * sample.m_facingDirection};
    }

    TrajectoryHistory::Sample operator-(TrajectoryHistory::Sample lhs, const TrajectoryHistory::Sample& rhs)
    {
        return {lhs.m_position - rhs.m_position, lhs.m_facingDirection - rhs.m_facingDirection};
    }

    TrajectoryHistory::Sample operator+(TrajectoryHistory::Sample lhs, const TrajectoryHistory::Sample& rhs)
    {
        return {lhs.m_position + rhs.m_position, lhs.m_facingDirection + rhs.m_facingDirection};
    }

    void TrajectoryHistory::Init(const Pose& pose, size_t jointIndex, const AZ::Vector3& facingAxisDir, float numSecondsToTrack)
    {
        AZ_Assert(numSecondsToTrack > 0.0f, "Number of seconds to track has to be greater than zero.");
        Clear();
        m_jointIndex = jointIndex;
        m_facingAxisDir = facingAxisDir;
        m_numSecondsToTrack = numSecondsToTrack;

        // Pre-fill the history with samples from the current joint position.
        PrefillSamples(pose, /*timeDelta=*/1.0f / 60.0f);
    }

    void TrajectoryHistory::AddSample(const Pose& pose)
    {
        Sample sample;
        const Transform worldSpaceTransform = pose.GetWorldSpaceTransform(m_jointIndex);
        sample.m_position = worldSpaceTransform.m_position;
        sample.m_facingDirection = worldSpaceTransform.TransformVector(m_facingAxisDir).GetNormalizedSafe();

        // The new key will be added at the end of the keytrack.
        m_keytrack.AddKey(m_currentTime, sample);

        while (m_keytrack.GetNumKeys() > 2 &&
            ((m_keytrack.GetKey(m_keytrack.GetNumKeys() - 2)->GetTime() - m_keytrack.GetFirstTime()) > m_numSecondsToTrack))
        {
            m_keytrack.RemoveKey(0); // Remove first (oldest) key
        }
    }

    void TrajectoryHistory::PrefillSamples(const Pose& pose, float timeDelta)
    {
        const size_t numKeyframes = aznumeric_caster<>(m_numSecondsToTrack / timeDelta);
        for (size_t i = 0; i < numKeyframes; ++i)
        {
            AddSample(pose);
            Update(timeDelta);
        }
    }

    void TrajectoryHistory::Clear()
    {
        m_jointIndex = 0;
        m_currentTime = 0.0f;
        m_keytrack.ClearKeys();
    }

    void TrajectoryHistory::Update(float timeDelta)
    {
        m_currentTime += timeDelta;
    }

    TrajectoryHistory::Sample TrajectoryHistory::Evaluate(float time) const
    {
        if (m_keytrack.GetNumKeys() == 0)
        {
            return {};
        }

        return m_keytrack.GetValueAtTime(m_keytrack.GetLastTime() - time);
    }

    TrajectoryHistory::Sample TrajectoryHistory::EvaluateNormalized(float normalizedTime) const
    {
        const float firstTime = m_keytrack.GetFirstTime();
        const float lastTime = m_keytrack.GetLastTime();
        const float range = lastTime - firstTime;

        const float time = (1.0f - normalizedTime) * range + firstTime;
        return m_keytrack.GetValueAtTime(time);
    }

    void TrajectoryHistory::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color, float timeStart) const
    {
        const size_t numKeyframes = m_keytrack.GetNumKeys();
        if (numKeyframes == 0)
        {
            return;
        }

        // Clip some of the newest samples.
        const float adjustedLastTime = m_keytrack.GetLastTime() - timeStart;
        size_t adjustedLastKey = m_keytrack.FindKeyNumber(adjustedLastTime);
        if (adjustedLastKey == InvalidIndex)
        {
            adjustedLastKey = m_keytrack.GetNumKeys() - 1;
        }
        const float firstTime = m_keytrack.GetFirstTime();
        const float range = adjustedLastTime - firstTime;

        debugDisplay.DepthTestOff();

        for (size_t i = 0; i < adjustedLastKey; ++i)
        {
            const float time = m_keytrack.GetKey(i)->GetTime();
            const float normalized = (time - firstTime) / range;
            if (normalized < 0.3f)
            {
                continue;
            }

            // Decrease size and fade out alpha the older the sample is.
            AZ::Color finalColor = color;
            finalColor.SetA(finalColor.GetA() * 0.6f * normalized);
            const float markerSize = m_debugMarkerSize * 0.7f * normalized;

            const Sample currentSample = m_keytrack.GetKey(i)->GetValue();
            debugDisplay.SetColor(finalColor);
            debugDisplay.DrawBall(currentSample.m_position, markerSize, /*drawShaded=*/false);

            const float facingDirectionLength = m_debugMarkerSize * 10.0f * normalized;
            debugDisplay.DrawLine(currentSample.m_position, currentSample.m_position + currentSample.m_facingDirection * facingDirectionLength);
        }
    }

    void TrajectoryHistory::DebugDrawSampled(AzFramework::DebugDisplayRequests& debugDisplay,
        size_t numSamples,
        const AZ::Color& color) const
    {
        debugDisplay.DepthTestOff();
        debugDisplay.SetColor(color);

        Sample lastSample = EvaluateNormalized(0.0f);
        for (size_t i = 0; i < numSamples; ++i)
        {
            const float sampleTime = i / static_cast<float>(numSamples - 1);
            const Sample currentSample = EvaluateNormalized(sampleTime);
            if (i > 0)
            {
                debugDisplay.DrawLine(lastSample.m_position, currentSample.m_position);
            }

            debugDisplay.DrawBall(currentSample.m_position, m_debugMarkerSize, /*drawShaded=*/false);

            lastSample = currentSample;
        }
    }
} // namespace EMotionFX::MotionMatching
