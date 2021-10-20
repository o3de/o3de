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
#include <EMotionFX/Source/EventManager.h>
#include <FeatureDirection.h>
#include <FrameDatabase.h>
#include <EMotionFX/Source/Pose.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <MCore/Source/AzCoreConversions.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(FeatureDirection, MotionMatchAllocator, 0)

        FeatureDirection::FeatureDirection()
            : Feature()
            , m_nodeIndex(MCORE_INVALIDINDEX32)
            , m_axis(AXIS_X)
            , m_flipAxis(false)
        {
        }

        bool FeatureDirection::Init(const InitSettings& settings)
        {
            AZ_UNUSED(settings);

            if (m_nodeIndex == MCORE_INVALIDINDEX32)
            {
                return false;
            }

            m_directions.resize(m_data->GetNumFrames());
            return true;
        }

        void FeatureDirection::SetNodeIndex(size_t nodeIndex)
        {
            m_nodeIndex = nodeIndex;
        }

        size_t FeatureDirection::GetNumDimensions() const
        {
            return 3;
        }
        
        AZ::Vector3 FeatureDirection::ExtractDirection(const AZ::Quaternion& quaternion) const
        {
            AZ::Vector3 axis = AZ::Vector3::CreateZero();
            axis.SetElement(static_cast<int>(m_axis), 1.0f);
            if (m_flipAxis)
            {
                axis *= -1.0f;
            }

            return quaternion.TransformVector(axis);
        }

        void FeatureDirection::ExtractFeatureValues(const ExtractFrameContext& context)
        {
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            const AZ::Vector3 nodeWorldDirection = ExtractDirection(context.m_pose->GetWorldSpaceTransform(m_nodeIndex).m_rotation);
            m_directions[context.m_frameIndex] = invRootTransform.TransformVector(nodeWorldDirection);
        }

        void FeatureDirection::DebugDrawDirection(EMotionFX::DebugDraw::ActorInstanceData& draw, size_t frameIndex, const AZ::Vector3 startPoint, const AZ::Transform& transform, const AZ::Color& color)
        {
            draw.DrawLine(startPoint, startPoint + transform.TransformVector(m_directions[frameIndex]), color);
        }

        float FeatureDirection::CalculateFrameCost([[maybe_unused]] size_t frameIndex, [[maybe_unused]] const FrameCostContext& context) const
        {
/*
            const Transform invRootTransform = context.m_pose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
            const AZ::Vector3 globalInputPosition = context.m_pose->GetWorldSpaceTransform(m_nodeIndex).mPosition;
            const AZ::Vector3 relativeInputPosition = invRootTransform.TransformPoint(globalInputPosition);    // Make the position relative to the root. TODO: optimize this by precalculating each frame
            const AZ::Vector3& framePosition = directions[frameIndex]; // This is already relative to the root node
            return (framePosition - relativeInputPosition).GetLength();
            */
            return 0.0f;
        }

        void FeatureDirection::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<FeatureDirection, Feature>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<FeatureDirection>("DirectionFrameData", "Joint direction data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
