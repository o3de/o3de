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

#include <AzCore/Debug/Timer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/TransformData.h>

#include <Allocators.h>
#include <Feature.h>
#include <FeatureTrajectory.h>
#include <FeatureSchemaDefault.h>
#include <FrameDatabase.h>
#include <ImGuiMonitorBus.h>
#include <KdTree.h>
#include <MotionMatchingConfig.h>
#include <MotionMatchingInstance.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionMatchingConfig, MotionMatchAllocator, 0)

    bool MotionMatchingConfig::RegisterFeatures(const InitSettings& settings)
    {
        FeatureSchema& featureSchema = m_featureDatabase.GetFeatureSchema();

        const Node* rootJoint = settings.m_actorInstance->GetActor()->GetMotionExtractionNode();
        if (!rootJoint)
        {
            AZ_Error("MotionMatching", false, "Cannot register features. Cannot find motion extraction joint.");
            return false;
        }

        DefaultFeatureSchemaInitSettings defaultSettings;
        defaultSettings.m_rootJointName = rootJoint->GetNameString();
        defaultSettings.m_leftFootJointName = "L_foot_JNT";
        defaultSettings.m_rightFootJointName = "R_foot_JNT";
        defaultSettings.m_pelvisJointName = "C_pelvis_JNT";
        DefaultFeatureSchema(featureSchema, defaultSettings);

        return true;
    }

    bool MotionMatchingConfig::Init(const InitSettings& settings)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingConfig::Init");

        // Import all motion frames.
        size_t totalNumFramesImported = 0;
        size_t totalNumFramesDiscarded = 0;
        for (Motion* motion : settings.m_motionList)
        {
            size_t numFrames = 0;
            size_t numDiscarded = 0;
            std::tie(numFrames, numDiscarded) = m_frameDatabase.ImportFrames(motion, settings.m_frameImportSettings, false);
            totalNumFramesImported += numFrames;
            totalNumFramesDiscarded += numDiscarded;

            if (settings.m_importMirrored)
            {
                std::tie(numFrames, numDiscarded) = m_frameDatabase.ImportFrames(motion, settings.m_frameImportSettings, true);
                totalNumFramesImported += numFrames;
                totalNumFramesDiscarded += numDiscarded;
            }
        }

        if (totalNumFramesImported > 0 || totalNumFramesDiscarded > 0)
        {
            AZ_TracePrintf("EMotionFX", "Motion matching config '%s' has imported a total of %d frames (%d frames discarded) across %d motions. This is %.2f seconds (%.2f minutes) of motion data.", RTTI_GetTypeName(), totalNumFramesImported, totalNumFramesDiscarded, settings.m_motionList.size(), totalNumFramesImported / (float)settings.m_frameImportSettings.m_sampleRate, (totalNumFramesImported / (float)settings.m_frameImportSettings.m_sampleRate) / 60.0f);
        }

        if (!RegisterFeatures(settings))
        {
            AZ_Error("EMotionFX", false, "Failed to register features inside motion matching config.");
            return false;
        }

        // Use all features other than the trajectory for the broad-phase search using the KD-Tree.
        for (Feature* feature : m_featureDatabase.GetFeatureSchema().GetFeatures())
        {
            if (feature->RTTI_GetType() != azrtti_typeid<FeatureTrajectory>())
            {
                m_featureDatabase.AddKdTreeFeature(feature);
            }
        }

        // Now build the per frame data (slow).
        if (!m_featureDatabase.ExtractFeatures(settings.m_actorInstance, &m_frameDatabase, settings.m_maxKdTreeDepth, settings.m_minFramesPerKdTreeNode))
        {
            AZ_Error("EMotionFX", false, "Failed to generate frame datas inside motion matching config.");
            return false;
        }

        return true;
    }

    void MotionMatchingConfig::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, MotionMatchingInstance* instance)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingConfig::DebugDraw");

        // Get the lowest cost frame index from the last search. As we're searching the feature database with a much lower
        // frequency and sample the animation onwards from this, the resulting frame index does not represent the current
        // feature values from the shown pose.
        const size_t curFrameIndex = instance->GetLowestCostFrameIndex();
        if (curFrameIndex == InvalidIndex)
        {
            return;
        }

        // Find the frame index in the frame database that belongs to the currently used pose.
        MotionInstance* motionInstance = instance->GetMotionInstance();
        const size_t currentFrame = m_frameDatabase.FindFrameIndex(motionInstance->GetMotion(), motionInstance->GetCurrentTime());
        if (currentFrame != InvalidIndex)
        {
            m_featureDatabase.DebugDraw(debugDisplay, instance, currentFrame);
        }

        // Draw the desired future trajectory and the sampled version of the past trajectory.
        const TrajectoryQuery& trajectoryQuery = instance->GetTrajectoryQuery();
        const AZ::Color trajectoryQueryColor = AZ::Color::CreateFromRgba(90,219,64,255);
        trajectoryQuery.DebugDraw(debugDisplay, trajectoryQueryColor);

        // Draw the trajectory history starting after the sampled version of the past trajectory.
        const FeatureTrajectory* trajectoryFeature = instance->GetTrajectoryFeature();
        const TrajectoryHistory& trajectoryHistory = instance->GetTrajectoryHistory();
        trajectoryHistory.DebugDraw(debugDisplay, trajectoryQueryColor, trajectoryFeature->GetPastTimeRange());
    }

    size_t MotionMatchingConfig::FindLowestCostFrameIndex(MotionMatchingInstance* instance, const Feature::FrameCostContext& context, AZStd::vector<float>& tempCosts, AZStd::vector<float>& minCosts)
    {
        AZ::Debug::Timer timer;
        timer.Stamp();

        AZ_PROFILE_SCOPE(Animation, "MotionMatching::FindLowestCostFrameIndex");

        FeatureDatabase& featureDatabase = instance->GetConfig()->GetFeatureDatabase();
        const FeatureSchema& featureSchema = featureDatabase.GetFeatureSchema();
        const FeatureTrajectory* trajectoryFeature = instance->GetTrajectoryFeature();

        // 1. Broad-phase search using KD-tree
        {
            // Build the input query features that will be compared to every entry in the feature database in the motion matching search.
            size_t startOffset = 0;
            AZStd::vector<float>& queryFeatureValues = instance->GetQueryFeatureValues();
            for (Feature* feature : featureDatabase.GetFeaturesInKdTree())
            {
                feature->FillQueryFeatureValues(startOffset, queryFeatureValues, context);
                startOffset += feature->GetNumDimensions();
            }
            AZ_Assert(startOffset == queryFeatureValues.size(), "Frame float vector is not the expected size.");

            // Find our nearest frames.
            featureDatabase.GetKdTree().FindNearestNeighbors(queryFeatureValues, instance->GetNearestFrames());
        }

        // 2. Narrow-phase, brute force find the actual best matching frame (frame with the minimal cost).
        float minCost = FLT_MAX;
        size_t minCostFrameIndex = 0;
        tempCosts.resize(featureSchema.GetNumFeatures());
        minCosts.resize(featureSchema.GetNumFeatures());
        float minTrajectoryPastCost = 0.0f;
        float minTrajectoryFutureCost = 0.0f;

        // Iterate through the frames filtered by the broad-phase search.
        for (const size_t frameIndex : instance->GetNearestFrames())
        {
            float frameCost = 0.0f;

            // Calculate the frame cost by accumulating the weighted feature costs.
            for (size_t featureIndex = 0; featureIndex < featureSchema.GetNumFeatures(); ++featureIndex)
            {
                Feature* feature = featureSchema.GetFeature(featureIndex);
                if (feature->RTTI_GetType() != azrtti_typeid<FeatureTrajectory>())
                {
                    const float featureCost = feature->CalculateFrameCost(frameIndex, context);
                    const float featureCostFactor = feature->GetCostFactor();
                    const float featureFinalCost = featureCost * featureCostFactor;

                    frameCost += featureFinalCost;
                    tempCosts[featureIndex] = featureFinalCost;
                }
            }

            // Manually add the trajectory cost.
            float trajectoryPastCost = 0.0f;
            float trajectoryFutureCost = 0.0f;
            if (trajectoryFeature)
            {
                trajectoryPastCost = trajectoryFeature->CalculatePastFrameCost(frameIndex, context);
                trajectoryFutureCost = trajectoryFeature->CalculateFutureFrameCost(frameIndex, context);
                frameCost += 0.5f * trajectoryPastCost; // TODO: This needs to be exposed to the edit context and not hard-coded.
                frameCost += 0.75f * trajectoryFutureCost;
            }

            // Track the minimum feature and frame costs.
            if (frameCost < minCost)
            {
                minCost = frameCost;
                minCostFrameIndex = frameIndex;

                for (size_t featureIndex = 0; featureIndex < featureSchema.GetNumFeatures(); ++featureIndex)
                {
                    Feature* feature = featureSchema.GetFeature(featureIndex);
                    if (feature->RTTI_GetType() != azrtti_typeid<FeatureTrajectory>())
                    {
                        minCosts[featureIndex] = tempCosts[featureIndex];
                    }
                }

                minTrajectoryPastCost = trajectoryPastCost;
                minTrajectoryFutureCost = trajectoryFutureCost;
            }
        }

        // 3. ImGui debug visualization
        {
            const float time = timer.GetDeltaTimeInSeconds();
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushPerformanceHistogramValue, "FindLowestCostFrameIndex", time * 1000.0f);

            for (size_t featureIndex = 0; featureIndex < featureSchema.GetNumFeatures(); ++featureIndex)
            {
                Feature* feature = featureSchema.GetFeature(featureIndex);
                if (feature->RTTI_GetType() != azrtti_typeid<FeatureTrajectory>())
                {
                    ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue,
                        feature->GetName().c_str(),
                        minCosts[featureIndex],
                        feature->GetDebugDrawColor());
                }
            }

            if (trajectoryFeature)
            {
                ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Trajectory Future", minTrajectoryFutureCost, trajectoryFeature->GetDebugDrawColor());
                ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Trajectory Past", minTrajectoryPastCost, trajectoryFeature->GetDebugDrawColor());
            }

            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Total Cost", minCost, AZ::Color::CreateFromRgba(202,255,191,255));
        }

        return minCostFrameIndex;
    }

    void MotionMatchingConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionMatchingConfig>()
            ->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<MotionMatchingConfig>("MotionMatchingConfig", "Base class for motion matching configs")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
    }
} // namespace EMotionFX::MotionMatching
