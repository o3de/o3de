/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TrajectoryQuery.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <FeatureTrajectory.h>

namespace EMotionFX::MotionMatching
{
    AZ::Vector3 SampleFunction(float offset, float radius, float phase)
    {
        phase += 10.7;
        AZ::Vector3 displacement = AZ::Vector3::CreateZero();
        displacement.SetX(radius * sinf(phase * 0.7f + offset) + radius * 0.75f * cosf(phase * 2.0f + offset * 2.0f));
        displacement.SetY(radius * cosf(phase * 0.4f + offset));
        return displacement;
    }

    void TrajectoryQuery::Update(const ActorInstance* actorInstance,
        const FeatureTrajectory* trajectoryFeature,
        const TrajectoryHistory& trajectoryHistory,
        EMode mode,
        [[maybe_unused]] AZ::Vector3 targetPos,
        [[maybe_unused]] AZ::Vector3 targetFacingDir,
        float timeDelta,
        float pathRadius,
        float pathSpeed)
    {
        // Build the future trajectory control points.
        const size_t numFutureSamples = trajectoryFeature->GetNumFutureSamples();
        m_futureControlPoints.resize(numFutureSamples);

        if (mode == MODE_TARGETDRIVEN)
        {
            const AZ::Vector3 curPos = actorInstance->GetWorldSpaceTransform().m_position;
            if (curPos.IsClose(targetPos, 0.1f))
            {
                for (size_t i = 0; i < numFutureSamples; ++i)
                {
                    m_futureControlPoints[i].m_position = curPos;
                }
            }
            else
            {
                // NOTE: Improve it by using a curve to the target.
                for (size_t i = 0; i < numFutureSamples; ++i)
                {
                    const float sampleTime = static_cast<float>(i) / (numFutureSamples - 1);
                    m_futureControlPoints[i].m_position = curPos.Lerp(targetPos, sampleTime);
                }
            }
        }
        else
        {
            m_automaticModePhase += timeDelta * pathSpeed;
            AZ::Vector3 base = SampleFunction(0.0f, pathRadius, m_automaticModePhase);
            for (size_t i = 0; i < numFutureSamples; ++i)
            {
                const float offset = i * 0.1f;
                const AZ::Vector3 curSample = SampleFunction(offset, pathRadius, m_automaticModePhase);
                AZ::Vector3 displacement = curSample - base;
                m_futureControlPoints[i].m_position = actorInstance->GetWorldSpaceTransform().m_position + displacement;

                // Evaluate a control point slightly further into the future than the actual
                // one and use the position difference as the facing direction.
                const AZ::Vector3 deltaSample = SampleFunction(offset + 0.01f, pathRadius, m_automaticModePhase);
                const AZ::Vector3 dir = deltaSample - curSample;
                m_futureControlPoints[i].m_facingDirection = dir.GetNormalizedSafe();
            }
        }

        // Build the past trajectory control points.
        const size_t numPastSamples = trajectoryFeature->GetNumPastSamples();
        m_pastControlPoints.resize(numPastSamples);
        const float pastTimeRange = trajectoryFeature->GetPastTimeRange();

        for (size_t i = 0; i < numPastSamples; ++i)
        {
            const float sampleTimeNormalized = i / static_cast<float>(numPastSamples - 1);
            const TrajectoryHistory::Sample sample = trajectoryHistory.Evaluate(sampleTimeNormalized * pastTimeRange);
            m_pastControlPoints[i] = { sample.m_position, sample.m_facingDirection };
        }
    }

    void TrajectoryQuery::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color) const
    {
        DebugDrawControlPoints(debugDisplay, m_pastControlPoints, color);
        DebugDrawControlPoints(debugDisplay, m_futureControlPoints, color);
    }

    void TrajectoryQuery::DebugDrawControlPoints(AzFramework::DebugDisplayRequests& debugDisplay,
        const AZStd::vector<ControlPoint>& controlPoints,
        const AZ::Color& color)
    {
        const float markerSize = 0.02f;

        const size_t numControlPoints = controlPoints.size();
        if (numControlPoints > 1)
        {
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(color);

            for (size_t i = 0; i < numControlPoints - 1; ++i)
            {
                const ControlPoint& current = controlPoints[i];
                const AZ::Vector3& posA = current.m_position;
                const AZ::Vector3& posB = controlPoints[i + 1].m_position;
                const AZ::Vector3 diff = posB - posA;

                debugDisplay.DrawSolidCylinder(/*center=*/(posB + posA) * 0.5f,
                    /*direction=*/diff.GetNormalizedSafe(),
                    /*radius=*/0.0025f,
                    /*height=*/diff.GetLength(),
                    /*drawShaded=*/false);

                FeatureTrajectory::DebugDrawFacingDirection(debugDisplay, current.m_position, current.m_facingDirection);
            }

            for (const ControlPoint& controlPoint : controlPoints)
            {
                debugDisplay.DrawBall(controlPoint.m_position, markerSize, /*drawShaded=*/false);
                FeatureTrajectory::DebugDrawFacingDirection(debugDisplay, controlPoint.m_position, controlPoint.m_facingDirection);
            }
        }
    }
} // namespace EMotionFX::MotionMatching
