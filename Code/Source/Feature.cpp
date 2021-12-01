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
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <Feature.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>

#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/Color.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(Feature, MotionMatchAllocator, 0)

        void Feature::SetDebugDrawColor(const AZ::Color& color)
        {
            m_debugColor = color;
        }

        const AZ::Color& Feature::GetDebugDrawColor() const
        {
            return m_debugColor;
        }

        void Feature::SetDebugDrawEnabled(bool enabled)
        {
            m_debugDrawEnabled = enabled;
        }

        bool Feature::GetDebugDrawEnabled() const
        {
            return m_debugDrawEnabled;
        }

        void Feature::SetData(FrameDatabase* data)
        {
            m_data = data;
        }

        FrameDatabase* Feature::GetData() const
        {
            return m_data;
        }

        void Feature::SetRelativeToNodeIndex(size_t nodeIndex)
        {
            m_relativeToNodeIndex = nodeIndex;
        }

        void Feature::SamplePose(float sampleTime, const Pose* bindPose, Motion* sourceMotion, MotionInstance* motionInstance, Pose* samplePose)
        {
            motionInstance->SetMotion(sourceMotion);
            motionInstance->SetCurrentTime(sampleTime, true);
            sourceMotion->Update(bindPose, samplePose, motionInstance);
        }

        void Feature::CalculateVelocity(size_t jointIndex, const Pose* curPose, const Pose* nextPose, float timeDelta, AZ::Vector3& outVelocity)
        {
            const AZ::Vector3 currentPosition = curPose->GetWorldSpaceTransform(jointIndex).m_position;
            const AZ::Vector3 nextPosition = nextPose->GetWorldSpaceTransform(jointIndex).m_position;
            const AZ::Vector3 velocity = (timeDelta > AZ::Constants::FloatEpsilon) ? (nextPosition - currentPosition) / timeDelta : AZ::Vector3::CreateZero();
            float speed = velocity.GetLength();
            if (speed > AZ::Constants::FloatEpsilon)
            {
                outVelocity = velocity;
            }
            else
            {
                outVelocity = AZ::Vector3::CreateZero();
            }
        }

        void Feature::CalculateVelocity(size_t jointIndex, size_t relativeToJointIndex, MotionInstance* motionInstance, AZ::Vector3& outVelocity)
        {
            const float originalTime = motionInstance->GetCurrentTime();

            // Prepare for sampling.
            ActorInstance* actorInstance = motionInstance->GetActorInstance();
            AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool();
            AnimGraphPose* prevPose = posePool.RequestPose(actorInstance);
            AnimGraphPose* currentPose = posePool.RequestPose(actorInstance);
            Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();

            const size_t numSamples = 3;
            const float timeRange = 0.05f; // secs
            const float halfTimeRange = timeRange * 0.5f;
            const float startTime = originalTime - halfTimeRange;
            const float frameDelta = timeRange / numSamples;

            AZ::Vector3 accumulatedVelocity = AZ::Vector3::CreateZero();

            for (size_t sampleIndex = 0; sampleIndex < numSamples + 1; ++sampleIndex)
            {
                float sampleTime = startTime + sampleIndex * frameDelta;
                if (sampleTime < 0.0f)
                {
                    sampleTime = 0.0f;
                }
                if (sampleTime >= motionInstance->GetMotion()->GetDuration())
                {
                    sampleTime = motionInstance->GetMotion()->GetDuration();
                }

                if (sampleIndex == 0)
                {
                    motionInstance->SetCurrentTime(sampleTime);
                    motionInstance->GetMotion()->Update(bindPose, &prevPose->GetPose(), motionInstance);
                    continue;
                }

                motionInstance->SetCurrentTime(sampleTime);
                motionInstance->GetMotion()->Update(bindPose, &currentPose->GetPose(), motionInstance);

                const Transform inverseJointWorldTransform = currentPose->GetPose().GetWorldSpaceTransform(relativeToJointIndex).Inversed();

                // Calculate the velocity.
                AZ::Vector3 velocity;
                CalculateVelocity(jointIndex, &prevPose->GetPose(), &currentPose->GetPose(), frameDelta, velocity);
                accumulatedVelocity += inverseJointWorldTransform.TransformVector(velocity);

                *prevPose = *currentPose;
            }

            outVelocity = accumulatedVelocity / aznumeric_cast<float>(numSamples);

            motionInstance->SetCurrentTime(originalTime); // set back to what it was

            posePool.FreePose(prevPose);
            posePool.FreePose(currentPose);
        }

        float Feature::GetNormalizedDirectionDifference(const AZ::Vector2& directionA, const AZ::Vector2& directionB) const
        {
            const float dotProduct = directionA.GetNormalized().Dot(directionB.GetNormalized());
            const float normalizedDirectionDifference = (2.0f - (1.0f + dotProduct)) * 0.5f;
            return AZ::GetAbs(normalizedDirectionDifference);
        }

        float Feature::GetNormalizedDirectionDifference(const AZ::Vector3& directionA, const AZ::Vector3& directionB) const
        {
            const float dotProduct = directionA.GetNormalized().Dot(directionB.GetNormalized());
            const float normalizedDirectionDifference = (2.0f - (1.0f + dotProduct)) * 0.5f;
            return AZ::GetAbs(normalizedDirectionDifference);
        }

        void Feature::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<Feature>()
                ->Version(1)
                ->Field("id", &Feature::m_id);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<Feature>("Feature", "Base class for the frame data")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
