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
#include <Behavior.h>
#include <BehaviorInstance.h>
#include <Feature.h>
#include <FeatureTrajectory.h>
#include <KdTree.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(BehaviorInstance, MotionMatchAllocator, 0)

        BehaviorInstance::~BehaviorInstance()
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

        MotionInstance* BehaviorInstance::CreateMotionInstance() const
        {
            MotionInstance* result = GetMotionInstancePool().RequestNew(m_behavior->GetFrameDatabase().GetFrame(0).GetSourceMotion(), m_actorInstance);
            return result;
        }

        void BehaviorInstance::Init(const InitSettings& settings)
        {
            AZ_Assert(settings.m_actorInstance, "The actor instance cannot be a nullptr.");
            AZ_Assert(settings.m_behavior, "The motion match data cannot be nullptr.");

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
            m_behavior = settings.m_behavior;
            if (settings.m_behavior->GetFrameDatabase().GetNumFrames() == 0)
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

            // Make sure we have enough space inside the frame floats array, which is used to search the kdTree.
            // It contains the value for each dimension.
            const size_t numValuesInKdTree = m_behavior->GetFeatures().CalcNumDataDimensionsForKdTree(m_behavior->GetFeatures());
            m_queryFeatureValues.resize(numValuesInKdTree);

            // Initialize the trajectory history.
            size_t rootJointIndex = m_actorInstance->GetActor()->GetMotionExtractionNodeIndex();
            if (rootJointIndex == InvalidIndex32)
            {
                rootJointIndex = 0;
            }
            m_trajectoryHistory.Init(*m_actorInstance->GetTransformData()->GetCurrentPose(),
                rootJointIndex,
                m_behavior->GetTrajectoryFeature()->GetFacingAxisDir(),
                m_trajectorySecsToTrack);
        }

        void BehaviorInstance::DebugDraw()
        {
            if (m_behavior && !m_debugDisplays.empty())
            {
                for (AzFramework::DebugDisplayRequests* debugDisplay : m_debugDisplays)
                {
                    if (debugDisplay)
                    {
                        const AZ::u32 prevState = debugDisplay->GetState();
                        m_behavior->DebugDraw(*debugDisplay, this);
                        debugDisplay->SetState(prevState);
                    }
                }
            }
        }

        void BehaviorInstance::SamplePose(MotionInstance* motionInstance, Pose& outputPose)
        {
            const Pose* bindPose = m_actorInstance->GetTransformData()->GetBindPose();
            motionInstance->GetMotion()->Update(bindPose, &outputPose, motionInstance);
            if (m_actorInstance->GetActor()->GetMotionExtractionNode() && m_actorInstance->GetMotionExtractionEnabled())
            {
                outputPose.CompensateForMotionExtraction();
            }
        }

        void BehaviorInstance::PostUpdate([[maybe_unused]] float timeDelta)
        {
            if (!m_behavior)
            {
                m_motionExtractionDelta.Identity();
                return;
            }

            const size_t lowestCostFrame = GetLowestCostFrameIndex();
            if (m_behavior->GetFrameDatabase().GetNumFrames() == 0 || lowestCostFrame == InvalidIndex)
            {
                m_motionExtractionDelta.Identity();
                return;
            }

            // Use the motion extraction delta from the target animation.
            m_motionInstance->ExtractMotion(m_motionExtractionDelta);
        }

        void BehaviorInstance::Output(Pose& outputPose)
        {
            AZ_PROFILE_SCOPE(Animation, "BehaviorInstance::Output");

            if (!m_behavior)
            {
                outputPose.InitFromBindPose(m_actorInstance);
                return;
            }

            const size_t lowestCostFrame = GetLowestCostFrameIndex();
            if (m_behavior->GetFrameDatabase().GetNumFrames() == 0 || lowestCostFrame == InvalidIndex)
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

        size_t BehaviorInstance::GetLowestCostFrameIndex() const
        {
            return m_lowestCostFrameIndex;
        }

        void BehaviorInstance::Update(float timePassedInSeconds, const AZ::Vector3& targetPos, const AZ::Vector3& targetFacingDir, TrajectoryQuery::EMode mode, float pathRadius, float pathSpeed)
        {
            AZ_PROFILE_SCOPE(Animation, "BehaviorInstance::Update");

            if (!m_behavior)
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
                m_behavior->GetTrajectoryFeature(),
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
            SetTimeSinceLastFrameSwitch(GetTimeSinceLastFrameSwitch() + timePassedInSeconds);

            if (m_blending)
            {
                const float maxBlendTime = AZ::GetMin(GetLowestCostSearchFrequency(), 0.2f);
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
                const Pose* currentPose = m_actorInstance->GetTransformData()->GetCurrentPose();
                const size_t lowestCostFrameIndex = m_behavior->FindLowestCostFrameIndex(this, *currentPose, currentFrameIndex);
                const FrameDatabase& frameDatabase = m_behavior->GetFrameDatabase();
                const Frame& currentFrame = frameDatabase.GetFrame(currentFrameIndex);
                const Frame& lowestCostFrame = frameDatabase.GetFrame(lowestCostFrameIndex);
                const bool sameMotion = (currentFrame.GetSourceMotion() == lowestCostFrame.GetSourceMotion());
                const float timeBetweenFrames = newMotionTime - lowestCostFrame.GetSampleTime();
                //        const float timeBetweenFrames = lowestCostFrame.GetSampleTime() - currentFrame.GetSampleTime();
                const bool sameLocation = sameMotion && (AZ::GetAbs(timeBetweenFrames) < 0.2f);
                
                if (lowestCostFrameIndex != currentFrameIndex && !sameLocation)
                {
                    // Start a blend.
                    m_blending = true;
                    m_blendWeight = 0.0f;
                    m_blendProgressTime = 0.0f;

                    // Store the current motion instance state, so we can sample this as source pose.
                    m_prevMotionInstance->SetMotion(m_motionInstance->GetMotion());
                    m_prevMotionInstance->SetMirrorMotion(m_motionInstance->GetMirrorMotion());
                    m_prevMotionInstance->SetCurrentTime(m_motionInstance->GetCurrentTime() + timePassedInSeconds, true);
                    m_prevMotionInstance->SetLastCurrentTime(m_prevMotionInstance->GetCurrentTime());

                    m_lowestCostFrameIndex = lowestCostFrameIndex;

                    m_motionInstance->SetMotion(lowestCostFrame.GetSourceMotion());
                    m_motionInstance->SetMirrorMotion(lowestCostFrame.GetMirrored());

                    // The new motion time will become the current time after this frame while the current time
                    // becomes the last current time. As we just start playing at the search frame, calculate
                    // the last time based on the time delta.
                    m_motionInstance->SetCurrentTime(lowestCostFrame.GetSampleTime(), true);
                    SetNewMotionTime(lowestCostFrame.GetSampleTime() + timePassedInSeconds);
                }

                // Do this always, elsewise we search for the lowest cost frame index too many times.
                SetTimeSinceLastFrameSwitch(0.0f);
            }

            //AZ_Printf("EMotionFX", "Update: %f   newTime=%f", m_motionInstance->GetCurrentTime(), newMotionTime);
        }

        void BehaviorInstance::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<BehaviorInstance>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<BehaviorInstance>("MotionMatchBehaviorInstance", "An instance of a motion matching behavior.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
