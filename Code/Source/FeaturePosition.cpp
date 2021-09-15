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
#include <FrameDatabase.h>
#include <FeaturePosition.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <MCore/Source/AzCoreConversions.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(FeaturePosition, MotionMatchAllocator, 0)

        FeaturePosition::FeaturePosition()
            : Feature()
            , m_nodeIndex(MCORE_INVALIDINDEX32)
        {
        }

        size_t FeaturePosition::CalcMemoryUsageInBytes() const
        {
            size_t total = 0;
            total += m_positions.capacity() * sizeof(AZ::Vector3);
            total += sizeof(m_positions);
            return total;
        }

        bool FeaturePosition::Init(const InitSettings& settings)
        {
            MCORE_UNUSED(settings);

            if (m_nodeIndex == MCORE_INVALIDINDEX32)
            {
                return false;
            }

            m_positions.resize(m_data->GetNumFrames());
            return true;
        }

        void FeaturePosition::SetNodeIndex(size_t nodeIndex)
        {
            m_nodeIndex = nodeIndex;
        }

        size_t FeaturePosition::GetNumDimensionsForKdTree() const
        {
            return 3;
        }

        void FeaturePosition::FillFrameFloats(size_t frameIndex, size_t startIndex, AZStd::vector<float>& frameFloats) const
        {
            const AZ::Vector3& value = m_positions[frameIndex];
            frameFloats[startIndex] = value.GetX();
            frameFloats[startIndex + 1] = value.GetY();
            frameFloats[startIndex + 2] = value.GetZ();
        }

        void FeaturePosition::FillFrameFloats(size_t startIndex, AZStd::vector<float>& frameFloats, const FrameCostContext& context)
        {
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            const AZ::Vector3 worldInputPosition = context.m_pose->GetWorldSpaceTransform(m_nodeIndex).m_position;
            const AZ::Vector3 relativeInputPosition = invRootTransform.TransformPoint(worldInputPosition);
            frameFloats[startIndex + 0] = relativeInputPosition.GetX();
            frameFloats[startIndex + 1] = relativeInputPosition.GetY();
            frameFloats[startIndex + 2] = relativeInputPosition.GetZ();
        }

        void FeaturePosition::CalcMedians(AZStd::vector<float>& medians, size_t startIndex) const
        {
            float sums[3] = { 0.0f, 0.0f, 0.0f };
            const size_t numFrames = m_positions.size();
            for (size_t i = 0; i < numFrames; ++i)
            {
                sums[0] += m_positions[i].GetX();
                sums[1] += m_positions[i].GetY();
                sums[2] += m_positions[i].GetZ();
            }

            const float fNumFrames = static_cast<float>(numFrames);
            for (size_t i = 0; i < 3; ++i)
            {
                medians[startIndex + i] = sums[i] / fNumFrames;
            }
        }

        void FeaturePosition::ExtractFrameData(const ExtractFrameContext& context)
        {
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            const AZ::Vector3 nodeWorldPosition = context.m_pose->GetWorldSpaceTransform(m_nodeIndex).m_position;
            m_positions[context.m_frameIndex] = invRootTransform.TransformPoint(nodeWorldPosition);
        }

        void FeaturePosition::DebugDraw([[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw, [[maybe_unused]] BehaviorInstance* behaviorInstance, [[maybe_unused]] size_t frameIndex)
        {
            /*AZ_UNUSED(behaviorInstance);

            const size_t numPositions = m_positions.size();
            if (numPositions < 2)
            {
                return;
            }

            for (size_t i = 0; i < numPositions-1; ++i)
            {
                draw.DrawLine(m_positions[i], m_positions[i+1], m_debugColor);
            }*/
        }

        float FeaturePosition::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            const AZ::Vector3 worldInputPosition = context.m_pose->GetWorldSpaceTransform(m_nodeIndex).m_position;
            const AZ::Vector3 relativeInputPosition = invRootTransform.TransformPoint(worldInputPosition);
            const AZ::Vector3& framePosition = m_positions[frameIndex]; // This is already relative to the root node
            return (framePosition - relativeInputPosition).GetLength();
        }

        void FeaturePosition::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<FeaturePosition, Feature>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<FeaturePosition>("PositionFrameData", "Joint position data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
