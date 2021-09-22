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

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <Behavior.h>
#include <BehaviorInstance.h>
#include <FeatureDirection.h>
#include <LocomotionBehavior.h>
#include <FeaturePosition.h>
#include <FeatureTrajectory.h>
#include <FeatureVelocity.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Skeleton.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(LocomotionBehavior, MotionMatchAllocator, 0)

        // Static members.
        AZ::TypeId LocomotionBehavior::s_rootTrajectoryId = AZ::TypeId("{61369BE4-A158-4FC6-8C45-267BB369FE3C}");
        AZ::TypeId LocomotionBehavior::s_leftFootPositionsId = AZ::TypeId("{20792202-8D0C-4F8E-B0FE-F979A39DFC2B}");
        AZ::TypeId LocomotionBehavior::s_leftFootVelocitiesId = AZ::TypeId("{C89AE5EB-953E-4448-89D9-995E87A80BCE}");
        AZ::TypeId LocomotionBehavior::s_rightFootPositionsId = AZ::TypeId("{D81C95CE-FDD8-4000-A37C-8B40887457C3}");
        AZ::TypeId LocomotionBehavior::s_rightFootVelocitiesId = AZ::TypeId("{0C2296AB-DFF5-4D5D-8242-49923650E05B}");
        AZ::TypeId LocomotionBehavior::s_rootDirectionId = AZ::TypeId("{7065E949-FFAF-4108-94E2-0BD429A5CD8F}");

        LocomotionBehavior::LocomotionBehavior()
            : Behavior()
        {
        }

        bool LocomotionBehavior::RegisterParameters(const InitSettings& settings)
        {
            AZ_UNUSED(settings);

            // TODO: register stuff here.

            return true;
        }

        bool LocomotionBehavior::RegisterFrameDatas(const InitSettings& settings)
        {
            // TODO: use the editor context to get the right value instead.

            //----------------------------------------------------------------------------------------------------------
            // Register the motion extraction trajectory (includes history and future)
            const Node* rootNode = settings.m_actorInstance->GetActor()->GetMotionExtractionNode();//GetActor()->GetSkeleton()->FindNodeByNameNoCase("root");
            m_rootNodeIndex = rootNode ? rootNode->GetNodeIndex() : 0;
            m_rootTrajectoryData = aznew FeatureTrajectory();
            m_rootTrajectoryData->SetNodeIndex(m_rootNodeIndex);
            m_rootTrajectoryData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_rootTrajectoryData->SetId(s_rootTrajectoryId);
            m_rootTrajectoryData->SetDebugDrawColor(AZ::Colors::Magenta);
            m_rootTrajectoryData->SetNumFutureSamplesPerFrame(6);
            m_rootTrajectoryData->SetNumPastSamplesPerFrame(6);
            m_rootTrajectoryData->SetFutureTimeRange(1.0f);
            m_rootTrajectoryData->SetPastTimeRange(1.0f);
            m_rootTrajectoryData->SetDebugDrawEnabled(false);
            m_rootTrajectoryData->SetIncludeInKdTree(false);
            m_features.RegisterFeature(m_rootTrajectoryData);
            //----------------------------------------------------------------------------------------------------------

            //----------------------------------------------------------------------------------------------------------
            // Grab the left foot positions
            const Node* leftFootNode = settings.m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase("L_foot_JNT");
            m_leftFootNodeIndex = leftFootNode ? leftFootNode->GetNodeIndex() : MCORE_INVALIDINDEX32;
            if (!leftFootNode)
            {
                return false;
            }
            m_leftFootPositionData = aznew FeaturePosition();
            m_leftFootPositionData->SetNodeIndex(m_leftFootNodeIndex);
            m_leftFootPositionData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_leftFootPositionData->SetId(s_leftFootPositionsId);
            m_leftFootPositionData->SetDebugDrawColor(AZ::Colors::Red);
            m_leftFootPositionData->SetDebugDrawEnabled(false);
            m_leftFootPositionData->SetIncludeInKdTree(true);
            m_features.RegisterFeature(m_leftFootPositionData);
            //----------------------------------------------------------------------------------------------------------

            //----------------------------------------------------------------------------------------------------------
            // Grab the right foot positions
            const Node* rightFootNode = settings.m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase("R_foot_JNT");
            m_rightFootNodeIndex = rightFootNode ? rightFootNode->GetNodeIndex() : MCORE_INVALIDINDEX32;
            if (!rightFootNode)
            {
                return false;
            }
            m_rightFootPositionData = aznew FeaturePosition();
            m_rightFootPositionData->SetNodeIndex(m_rightFootNodeIndex);
            m_rightFootPositionData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_rightFootPositionData->SetId(s_rightFootPositionsId);
            m_rightFootPositionData->SetDebugDrawColor(AZ::Colors::Green);
            m_rightFootPositionData->SetDebugDrawEnabled(false);
            m_rightFootPositionData->SetIncludeInKdTree(true);
            m_features.RegisterFeature(m_rightFootPositionData);
            //----------------------------------------------------------------------------------------------------------

            //----------------------------------------------------------------------------------------------------------
            // Grab the left foot velocities
            m_leftFootVelocityData = aznew FeatureVelocity();
            m_leftFootVelocityData->SetNodeIndex(m_leftFootNodeIndex);
            m_leftFootVelocityData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_leftFootVelocityData->SetId(s_leftFootVelocitiesId);
            m_leftFootVelocityData->SetDebugDrawColor(AZ::Colors::Teal);
            m_leftFootVelocityData->SetDebugDrawEnabled(true);
            m_leftFootVelocityData->SetIncludeInKdTree(true);
            m_features.RegisterFeature(m_leftFootVelocityData);
            //----------------------------------------------------------------------------------------------------------

            //----------------------------------------------------------------------------------------------------------
            // Grab the right foot velocities
            m_rightFootVelocityData = aznew FeatureVelocity();
            m_rightFootVelocityData->SetNodeIndex(m_rightFootNodeIndex);
            m_rightFootVelocityData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_rightFootVelocityData->SetId(s_rightFootVelocitiesId);
            m_rightFootVelocityData->SetDebugDrawColor(AZ::Colors::Cyan);
            m_rightFootVelocityData->SetDebugDrawEnabled(true);
            m_rightFootVelocityData->SetIncludeInKdTree(true);
            m_features.RegisterFeature(m_rightFootVelocityData);
            //----------------------------------------------------------------------------------------------------------

            // Root direction.
            m_rootDirectionData = aznew FeatureDirection();
            m_rootDirectionData->SetNodeIndex(m_rootNodeIndex);
            m_rootDirectionData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_rootDirectionData->SetId(s_rootDirectionId);
            m_rootDirectionData->SetDebugDrawColor(AZ::Colors::Yellow);
            m_rootDirectionData->SetDebugDrawEnabled(false);
            m_rootDirectionData->SetIncludeInKdTree(false);
            m_features.RegisterFeature(m_rootDirectionData);
            //----------------------------------------------------------------------------------------------------------

            return true;
        }

        void LocomotionBehavior::DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance)
        {
            AZ_PROFILE_SCOPE(Animation, "LocomotionBehavior::DebugDraw");

            Behavior::DebugDraw(draw, behaviorInstance);
            DebugDrawControlSpline(draw, behaviorInstance);

            // Get the lowest cost frame index from the last search. As we're searching the feature database with a much lower
            // frequency and sample the animation onwards from this, the resulting frame index does not represent the current
            // feature values from the shown pose.
            const size_t curFrameIndex = behaviorInstance->GetLowestCostFrameIndex();
            if (curFrameIndex == InvalidIndex)
            {
                return;
            }

            // Find the frame index in the frame database that belongs to the currently used pose.
            MotionInstance* motionInstance = behaviorInstance->GetMotionInstance();
            const size_t currentFrame = m_data.FindFrameIndex(motionInstance->GetMotion(), motionInstance->GetCurrentTime());
            if (currentFrame != InvalidIndex)
            {
                m_features.DebugDraw(draw, behaviorInstance, currentFrame);
            }

            //// Draw the root direction
            //const size_t frameIndex = behaviorInstance->GetLowestCostFrameIndex();
            //const AZ::Vector3 direction = m_rootDirectionData->GetDirection(frameIndex).GetNormalizedSafeExact();
            //draw.DrawLine(actorInstance->GetWorldSpaceTransform().mPosition,
            //              actorInstance->GetWorldSpaceTransform().mPosition + direction, AZ::Colors::LightCyan);

            const ActorInstance* actorInstance = behaviorInstance->GetActorInstance();
            const Transform transform = actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(m_rootNodeIndex);
            m_rootTrajectoryData->DebugDrawFutureTrajectory(draw, curFrameIndex, transform, AZ::Colors::LawnGreen);
            m_rootTrajectoryData->DebugDrawPastTrajectory(draw, curFrameIndex, transform, AZ::Colors::Red);
        }

        void LocomotionBehavior::DebugDrawControlSpline(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance)
        {
            const BehaviorInstance::ControlSpline& spline = behaviorInstance->GetControlSpline();
            if (spline.m_futureSplinePoints.size() > 1)
            {
                for (size_t i = 0; i < spline.m_futureSplinePoints.size() - 1; ++i)
                {
                    const AZ::Vector3& posA = spline.m_futureSplinePoints[i].m_position;
                    const AZ::Vector3& posB = spline.m_futureSplinePoints[i + 1].m_position;
                    draw.DrawLine(posA, posB, AZ::Colors::Magenta);
                }

                for (size_t i = 0; i < spline.m_futureSplinePoints.size(); ++i)
                {
                    draw.DrawMarker(spline.m_futureSplinePoints[i].m_position, AZ::Colors::White, 0.02f);
                }
            }

            if (spline.m_pastSplinePoints.size() > 1)
            {
                for (size_t i = 0; i < spline.m_pastSplinePoints.size() - 1; ++i)
                {
                    const AZ::Vector3& posA = spline.m_pastSplinePoints[i].m_position;
                    const AZ::Vector3& posB = spline.m_pastSplinePoints[i + 1].m_position;
                    draw.DrawLine(posA, posB, AZ::Colors::Orange);
                }

                for (size_t i = 0; i < spline.m_pastSplinePoints.size(); ++i)
                {
                    draw.DrawMarker(spline.m_pastSplinePoints[i].m_position, AZ::Colors::Yellow, 0.02f);
                }
            }
        }

        void LocomotionBehavior::OnSettingsChanged()
        {
        }

        size_t LocomotionBehavior::FindLowestCostFrameIndex(BehaviorInstance* behaviorInstance, const Pose& inputPose, [[maybe_unused]] const Pose& previousPose, size_t currentFrameIndex, [[maybe_unused]] float timeDelta)
        {
            AZ_PROFILE_SCOPE(Animation, "LocomotionBehavior::FindLowestCostFrameIndex");

            // Prepare our current pose data.
            //const BehaviorInstance::ControlSpline& controlSpline = behaviorInstance->GetControlSpline();
            const Frame& currentFrame = m_data.GetFrame(currentFrameIndex);
            MotionInstance* motionInstance = behaviorInstance->GetMotionInstance();
            FeaturePosition::FrameCostContext leftFootPosContext;
            FeaturePosition::FrameCostContext rightFootPosContext;
            FeatureTrajectory::FrameCostContext rootTrajectoryContext;
            FeatureVelocity::FrameCostContext leftFootVelocityContext;
            FeatureVelocity::FrameCostContext rightFootVelocityContext;
            //DirectionFrameData::FrameCostContext rootDirectionContext;
            Feature::CalculateVelocity(m_leftFootNodeIndex, m_rootNodeIndex, motionInstance, leftFootVelocityContext.m_direction, leftFootVelocityContext.m_speed);
            Feature::CalculateVelocity(m_rightFootNodeIndex, m_rootNodeIndex, motionInstance, rightFootVelocityContext.m_direction, rightFootVelocityContext.m_speed); // TODO: group this with left foot for faster performance
            leftFootPosContext.m_pose = &inputPose;
            rightFootPosContext.m_pose = &inputPose;
            rootTrajectoryContext.m_pose = &inputPose;
            rootTrajectoryContext.m_facingDirectionRelative = AZ::Vector3(0.0f, 1.0f, 0.0f);
            //rootDirectionContext.m_pose = &inputPose;
            rootTrajectoryContext.m_controlSpline = &behaviorInstance->GetControlSpline();

            //-----------------------
            // Build the frame floats.
            // Remember that the order is very important. It has to be the order in which the frame datas are registered, and only the ones included in the kdTree.
            size_t startOffset = 0;
            AZStd::vector<float>& frameFloats = behaviorInstance->GetFrameFloats();

            // Left foot position.
            m_leftFootPositionData->FillFrameFloats(startOffset, frameFloats, leftFootPosContext);
            startOffset += m_leftFootPositionData->GetNumDimensionsForKdTree();

            // Right foot position.
            m_rightFootPositionData->FillFrameFloats(startOffset, frameFloats, rightFootPosContext);
            startOffset += m_rightFootPositionData->GetNumDimensionsForKdTree();

            // Left foot velocity.
            m_leftFootVelocityData->FillFrameFloats(startOffset, frameFloats, leftFootVelocityContext);
            startOffset += m_leftFootVelocityData->GetNumDimensionsForKdTree();

            // Right foot velocity.
            m_rightFootVelocityData->FillFrameFloats(startOffset, frameFloats, rightFootVelocityContext);
            startOffset += m_leftFootVelocityData->GetNumDimensionsForKdTree();

            AZ_Assert(startOffset == frameFloats.size(), "Frame float vector is not the expected size.");
            //-----------------------

            // Find our nearest frames.
            behaviorInstance->UpdateNearestFrames();

            // Find the actual best frame.
            float minCost = FLT_MAX;
            size_t minCostFrameIndex = 0;
            for (const size_t frameIndex : behaviorInstance->GetNearestFrames())
            {
                const Frame& frame = m_data.GetFrame(frameIndex);

                if (frame.GetSampleTime() >= frame.GetSourceMotion()->GetDuration() - 1.0f)
                {
                    continue;
                }

                float totalCost =
                    m_tweakFactors.m_footPositionFactor * m_leftFootPositionData->CalculateFrameCost(frameIndex, leftFootPosContext) +
                    m_tweakFactors.m_footPositionFactor * m_rightFootPositionData->CalculateFrameCost(frameIndex, rightFootPosContext) +
                    m_tweakFactors.m_rootFutureFactor * m_rootTrajectoryData->CalculateFutureFrameCost(frameIndex, rootTrajectoryContext) +
                    m_tweakFactors.m_rootPastFactor * m_rootTrajectoryData->CalculatePastFrameCost(frameIndex, rootTrajectoryContext) +
                    m_tweakFactors.m_rootDirectionFactor * m_rootTrajectoryData->CalculateDirectionCost(frameIndex, rootTrajectoryContext) +
                    m_tweakFactors.m_footVelocityFactor * m_leftFootVelocityData->CalculateFrameCost(frameIndex, leftFootVelocityContext) +
                    m_tweakFactors.m_footVelocityFactor * m_rightFootVelocityData->CalculateFrameCost(frameIndex, rightFootVelocityContext);// +
                    //m_tweakFactors.m_rootDirectionFactor * m_rootDirectionData->CalculateFrameCost(frameIndex, rootDirectionContext);

                if (frame.GetSourceMotion() != currentFrame.GetSourceMotion())
                {
                    totalCost *= m_tweakFactors.m_differentMotionFactor;
                }

                // Track the minimum cost value and frame.
                if (totalCost < minCost)
                {
                    minCost = totalCost;
                    minCostFrameIndex = frameIndex;
                }
            }

            //AZ_Printf("EMotionFX", "Frame %d = %f    %f/%f   %d nearest",
            //    minCostFrameIndex,
            //    minCost,
            //    m_data.GetFrame(minCostFrameIndex).GetSampleTime(),
            //    m_data.GetFrame(minCostFrameIndex).GetSourceMotion()->GetMaxTime(),
            //    behaviorInstance->GetNearestFrames().size()
            //);
            return minCostFrameIndex;
        }

/*
        size_t LocomotionBehavior::FindLowestCostFrameIndex(BehaviorInstance* behaviorInstance, const Pose& inputPose, const Pose& previousPose, size_t currentFrameIndex, float timeDelta)
        {
            const BehaviorInstance::ControlSpline& controlSpline = behaviorInstance->GetControlSpline();
            const Frame& currentFrame = m_data.GetFrame(currentFrameIndex);
            MotionInstance* motionInstance = behaviorInstance->GetMotionInstance();

            float minCost = FLT_MAX;
            float maxCost = -1.0f;
            size_t minCostFrameIndex = 0;

            PositionFrameData::FrameCostContext leftFootPosContext;
            PositionFrameData::FrameCostContext rightFootPosContext;
            TrajectoryFrameData::FrameCostContext rootTrajectoryContext;
            VelocityFrameData::FrameCostContext leftFootVelocityContext;
            VelocityFrameData::FrameCostContext rightFootVelocityContext;
            //DirectionFrameData::FrameCostContext rootDirectionContext;
            FrameData::CalculateVelocity(m_leftFootNodeIndex, m_rootNodeIndex, motionInstance, leftFootVelocityContext.m_direction, leftFootVelocityContext.m_speed);
            FrameData::CalculateVelocity(m_rightFootNodeIndex, m_rootNodeIndex, motionInstance, rightFootVelocityContext.m_direction, rightFootVelocityContext.m_speed); // TODO: group this with left foot for faster performance
            leftFootPosContext.m_pose = &inputPose;
            rightFootPosContext.m_pose = &inputPose;
            rootTrajectoryContext.m_pose = &inputPose;
            rootTrajectoryContext.m_facingDirectionRelative = AZ::Vector3(0.0f, 1.0f, 0.0f);
            //rootDirectionContext.m_pose = &inputPose;
            rootTrajectoryContext.m_controlSpline = &behaviorInstance->GetControlSpline();

            for (Frame& frame : m_data.GetFrames())
            {
                if (frame.GetSampleTime() >= frame.GetSourceMotion()->GetMaxTime() - 1.0f)
                {
                    continue;
                }

                const size_t frameIndex = frame.GetFrameIndex();
                float totalCost =
                    m_tweakFactors.m_footPositionFactor * m_leftFootPositionData->CalculateFrameCost(frameIndex, leftFootPosContext) +
                    m_tweakFactors.m_footPositionFactor * m_rightFootPositionData->CalculateFrameCost(frameIndex, rightFootPosContext) +
                    m_tweakFactors.m_rootFutureFactor * m_rootTrajectoryData->CalculateFutureFrameCost(frameIndex, rootTrajectoryContext) +
                    m_tweakFactors.m_rootPastFactor * m_rootTrajectoryData->CalculatePastFrameCost(frameIndex, rootTrajectoryContext) +
                    m_tweakFactors.m_rootDirectionFactor * m_rootTrajectoryData->CalculateDirectionCost(frameIndex, rootTrajectoryContext) +
                    m_tweakFactors.m_footVelocityFactor * m_leftFootVelocityData->CalculateFrameCost(frameIndex, leftFootVelocityContext) +
                    m_tweakFactors.m_footVelocityFactor * m_rightFootVelocityData->CalculateFrameCost(frameIndex, rightFootVelocityContext);// +
                    //m_tweakFactors.m_rootDirectionFactor * m_rootDirectionData->CalculateFrameCost(frameIndex, rootDirectionContext);

                if (frame.GetSourceMotion() != currentFrame.GetSourceMotion())
                {
                    totalCost *= m_tweakFactors.m_differentMotionFactor;
                }

                frame.SetCost(totalCost);

                // Track the minimum cost value and frame.
                if (totalCost < minCost)
                {
                    minCost = totalCost;
                    minCostFrameIndex = frameIndex;
                }

                if (totalCost > maxCost)
                {
                    maxCost = totalCost;
                }
            }

            // Normalize frame costs
            for (Frame& frame : m_data.GetFrames())
            {
                frame.SetCost(frame.GetCost() / maxCost);
            }

            //AZ_Printf("EMotionFX", "Frame %d = %f    %f/%f  max=%f", minCostFrameIndex, minCost, m_data.GetFrame(minCostFrameIndex).GetSampleTime(), m_data.GetFrame(minCostFrameIndex).GetSourceMotion()->GetMaxTime(), maxCost);
            return minCostFrameIndex;
        }
*/
        AZ::Vector3 SampleFunction1(float offset, float radius, float phase)
        {
            AZ::Vector3 displacement = AZ::Vector3::CreateZero();
            displacement.SetX(radius * sinf(phase + offset) + radius * 0.75f * cosf(phase * 2.0f + offset * 2.0f));
            displacement.SetY(radius * cosf(phase + offset));
            return displacement;
        }

        AZ::Vector3 SampleFunction2(float offset, float radius, float phase)
        {
            AZ::Vector3 displacement = AZ::Vector3::CreateZero();
            displacement.SetX(radius * sinf(phase + offset) );
            displacement.SetY(cosf(phase + offset));
            return displacement;
        }

        AZ::Vector3 SampleFunction3(float offset, float radius, float phase)
        {
            AZ::Vector3 displacement = AZ::Vector3::CreateZero();
            const float rad = radius * cosf(radius + phase*0.2f);
            displacement.SetX(rad * sinf(phase + offset));
            displacement.SetY(rad * cosf(phase + offset));
            return displacement;
        }

        AZ::Vector3 SampleFunction4(float offset, float radius, float phase)
        {
            AZ::Vector3 displacement = AZ::Vector3::CreateZero();
            displacement.SetX(radius * sinf(phase + offset));
            displacement.SetY(radius*2.0f * cosf(phase + offset));
            return displacement;
        }

        AZ::Vector3 SampleFunction(EControlSplineMode mode, float offset, float pathRadius, float phase, [[maybe_unused]] const AZ::Vector3& targetPos)
        {
            switch (mode)
            {
                case MODE_ONE:
                    return SampleFunction1(offset, pathRadius, phase);

                case MODE_TWO:
                    return SampleFunction2(offset, pathRadius, phase);

                case MODE_THREE:
                    return SampleFunction3(offset, pathRadius, phase);

                case MODE_FOUR:
                    return SampleFunction4(offset, pathRadius, phase);
            }

            return SampleFunction1(offset, pathRadius, phase);
        }

        void LocomotionBehavior::BuildControlSpline(BehaviorInstance* behaviorInstance, EControlSplineMode mode, const AZ::Vector3& targetPos, const TrajectoryHistory& trajectoryHistory, float timeDelta, float pathRadius, float pathSpeed)
        {
            const ActorInstance* actorInstance = behaviorInstance->GetActorInstance();

            BehaviorInstance::ControlSpline& controlSpline = behaviorInstance->GetControlSpline();
            const AZ::u32 splinePointCounts = 6;
            controlSpline.m_futureSplinePoints.resize(splinePointCounts);
            controlSpline.m_pastSplinePoints.resize(splinePointCounts);

            if (mode == MODE_TARGETDRIVEN)
            {
                const AZ::Vector3 curPos = actorInstance->GetWorldSpaceTransform().m_position;
                if (curPos.IsClose(targetPos, 0.1f))
                {
                    for (size_t i = 0; i < splinePointCounts; ++i)
                    {
                        controlSpline.m_futureSplinePoints[i].m_position = curPos;
                    }
                }
                else
                {
                    // NOTE: Improve it by using a curve to the target.
                    for (size_t i = 0; i < splinePointCounts; ++i)
                    {
                        const float sampleTime = static_cast<float>(i) / (splinePointCounts - 1);
                        controlSpline.m_futureSplinePoints[i].m_position = curPos.Lerp(targetPos, sampleTime);
                    }
                }
            }
            else
            {
                static float phase = 0.0f;
                phase += timeDelta * pathSpeed;
                AZ::Vector3 base = SampleFunction(mode, 0.0f, pathRadius, phase, targetPos);
                for (size_t i = 0; i < splinePointCounts; ++i)
                {
                    const float offset = i * 0.1f;
                    const AZ::Vector3 curSample = SampleFunction(mode, offset, pathRadius, phase, targetPos);
                    AZ::Vector3 displacement = curSample - base;
                    controlSpline.m_futureSplinePoints[i].m_position = actorInstance->GetWorldSpaceTransform().m_position + displacement;
                    //controlSpline.m_futureSplinePoints[i].m_position = curSample;
                }
            }

            // Provide the trajectory history.
            for (size_t i = 0; i < splinePointCounts; ++i)
            {
                const float sampleTime = i / static_cast<float>(splinePointCounts - 1);
                const AZ::Vector3 position = trajectoryHistory.SampleNormalized(sampleTime);
                controlSpline.m_pastSplinePoints[i].m_position = position;
            }
        }

        void LocomotionBehavior::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<LocomotionBehavior, Behavior>()
                ->Version(1)
                ->Field("leftFootNodeIndex", &LocomotionBehavior::m_leftFootNodeIndex)
                ->Field("rightFootNodeIndex", &LocomotionBehavior::m_rightFootNodeIndex)
                ->Field("rootNodeIndex", &LocomotionBehavior::m_rootNodeIndex);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<LocomotionBehavior>("LocomotionBehavior", "Locomotion behavior for motion matching")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &LocomotionBehavior::m_rootNodeIndex, "Root node", "The root node.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &LocomotionBehavior::OnSettingsChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &LocomotionBehavior::m_leftFootNodeIndex, "Left foot node", "The left foot node.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &LocomotionBehavior::OnSettingsChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &LocomotionBehavior::m_rightFootNodeIndex, "Right foot node", "The right foot node.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &LocomotionBehavior::OnSettingsChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
