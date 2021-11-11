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

#include <TrajectoryQuery.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <FeatureTrajectory.h>

namespace EMotionFX::MotionMatching
{
    AZ::Vector3 SampleFunction(TrajectoryQuery::EMode mode, float offset, float radius, float phase)
    {
        switch (mode)
        {
        case TrajectoryQuery::MODE_TWO:
        {
            AZ::Vector3 displacement = AZ::Vector3::CreateZero();
            displacement.SetX(radius * sinf(phase + offset) );
            displacement.SetY(cosf(phase + offset));
            return displacement;
        }

        case TrajectoryQuery::MODE_THREE:
        {
            AZ::Vector3 displacement = AZ::Vector3::CreateZero();
            const float rad = radius * cosf(radius + phase*0.2f);
            displacement.SetX(rad * sinf(phase + offset));
            displacement.SetY(rad * cosf(phase + offset));
            return displacement;
        }

        case TrajectoryQuery::MODE_FOUR:
        {
            AZ::Vector3 displacement = AZ::Vector3::CreateZero();
            displacement.SetX(radius * sinf(phase + offset));
            displacement.SetY(radius*2.0f * cosf(phase + offset));
            return displacement;
        }

        // MODE_ONE and default
        default:
        {
            AZ::Vector3 displacement = AZ::Vector3::CreateZero();
            displacement.SetX(radius * sinf(phase * 0.7f + offset) + radius * 0.75f * cosf(phase * 2.0f + offset * 2.0f));
            displacement.SetY(radius * cosf(phase * 0.4f + offset));
            return displacement;
        }
        }
    }

    void TrajectoryQuery::Update(const ActorInstance* actorInstance,
        const FeatureTrajectory* trajectoryFeature,
        const TrajectoryHistory& trajectoryHistory,
        EMode mode,
        const AZ::Vector3& targetPos,
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
            static float phase = 0.0f;
            phase += timeDelta * pathSpeed;
            AZ::Vector3 base = SampleFunction(mode, 0.0f, pathRadius, phase);
            for (size_t i = 0; i < numFutureSamples; ++i)
            {
                const float offset = i * 0.1f;
                const AZ::Vector3 curSample = SampleFunction(mode, offset, pathRadius, phase);
                AZ::Vector3 displacement = curSample - base;
                m_futureControlPoints[i].m_position = actorInstance->GetWorldSpaceTransform().m_position + displacement;
            }
        }

        // Build the past trajectory control points.
        const size_t numPastSamples = trajectoryFeature->GetNumPastSamples();
        m_pastControlPoints.resize(numPastSamples);
        const float pastTimeRange = trajectoryFeature->GetPastTimeRange();

        for (size_t i = 0; i < numPastSamples; ++i)
        {
            const float sampleTimeNormalized = i / static_cast<float>(numPastSamples - 1);
            const AZ::Vector3 position = trajectoryHistory.Sample(sampleTimeNormalized * pastTimeRange);
            m_pastControlPoints[i].m_position = position;
        }
    }

    void TrajectoryQuery::DebugDraw(AZ::RPI::AuxGeomDrawPtr& drawQueue, const AZ::Color& color) const
    {
        DebugDrawControlPoints(drawQueue, m_pastControlPoints, color);
        DebugDrawControlPoints(drawQueue, m_futureControlPoints, color);
    }

    void TrajectoryQuery::DebugDrawControlPoints(AZ::RPI::AuxGeomDrawPtr& drawQueue,
        const AZStd::vector<ControlPoint>& controlPoints,
        const AZ::Color& color)
    {
        const float markerSize = 0.02f;

        const size_t numControlPoints = controlPoints.size();
        if (numControlPoints > 1)
        {
            for (size_t i = 0; i < numControlPoints - 1; ++i)
            {
                const AZ::Vector3& posA = controlPoints[i].m_position;
                const AZ::Vector3& posB = controlPoints[i + 1].m_position;
                const AZ::Vector3 diff = posB - posA;

                drawQueue->DrawCylinder(/*center=*/(posB + posA) * 0.5f,
                    /*direction=*/diff.GetNormalizedSafe(),
                    /*radius=*/0.0025f,
                    /*height=*/diff.GetLength(),
                    color,
                    AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                    AZ::RPI::AuxGeomDraw::DepthTest::Off);
            }

            for (const ControlPoint& controlPoint : controlPoints)
            {
                drawQueue->DrawSphere(controlPoint.m_position,
                    markerSize,
                    color,
                    AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                    AZ::RPI::AuxGeomDraw::DepthTest::Off);
            }
        }
    }
} // namespace EMotionFX::MotionMatching
