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

        bool FeaturePosition::Init(const InitSettings& settings)
        {
            MCORE_UNUSED(settings);

            if (m_nodeIndex == MCORE_INVALIDINDEX32)
            {
                return false;
            }

            return true;
        }

        void FeaturePosition::SetNodeIndex(size_t nodeIndex)
        {
            m_nodeIndex = nodeIndex;
        }

        void FeaturePosition::FillQueryFeatureValues(size_t startIndex, AZStd::vector<float>& queryFeatureValues, const FrameCostContext& context)
        {
            const Transform invRootTransform = context.m_pose.GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            const AZ::Vector3 worldInputPosition = context.m_pose.GetWorldSpaceTransform(m_nodeIndex).m_position;
            const AZ::Vector3 relativeInputPosition = invRootTransform.TransformPoint(worldInputPosition);
            queryFeatureValues[startIndex + 0] = relativeInputPosition.GetX();
            queryFeatureValues[startIndex + 1] = relativeInputPosition.GetY();
            queryFeatureValues[startIndex + 2] = relativeInputPosition.GetZ();
        }

        void FeaturePosition::ExtractFeatureValues(const ExtractFrameContext& context)
        {
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            const AZ::Vector3 nodeWorldPosition = context.m_pose->GetWorldSpaceTransform(m_nodeIndex).m_position;
            const AZ::Vector3 position = invRootTransform.TransformPoint(nodeWorldPosition);
            SetFeatureData(context.m_featureMatrix, context.m_frameIndex, position);
        }

        void FeaturePosition::DebugDraw([[maybe_unused]] AZ::RPI::AuxGeomDrawPtr& drawQueue,
            [[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw,
            [[maybe_unused]] BehaviorInstance* behaviorInstance,
            [[maybe_unused]] size_t frameIndex)
        {
        }

        float FeaturePosition::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            const Transform invRootTransform = context.m_pose.GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            const AZ::Vector3 worldInputPosition = context.m_pose.GetWorldSpaceTransform(m_nodeIndex).m_position;
            const AZ::Vector3 relativeInputPosition = invRootTransform.TransformPoint(worldInputPosition);
            const AZ::Vector3 framePosition = GetFeatureData(context.m_featureMatrix, frameIndex); // This is already relative to the root node
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

        size_t FeaturePosition::GetNumDimensions() const
        {
            return 3;
        }

        AZStd::string FeaturePosition::GetDimensionName(size_t index, Skeleton* skeleton) const
        {
            AZStd::string result;

            Node* joint = skeleton->GetNode(m_nodeIndex);
            if (joint)
            {
                result = joint->GetName();
                result += '.';
            }

            switch (index)
            {
                case 0: { result += "PosX"; break; }
                case 1: { result += "PosY"; break; }
                case 2: { result += "PosZ"; break; }
                default: { result += Feature::GetDimensionName(index, skeleton); }
            }

            return result;
        }

        AZ::Vector3 FeaturePosition::GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex) const
        {
            return featureMatrix.GetVector3(frameIndex, m_featureColumnOffset);
        }

        void FeaturePosition::SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, const AZ::Vector3& position)
        {
            featureMatrix.SetVector3(frameIndex, m_featureColumnOffset, position);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
