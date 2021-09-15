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
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <BehaviorInstance.h>
#include <FrameDatabase.h>
#include <FeatureVelocity.h>

#include <MCore/Source/AzCoreConversions.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(FeatureVelocity, MotionMatchAllocator, 0)

        FeatureVelocity::FeatureVelocity()
            : Feature()
        {
        }

        size_t FeatureVelocity::CalcMemoryUsageInBytes() const
        {
            size_t total = 0;
            total += m_velocities.capacity() * sizeof(Velocity);
            total += sizeof(m_velocities);
            return total;
        }

        bool FeatureVelocity::Init(const InitSettings& settings)
        {
            MCORE_UNUSED(settings);

            if (m_nodeIndex == InvalidIndex)
            {
                return false;
            }

            m_velocities.resize(m_data->GetNumFrames());
            return true;
        }

        void FeatureVelocity::SetNodeIndex(size_t nodeIndex)
        {
            m_nodeIndex = nodeIndex;
        }

        size_t FeatureVelocity::GetNumDimensionsForKdTree() const
        {
            return 4;
        }

        void FeatureVelocity::FillFrameFloats(size_t frameIndex, size_t startIndex, AZStd::vector<float>& frameFloats) const
        {
            const Velocity& value = m_velocities[frameIndex];
            frameFloats[startIndex] = value.m_direction.GetX();
            frameFloats[startIndex + 1] = value.m_direction.GetY();
            frameFloats[startIndex + 2] = value.m_direction.GetZ();
            frameFloats[startIndex + 3] = value.m_speed;
        }

        void FeatureVelocity::FillFrameFloats(size_t startIndex, AZStd::vector<float>& frameFloats, const FrameCostContext& context)
        {
            frameFloats[startIndex + 0] = context.m_direction.GetX();
            frameFloats[startIndex + 1] = context.m_direction.GetY();
            frameFloats[startIndex + 2] = context.m_direction.GetZ();
            frameFloats[startIndex + 3] = context.m_speed;
        }

        void FeatureVelocity::CalcMedians(AZStd::vector<float>& medians, size_t startIndex) const
        {
            float sums[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            const size_t numFrames = m_velocities.size();
            for (size_t i = 0; i < numFrames; ++i)
            {
                sums[0] += m_velocities[i].m_direction.GetX();
                sums[1] += m_velocities[i].m_direction.GetY();
                sums[2] += m_velocities[i].m_direction.GetZ();
                sums[3] += m_velocities[i].m_speed;
            }

            const float fNumFrames = static_cast<float>(numFrames);
            for (size_t i = 0; i < 4; ++i)
            {
                medians[startIndex + i] = sums[i] / fNumFrames;
            }
        }

        void FeatureVelocity::ExtractFrameData(const ExtractFrameContext& context)
        {
            Velocity& currentVelocity = m_velocities[context.m_frameIndex];
            CalculateVelocity(m_nodeIndex, m_relativeToNodeIndex, context.m_motionInstance, currentVelocity.m_direction, currentVelocity.m_speed);
        }

        void FeatureVelocity::DebugDraw([[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw, [[maybe_unused]] BehaviorInstance* behaviorInstance, [[maybe_unused]] size_t frameIndex)
        {
            if (m_nodeIndex == InvalidIndex)
            {
                return;
            }

            const ActorInstance* actorInstance = behaviorInstance->GetActorInstance();
            const Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();
            const Transform jointModelTM = pose->GetModelSpaceTransform(m_nodeIndex);
            const Transform relativeToWorldTM = pose->GetWorldSpaceTransform(m_relativeToNodeIndex);

            const Velocity& velocity = m_velocities[frameIndex];
            const float scale = 0.15f;
            const AZ::Vector3 jointPosition = relativeToWorldTM.TransformPoint(jointModelTM.m_position);
            const AZ::Vector3 directionWorldSpace = relativeToWorldTM.TransformVector(velocity.m_direction * velocity.m_speed * scale);
            draw.DrawLine(jointPosition, jointPosition + directionWorldSpace,
                AZ::Color(1.0f, 1.0f, 0.0f, 1.0f));
        }

        float FeatureVelocity::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            const Velocity& frameVelocity = m_velocities[frameIndex];
            const float dotResult = frameVelocity.m_direction.Dot(context.m_direction);
            //const float speedDiff = AZ::GetAbs(frameVelocity.m_speed - context.m_speed);

            float totalCost = 0.0f;
            totalCost += 2.0f - (1.0f + dotResult);
            //totalCost *= speedDiff;
            return totalCost;
        }

        void FeatureVelocity::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<FeatureVelocity, Feature>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<FeatureVelocity>("VelocityFrameData", "Joint velocity data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
