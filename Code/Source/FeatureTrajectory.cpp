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
#include <EMotionFX/Source/MotionInstance.h>
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
        }

        AZ::Vector3 FeatureTrajectory::CalculateFacingDirectionWorldSpace(const Pose& pose, Axis facingAxis, size_t jointIndex) const
        {
            AZ::Vector3 facingDirection = AZ::Vector3::CreateZero();
            facingDirection.SetElement(static_cast<int>(facingAxis), 1.0f);
            return pose.GetWorldSpaceTransform(jointIndex).TransformVector(facingDirection).GetNormalizedSafe();
        }
/*
        // Calculate the angle difference between the direction we're heading in and where the root is pointing towards.
        // This allows us to detect when we're strafing for example, when the angle is 90 degrees between these two vectors.
        float TrajectoryFrameData::CalculateFacingAngle(const Transform& invBaseTransform, const Pose& pose, const AZ::Vector3& velocityDirection) const
        {
            const AZ::Vector3 facingDirectionWorldSpace = CalculateWorldSpaceDirection(pose, m_facingAxis, m_nodeIndex);
            const AZ::Vector3 relativeFacingDirection = invBaseTransform.TransformVector(facingDirectionWorldSpace).GetNormalizedSafeExact();;
            return velocityDirection.Dot(relativeFacingDirection);
        }
*/

        FeatureTrajectory::Sample FeatureTrajectory::GetSampleFromPose(const Pose& pose, const Transform& invRootTransform) const
        {
            // Extract the position.
            const AZ::Vector2 position = AZ::Vector2(invRootTransform.TransformPoint(pose.GetWorldSpaceTransform(m_nodeIndex).m_position));

            // Calculate the facing direction.
            //facingDirectionWorldSpace = CalculateFacingDirectionWorldSpace(samplePose->GetPose(), m_facingAxis, m_nodeIndex);
            const AZ::Vector2 facingDirection = AZ::Vector2::CreateZero();

            return { position, facingDirection };
        }

        void FeatureTrajectory::ExtractFeatureValues(const ExtractFrameContext& context)
        {
            ActorInstance* actorInstance = context.m_motionInstance->GetActorInstance();
            Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
            AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool();
            AnimGraphPose* samplePose = posePool.RequestPose(actorInstance);
            AnimGraphPose* nextSamplePose = posePool.RequestPose(actorInstance);

            const size_t frameIndex = context.m_frameIndex;
            const Frame& currentFrame = context.m_data->GetFrame(context.m_frameIndex);
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();

            const size_t midSampleIndex = CalcMidFrameDataIndex();
            const Sample midSample = GetSampleFromPose(*context.m_pose, invRootTransform);
            SetFeatureData(context.m_featureMatrix, frameIndex, midSampleIndex, midSample);

            // Sample the past.
            Motion* sourceMotion = currentFrame.GetSourceMotion();
            const float pastFrameTimeDelta = m_pastTimeRange / static_cast<float>(m_numPastSamples - 1);
            SamplePose(currentFrame.GetSampleTime(), bindPose, sourceMotion, context.m_motionInstance, &samplePose->GetPose());
            for (size_t i = 0; i < m_numPastSamples; ++i)
            {
                const size_t sampleIndex = CalcPastFrameDataIndex(i);

                // Increase the sample index by one as the zeroth past/future sample actually needs one time delta time difference to the current frame.
                const float sampleTime = AZ::GetMax(0.0f, currentFrame.GetSampleTime() - (i+1) * pastFrameTimeDelta);
                SamplePose(sampleTime, bindPose, sourceMotion, context.m_motionInstance, &nextSamplePose->GetPose());

                const Sample sample = GetSampleFromPose(samplePose->GetPose(), invRootTransform);
                SetFeatureData(context.m_featureMatrix, frameIndex, sampleIndex, sample);

                *samplePose = *nextSamplePose;
            }

            // Sample into the future.
            const float futureFrameTimeDelta = m_futureTimeRange / (float)(m_numFutureSamples - 1);
            SamplePose(currentFrame.GetSampleTime(), bindPose, sourceMotion, context.m_motionInstance, &samplePose->GetPose());
            for (size_t i = 0; i < m_numFutureSamples; ++i)
            {
                const size_t sampleIndex = CalcFutureFrameDataIndex(i);

                // Sample the value at the future sample point.
                const float sampleTime = AZ::GetMin(currentFrame.GetSampleTime() + (i+1) * futureFrameTimeDelta, sourceMotion->GetDuration());
                SamplePose(sampleTime, bindPose, sourceMotion, context.m_motionInstance, &nextSamplePose->GetPose());

                const Sample sample = GetSampleFromPose(samplePose->GetPose(), invRootTransform);
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

        void FeatureTrajectory::DebugDrawTrajectory(AZ::RPI::AuxGeomDrawPtr& drawQueue,
            [[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw,
            BehaviorInstance* behaviorInstance,
            size_t frameIndex,
            const Transform& transform,
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

            AZ::Vector3 nextSamplePos;
            for (size_t i = 0; i < numSamples - 1; ++i)
            {
                const Sample currentSample = GetFeatureData(featureMatrix, frameIndex, splineToFeatureMatrixIndex(i));
                const Sample nextSample = GetFeatureData(featureMatrix, frameIndex, splineToFeatureMatrixIndex(i + 1));

                const AZ::Vector3 currentSamplePos = transform.TransformPoint(AZ::Vector3(currentSample.m_position));
                nextSamplePos = transform.TransformPoint(AZ::Vector3(nextSample.m_position));

                drawQueue->DrawCylinder(/*center=*/(nextSamplePos + currentSamplePos) * 0.5f,
                    /*direction=*/(nextSamplePos - currentSamplePos).GetNormalizedSafe(),
                    /*radius=*/0.0025f,
                    /*height=*/(nextSamplePos - currentSamplePos).GetLength(),
                    color,
                    AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                    AZ::RPI::AuxGeomDraw::DepthTest::Off);

                drawQueue->DrawSphere(currentSamplePos,
                    markerSize,
                    color,
                    AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                    AZ::RPI::AuxGeomDraw::DepthTest::Off);
            }

            drawQueue->DrawSphere(nextSamplePos,
                markerSize, color,
                AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                AZ::RPI::AuxGeomDraw::DepthTest::Off);
        }

        void FeatureTrajectory::DebugDraw(AZ::RPI::AuxGeomDrawPtr& drawQueue,
            EMotionFX::DebugDraw::ActorInstanceData& draw,
            BehaviorInstance* behaviorInstance,
            size_t frameIndex)
        {
            const ActorInstance* actorInstance = behaviorInstance->GetActorInstance();
            const Transform transform = actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(m_nodeIndex);

            DebugDrawTrajectory(drawQueue, draw, behaviorInstance, frameIndex, transform,
                m_debugColor, m_numPastSamples, AZStd::bind(&FeatureTrajectory::CalcPastFrameDataIndex, this, AZStd::placeholders::_1));

            DebugDrawTrajectory(drawQueue, draw, behaviorInstance, frameIndex, transform,
                m_debugColor, m_numFutureSamples, AZStd::bind(&FeatureTrajectory::CalcFutureFrameDataIndex, this, AZStd::placeholders::_1));
        }

        size_t FeatureTrajectory::CalcMidFrameDataIndex() const
        {
            return m_numPastSamples;
        }

        size_t FeatureTrajectory::CalcPastFrameDataIndex(size_t historyFrameIndex) const
        {
            AZ_Assert(historyFrameIndex < m_numPastSamples, "The history frame index is out of range");
            return m_numPastSamples - historyFrameIndex - 1;
        }

        size_t FeatureTrajectory::CalcFutureFrameDataIndex(size_t futureFrameIndex) const
        {
            AZ_Assert(futureFrameIndex < m_numFutureSamples, "The future frame index is out of range");
            return CalcMidFrameDataIndex() + 1 + futureFrameIndex;
        }
        /*
        float FeatureTrajectory::CalculateDirectionCost(size_t frameIndex, const FrameCostContext& context) const
        {
            const AZ::Vector3 frameFacingDirection = m_samples[CalcMidFrameDataIndex(frameIndex)].m_facingDirection;
            const float dotResult = context.m_facingDirectionRelative.Dot(frameFacingDirection);
            return 2.0f - (1.0f - dotResult);
        }
        */

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

                    cost += posDistance + posDeltaDistance;
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
            return CalculateCost(context.m_featureMatrix, frameIndex, invRootTransform, context.m_trajectoryQuery->GetFutureControlPoints(), AZStd::bind(&FeatureTrajectory::CalcFutureFrameDataIndex, this, AZStd::placeholders::_1));
        }

        float FeatureTrajectory::CalculatePastFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            AZ_Assert(context.m_trajectoryQuery->GetPastControlPoints().size() == m_numPastSamples, "Number of past control points does not match trajecotry frame data number of past points.");
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            return CalculateCost(context.m_featureMatrix, frameIndex, invRootTransform, context.m_trajectoryQuery->GetPastControlPoints(), AZStd::bind(&FeatureTrajectory::CalcPastFrameDataIndex, this, AZStd::placeholders::_1));
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

            editContext->Class<FeatureTrajectory>("TrajectoryFrameData", "Joint past and future trajectory data.")
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
            const int midSampleIndex = aznumeric_cast<int>(CalcMidFrameDataIndex());

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
