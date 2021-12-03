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

#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/EventManager.h>
#include <Behavior.h>
#include <BehaviorInstance.h>
#include <FrameDatabase.h>
#include <FeatureTrajectory.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Source/TransformData.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(FeatureTrajectory, MotionMatchAllocator, 0)

        FeatureTrajectory::FeatureTrajectory()
            : Feature()
        {
        }

        bool FeatureTrajectory::Init(const InitSettings& settings)
        {
            MCORE_UNUSED(settings);

            if (m_nodeIndex == MCORE_INVALIDINDEX32)
            {
                return false;
            }

            return true;
        }

        void FeatureTrajectory::SetNodeIndex(size_t nodeIndex)
        {
            m_nodeIndex = nodeIndex;
        }

        size_t FeatureTrajectory::CalcNumSamplesPerFrame() const
        {
            return m_numPastSamples + 1 + m_numFutureSamples;
        }

        void FeatureTrajectory::SetFacingAxis(const Axis axis)
        {
            m_facingAxis = axis;

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

        AZ::Vector2 FeatureTrajectory::CalculateFacingDirection(const Pose& pose, const Transform& invRootTransform) const
        {
            // Get the facing direction of the given joint for the given pose in animation world space.
            // The given pose is either sampled into the relative past or future based on the frame we want to extract the feature for.
            const AZ::Vector3 facingDirAnimationWorldSpace = pose.GetWorldSpaceTransform(m_nodeIndex).TransformVector(m_facingAxisDir);

            // The invRootTransform is the inverse of the world space transform for the given joint at the frame we want to extract the feature for.
            // The result after this will be the facing direction relative to the frame we want to extract the feature for.
            const AZ::Vector3 facingDirection = invRootTransform.TransformVector(facingDirAnimationWorldSpace);

            // Project to the ground plane and make sure the direction is normalized.
            return AZ::Vector2(facingDirection).GetNormalizedSafe();
        }

        FeatureTrajectory::Sample FeatureTrajectory::GetSampleFromPose(const Pose& pose, const Transform& invRootTransform) const
        {
            // Position of the root joint in the model space relative to frame to extract.
            const AZ::Vector2 position = AZ::Vector2(invRootTransform.TransformPoint(pose.GetWorldSpaceTransform(m_nodeIndex).m_position));

            // Calculate the facing direction.
            const AZ::Vector2 facingDirection = CalculateFacingDirection(pose, invRootTransform);

            return { position, facingDirection };
        }

        void FeatureTrajectory::ExtractFeatureValues(const ExtractFrameContext& context)
        {
            const ActorInstance* actorInstance = context.m_actorInstance;
            AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool();
            AnimGraphPose* samplePose = posePool.RequestPose(actorInstance);
            AnimGraphPose* nextSamplePose = posePool.RequestPose(actorInstance);

            const size_t frameIndex = context.m_frameIndex;
            const Frame& currentFrame = context.m_data->GetFrame(context.m_frameIndex);

            // Inverse of the root transform for the frame that we want to extract data from.
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();

            const size_t midSampleIndex = CalcMidFrameIndex();
            const Sample midSample = GetSampleFromPose(*context.m_pose, invRootTransform);
            SetFeatureData(context.m_featureMatrix, frameIndex, midSampleIndex, midSample);

            // Sample the past.
            const float pastFrameTimeDelta = m_pastTimeRange / static_cast<float>(m_numPastSamples - 1);
            currentFrame.SamplePose(&samplePose->GetPose());
            for (size_t i = 0; i < m_numPastSamples; ++i)
            {
                // Increase the sample index by one as the zeroth past/future sample actually needs one time delta time difference to the current frame.
                const float sampleTimeOffset = (i+1) * pastFrameTimeDelta * (-1.0f);
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
                const float sampleTimeOffset = (i+1) * futureFrameTimeDelta;
                currentFrame.SamplePose(&nextSamplePose->GetPose(), sampleTimeOffset);

                const Sample sample = GetSampleFromPose(samplePose->GetPose(), invRootTransform);
                const size_t sampleIndex = CalcFutureFrameIndex(i);
                SetFeatureData(context.m_featureMatrix, frameIndex, sampleIndex, sample);

                *samplePose = *nextSamplePose;
            }

            posePool.FreePose(samplePose);
            posePool.FreePose(nextSamplePose);
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

        void FeatureTrajectory::DebugDrawFacingDirection(AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::Vector3& positionWorldSpace,
            const AZ::Vector3& facingDirectionWorldSpace)
        {
            const float length = 0.2f;
            const float radius = 0.01f;

            const AZ::Vector3 facingDirectionTarget = positionWorldSpace + facingDirectionWorldSpace * length;
            debugDisplay.DrawSolidCylinder(/*center=*/(facingDirectionTarget + positionWorldSpace) * 0.5f,
                /*direction=*/facingDirectionWorldSpace,
                radius,
                /*height=*/length,
                /*drawShaded=*/false);
        }

        void FeatureTrajectory::DebugDrawFacingDirection(AzFramework::DebugDisplayRequests& debugDisplay,
            const Transform& worldSpaceTransform,
            const Sample& sample,
            const AZ::Vector3& samplePosWorldSpace) const
        {
            const AZ::Vector3 facingDirectionWorldSpace = worldSpaceTransform.TransformVector(AZ::Vector3(sample.m_facingDirection)).GetNormalizedSafe();
            DebugDrawFacingDirection(debugDisplay, samplePosWorldSpace, facingDirectionWorldSpace);
        }

        void FeatureTrajectory::DebugDrawTrajectory(AzFramework::DebugDisplayRequests& debugDisplay,
            BehaviorInstance* behaviorInstance,
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
            const FeatureMatrix& featureMatrix = behaviorInstance->GetBehavior()->GetFeatures().GetFeatureMatrix();

            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(color);

            Sample nextSample;
            AZ::Vector3 nextSamplePos;
            for (size_t i = 0; i < numSamples - 1; ++i)
            {
                const Sample currentSample = GetFeatureData(featureMatrix, frameIndex, splineToFeatureMatrixIndex(i));
                nextSample = GetFeatureData(featureMatrix, frameIndex, splineToFeatureMatrixIndex(i + 1));

                const AZ::Vector3 currentSamplePos = worldSpaceTransform.TransformPoint(AZ::Vector3(currentSample.m_position));
                nextSamplePos = worldSpaceTransform.TransformPoint(AZ::Vector3(nextSample.m_position));

                // Line between current and next sample.
                debugDisplay.DrawSolidCylinder(/*center=*/(nextSamplePos + currentSamplePos) * 0.5f,
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

        void FeatureTrajectory::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
            BehaviorInstance* behaviorInstance,
            size_t frameIndex)
        {
            const ActorInstance* actorInstance = behaviorInstance->GetActorInstance();
            const Transform transform = actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(m_nodeIndex);

            DebugDrawTrajectory(debugDisplay, behaviorInstance, frameIndex, transform,
                m_debugColor, m_numPastSamples, AZStd::bind(&FeatureTrajectory::CalcPastFrameIndex, this, AZStd::placeholders::_1));

            DebugDrawTrajectory(debugDisplay, behaviorInstance, frameIndex, transform,
                m_debugColor, m_numFutureSamples, AZStd::bind(&FeatureTrajectory::CalcFutureFrameIndex, this, AZStd::placeholders::_1));
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

        float FeatureTrajectory::CalculateCost(const FeatureMatrix& featureMatrix,
            size_t frameIndex,
            const Transform& invRootTransform,
            const AZStd::vector<TrajectoryQuery::ControlPoint>& controlPoints,
            const SplineToFeatureMatrixIndex& splineToFeatureMatrixIndex) const
        {
            float cost = 0.0f;
            AZ::Vector2 lastControlPoint, lastSamplePos;

            for (size_t i = 0; i < controlPoints.size(); ++i)
            {
                const TrajectoryQuery::ControlPoint& controlPoint = controlPoints[i];
                const Sample sample = GetFeatureData(featureMatrix, frameIndex, splineToFeatureMatrixIndex(i));
                const AZ::Vector2& samplePos = sample.m_position;
                const AZ::Vector2 controlPointPos = AZ::Vector2(invRootTransform.TransformPoint(controlPoint.m_position)); // Convert so it is relative to where we are and pointing to.

                if (i != 0)
                {
                    const AZ::Vector2 controlPointDelta = controlPointPos - lastControlPoint;
                    const AZ::Vector2 sampleDelta = samplePos - lastSamplePos;

                    const float posDistance = (samplePos - controlPointPos).GetLength();
                    const float posDeltaDistance = (controlPointDelta - sampleDelta).GetLength();

                    // The facing direction from the control point (trajectory query) is in world space while the facing direction from the
                    // sample of this trajectory feature is in relative-to-frame-root-joint space.
                    const AZ::Vector2 controlPointFacingDirRelativeSpace = AZ::Vector2(invRootTransform.TransformVector(controlPoint.m_facingDirection));
                    const float facingDirectionCost = GetNormalizedDirectionDifference(sample.m_facingDirection,
                        controlPointFacingDirRelativeSpace);

                    cost += posDistance + posDeltaDistance + facingDirectionCost;
                }

                lastControlPoint = controlPointPos;
                lastSamplePos = samplePos;
            }

            return cost;
        }

        float FeatureTrajectory::CalculateFutureFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            AZ_Assert(context.m_trajectoryQuery->GetFutureControlPoints().size() == m_numFutureSamples, "Number of future control points does not match trajecotry frame data number of future points.");
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            return CalculateCost(context.m_featureMatrix, frameIndex, invRootTransform, context.m_trajectoryQuery->GetFutureControlPoints(), AZStd::bind(&FeatureTrajectory::CalcFutureFrameIndex, this, AZStd::placeholders::_1));
        }

        float FeatureTrajectory::CalculatePastFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            AZ_Assert(context.m_trajectoryQuery->GetPastControlPoints().size() == m_numPastSamples, "Number of past control points does not match trajecotry frame data number of past points.");
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            return CalculateCost(context.m_featureMatrix, frameIndex, invRootTransform, context.m_trajectoryQuery->GetPastControlPoints(), AZStd::bind(&FeatureTrajectory::CalcPastFrameIndex, this, AZStd::placeholders::_1));
        }

        void FeatureTrajectory::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<FeatureTrajectory, Feature>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<FeatureTrajectory>("FeatureTrajectory", "Joint past and future trajectory data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }

        size_t FeatureTrajectory::GetNumDimensions() const
        {
            return CalcNumSamplesPerFrame() * Sample::s_componentsPerSample;
        }

        AZStd::string FeatureTrajectory::GetDimensionName(size_t index, Skeleton* skeleton) const
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
                result += AZStd::string::format(".Past%i.", sampleIndex - m_numPastSamples);
            }
            else
            {
                result += AZStd::string::format(".Future%i.", sampleIndex - m_numPastSamples);
            }

            switch (componentIndex)
            {
                case 0: { result += "PosX"; break; }
                case 1: { result += "PosY"; break; }
                case 2: { result += "FacingDirX"; break; }
                case 3: { result += "FacingDirY"; break; }
                default: { result += Feature::GetDimensionName(index, skeleton); }
            }

            return result;
        }

        FeatureTrajectory::Sample FeatureTrajectory::GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex) const
        {
            const size_t columnOffset = m_featureColumnOffset + sampleIndex * Sample::s_componentsPerSample;
            return {
                /*.m_position           =*/ featureMatrix.GetVector2(frameIndex, columnOffset + 0),
                /*.m_facingDirection    =*/ featureMatrix.GetVector2(frameIndex, columnOffset + 2),
            };
        }

        void FeatureTrajectory::SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex, const Sample& sample)
        {
            const size_t columnOffset = m_featureColumnOffset + sampleIndex * Sample::s_componentsPerSample;
            featureMatrix.SetVector2(frameIndex, columnOffset + 0, sample.m_position);
            featureMatrix.SetVector2(frameIndex, columnOffset + 2, sample.m_facingDirection);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
