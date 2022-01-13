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
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <MotionMatchingConfig.h>
#include <MotionMatchingInstance.h>
#include <LocomotionConfig.h>
#include <FeaturePosition.h>
#include <FeatureTrajectory.h>
#include <FeatureVelocity.h>
#include <KdTree.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Skeleton.h>
#include <ImGuiMonitorBus.h>
#include <PoseDataJointVelocities.h>

#include <AzCore/Debug/Timer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(LocomotionConfig, MotionMatchAllocator, 0)

        LocomotionConfig::LocomotionConfig()
            : MotionMatchingConfig()
        {
        }

        bool LocomotionConfig::RegisterFeatures(const InitSettings& settings)
        {
            FeatureSchema& featureSchema = m_featureDatabase.GetFeatureSchema();

            const Node* rootJoint = settings.m_actorInstance->GetActor()->GetMotionExtractionNode();
            const AZStd::string& rootJointName = rootJoint->GetNameString();

            //----------------------------------------------------------------------------------------------------------
            // Past and future root trajectory
            m_rootTrajectoryData = aznew FeatureTrajectory();
            m_rootTrajectoryData->SetJointName(rootJointName);
            m_rootTrajectoryData->SetRelativeToJointName(rootJointName);
            m_rootTrajectoryData->SetDebugDrawColor(AZ::Color::CreateFromRgba(157,78,221,255));
            m_rootTrajectoryData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_rootTrajectoryData);

            //----------------------------------------------------------------------------------------------------------
            // Left foot position
            m_leftFootPositionData = aznew FeaturePosition();
            m_leftFootPositionData->SetName("Left Foot Position");
            m_leftFootPositionData->SetJointName("L_foot_JNT");
            m_leftFootPositionData->SetRelativeToJointName(rootJointName);
            m_leftFootPositionData->SetDebugDrawColor(AZ::Color::CreateFromRgba(255,173,173,255));
            m_leftFootPositionData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_leftFootPositionData);
            m_featureDatabase.AddKdTreeFeature(m_leftFootPositionData);

            //----------------------------------------------------------------------------------------------------------
            // Right foot position
            m_rightFootPositionData = aznew FeaturePosition();
            m_rightFootPositionData->SetName("Right Foot Position");
            m_rightFootPositionData->SetJointName("R_foot_JNT");
            m_rightFootPositionData->SetRelativeToJointName(rootJointName);
            m_rightFootPositionData->SetDebugDrawColor(AZ::Color::CreateFromRgba(253,255,182,255));
            m_rightFootPositionData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_rightFootPositionData);
            m_featureDatabase.AddKdTreeFeature(m_rightFootPositionData);

            //----------------------------------------------------------------------------------------------------------
            // Left foot velocity
            m_leftFootVelocityData = aznew FeatureVelocity();
            m_leftFootVelocityData->SetName("Left Foot Velocity");
            m_leftFootVelocityData->SetJointName("L_foot_JNT");
            m_leftFootVelocityData->SetRelativeToJointName(rootJointName);
            m_leftFootVelocityData->SetDebugDrawColor(AZ::Color::CreateFromRgba(155,246,255,255));
            m_leftFootVelocityData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_leftFootVelocityData);
            m_featureDatabase.AddKdTreeFeature(m_leftFootVelocityData);

            //----------------------------------------------------------------------------------------------------------
            // Right foot velocity
            m_rightFootVelocityData = aznew FeatureVelocity();
            m_rightFootVelocityData->SetName("Right Foot Velocity");
            m_rightFootVelocityData->SetJointName("R_foot_JNT");
            m_rightFootVelocityData->SetRelativeToJointName(rootJointName);
            m_rightFootVelocityData->SetDebugDrawColor(AZ::Color::CreateFromRgba(189,178,255,255));
            m_rightFootVelocityData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_rightFootVelocityData);
            m_featureDatabase.AddKdTreeFeature(m_rightFootVelocityData);

            //----------------------------------------------------------------------------------------------------------
            // Pelvis velocity
            m_pelvisVelocityData = aznew FeatureVelocity();
            m_pelvisVelocityData->SetName("Pelvis Velocity");
            m_pelvisVelocityData->SetJointName("C_pelvis_JNT");
            m_pelvisVelocityData->SetRelativeToJointName(rootJointName);
            m_pelvisVelocityData->SetDebugDrawColor(AZ::Color::CreateFromRgba(185,255,175,255));
            m_pelvisVelocityData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_pelvisVelocityData);
            m_featureDatabase.AddKdTreeFeature(m_pelvisVelocityData);

            return true;
        }

        size_t LocomotionConfig::FindLowestCostFrameIndex(MotionMatchingInstance* instance, const Feature::FrameCostContext& context, size_t currentFrameIndex)
        {
            AZ::Debug::Timer timer;
            timer.Stamp();

            AZ_PROFILE_SCOPE(Animation, "LocomotionConfig::FindLowestCostFrameIndex");

            // 1. Broad-phase search using KD-tree
            {
                // Build the input query features that will be compared to every entry in the feature database in the motion matching search.
                size_t startOffset = 0;
                AZStd::vector<float>& queryFeatureValues = instance->GetQueryFeatureValues();
                const FeatureDatabase& featureDatabase = instance->GetConfig()->GetFeatureDatabase();
                for (Feature* feature : featureDatabase.GetFeaturesInKdTree())
                {
                    feature->FillQueryFeatureValues(startOffset, queryFeatureValues, context);
                    startOffset += feature->GetNumDimensions();
                }
                AZ_Assert(startOffset == queryFeatureValues.size(), "Frame float vector is not the expected size.");

                // Find our nearest frames.
                featureDatabase.GetKdTree().FindNearestNeighbors(queryFeatureValues, instance->GetNearestFrames());
            }

            // 2. Narrow-phase, brute force find the actual best matching frame.
            float minCost = FLT_MAX;
            size_t minCostFrameIndex = 0;

            float minLeftFootPositionCost = 0.0f;
            float minRightFootPositionCost = 0.0f;
            float minLeftFootVelocityCost = 0.0f;
            float minRightFootVelocityCost = 0.0f;
            float minPelvisVelocityCost = 0.0f;
            float minTrajectoryPastCost = 0.0f;
            float minTrajectoryFutureCost = 0.0f;

            for (const size_t frameIndex : instance->GetNearestFrames())
            {
                const Frame& frame = m_frameDatabase.GetFrame(frameIndex);

                // TODO: This shouldn't be there, we should be discarding the frames when extracting the features and not at runtime when checking the cost.
                if (frame.GetSampleTime() >= frame.GetSourceMotion()->GetDuration() - 1.0f)
                {
                    continue;
                }

                const float leftFootPositionCost = m_leftFootPositionData->CalculateFrameCost(frameIndex, context);
                const float rightFootPositionCost = m_rightFootPositionData->CalculateFrameCost(frameIndex, context);
                
                const float leftFootVelocityCost = m_leftFootVelocityData->CalculateFrameCost(frameIndex, context);
                const float rightFootVelocityCost = m_rightFootVelocityData->CalculateFrameCost(frameIndex, context);
                const float pelvisVelocityCost = m_pelvisVelocityData->CalculateFrameCost(frameIndex, context);

                const float trajectoryPastCost = m_rootTrajectoryData->CalculatePastFrameCost(frameIndex, context);
                const float trajectoryFutureCost = m_rootTrajectoryData->CalculateFutureFrameCost(frameIndex, context);

                float totalCost =
                    m_factorWeights.m_footPositionFactor * leftFootPositionCost + m_factorWeights.m_footPositionFactor * rightFootPositionCost + // foot position
                    m_factorWeights.m_footVelocityFactor * leftFootVelocityCost + m_factorWeights.m_footVelocityFactor * rightFootVelocityCost + // foot velocity
                    pelvisVelocityCost + // pelvis velocity
                    m_factorWeights.m_rootPastFactor * trajectoryPastCost + m_factorWeights.m_rootFutureFactor * trajectoryFutureCost; // trajectory

                const Frame& currentFrame = m_frameDatabase.GetFrame(currentFrameIndex);
                if (frame.GetSourceMotion() != currentFrame.GetSourceMotion())
                {
                    totalCost *= m_factorWeights.m_differentMotionFactor;
                }

                // Track the minimum cost value and frame.
                if (totalCost < minCost)
                {
                    minCost = totalCost;
                    minCostFrameIndex = frameIndex;

                    minLeftFootPositionCost = leftFootPositionCost;
                    minRightFootPositionCost = rightFootPositionCost;
                    minLeftFootVelocityCost = leftFootVelocityCost;
                    minRightFootVelocityCost = rightFootVelocityCost;
                    minPelvisVelocityCost = pelvisVelocityCost;
                    minTrajectoryPastCost = trajectoryPastCost;
                    minTrajectoryFutureCost = trajectoryFutureCost;
                }
            }

            const float time = timer.GetDeltaTimeInSeconds();
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushPerformanceHistogramValue, "FindLowestCostFrameIndex", time * 1000.0f);

            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, m_leftFootPositionData->GetName().c_str(), minLeftFootPositionCost, m_leftFootPositionData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, m_rightFootPositionData->GetName().c_str(), minRightFootPositionCost, m_rightFootPositionData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, m_leftFootVelocityData->GetName().c_str(), minLeftFootVelocityCost, m_leftFootVelocityData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, m_rightFootVelocityData->GetName().c_str(), minRightFootVelocityCost, m_rightFootVelocityData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, m_pelvisVelocityData->GetName().c_str(), minPelvisVelocityCost, m_pelvisVelocityData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Trajectory Past Cost", minTrajectoryPastCost, m_rootTrajectoryData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Trajectory Future Cost", minTrajectoryFutureCost, m_rootTrajectoryData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Total Cost", minCost, AZ::Color::CreateFromRgba(202,255,191,255));

            return minCostFrameIndex;
        }
    } // namespace MotionMatching
} // namespace EMotionFX
