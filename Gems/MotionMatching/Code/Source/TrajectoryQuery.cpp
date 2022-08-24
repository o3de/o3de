/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <TrajectoryQuery.h>
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

    void TrajectoryQuery::PredictFutureTrajectory(const ActorInstance& actorInstance,
        const FeatureTrajectory* trajectoryFeature,
        const AZ::Vector3& targetPos,
        const AZ::Vector3& targetFacingDir,
        bool useTargetFacingDir)
    {
        const AZ::Vector3 actorInstanceWorldPosition = actorInstance.GetWorldSpaceTransform().m_position;
        const AZ::Quaternion actorInstanceWorldRotation = actorInstance.GetWorldSpaceTransform().m_rotation;
        const AZ::Vector3 actorInstanceToTarget = (targetPos - actorInstanceWorldPosition);

        const size_t numFutureSamples = trajectoryFeature->GetNumFutureSamples();
        const float numSections = aznumeric_cast<float>(numFutureSamples-1);

        float linearDisplacementPerSample = 0.0f;
        AZ::Quaternion targetFacingDirQuat = actorInstanceWorldRotation;
        if (!actorInstanceToTarget.IsClose(AZ::Vector3::CreateZero(), m_deadZone))
        {
            // Calculate the desired linear velocity from the current position to the target position based on the trajectory future time range.
            AZ_Assert(trajectoryFeature->GetFutureTimeRange() > AZ::Constants::FloatEpsilon, "Trajectory feature future time range is too small.");
            const float velocity = actorInstanceToTarget.GetLength() / trajectoryFeature->GetFutureTimeRange();

            linearDisplacementPerSample = (velocity / numSections);
        }
        else
        {
            // Force using the target facing direction in the dead zone as the samples of the future trajectory will be all at the same
            // location.
            useTargetFacingDir = true;
        }

        if (useTargetFacingDir)
        {
            // Use the given target facing direction and convert the direction vector to a quaternion.
            targetFacingDirQuat = AZ::Quaternion::CreateShortestArc(trajectoryFeature->GetFacingAxisDir(), targetFacingDir);
        }
        else
        {
            // Use the direction from the current actor instance position to the target as the target facing direction
            // and convert the direction vector to a quaternion.
            targetFacingDirQuat = AZ::Quaternion::CreateShortestArc(trajectoryFeature->GetFacingAxisDir(), actorInstanceToTarget);
        }

        // Set the first control point to the current position and facing direction.
        m_futureControlPoints[0].m_position = actorInstanceWorldPosition;
        m_futureControlPoints[0].m_facingDirection = actorInstanceWorldRotation.TransformVector(trajectoryFeature->GetFacingAxisDir());

        if (useTargetFacingDir)
        {
            for (size_t i = 0; i < numFutureSamples; ++i)
            {
                const float sampleTime = static_cast<float>(i) / (numFutureSamples - 1);
                m_futureControlPoints[i].m_position = actorInstanceWorldPosition.Lerp(targetPos, sampleTime);
                m_futureControlPoints[i].m_facingDirection = targetFacingDir;
            }
            return;
        }

        for (size_t i = 1; i < numFutureSamples; ++i)
        {
            const float t = aznumeric_cast<float>(i) / numSections;

            // Position
            {
                const AZ::Vector3 prevFacingDir = m_futureControlPoints[i - 1].m_facingDirection;

                // Interpolate between the linear direction to target and the facing direction from the previous sample.
                // This will make sure the facing direction close to the current time matches the current facing direction and
                // the facing direction in the most far future matches the desired target facing direction.
                const float weight = 1.0f - AZStd::pow(1.0f - t, m_positionBias);
                const AZ::Vector3 interpolatedPosDelta = prevFacingDir.Lerp(actorInstanceToTarget.GetNormalized(), weight);

                // Scale it by the desired velocity.
                const AZ::Vector3 scaledPosDelta = interpolatedPosDelta * linearDisplacementPerSample;

                m_futureControlPoints[i].m_position = m_futureControlPoints[i - 1].m_position + scaledPosDelta;
            }

            // Facing direction
            {
                // Interpolate facing direction from current character facing direction (first sample) to the target facing direction (most far future sample).
                const float weight = 1.0f - AZStd::pow(1.0f - t, m_rotationBias);
                const AZ::Quaternion interpolatedRotation = actorInstanceWorldRotation.Slerp(targetFacingDirQuat, weight);

                // Convert the interpolated rotation result back to a facing direction vector.
                const AZ::Vector3 interpolatedFacingDir = interpolatedRotation.TransformVector(trajectoryFeature->GetFacingAxisDir());

                m_futureControlPoints[i].m_facingDirection = interpolatedFacingDir.GetNormalizedSafe();
            }
        }
    }

    void TrajectoryQuery::Update(const ActorInstance& actorInstance,
        const FeatureTrajectory* trajectoryFeature,
        const TrajectoryHistory& trajectoryHistory,
        EMode mode,
        const AZ::Vector3& targetPos,
        const AZ::Vector3& targetFacingDir,
        bool useTargetFacingDir,
        float timeDelta,
        float pathRadius,
        float pathSpeed)
    {
        AZ_PROFILE_SCOPE(Animation, "TrajectoryQuery::Update");

        // Build the past trajectory control points.
        const size_t numPastSamples = trajectoryFeature->GetNumPastSamples();
        m_pastControlPoints.resize(numPastSamples);
        const float pastTimeRange = trajectoryFeature->GetPastTimeRange();

        for (size_t i = 0; i < numPastSamples; ++i)
        {
            const float sampleTimeNormalized = i / aznumeric_cast<float>(numPastSamples - 1);
            const TrajectoryHistory::Sample sample = trajectoryHistory.Evaluate(sampleTimeNormalized * pastTimeRange);
            m_pastControlPoints[i] = { sample.m_position, sample.m_facingDirection };
        }

        // Build the future trajectory control points.
        const size_t numFutureSamples = trajectoryFeature->GetNumFutureSamples();
        m_futureControlPoints.resize(numFutureSamples);

        if (mode == MODE_TARGETDRIVEN)
        {
            PredictFutureTrajectory(actorInstance, trajectoryFeature, targetPos, targetFacingDir, useTargetFacingDir);
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
                m_futureControlPoints[i].m_position = actorInstance.GetWorldSpaceTransform().m_position + displacement;

                // Evaluate a control point slightly further into the future than the actual
                // one and use the position difference as the facing direction.
                const AZ::Vector3 deltaSample = SampleFunction(offset + 0.01f, pathRadius, m_automaticModePhase);
                const AZ::Vector3 dir = deltaSample - curSample;
                m_futureControlPoints[i].m_facingDirection = dir.GetNormalizedSafe();
            }
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
