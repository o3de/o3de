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
        void FeatureTrajectory::ExtractFeatureValues(const ExtractFrameContext& context)
        {
            // Get a temp sample pose.
            ActorInstance* actorInstance = context.m_motionInstance->GetActorInstance();
            Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
            AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool();
            AnimGraphPose* samplePose = posePool.RequestPose(actorInstance);
            AnimGraphPose* nextSamplePose = posePool.RequestPose(actorInstance);

            const size_t frameIndex = context.m_frameIndex;
            const Frame& currentFrame = context.m_data->GetFrame(context.m_frameIndex);
            const size_t midSampleIndex = CalcMidFrameDataIndex();
            Sample midSample;

            // Get the current frame values.
            const AZ::Vector3& currentPosition = context.m_pose->GetWorldSpaceTransform(m_nodeIndex).m_position;
            Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            midSample.m_position = invRootTransform.TransformPoint(currentPosition);

            // Calculate the linear velocity of the sample point in the middle.
            CalculateVelocity(m_nodeIndex, context.m_pose, context.m_nextPose, context.m_timeDelta, midSample.m_direction, midSample.m_speed);
            midSample.m_direction = invRootTransform.TransformVector(midSample.m_direction);

            // Calculate the facing direction.
            AZ::Vector3 facingDirectionWorldSpace = CalculateFacingDirectionWorldSpace(*context.m_pose, m_facingAxis, m_nodeIndex);
            midSample.m_facingDirection = invRootTransform.TransformVector(facingDirectionWorldSpace);

            // Sample the past.
            Motion* sourceMotion = currentFrame.GetSourceMotion();
            const float pastFrameTime = m_pastTimeRange / static_cast<float>(m_numPastSamples - 1);
            const float lastHistorySampleTime = currentFrame.GetSampleTime() - pastFrameTime;
            float sampleTime = AZ::GetMax(0.0f, lastHistorySampleTime);
            SamplePose(sampleTime, bindPose, sourceMotion, context.m_motionInstance, &samplePose->GetPose());
            for (size_t i = 0; i < m_numPastSamples; ++i)
            {
                const size_t sampleIndex = m_numPastSamples - i - 1;
                Sample sample;

                sampleTime = AZ::GetMax(0.0f, lastHistorySampleTime - i * pastFrameTime);
                SamplePose(sampleTime, bindPose, sourceMotion, context.m_motionInstance, &nextSamplePose->GetPose());

                // Extract the position.
                sample.m_position = invRootTransform.TransformPoint(samplePose->GetPose().GetWorldSpaceTransform(m_nodeIndex).m_position);

                // Calculate the velocity
                CalculateVelocity(m_nodeIndex, &samplePose->GetPose(), &nextSamplePose->GetPose(), pastFrameTime, sample.m_direction, sample.m_speed);
                const Transform invSampleTransform = samplePose->GetPose().GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
                sample.m_direction = invSampleTransform.TransformVector(sample.m_direction);

                // Calculate the facing direction.
                facingDirectionWorldSpace = CalculateFacingDirectionWorldSpace(samplePose->GetPose(), m_facingAxis, m_nodeIndex);
                midSample.m_facingDirection = invSampleTransform.TransformVector(facingDirectionWorldSpace);

                *samplePose = *nextSamplePose;

                SetFeatureData(context.m_featureMatrix, frameIndex, sampleIndex, sample);
            }

            // Sample into the future.
            const float futureFrameTime = m_futureTimeRange / (float)(m_numFutureSamples - 1);
            sampleTime = AZ::GetMin(currentFrame.GetSampleTime(), sourceMotion->GetDuration());
            SamplePose(sampleTime, bindPose, sourceMotion, context.m_motionInstance, &samplePose->GetPose());
            for (size_t i = 0; i < m_numFutureSamples; ++i)
            {
                const size_t sampleIndex = CalcFutureFrameDataIndex(i);
                Sample sample;

                // Sample the value at the future sample point.
                sampleTime = AZ::GetMin(currentFrame.GetSampleTime() + i * futureFrameTime, sourceMotion->GetDuration());
                SamplePose(sampleTime, bindPose, sourceMotion, context.m_motionInstance, &nextSamplePose->GetPose());

                // Extract the position.
                sample.m_position = invRootTransform.TransformPoint(samplePose->GetPose().GetWorldSpaceTransform(m_nodeIndex).m_position);

                // Calculate the velocity
                CalculateVelocity(m_nodeIndex, &samplePose->GetPose(), &nextSamplePose->GetPose(), futureFrameTime, sample.m_direction, sample.m_speed);
                const Transform invSampleTransform = samplePose->GetPose().GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
                sample.m_direction = invSampleTransform.TransformVector(sample.m_direction);

                // Calculate the facing direction.
                facingDirectionWorldSpace = CalculateFacingDirectionWorldSpace(samplePose->GetPose(), m_facingAxis, m_nodeIndex);
                midSample.m_facingDirection = invSampleTransform.TransformVector(facingDirectionWorldSpace);

                *samplePose = *nextSamplePose;

                SetFeatureData(context.m_featureMatrix, frameIndex, sampleIndex, sample);
            }

            posePool.FreePose(samplePose);
            posePool.FreePose(nextSamplePose);

            SetFeatureData(context.m_featureMatrix, frameIndex, midSampleIndex, midSample);
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

        void FeatureTrajectory::DebugDrawFutureTrajectory(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance, size_t frameIndex, const Transform& transform, const AZ::Color& color)
        {
            if (frameIndex == InvalidIndex)
            {
                return;
            }

            const FeatureMatrix& featureMatrix = behaviorInstance->GetBehavior()->GetFeatures().GetFeatureMatrix();

            for (size_t i = 0; i < m_numFutureSamples - 1; ++i)
            {
                const Sample firstSample = GetFeatureData(featureMatrix, frameIndex, CalcFutureFrameDataIndex(i));
                const Sample nextSample = GetFeatureData(featureMatrix, frameIndex, CalcFutureFrameDataIndex(i + 1));

                draw.DrawLine(
                    transform.TransformPoint(firstSample.m_position),                    
                    transform.TransformPoint(nextSample.m_position),
                    color);

                //draw.DrawLine(
                //    transform.TransformPoint(firstSample.m_position),                    
                //    transform.TransformPoint(firstSample.m_position) + transform.TransformVector(firstSample.m_direction) * 0.2f,
                //    color);
            }
        }

        void FeatureTrajectory::DebugDrawPastTrajectory(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance, size_t frameIndex, const Transform& transform, const AZ::Color& color)
        {
            if (frameIndex == InvalidIndex)
            {
                return;
            }

            const FeatureMatrix& featureMatrix = behaviorInstance->GetBehavior()->GetFeatures().GetFeatureMatrix();

            for (size_t i = 0; i < m_numPastSamples - 1; ++i)
            {
                const Sample firstSample = GetFeatureData(featureMatrix, frameIndex, CalcPastFrameDataIndex(i));
                const Sample nextSample = GetFeatureData(featureMatrix, frameIndex, CalcPastFrameDataIndex(i + 1));

                draw.DrawLine(
                    transform.TransformPoint(firstSample.m_position),
                    transform.TransformPoint(nextSample.m_position),
                    color);
            }
        }

        void FeatureTrajectory::DebugDraw([[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw, [[maybe_unused]] BehaviorInstance* behaviorInstance, [[maybe_unused]] size_t frameIndex)
        {
            /*
            if (m_samples.empty())
            {
                return;
            }

            const size_t numFrames = m_data->GetNumFrames();

            static float offset = 0.0f;
            offset += 0.15f * GetEMotionFX().GetGlobalSimulationSpeed();
            size_t frameNr = (size_t)offset;
            if (frameNr >= numFrames - 1)
            {
                offset = 0.0f;
            }

            const float s = 0.1f;
            const Sample& frameSample = m_samples[CalcMidFrameDataIndex(frameNr)];
            draw.DrawLine(frameSample.m_position - AZ::Vector3(s, 0.0f, 0.0f), frameSample.m_position + AZ::Vector3(s, 0.0f, 0.0f), AZ::Colors::Yellow);
            draw.DrawLine(frameSample.m_position - AZ::Vector3(0.0f, s, 0.0f), frameSample.m_position + AZ::Vector3(0.0f, s, 0.0f), AZ::Colors::Yellow);

            for (size_t i = 0; i < m_numFutureSamples - 1; ++i)
            {
                const Sample& firstSample = m_samples[CalcFutureFrameDataIndex(frameNr, i)];
                //const Sample& nextSample = m_samples[CalcFutureFrameDataIndex(frameNr, i + 1)];
                //draw.DrawLine(firstSample.m_position, nextSample.m_position, m_debugColor);
                draw.DrawLine(firstSample.m_position, firstSample.m_position + (firstSample.m_direction * firstSample.m_speed) * 0.1f, AZ::Colors::Green);
            }

            for (size_t i = 0; i < m_numPastSamples - 1; ++i)
            {
                const Sample& firstSample = m_samples[CalcPastFrameDataIndex(frameNr, i)];
                //const Sample& nextSample = m_samples[CalcPastFrameDataIndex(frameNr, i + 1)];
                //draw.DrawLine(firstSample.m_position, nextSample.m_position, m_debugColor);
                draw.DrawLine(firstSample.m_position, firstSample.m_position + (firstSample.m_direction * firstSample.m_speed) * 0.1f, AZ::Colors::Red);
            }*/
        }

        size_t FeatureTrajectory::CalcMidFrameDataIndex() const
        {
            return m_numPastSamples;
        }

        size_t FeatureTrajectory::CalcPastFrameDataIndex(size_t historyFrameIndex) const
        {
            AZ_Assert(historyFrameIndex < m_numPastSamples, "The history frame index is out of range");
            return historyFrameIndex;
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
        float FeatureTrajectory::CalculateFutureFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            //AZ_Assert(context.m_controlSpline->m_futureSplinePoints.size() == m_numFutureSamples, "Spline number of future points does not match trajecotry frame data number of future points.");
            //AZ_Assert(context.m_controlSpline->m_pastSplinePoints.size() == m_numPastSamples, "Spline number of past points does not match trajecotry frame data number of past points.");

            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();

            float futureCost = 0.0f;
            for (size_t i = 0; i < context.m_controlSpline->m_futureSplinePoints.size(); ++i)
            {
                const BehaviorInstance::SplinePoint& splinePoint = context.m_controlSpline->m_futureSplinePoints[i];
                const Sample futureSample = GetFeatureData(context.m_featureMatrix, frameIndex, CalcFutureFrameDataIndex(i));
                const AZ::Vector3 splinePointPos = invRootTransform.TransformPoint(splinePoint.m_position); // Convert so it is relative to where we are and pointing to.
                const float posDistance = (futureSample.m_position - splinePointPos).GetLength();
                futureCost += posDistance;
            }

            return futureCost;
        }

        float FeatureTrajectory::CalculatePastFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            //AZ_Assert(context.m_controlSpline->m_futureSplinePoints.size() == m_numFutureSamples, "Spline number of future points does not match trajecotry frame data number of future points.");
            //AZ_Assert(context.m_controlSpline->m_pastSplinePoints.size() == m_numPastSamples, "Spline number of past points does not match trajecotry frame data number of past points.");

            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();

            float pastCost = 0.0f;
            for (size_t i = 0; i < context.m_controlSpline->m_pastSplinePoints.size(); ++i)
            {
                const size_t splinePointIndex = context.m_controlSpline->m_pastSplinePoints.size() - 1 - i;
                const BehaviorInstance::SplinePoint& splinePoint = context.m_controlSpline->m_pastSplinePoints[splinePointIndex];
                const Sample sample = GetFeatureData(context.m_featureMatrix, frameIndex, CalcPastFrameDataIndex(i));
                const AZ::Vector3 splinePointPos = invRootTransform.TransformPoint(splinePoint.m_position); // Convert so it is relative to where we are and pointing to.
                const float posDistance = (sample.m_position - splinePointPos).GetLength();
                pastCost += posDistance;
            }

            return pastCost;
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
                case 2: { result += "PosZ"; break; }
                case 3: { result += "DirX"; break; }
                case 4: { result += "DirY"; break; }
                case 5: { result += "DirZ"; break; }
                case 6: { result += "FacingDirX"; break; }
                case 7: { result += "FacingDirY"; break; }
                case 8: { result += "FacingDirZ"; break; }
                case 9: { result += "Speed"; break; }
                default: { result += Feature::GetDimensionName(index, skeleton); }
            }

            return result;
        }

        FeatureTrajectory::Sample FeatureTrajectory::GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex) const
        {
            Sample result;
            const size_t columnOffset = m_featureColumnOffset + sampleIndex * Sample::s_componentsPerSample;
            result.m_position           = featureMatrix.GetVector3(frameIndex, columnOffset + 0);
            result.m_direction          = featureMatrix.GetVector3(frameIndex, columnOffset + 3);
            result.m_facingDirection    = featureMatrix.GetVector3(frameIndex, columnOffset + 6);
            result.m_speed              = featureMatrix(frameIndex,            columnOffset + 9);
            return result;
        }

        void FeatureTrajectory::SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex, const Sample& sample)
        {
            const size_t columnOffset = m_featureColumnOffset + sampleIndex * Sample::s_componentsPerSample;
            featureMatrix.SetVector3(frameIndex, columnOffset + 0, sample.m_position);
            featureMatrix.SetVector3(frameIndex, columnOffset + 3 , sample.m_direction);
            featureMatrix.SetVector3(frameIndex, columnOffset + 6, sample.m_facingDirection);
            featureMatrix(frameIndex,            columnOffset + 9) = sample.m_speed;
        }
    } // namespace MotionMatching
} // namespace EMotionFX
