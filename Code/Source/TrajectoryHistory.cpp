/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <TrajectoryHistory.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <AzCore/std/algorithm.h>

namespace EMotionFX::MotionMatching
{
    void TrajectoryHistory::Init(const Pose& pose, size_t jointIndex, float numSecondsToTrack)
    {
        AZ_Assert(numSecondsToTrack > 0.0f, "Number of seconds to track has to be greater than zero.");
        Clear();
        m_numSecondsToTrack = numSecondsToTrack;
        m_jointIndex = jointIndex;

        // Pre-fill the history with samples from the current joint position.
        PrefillSamples(pose, /*timeDelta=*/1.0f / 60.0f);
    }

    void TrajectoryHistory::AddSample(const Pose& pose)
    {
        const AZ::Vector3 position = pose.GetWorldSpaceTransform(m_jointIndex).m_position;

        // The new key will be added at the end of the keytrack.
        m_keytrack.AddKey(m_currentTime, position);

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

    AZ::Vector3 TrajectoryHistory::Sample(float time) const
    {
        if (m_keytrack.GetNumKeys() == 0)
        {
            return AZ::Vector3::CreateZero();
        }

        return m_keytrack.GetValueAtTime(m_keytrack.GetLastTime() - time);
    }

    AZ::Vector3 TrajectoryHistory::SampleNormalized(float normalizedTime) const
    {
        const float firstTime = m_keytrack.GetFirstTime();
        const float lastTime = m_keytrack.GetLastTime();
        const float range = lastTime - firstTime;

        const float time = (1.0f - normalizedTime) * range + firstTime;
        return m_keytrack.GetValueAtTime(time);
    }

    void TrajectoryHistory::DebugDraw(AZ::RPI::AuxGeomDrawPtr& drawQueue,
        [[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw,
        const AZ::Color& color,
        float timeStart) const
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

            const AZ::Vector3 currentPosition = m_keytrack.GetKey(i)->GetValue();
            drawQueue->DrawSphere(currentPosition,
                markerSize,
                finalColor,
                AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                AZ::RPI::AuxGeomDraw::DepthTest::Off);
        }
    }

    void TrajectoryHistory::DebugDrawSampled([[maybe_unused]] AZ::RPI::AuxGeomDrawPtr& drawQueue,
        EMotionFX::DebugDraw::ActorInstanceData& draw,
        size_t numSamples,
        const AZ::Color& color) const
    {
        AZ::Vector3 lastPos = SampleNormalized(0.0f);
        for (size_t i = 0; i < numSamples; ++i)
        {
            const float sampleTime = i / static_cast<float>(numSamples - 1);
            const AZ::Vector3 currentPos = SampleNormalized(sampleTime);
            if (i > 0)
            {
                draw.DrawLine(lastPos, currentPos, color);
            }

            draw.DrawMarker(currentPos, color, m_debugMarkerSize);
            lastPos = currentPos;
        }
    }
} // namespace EMotionFX::MotionMatching
