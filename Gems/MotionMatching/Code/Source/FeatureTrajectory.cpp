/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Allocators.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Source/TransformData.h>
#include <FeatureMatrixTransformer.h>
#include <FeatureTrajectory.h>
#include <FrameDatabase.h>
#include <MotionMatchingInstance.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FeatureTrajectory, MotionMatchAllocator)

    bool FeatureTrajectory::Init(const InitSettings& settings)
    {
        const bool result = Feature::Init(settings);
        UpdateFacingAxis();
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////

    AZ::Vector2 FeatureTrajectory::CalculateFacingDirection(const Pose& pose, const Transform& invRootTransform) const
    {
        // Get the facing direction of the given joint for the given pose in animation world space.
        // The given pose is either sampled into the relative past or future based on the frame we want to extract the feature for.
        const AZ::Vector3 facingDirAnimationWorldSpace = pose.GetWorldSpaceTransform(m_jointIndex).TransformVector(m_facingAxisDir);

        // The invRootTransform is the inverse of the world space transform for the given joint at the frame we want to extract the feature
        // for. The result after this will be the facing direction relative to the frame we want to extract the feature for.
        const AZ::Vector3 facingDirection = invRootTransform.TransformVector(facingDirAnimationWorldSpace);

        // Project to the ground plane and make sure the direction is normalized.
        return AZ::Vector2(facingDirection).GetNormalizedSafe();
    }

    FeatureTrajectory::Sample FeatureTrajectory::GetSampleFromPose(const Pose& pose, const Transform& invRootTransform) const
    {
        // Position of the root joint in the model space relative to frame to extract.
        const AZ::Vector2 position = AZ::Vector2(invRootTransform.TransformPoint(pose.GetWorldSpaceTransform(m_jointIndex).m_position));

        // Calculate the facing direction.
        const AZ::Vector2 facingDirection = CalculateFacingDirection(pose, invRootTransform);

        return { position, facingDirection };
    }

    void FeatureTrajectory::ExtractFeatureValues(const ExtractFeatureContext& context)
    {
        const ActorInstance* actorInstance = context.m_actorInstance;
        AnimGraphPose* samplePose = context.m_posePool.RequestPose(actorInstance);
        AnimGraphPose* nextSamplePose = context.m_posePool.RequestPose(actorInstance);

        const size_t frameIndex = context.m_frameIndex;
        const Frame& currentFrame = context.m_frameDatabase->GetFrame(context.m_frameIndex);

        // Inverse of the root transform for the frame that we want to extract data from.
        const Transform invRootTransform = context.m_framePose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();

        const size_t midSampleIndex = CalcMidFrameIndex();
        const Sample midSample = GetSampleFromPose(*context.m_framePose, invRootTransform);
        SetFeatureData(context.m_featureMatrix, frameIndex, midSampleIndex, midSample);

        // Sample the past.
        const float pastFrameTimeDelta = m_pastTimeRange / static_cast<float>(m_numPastSamples - 1);
        currentFrame.SamplePose(&samplePose->GetPose());
        for (size_t i = 0; i < m_numPastSamples; ++i)
        {
            // Increase the sample index by one as the zeroth past/future sample actually needs one time delta time difference to the
            // current frame.
            const float sampleTimeOffset = (i + 1) * pastFrameTimeDelta * (-1.0f);
            currentFrame.SamplePose(&nextSamplePose->GetPose(), sampleTimeOffset);

            const Sample sample = GetSampleFromPose(samplePose->GetPose(), invRootTransform);
            const size_t sampleIndex = CalcPastFrameIndex(i);
            SetFeatureData(context.m_featureMatrix, frameIndex, sampleIndex, sample);

            *samplePose = *nextSamplePose;
        }

        // Sample into the future.
        const float futureFrameTimeDelta = m_futureTimeRange / (float)(m_numFutureSamples - 1);
        currentFrame.SamplePose(&samplePose->GetPose());
        for (size_t i = 0; i < m_numFutureSamples; ++i)
        {
            // Sample the value at the future sample point.
            const float sampleTimeOffset = (i + 1) * futureFrameTimeDelta;
            currentFrame.SamplePose(&nextSamplePose->GetPose(), sampleTimeOffset);

            const Sample sample = GetSampleFromPose(samplePose->GetPose(), invRootTransform);
            const size_t sampleIndex = CalcFutureFrameIndex(i);
            SetFeatureData(context.m_featureMatrix, frameIndex, sampleIndex, sample);

            *samplePose = *nextSamplePose;
        }

        context.m_posePool.FreePose(samplePose);
        context.m_posePool.FreePose(nextSamplePose);
    }

    ///////////////////////////////////////////////////////////////////////////

    void FeatureTrajectory::FillQueryVector(QueryVector& queryVector, const QueryVectorContext& context)
    {
        const Transform invRootTransform = context.m_currentPose.GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();

        auto FillControlPoints =
            [this, &queryVector, &invRootTransform](
                const AZStd::vector<TrajectoryQuery::ControlPoint>& controlPoints, const AZStd::function<size_t(size_t)>& CalcFrameIndex)
        {
            const size_t numControlPoints = controlPoints.size();
            for (size_t i = 0; i < numControlPoints; ++i)
            {
                TrajectoryQuery::ControlPoint controlPoint = controlPoints[i];
                controlPoint.m_position =
                    invRootTransform.TransformPoint(controlPoint.m_position); // Convert so it is relative to where we are and pointing to.
                controlPoint.m_facingDirection = invRootTransform.TransformVector(controlPoint.m_facingDirection);

                const size_t sampleIndex = CalcFrameIndex(i);
                const size_t sampleColumnStart = m_featureColumnOffset + sampleIndex * Sample::s_componentsPerSample;

                queryVector.SetVector2(AZ::Vector2(controlPoint.m_position), sampleColumnStart + 0); // m_position
                queryVector.SetVector2(AZ::Vector2(controlPoint.m_facingDirection), sampleColumnStart + 2); // m_facingDirection
            }
        };

        AZ_Assert(
            context.m_trajectoryQuery.GetFutureControlPoints().size() == m_numFutureSamples,
            "Number of future control points from the trajectory query does not match the one from the trajectory feature.");
        AZ_Assert(
            context.m_trajectoryQuery.GetPastControlPoints().size() == m_numPastSamples,
            "Number of past control points from the trajectory query does not match the one from the trajectory feature");

        FillControlPoints(
            context.m_trajectoryQuery.GetPastControlPoints(),
            [this](size_t frameIndex)
            {
                return CalcPastFrameIndex(frameIndex);
            });
        FillControlPoints(
            context.m_trajectoryQuery.GetFutureControlPoints(),
            [this](size_t frameIndex)
            {
                return CalcFutureFrameIndex(frameIndex);
            });
    }

    ///////////////////////////////////////////////////////////////////////////

    float FeatureTrajectory::CalculateCost(
        const FeatureMatrix& featureMatrix,
        size_t frameIndex,
        size_t numControlPoints,
        const SplineToFeatureMatrixIndex& splineToFeatureMatrixIndex,
        const FrameCostContext& context) const
    {
        float cost = 0.0f;
        AZ::Vector2 lastControlPoint, lastSamplePos;

        for (size_t i = 0; i < numControlPoints; ++i)
        {
            const Sample sample = GetFeatureData(featureMatrix, frameIndex, splineToFeatureMatrixIndex(i));
            const AZ::Vector2& samplePos = sample.m_position;

            const size_t sampleColumnStart = m_featureColumnOffset + splineToFeatureMatrixIndex(i) * Sample::s_componentsPerSample;
            const AZ::Vector2 controlPointPos = context.m_queryVector.GetVector2(sampleColumnStart + 0);
            const AZ::Vector2 controlPointFacingDirRelativeSpace = context.m_queryVector.GetVector2(sampleColumnStart + 2);

            if (i != 0)
            {
                const AZ::Vector2 controlPointDelta = controlPointPos - lastControlPoint;
                const AZ::Vector2 sampleDelta = samplePos - lastSamplePos;

                const float posDistance = (samplePos - controlPointPos).GetLength();
                const float posDeltaDistance = (controlPointDelta - sampleDelta).GetLength();

                // The facing direction from the control point (trajectory query) is in world space while the facing direction from the
                // sample of this trajectory feature is in relative-to-frame-root-joint space.
                const float facingDirectionCost =
                    GetNormalizedDirectionDifference(sample.m_facingDirection, controlPointFacingDirRelativeSpace);

                // As we got two different costs for the position, double the cost of the facing direction to equal out the influence.
                cost += CalcResidual(posDistance) + CalcResidual(posDeltaDistance) + CalcResidual(facingDirectionCost) * 2.0f;
            }

            lastControlPoint = controlPointPos;
            lastSamplePos = samplePos;
        }

        return cost;
    }

    float FeatureTrajectory::CalculateFutureFrameCost(size_t frameIndex, const FrameCostContext& context) const
    {
        return CalculateCost(
            context.m_featureMatrix, frameIndex, m_numFutureSamples,
            [this](size_t frameIndex)
            {
                return CalcFutureFrameIndex(frameIndex);
            },
            context);
    }

    float FeatureTrajectory::CalculatePastFrameCost(size_t frameIndex, const FrameCostContext& context) const
    {
        return CalculateCost(
            context.m_featureMatrix, frameIndex, m_numPastSamples,
            [this](size_t frameIndex)
            {
                return CalcPastFrameIndex(frameIndex);
            },
            context);
    }

    ///////////////////////////////////////////////////////////////////////////

    size_t FeatureTrajectory::CalcNumSamplesPerFrame() const
    {
        return m_numPastSamples + 1 + m_numFutureSamples;
    }

    size_t FeatureTrajectory::CalcMidFrameIndex() const
    {
        return m_numPastSamples;
    }

    size_t FeatureTrajectory::CalcPastFrameIndex(size_t historyFrameIndex) const
    {
        AZ_Assert(historyFrameIndex < m_numPastSamples, "The history frame index is out of range");
        return m_numPastSamples - historyFrameIndex - 1;
    }

    size_t FeatureTrajectory::CalcFutureFrameIndex(size_t futureFrameIndex) const
    {
        AZ_Assert(futureFrameIndex < m_numFutureSamples, "The future frame index is out of range");
        return CalcMidFrameIndex() + 1 + futureFrameIndex;
    }

    ///////////////////////////////////////////////////////////////////////////

    void FeatureTrajectory::SetFacingAxis(const Axis axis)
    {
        m_facingAxis = axis;
        UpdateFacingAxis();
    }

    void FeatureTrajectory::UpdateFacingAxis()
    {
        switch (m_facingAxis)
        {
        case Axis::X:
            {
                m_facingAxisDir = AZ::Vector3::CreateAxisX();
                break;
            }
        case Axis::Y:
            {
                m_facingAxisDir = AZ::Vector3::CreateAxisY();
                break;
            }
        case Axis::X_NEGATIVE:
            {
                m_facingAxisDir = -AZ::Vector3::CreateAxisX();
                break;
            }
        case Axis::Y_NEGATIVE:
            {
                m_facingAxisDir = -AZ::Vector3::CreateAxisY();
                break;
            }
        default:
            {
                AZ_Assert(false, "Facing direction axis unknown.");
            }
        }
    }

    void FeatureTrajectory::SetPastTimeRange(float timeInSeconds)
    {
        m_pastTimeRange = timeInSeconds;
    }

    void FeatureTrajectory::SetFutureTimeRange(float timeInSeconds)
    {
        m_futureTimeRange = timeInSeconds;
    }

    void FeatureTrajectory::SetNumPastSamplesPerFrame(size_t numHistorySamples)
    {
        m_numPastSamples = numHistorySamples;
    }

    void FeatureTrajectory::SetNumFutureSamplesPerFrame(size_t numFutureSamples)
    {
        m_numFutureSamples = numFutureSamples;
    }

    void FeatureTrajectory::DebugDrawFacingDirection(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::Vector3& positionWorldSpace,
        const AZ::Vector3& facingDirectionWorldSpace)
    {
        const float length = 0.2f;
        const float radius = 0.01f;

        const AZ::Vector3 facingDirectionTarget = positionWorldSpace + facingDirectionWorldSpace * length;
        debugDisplay.DrawSolidCylinder(
            /*center=*/(facingDirectionTarget + positionWorldSpace) * 0.5f,
            /*direction=*/facingDirectionWorldSpace, radius,
            /*height=*/length,
            /*drawShaded=*/false);
    }

    void FeatureTrajectory::DebugDrawFacingDirection(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const Transform& worldSpaceTransform,
        const Sample& sample,
        const AZ::Vector3& samplePosWorldSpace) const
    {
        const AZ::Vector3 facingDirectionWorldSpace =
            worldSpaceTransform.TransformVector(AZ::Vector3(sample.m_facingDirection)).GetNormalizedSafe();
        DebugDrawFacingDirection(debugDisplay, samplePosWorldSpace, facingDirectionWorldSpace);
    }

    void FeatureTrajectory::DebugDrawTrajectory(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const FeatureMatrix& featureMatrix,
        const FeatureMatrixTransformer* featureTransformer,
        size_t frameIndex,
        const Transform& worldSpaceTransform,
        const AZ::Color& color,
        size_t numSamples,
        const SplineToFeatureMatrixIndex& splineToFeatureMatrixIndex) const
    {
        if (frameIndex == InvalidIndex)
        {
            return;
        }

        constexpr float markerSize = 0.02f;

        debugDisplay.DepthTestOff();
        debugDisplay.SetColor(color);

        Sample nextSample;
        AZ::Vector3 nextSamplePos;
        for (size_t i = 0; i < numSamples - 1; ++i)
        {
            Sample currentSample =
                GetFeatureDataInverseTransformed(featureMatrix, featureTransformer, frameIndex, splineToFeatureMatrixIndex(i));
            nextSample = GetFeatureDataInverseTransformed(featureMatrix, featureTransformer, frameIndex, splineToFeatureMatrixIndex(i + 1));

            const AZ::Vector3 currentSamplePos = worldSpaceTransform.TransformPoint(AZ::Vector3(currentSample.m_position));
            nextSamplePos = worldSpaceTransform.TransformPoint(AZ::Vector3(nextSample.m_position));

            // Line between current and next sample.
            debugDisplay.DrawSolidCylinder(
                /*center=*/(nextSamplePos + currentSamplePos) * 0.5f,
                /*direction=*/(nextSamplePos - currentSamplePos).GetNormalizedSafe(),
                /*radius=*/0.0025f,
                /*height=*/(nextSamplePos - currentSamplePos).GetLength(),
                /*drawShaded=*/false);

            // Sphere at the sample position and a cylinder to indicate the facing direction.
            debugDisplay.DrawBall(currentSamplePos, markerSize, /*drawShaded=*/false);
            DebugDrawFacingDirection(debugDisplay, worldSpaceTransform, currentSample, currentSamplePos);
        }

        debugDisplay.DrawBall(nextSamplePos, markerSize, /*drawShaded=*/false);
        DebugDrawFacingDirection(debugDisplay, worldSpaceTransform, nextSample, nextSamplePos);
    }

    void FeatureTrajectory::DebugDraw(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const Pose& currentPose,
        const FeatureMatrix& featureMatrix,
        const FeatureMatrixTransformer* featureTransformer,
        size_t frameIndex)
    {
        const Transform transform = currentPose.GetWorldSpaceTransform(m_jointIndex);

        DebugDrawTrajectory(
            debugDisplay, featureMatrix, featureTransformer, frameIndex, transform, m_debugColor, m_numPastSamples,
            AZStd::bind(&FeatureTrajectory::CalcPastFrameIndex, this, AZStd::placeholders::_1));

        DebugDrawTrajectory(
            debugDisplay, featureMatrix, featureTransformer, frameIndex, transform, m_debugColor, m_numFutureSamples,
            AZStd::bind(&FeatureTrajectory::CalcFutureFrameIndex, this, AZStd::placeholders::_1));
    }

    AZ::Crc32 FeatureTrajectory::GetCostFactorVisibility() const
    {
        return AZ::Edit::PropertyVisibility::Hide;
    }

    void FeatureTrajectory::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<FeatureTrajectory, Feature>()
            ->Version(2)
            ->Field("pastTimeRange", &FeatureTrajectory::m_pastTimeRange)
            ->Field("numPastSamples", &FeatureTrajectory::m_numPastSamples)
            ->Field("pastCostFactor", &FeatureTrajectory::m_pastCostFactor)
            ->Field("futureTimeRange", &FeatureTrajectory::m_futureTimeRange)
            ->Field("numFutureSamples", &FeatureTrajectory::m_numFutureSamples)
            ->Field("futureCostFactor", &FeatureTrajectory::m_futureCostFactor)
            ->Field("facingAxis", &FeatureTrajectory::m_facingAxis);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<FeatureTrajectory>("FeatureTrajectory", "Matches the joint past and future trajectory.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &FeatureTrajectory::m_numPastSamples, "Past Samples",
                "The number of samples stored per frame for the past trajectory. [Default = 4 samples to represent the trajectory history]")
            ->Attribute(AZ::Edit::Attributes::Min, 1)
            ->Attribute(AZ::Edit::Attributes::Max, 100)
            ->Attribute(AZ::Edit::Attributes::Step, 1)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &FeatureTrajectory::m_pastTimeRange, "Past Time Range",
                "The time window the samples are distributed along for the trajectory history. [Default = 0.7 seconds]")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &FeatureTrajectory::m_pastCostFactor, "Past Cost Factor",
                "The cost factor is multiplied with the cost from the trajectory history and can be used to change the influence of the "
                "trajectory history match in the motion matching search.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &FeatureTrajectory::m_numFutureSamples, "Future Samples",
                "The number of samples stored per frame for the future trajectory. [Default = 6 samples to represent the future "
                "trajectory]")
            ->Attribute(AZ::Edit::Attributes::Min, 1)
            ->Attribute(AZ::Edit::Attributes::Max, 100)
            ->Attribute(AZ::Edit::Attributes::Step, 1)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &FeatureTrajectory::m_futureTimeRange, "Future Time Range",
                "The time window the samples are distributed along for the future trajectory. [Default = 1.2 seconds]")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &FeatureTrajectory::m_futureCostFactor, "Future Cost Factor",
                "The cost factor is multiplied with the cost from the future trajectory and can be used to change the influence of the "
                "future trajectory match in the motion matching search.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox, &FeatureTrajectory::m_facingAxis, "Facing Axis",
                "The facing direction of the character. Which axis of the joint transform is facing forward? [Default = Looking into "
                "Y-axis direction]")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FeatureTrajectory::UpdateFacingAxis)
            ->EnumAttribute(Axis::X, "X")
            ->EnumAttribute(Axis::X_NEGATIVE, "-X")
            ->EnumAttribute(Axis::Y, "Y")
            ->EnumAttribute(Axis::Y_NEGATIVE, "-Y");
    }

    size_t FeatureTrajectory::GetNumDimensions() const
    {
        return CalcNumSamplesPerFrame() * Sample::s_componentsPerSample;
    }

    AZStd::string FeatureTrajectory::GetDimensionName(size_t index) const
    {
        AZStd::string result = "Trajectory";

        const int sampleIndex = aznumeric_cast<int>(index) / aznumeric_cast<int>(Sample::s_componentsPerSample);
        const int componentIndex = index % Sample::s_componentsPerSample;
        const int midSampleIndex = aznumeric_cast<int>(CalcMidFrameIndex());

        if (sampleIndex == midSampleIndex)
        {
            result += ".Current.";
        }
        else if (sampleIndex < midSampleIndex)
        {
            result += AZStd::string::format(".Past%i.", sampleIndex - static_cast<int>(m_numPastSamples));
        }
        else
        {
            result += AZStd::string::format(".Future%i.", sampleIndex - static_cast<int>(m_numPastSamples));
        }

        switch (componentIndex)
        {
        case 0:
            {
                result += "PosX";
                break;
            }
        case 1:
            {
                result += "PosY";
                break;
            }
        case 2:
            {
                result += "FacingDirX";
                break;
            }
        case 3:
            {
                result += "FacingDirY";
                break;
            }
        default:
            {
                result += Feature::GetDimensionName(index);
            }
        }

        return result;
    }

    FeatureTrajectory::Sample FeatureTrajectory::GetFeatureData(
        const FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex) const
    {
        const size_t columnOffset = m_featureColumnOffset + sampleIndex * Sample::s_componentsPerSample;
        return {
            /*.m_position           =*/featureMatrix.GetVector2(frameIndex, columnOffset + 0),
            /*.m_facingDirection    =*/featureMatrix.GetVector2(frameIndex, columnOffset + 2),
        };
    }

    FeatureTrajectory::Sample FeatureTrajectory::GetFeatureDataInverseTransformed(
        const FeatureMatrix& featureMatrix, const FeatureMatrixTransformer* featureTransformer, size_t frameIndex, size_t sampleIndex) const
    {
        Sample sample = GetFeatureData(featureMatrix, frameIndex, sampleIndex);
        if (featureTransformer)
        {
            const size_t columnOffset = m_featureColumnOffset + sampleIndex * Sample::s_componentsPerSample;

            sample.m_position = featureTransformer->InverseTransform(sample.m_position, columnOffset + 0);
            sample.m_facingDirection = featureTransformer->InverseTransform(sample.m_facingDirection, columnOffset + 2);
        }
        return sample;
    }

    void FeatureTrajectory::SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex, const Sample& sample)
    {
        const size_t columnOffset = m_featureColumnOffset + sampleIndex * Sample::s_componentsPerSample;
        featureMatrix.SetVector2(frameIndex, columnOffset + 0, sample.m_position);
        featureMatrix.SetVector2(frameIndex, columnOffset + 2, sample.m_facingDirection);
    }
} // namespace EMotionFX::MotionMatching
