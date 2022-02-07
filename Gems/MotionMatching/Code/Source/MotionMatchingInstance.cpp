/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Timer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionInstancePool.h>
#include <MotionMatchingData.h>
#include <MotionMatchingInstance.h>
#include <Feature.h>
#include <FeatureSchema.h>
#include <FeatureTrajectory.h>
#include <KdTree.h>
#include <ImGuiMonitorBus.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>
#include <PoseDataJointVelocities.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionMatchingInstance, MotionMatchAllocator, 0)

    MotionMatchingInstance::~MotionMatchingInstance()
    {
        DebugDrawRequestBus::Handler::BusDisconnect();

        if (m_motionInstance)
        {
            GetMotionInstancePool().Free(m_motionInstance);
        }

        if (m_prevMotionInstance)
        {
            GetMotionInstancePool().Free(m_prevMotionInstance);
        }
    }

    MotionInstance* MotionMatchingInstance::CreateMotionInstance() const
    {
        MotionInstance* result = GetMotionInstancePool().RequestNew(m_data->GetFrameDatabase().GetFrame(0).GetSourceMotion(), m_actorInstance);
        return result;
    }

    void MotionMatchingInstance::Init(const InitSettings& settings)
    {
        AZ_Assert(settings.m_actorInstance, "The actor instance cannot be a nullptr.");
        AZ_Assert(settings.m_data, "The motion match data cannot be nullptr.");

        DebugDrawRequestBus::Handler::BusConnect();

        // Update the cached pointer to the trajectory feature.
        const FeatureSchema& featureSchema = settings.m_data->GetFeatureSchema();
        for (Feature* feature : featureSchema.GetFeatures())
        {
            if (feature->RTTI_GetType() == azrtti_typeid<FeatureTrajectory>())
            {
                m_cachedTrajectoryFeature = static_cast<FeatureTrajectory*>(feature);
                break;
            }
        }

        m_actorInstance = settings.m_actorInstance;
        m_data = settings.m_data;
        if (settings.m_data->GetFrameDatabase().GetNumFrames() == 0)
        {
            return;
        }

        if (!m_motionInstance)
        {
            m_motionInstance = CreateMotionInstance();
        }

        if (!m_prevMotionInstance)
        {
            m_prevMotionInstance = CreateMotionInstance();
        }

        m_blendSourcePose.LinkToActorInstance(m_actorInstance);
        m_blendSourcePose.InitFromBindPose(m_actorInstance);

        m_blendTargetPose.LinkToActorInstance(m_actorInstance);
        m_blendTargetPose.InitFromBindPose(m_actorInstance);

        m_queryPose.LinkToActorInstance(m_actorInstance);
        m_queryPose.InitFromBindPose(m_actorInstance);

        // Make sure we have enough space inside the frame floats array, which is used to search the kdTree.
        const size_t numValuesInKdTree = m_data->GetKdTree().GetNumDimensions();
        m_queryFeatureValues.resize(numValuesInKdTree);

        // Initialize the trajectory history.
        if (m_cachedTrajectoryFeature)
        {
            size_t rootJointIndex = m_actorInstance->GetActor()->GetMotionExtractionNodeIndex();
            if (rootJointIndex == InvalidIndex32)
            {
                rootJointIndex = 0;
            }
            m_trajectoryHistory.Init(*m_actorInstance->GetTransformData()->GetCurrentPose(),
                rootJointIndex,
                m_cachedTrajectoryFeature->GetFacingAxisDir(),
                m_trajectorySecsToTrack);
        }
    }

    void MotionMatchingInstance::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingInstance::DebugDraw");

        // Get the lowest cost frame index from the last search. As we're searching the feature database with a much lower
        // frequency and sample the animation onwards from this, the resulting frame index does not represent the current
        // feature values from the shown pose.
        const size_t curFrameIndex = GetLowestCostFrameIndex();
        if (curFrameIndex == InvalidIndex)
        {
            return;
        }

        const FrameDatabase& frameDatabase = m_data->GetFrameDatabase();
        const FeatureSchema& featureSchema = m_data->GetFeatureSchema();

        // Find the frame index in the frame database that belongs to the currently used pose.
        const size_t currentFrame = frameDatabase.FindFrameIndex(m_motionInstance->GetMotion(), m_motionInstance->GetCurrentTime());

        // Render the feature debug visualizations for the current frame.
        if (currentFrame != InvalidIndex)
        {
            for (Feature* feature: featureSchema.GetFeatures())
            {
                if (feature->GetDebugDrawEnabled())
                {
                    feature->DebugDraw(debugDisplay, this, currentFrame);
                }
            }
        }

        // Draw the desired future trajectory and the sampled version of the past trajectory.
        const AZ::Color trajectoryQueryColor = AZ::Color::CreateFromRgba(90,219,64,255);
        m_trajectoryQuery.DebugDraw(debugDisplay, trajectoryQueryColor);

        // Draw the trajectory history starting after the sampled version of the past trajectory.
        m_trajectoryHistory.DebugDraw(debugDisplay, trajectoryQueryColor, m_cachedTrajectoryFeature->GetPastTimeRange());
    }

    void MotionMatchingInstance::SamplePose(MotionInstance* motionInstance, Pose& outputPose)
    {
        const Pose* bindPose = m_actorInstance->GetTransformData()->GetBindPose();
        motionInstance->GetMotion()->Update(bindPose, &outputPose, motionInstance);
        if (m_actorInstance->GetActor()->GetMotionExtractionNode() && m_actorInstance->GetMotionExtractionEnabled())
        {
            outputPose.CompensateForMotionExtraction();
        }
    }

    void MotionMatchingInstance::SamplePose(Motion* motion, Pose& outputPose, float sampleTime) const
    {
        MotionDataSampleSettings sampleSettings;
        sampleSettings.m_actorInstance = outputPose.GetActorInstance();
        sampleSettings.m_inPlace = false;
        sampleSettings.m_mirror = false;
        sampleSettings.m_retarget = false;
        sampleSettings.m_inputPose = sampleSettings.m_actorInstance->GetTransformData()->GetBindPose();

        sampleSettings.m_sampleTime = sampleTime;
        sampleSettings.m_sampleTime = AZ::GetClamp(sampleTime, 0.0f, motion->GetDuration());

        motion->SamplePose(&outputPose, sampleSettings);
    }

    void MotionMatchingInstance::PostUpdate([[maybe_unused]] float timeDelta)
    {
        if (!m_data)
        {
            m_motionExtractionDelta.Identity();
            return;
        }

        const size_t lowestCostFrame = GetLowestCostFrameIndex();
        if (m_data->GetFrameDatabase().GetNumFrames() == 0 || lowestCostFrame == InvalidIndex)
        {
            m_motionExtractionDelta.Identity();
            return;
        }

        // Blend the motion extraction deltas.
        // Note: Make sure to update the previous as well as the current/target motion instances.
        if (m_blendWeight >= 1.0f - AZ::Constants::FloatEpsilon)
        {
            m_motionInstance->ExtractMotion(m_motionExtractionDelta);
        }
        else if (m_blendWeight > AZ::Constants::FloatEpsilon && m_blendWeight < 1.0f - AZ::Constants::FloatEpsilon)
        {
            Transform targetMotionExtractionDelta;
            m_motionInstance->ExtractMotion(m_motionExtractionDelta);
            m_prevMotionInstance->ExtractMotion(targetMotionExtractionDelta);
            m_motionExtractionDelta.Blend(targetMotionExtractionDelta, m_blendWeight);
        }
        else
        {
            m_prevMotionInstance->ExtractMotion(m_motionExtractionDelta);
        }
    }

    void MotionMatchingInstance::Output(Pose& outputPose)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingInstance::Output");

        if (!m_data)
        {
            outputPose.InitFromBindPose(m_actorInstance);
            return;
        }

        const size_t lowestCostFrame = GetLowestCostFrameIndex();
        if (m_data->GetFrameDatabase().GetNumFrames() == 0 || lowestCostFrame == InvalidIndex)
        {
            outputPose.InitFromBindPose(m_actorInstance);
            return;
        }

        // Sample the motions and blend the results when needed.
        if (m_blendWeight >= 1.0f - AZ::Constants::FloatEpsilon)
        {
            m_blendTargetPose.InitFromBindPose(m_actorInstance);
            if (m_motionInstance)
            {
                SamplePose(m_motionInstance, m_blendTargetPose);
            }
            outputPose = m_blendTargetPose;
        }
        else if (m_blendWeight > AZ::Constants::FloatEpsilon && m_blendWeight < 1.0f - AZ::Constants::FloatEpsilon)
        {
            m_blendSourcePose.InitFromBindPose(m_actorInstance);
            m_blendTargetPose.InitFromBindPose(m_actorInstance);
            if (m_motionInstance)
            {
                SamplePose(m_motionInstance, m_blendTargetPose);
            }
            if (m_prevMotionInstance)
            {
                SamplePose(m_prevMotionInstance, m_blendSourcePose);
            }

            outputPose = m_blendSourcePose;
            outputPose.Blend(&m_blendTargetPose, m_blendWeight);
        }
        else
        {
            m_blendSourcePose.InitFromBindPose(m_actorInstance);
            if (m_prevMotionInstance)
            {
                SamplePose(m_prevMotionInstance, m_blendSourcePose);
            }
            outputPose = m_blendSourcePose;
        }
    }

    void MotionMatchingInstance::Update(float timePassedInSeconds, const AZ::Vector3& targetPos, const AZ::Vector3& targetFacingDir, TrajectoryQuery::EMode mode, float pathRadius, float pathSpeed)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingInstance::Update");

        if (!m_data || !m_motionInstance)
        {
            return;
        }

        size_t currentFrameIndex = GetLowestCostFrameIndex();
        if (currentFrameIndex == InvalidIndex)
        {
            currentFrameIndex = 0;
        }

        // Add the sample from the last frame (post-motion extraction)
        m_trajectoryHistory.AddSample(*m_actorInstance->GetTransformData()->GetCurrentPose());
        // Update the time. After this there is no sample for the updated time in the history as we're about to prepare this with the current update.
        m_trajectoryHistory.Update(timePassedInSeconds);

        // Update the trajectory query control points.
        m_trajectoryQuery.Update(m_actorInstance,
            m_cachedTrajectoryFeature,
            m_trajectoryHistory,
            mode,
            targetPos,
            targetFacingDir,
            timePassedInSeconds,
            pathRadius,
            pathSpeed);

        // Calculate the new time value of the motion, but don't set it yet (the syncing might adjust this again)
        m_motionInstance->SetFreezeAtLastFrame(true);
        m_motionInstance->SetMaxLoops(1);
        const float newMotionTime = m_motionInstance->CalcPlayStateAfterUpdate(timePassedInSeconds).m_currentTime;
        m_newMotionTime = newMotionTime;

        // Keep on playing the previous instance as we're blending the poses and motion extraction deltas.
        m_prevMotionInstance->Update(timePassedInSeconds);

        m_timeSinceLastFrameSwitch += timePassedInSeconds;

        const float lowestCostSearchTimeInterval = 1.0f / m_lowestCostSearchFrequency;

        if (m_blending)
        {
            const float maxBlendTime = lowestCostSearchTimeInterval;
            m_blendProgressTime += timePassedInSeconds;
            if (m_blendProgressTime > maxBlendTime)
            {
                m_blendWeight = 1.0f;
                m_blendProgressTime = maxBlendTime;
                m_blending = false;
            }
            else
            {
                m_blendWeight = AZ::GetClamp(m_blendProgressTime / maxBlendTime, 0.0f, 1.0f);
            }
        }

        const bool searchLowestCostFrame = m_timeSinceLastFrameSwitch >= lowestCostSearchTimeInterval;
        if (searchLowestCostFrame)
        {
            // Calculate the input query pose for the motion matching search algorithm.
            {
                // Sample the pose for the new motion time as the motion instance has not been updated with the timeDelta from this frame yet.
                SamplePose(m_motionInstance->GetMotion(), m_queryPose, newMotionTime);

                // Copy over the motion extraction joint transform from the current pose to the newly sampled pose.
                // When sampling a motion, the motion extraction joint is in animation space, while we need the query pose to be in world space.
                // Note: This does not yet take the extraction delta from the current tick into account.
                if (m_actorInstance->GetActor()->GetMotionExtractionNode())
                {
                    const Pose* currentPose = m_actorInstance->GetTransformData()->GetCurrentPose();
                    const size_t motionExtractionJointIndex = m_actorInstance->GetActor()->GetMotionExtractionNodeIndex();
                    m_queryPose.SetWorldSpaceTransform(motionExtractionJointIndex,
                        currentPose->GetWorldSpaceTransform(motionExtractionJointIndex));
                }

                // Calculate the joint velocities for the sampled pose using the same method as we do for the frame database.
                PoseDataJointVelocities* velocityPoseData = m_queryPose.GetAndPreparePoseData<PoseDataJointVelocities>(m_actorInstance);
                velocityPoseData->CalculateVelocity(m_motionInstance, m_cachedTrajectoryFeature->GetRelativeToNodeIndex());
            }

            const FeatureMatrix& featureMatrix = m_data->GetFeatureMatrix();
            const FrameDatabase& frameDatabase = m_data->GetFrameDatabase();

            Feature::FrameCostContext frameCostContext(featureMatrix, m_queryPose);
            frameCostContext.m_trajectoryQuery = &m_trajectoryQuery;
            frameCostContext.m_actorInstance = m_actorInstance;
            const size_t lowestCostFrameIndex = FindLowestCostFrameIndex(frameCostContext);

            const Frame& currentFrame = frameDatabase.GetFrame(currentFrameIndex);
            const Frame& lowestCostFrame = frameDatabase.GetFrame(lowestCostFrameIndex);
            const bool sameMotion = (currentFrame.GetSourceMotion() == lowestCostFrame.GetSourceMotion());
            const float timeBetweenFrames = newMotionTime - lowestCostFrame.GetSampleTime();
            const bool sameLocation = sameMotion && (AZ::GetAbs(timeBetweenFrames) < 0.1f);

            if (lowestCostFrameIndex != currentFrameIndex && !sameLocation)
            {
                // Start a blend.
                m_blending = true;
                m_blendWeight = 0.0f;
                m_blendProgressTime = 0.0f;

                // Store the current motion instance state, so we can sample this as source pose.
                m_prevMotionInstance->SetMotion(m_motionInstance->GetMotion());
                m_prevMotionInstance->SetMirrorMotion(m_motionInstance->GetMirrorMotion());
                m_prevMotionInstance->SetCurrentTime(newMotionTime, true);
                m_prevMotionInstance->SetLastCurrentTime(m_prevMotionInstance->GetCurrentTime() - timePassedInSeconds);

                m_lowestCostFrameIndex = lowestCostFrameIndex;

                m_motionInstance->SetMotion(lowestCostFrame.GetSourceMotion());
                m_motionInstance->SetMirrorMotion(lowestCostFrame.GetMirrored());

                // The new motion time will become the current time after this frame while the current time
                // becomes the last current time. As we just start playing at the search frame, calculate
                // the last time based on the time delta.
                m_motionInstance->SetCurrentTime(lowestCostFrame.GetSampleTime() -  timePassedInSeconds, true);
                m_newMotionTime = lowestCostFrame.GetSampleTime();
            }

            // Do this always, else wise we search for the lowest cost frame index too many times.
            m_timeSinceLastFrameSwitch = 0.0f;
        }

        // ImGui monitor
        {
#ifdef IMGUI_ENABLED
            const FrameDatabase& frameDatabase = m_data->GetFrameDatabase();
            ImGuiMonitorRequests::FrameDatabaseInfo frameDatabaseInfo{frameDatabase.CalcMemoryUsageInBytes(), frameDatabase.GetNumFrames(), frameDatabase.GetNumUsedMotions(), frameDatabase.GetNumFrames() / (float)frameDatabase.GetSampleRate()};
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetFrameDatabaseInfo, frameDatabaseInfo);

            const KdTree& kdTree = m_data->GetKdTree();
            ImGuiMonitorRequests::KdTreeInfo kdTreeInfo{kdTree.CalcMemoryUsageInBytes(), kdTree.GetNumNodes(), kdTree.GetNumDimensions()};
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetKdTreeInfo, kdTreeInfo);
            
            const FeatureMatrix& featureMatrix = m_data->GetFeatureMatrix();
            ImGuiMonitorRequests::FeatureMatrixInfo featureMatrixInfo{featureMatrix.CalcMemoryUsageInBytes(), static_cast<size_t>(featureMatrix.rows()), static_cast<size_t>(featureMatrix.cols())};
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::SetFeatureMatrixInfo, featureMatrixInfo);
#endif
        }
    }

    size_t MotionMatchingInstance::FindLowestCostFrameIndex(const Feature::FrameCostContext& context)
    {
        AZ::Debug::Timer timer;
        timer.Stamp();

        AZ_PROFILE_SCOPE(Animation, "MotionMatchingInstance::FindLowestCostFrameIndex");

        const FrameDatabase& frameDatabase = m_data->GetFrameDatabase();
        const FeatureSchema& featureSchema = m_data->GetFeatureSchema();
        const FeatureTrajectory* trajectoryFeature = m_cachedTrajectoryFeature;

        // 1. Broad-phase search using KD-tree
        {
            // Build the input query features that will be compared to every entry in the feature database in the motion matching search.
            size_t startOffset = 0;
            for (Feature* feature : m_data->GetFeaturesInKdTree())
            {
                feature->FillQueryFeatureValues(startOffset, m_queryFeatureValues, context);
                startOffset += feature->GetNumDimensions();
            }
            AZ_Assert(startOffset == m_queryFeatureValues.size(), "Frame float vector is not the expected size.");

            // Find our nearest frames.
            m_data->GetKdTree().FindNearestNeighbors(m_queryFeatureValues, m_nearestFrames);
        }

        // 2. Narrow-phase, brute force find the actual best matching frame (frame with the minimal cost).
        float minCost = FLT_MAX;
        size_t minCostFrameIndex = 0;
        m_tempCosts.resize(featureSchema.GetNumFeatures());
        m_minCosts.resize(featureSchema.GetNumFeatures());
        float minTrajectoryPastCost = 0.0f;
        float minTrajectoryFutureCost = 0.0f;

        // Iterate through the frames filtered by the broad-phase search.
        for (const size_t frameIndex : m_nearestFrames)
        {
            const Frame& frame = frameDatabase.GetFrame(frameIndex);

            // TODO: This shouldn't be there, we should be discarding the frames when extracting the features and not at runtime when checking the cost.
            if (frame.GetSampleTime() >= frame.GetSourceMotion()->GetDuration() - 1.0f)
            {
                continue;
            }

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
                    m_tempCosts[featureIndex] = featureFinalCost;
                }
            }

            // Manually add the trajectory cost.
            float trajectoryPastCost = 0.0f;
            float trajectoryFutureCost = 0.0f;
            if (trajectoryFeature)
            {
                trajectoryPastCost = trajectoryFeature->CalculatePastFrameCost(frameIndex, context) * trajectoryFeature->GetPastCostFactor();
                trajectoryFutureCost = trajectoryFeature->CalculateFutureFrameCost(frameIndex, context) * trajectoryFeature->GetFutureCostFactor();
                frameCost += trajectoryPastCost;
                frameCost += trajectoryFutureCost;
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
                        m_minCosts[featureIndex] = m_tempCosts[featureIndex];
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
                        m_minCosts[featureIndex],
                        feature->GetDebugDrawColor());
                }
            }

            if (trajectoryFeature)
            {
                ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Future Trajectory", minTrajectoryFutureCost, trajectoryFeature->GetDebugDrawColor());
                ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Past Trajectory", minTrajectoryPastCost, trajectoryFeature->GetDebugDrawColor());
            }

            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushCostHistogramValue, "Total Cost", minCost, AZ::Color::CreateFromRgba(202,255,191,255));
        }

        return minCostFrameIndex;
    }
} // namespace EMotionFX::MotionMatching
