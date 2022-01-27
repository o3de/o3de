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
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionInstancePool.h>
#include <Behavior.h>
#include <BehaviorInstance.h>
#include <FrameData.h>
#include <KdTree.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

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
            MotionInstance* result = GetMotionInstancePool().RequestNew(m_behavior->GetData().GetFrame(0).GetSourceMotion(), m_actorInstance);
            /*if (!result->GetIsReadyForSampling())
            {
                result->InitForSampling();
            }*/
            return result;
        }

        void BehaviorInstance::Init(const InitSettings& settings)
        {
            AZ_Assert(settings.m_actorInstance, "The actor instance cannot be a nullptr.");
            AZ_Assert(settings.m_behavior, "The motion match data cannot be nullptr.");

            m_actorInstance = settings.m_actorInstance;
            m_behavior = settings.m_behavior;
            if (settings.m_behavior->GetData().GetNumFrames() == 0)
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
            size_t numFloatsRequired = 0;
            for (const FrameData* frameData : m_behavior->GetData().GetFrameData())
            {
                if (!frameData->GetIncludeInKdTree())
                {
                    continue;
                }

                numFloatsRequired += frameData->GetNumDimensionsForKdTree();
            }
            m_frameFloats.resize(numFloatsRequired);
        }

        void BehaviorInstance::UpdateNearestFrames()
        {
            m_behavior->GetData().GetKdTree().FindNearestNeighbors(m_frameFloats, m_nearestFrames);
        }

        void BehaviorInstance::DebugDraw()
        {
            if (!m_behavior)
            {
                return;
            }

            // Start drawing.
            EMotionFX::DebugDraw& drawSystem = GetDebugDraw();
            drawSystem.Lock();
            EMotionFX::DebugDraw::ActorInstanceData* draw = drawSystem.GetActorInstanceData(m_actorInstance);
            draw->Lock();

            // Draw.
            m_behavior->DebugDraw(*draw, this);

            // End drawing.
            draw->Unlock();
            drawSystem.Unlock();
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

        void BehaviorInstance::Output(Pose& outputPose)
        {
            if (!m_behavior)
            {
                outputPose.InitFromBindPose(m_actorInstance);
                m_motionExtractionDelta.Identity();
                return;
            }

            const size_t lowestCostFrame = GetLowestCostFrameIndex();
            if (m_behavior->GetData().GetNumFrames() == 0 || lowestCostFrame == ~0)
            {
                outputPose.InitFromBindPose(m_actorInstance);
                m_motionExtractionDelta.Identity();
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
                //m_motionInstance->ExtractMotion(m_motionExtractionDelta);
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

                // Blend the motion extraction delta.
                //Transform targetMotionExtractionDelta;
                //m_motionInstance->ExtractMotion(m_motionExtractionDelta);
                //m_prevMotionInstance->ExtractMotion(targetMotionExtractionDelta);
                //m_motionExtractionDelta.Blend(targetMotionExtractionDelta, m_blendWeight);
            }
            else
            {
                m_blendSourcePose.InitFromBindPose(m_actorInstance);
                if (m_prevMotionInstance)
                {
                    SamplePose(m_prevMotionInstance, m_blendSourcePose);
                }
                outputPose = m_blendSourcePose;
                //m_prevMotionInstance->ExtractMotion(m_motionExtractionDelta);
            }

            // Always use the target motion extraction delta. This gives the nicest visual results.
            m_motionInstance->ExtractMotion(m_motionExtractionDelta);
        }

        void BehaviorInstance::Update(float timePassedInSeconds)
        {
            if (!m_behavior)
            {
                return;
            }

            size_t currentFrameIndex = GetLowestCostFrameIndex();
            if (currentFrameIndex == ~0)
            {
                currentFrameIndex = 0;
            }

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
                const size_t lowestCostFrameIndex = m_behavior->FindLowestCostFrameIndex(this, *currentPose, m_blendTargetPose, currentFrameIndex, timePassedInSeconds); // m_blendTargetPose is the pose we are basically currently in.
                const Frame& currentFrame = m_behavior->GetData().GetFrame(currentFrameIndex);
                const Frame& lowestCostFrame = m_behavior->GetData().GetFrame(lowestCostFrameIndex);
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
                    m_prevMotionInstance->SetCurrentTime(m_motionInstance->GetCurrentTime(), true);
                    m_prevMotionInstance->SetMirrorMotion(m_motionInstance->GetMirrorMotion());

                    //AZ_Printf("EMotionFX", "Frame %d = %f/%f  %f/%f", lowestCostFrameIndex, lowestCostFrame.GetSampleTime(), lowestCostFrame.GetSourceMotion()->GetMaxTime(), newMotionTime, m_motionInstance->GetDuration());
                    SetTimeSinceLastFrameSwitch(0.0f);
                    SetLowestCostFrameIndex(lowestCostFrameIndex);

                    // Update the motion instance that will generate the target pose later on.
                    m_motionInstance->SetMotion(lowestCostFrame.GetSourceMotion());
                    m_motionInstance->SetCurrentTime(lowestCostFrame.GetSampleTime(), true);
                    m_motionInstance->SetMirrorMotion(lowestCostFrame.GetMirrored());

                    SetNewMotionTime(m_motionInstance->GetCurrentTime());
                }
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
