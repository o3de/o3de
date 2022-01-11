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

            //----------------------------------------------------------------------------------------------------------
            // Register the motion extraction trajectory (includes history and future)
            const Node* rootNode = settings.m_actorInstance->GetActor()->GetMotionExtractionNode();//GetActor()->GetSkeleton()->FindNodeByNameNoCase("root");
            m_rootNodeIndex = rootNode ? rootNode->GetNodeIndex() : 0;
            m_rootTrajectoryData = aznew FeatureTrajectory();
            m_rootTrajectoryData->SetNodeIndex(m_rootNodeIndex);
            m_rootTrajectoryData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_rootTrajectoryData->SetDebugDrawColor(AZ::Color::CreateFromRgba(157,78,221,255));
            m_rootTrajectoryData->SetNumFutureSamplesPerFrame(6);
            m_rootTrajectoryData->SetNumPastSamplesPerFrame(4);
            m_rootTrajectoryData->SetFutureTimeRange(1.2f);
            m_rootTrajectoryData->SetPastTimeRange(0.7f);
            m_rootTrajectoryData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_rootTrajectoryData);
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
            m_leftFootPositionData->SetDebugDrawColor(AZ::Color::CreateFromRgba(255,173,173,255));
            m_leftFootPositionData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_leftFootPositionData);
            m_featureDatabase.AddKdTreeFeature(m_leftFootPositionData);
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
            m_rightFootPositionData->SetDebugDrawColor(AZ::Color::CreateFromRgba(253,255,182,255));
            m_rightFootPositionData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_rightFootPositionData);
            m_featureDatabase.AddKdTreeFeature(m_rightFootPositionData);
            //----------------------------------------------------------------------------------------------------------

            //----------------------------------------------------------------------------------------------------------
            // Grab the left foot velocities
            m_leftFootVelocityData = aznew FeatureVelocity();
            m_leftFootVelocityData->SetNodeIndex(m_leftFootNodeIndex);
            m_leftFootVelocityData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_leftFootVelocityData->SetDebugDrawColor(AZ::Color::CreateFromRgba(155,246,255,255));
            m_leftFootVelocityData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_leftFootVelocityData);
            m_featureDatabase.AddKdTreeFeature(m_leftFootVelocityData);
            //----------------------------------------------------------------------------------------------------------

            //----------------------------------------------------------------------------------------------------------
            // Grab the right foot velocities
            m_rightFootVelocityData = aznew FeatureVelocity();
            m_rightFootVelocityData->SetNodeIndex(m_rightFootNodeIndex);
            m_rightFootVelocityData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_rightFootVelocityData->SetDebugDrawColor(AZ::Color::CreateFromRgba(189,178,255,255));
            m_rightFootVelocityData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_rightFootVelocityData);
            m_featureDatabase.AddKdTreeFeature(m_rightFootVelocityData);
            //----------------------------------------------------------------------------------------------------------

            //----------------------------------------------------------------------------------------------------------
            // Grab the pelvis velocity
            const Node* pelvisNode = settings.m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase("C_pelvis_JNT");
            m_pelvisNodeIndex = pelvisNode ? pelvisNode->GetNodeIndex() : MCORE_INVALIDINDEX32;
            if (!pelvisNode)
            {
                return false;
            }
            m_pelvisVelocityData = aznew FeatureVelocity();
            m_pelvisVelocityData->SetNodeIndex(m_pelvisNodeIndex);
            m_pelvisVelocityData->SetRelativeToNodeIndex(m_rootNodeIndex);
            m_pelvisVelocityData->SetDebugDrawColor(AZ::Color::CreateFromRgba(185,255,175,255));
            m_pelvisVelocityData->SetDebugDrawEnabled(true);
            featureSchema.AddFeature(m_pelvisVelocityData);
            m_featureDatabase.AddKdTreeFeature(m_pelvisVelocityData);
            //----------------------------------------------------------------------------------------------------------

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

            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Left Foot Position Cost", minLeftFootPositionCost, m_leftFootPositionData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Right Foot Position Cost", minRightFootPositionCost, m_rightFootPositionData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Left Foot Velocity Cost", minLeftFootVelocityCost, m_leftFootVelocityData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Right Foot Velocity Cost", minRightFootVelocityCost, m_rightFootVelocityData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Pelvis Velocity Cost", minPelvisVelocityCost, m_pelvisVelocityData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Trajectory Past Cost", minTrajectoryPastCost, m_rootTrajectoryData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Trajectory Future Cost", minTrajectoryFutureCost, m_rootTrajectoryData->GetDebugDrawColor());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Total Cost", minCost, AZ::Color::CreateFromRgba(202,255,191,255));

            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetKdTreeMemoryUsage, m_featureDatabase.GetKdTree().CalcMemoryUsageInBytes());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetKdTreeNumNodes, m_featureDatabase.GetKdTree().GetNumNodes());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetKdTreeNumDimensions, m_featureDatabase.GetKdTree().GetNumDimensions());

            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetFeatureMatrixMemoryUsage, m_featureDatabase.GetFeatureMatrix().CalcMemoryUsageInBytes());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetFeatureMatrixNumFrames, m_featureDatabase.GetFeatureMatrix().rows());
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetFeatureMatrixNumComponents, m_featureDatabase.GetFeatureMatrix().cols());

            //AZ_Printf("EMotionFX", "Frame %d = %f    %f/%f   %d nearest",
            //    minCostFrameIndex,
            //    minCost,
            //    m_frameDatabase.GetFrame(minCostFrameIndex).GetSampleTime(),
            //    m_frameDatabase.GetFrame(minCostFrameIndex).GetSourceMotion()->GetMaxTime(),
            //    instance->GetNearestFrames().size()
            //);
            return minCostFrameIndex;
        }

        void LocomotionConfig::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<LocomotionConfig, MotionMatchingConfig>()
                ->Version(1)
                ->Field("leftFootNodeIndex", &LocomotionConfig::m_leftFootNodeIndex)
                ->Field("rightFootNodeIndex", &LocomotionConfig::m_rightFootNodeIndex)
                ->Field("rootNodeIndex", &LocomotionConfig::m_rootNodeIndex);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<LocomotionConfig>("LocomotionConfig", "Locomotion config for motion matching")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &LocomotionConfig::m_rootNodeIndex, "Root node", "The root node.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &LocomotionConfig::m_leftFootNodeIndex, "Left foot node", "The left foot node.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &LocomotionConfig::m_rightFootNodeIndex, "Right foot node", "The right foot node.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
