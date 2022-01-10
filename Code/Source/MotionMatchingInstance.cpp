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
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionInstancePool.h>
#include <MotionMatchingConfig.h>
#include <MotionMatchingInstance.h>
#include <Feature.h>
#include <FeatureTrajectory.h>
#include <KdTree.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>
#include <PoseDataJointVelocities.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionMatchingInstance, MotionMatchAllocator, 0)

    MotionMatchingInstance::~MotionMatchingInstance()
    {
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
        MotionInstance* result = GetMotionInstancePool().RequestNew(m_config->GetFrameDatabase().GetFrame(0).GetSourceMotion(), m_actorInstance);
        return result;
    }

    void MotionMatchingInstance::Init(const InitSettings& settings)
    {
        AZ_Assert(settings.m_actorInstance, "The actor instance cannot be a nullptr.");
        AZ_Assert(settings.m_config, "The motion match data cannot be nullptr.");

        // Debug display initialization.
        const auto AddDebugDisplay = [=](AZ::s32 debugDisplayId)
        {
            if (debugDisplayId == -1)
            {
                return;
            }

            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, debugDisplayId);

            AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
            if (debugDisplay)
            {
                m_debugDisplays.emplace_back(debugDisplay);
            }
        };
        // Draw the debug visualizations to the Animation Editor as well as the LY Editor viewport.
        AZ::s32 animationEditorViewportId = -1;
        EMStudio::ViewportPluginRequestBus::BroadcastResult(animationEditorViewportId, &EMStudio::ViewportPluginRequestBus::Events::GetViewportId);
        AddDebugDisplay(animationEditorViewportId);
        AddDebugDisplay(AzFramework::g_defaultSceneEntityDebugDisplayId);

        m_actorInstance = settings.m_actorInstance;
        m_config = settings.m_config;
        if (settings.m_config->GetFrameDatabase().GetNumFrames() == 0)
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
        // It contains the value for each dimension.
        const size_t numValuesInKdTree = m_config->GetFeatures().CalcNumDataDimensionsForKdTree(m_config->GetFeatures());
        m_queryFeatureValues.resize(numValuesInKdTree);

        // Initialize the trajectory history.
        size_t rootJointIndex = m_actorInstance->GetActor()->GetMotionExtractionNodeIndex();
        if (rootJointIndex == InvalidIndex32)
        {
            rootJointIndex = 0;
        }
        m_trajectoryHistory.Init(*m_actorInstance->GetTransformData()->GetCurrentPose(),
            rootJointIndex,
            m_config->GetTrajectoryFeature()->GetFacingAxisDir(),
            m_trajectorySecsToTrack);
    }

    void MotionMatchingInstance::DebugDraw()
    {
        if (m_config && !m_debugDisplays.empty())
        {
            for (AzFramework::DebugDisplayRequests* debugDisplay : m_debugDisplays)
            {
                if (debugDisplay)
                {
                    const AZ::u32 prevState = debugDisplay->GetState();
                    m_config->DebugDraw(*debugDisplay, this);
                    debugDisplay->SetState(prevState);
                }
            }
        }
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
        if (!m_config)
        {
            m_motionExtractionDelta.Identity();
            return;
        }

        const size_t lowestCostFrame = GetLowestCostFrameIndex();
        if (m_config->GetFrameDatabase().GetNumFrames() == 0 || lowestCostFrame == InvalidIndex)
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

        if (!m_config)
        {
            outputPose.InitFromBindPose(m_actorInstance);
            return;
        }

        const size_t lowestCostFrame = GetLowestCostFrameIndex();
        if (m_config->GetFrameDatabase().GetNumFrames() == 0 || lowestCostFrame == InvalidIndex)
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

    size_t MotionMatchingInstance::GetLowestCostFrameIndex() const
    {
        return m_lowestCostFrameIndex;
    }

    void MotionMatchingInstance::Update(float timePassedInSeconds, const AZ::Vector3& targetPos, const AZ::Vector3& targetFacingDir, TrajectoryQuery::EMode mode, float pathRadius, float pathSpeed)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingInstance::Update");

        if (!m_config)
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

        // Register the current actor instance position to the history data of the spline.
        m_trajectoryQuery.Update(m_actorInstance,
            m_config->GetTrajectoryFeature(),
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
        SetNewMotionTime(newMotionTime);

        // Keep on playing the previous instance as we're blending the poses and motion extraction deltas.
        m_prevMotionInstance->Update(timePassedInSeconds);

        SetTimeSinceLastFrameSwitch(GetTimeSinceLastFrameSwitch() + timePassedInSeconds);

        if (m_blending)
        {
            const float maxBlendTime = GetLowestCostSearchFrequency();
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

        if (GetTimeSinceLastFrameSwitch() >= GetLowestCostSearchFrequency())
        {
            // Calculate the input query pose for the motion matching search algorithm.
            {
                // Sample the pose for the new motion time as the motion instance has not been updated with the timeDelta from this frame yet.
                SamplePose(m_motionInstance->GetMotion(), m_queryPose, newMotionTime);

                // Copy over the motion extraction joint transform from the current pose to the newly sampled pose.
                // When sampling a motion, the motion extraction joint is in animation space, while we need the query pose to be in
                // world space.
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
                velocityPoseData->CalculateVelocity(m_motionInstance, m_config->GetTrajectoryFeature()->GetRelativeToNodeIndex());
            }

            const FeatureMatrix& featureMatrix = m_config->GetFeatures().GetFeatureMatrix();
            Feature::FrameCostContext frameCostContext(featureMatrix, m_queryPose);
            frameCostContext.m_trajectoryQuery = &m_trajectoryQuery;
            frameCostContext.m_actorInstance = m_actorInstance;

            const size_t lowestCostFrameIndex = m_config->FindLowestCostFrameIndex(this, frameCostContext, currentFrameIndex);
            const FrameDatabase& frameDatabase = m_config->GetFrameDatabase();
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
                SetNewMotionTime(lowestCostFrame.GetSampleTime());
            }

            // Do this always, else wise we search for the lowest cost frame index too many times.
            SetTimeSinceLastFrameSwitch(0.0f);
        }
    }

    void MotionMatchingInstance::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionMatchingInstance>()
            ->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<MotionMatchingInstance>("MotionMatchingInstance", "Instanced data for motion matching.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
    }
} // namespace EMotionFX::MotionMatching
