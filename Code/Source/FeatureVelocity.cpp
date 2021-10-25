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
#include <Behavior.h>
#include <BehaviorInstance.h>
#include <FrameDatabase.h>
#include <FeatureVelocity.h>

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

        bool FeatureVelocity::Init(const InitSettings& settings)
        {
            MCORE_UNUSED(settings);

            if (m_nodeIndex == InvalidIndex)
            {
                return false;
            }

            return true;
        }

        void FeatureVelocity::SetNodeIndex(size_t nodeIndex)
        {
            m_nodeIndex = nodeIndex;
        }

        void FeatureVelocity::FillQueryFeatureValues(size_t startIndex, AZStd::vector<float>& queryFeatureValues, const FrameCostContext& context)
        {
            queryFeatureValues[startIndex + 0] = context.m_direction.GetX();
            queryFeatureValues[startIndex + 1] = context.m_direction.GetY();
            queryFeatureValues[startIndex + 2] = context.m_direction.GetZ();
            queryFeatureValues[startIndex + 3] = context.m_speed;
        }

        void FeatureVelocity::ExtractFeatureValues(const ExtractFrameContext& context)
        {
            Velocity velocity;
            CalculateVelocity(m_nodeIndex, m_relativeToNodeIndex, context.m_motionInstance, velocity.m_direction, velocity.m_speed);

            SetFeatureData(context.m_featureMatrix, context.m_frameIndex, velocity);
        }

        void FeatureVelocity::DebugDraw(AZ::RPI::AuxGeomDrawPtr& drawQueue,
            [[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw,
            BehaviorInstance* behaviorInstance,
            size_t frameIndex)
        {
            if (m_nodeIndex == InvalidIndex)
            {
                return;
            }

            const ActorInstance* actorInstance = behaviorInstance->GetActorInstance();
            const Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();
            const Transform jointModelTM = pose->GetModelSpaceTransform(m_nodeIndex);
            const Transform relativeToWorldTM = pose->GetWorldSpaceTransform(m_relativeToNodeIndex);

            const Behavior* behavior = behaviorInstance->GetBehavior();
            const Velocity velocity = GetFeatureData(behavior->GetFeatures().GetFeatureMatrix(), frameIndex);
            const float scale = 0.15f;
            const AZ::Vector3 jointPosition = relativeToWorldTM.TransformPoint(jointModelTM.m_position);
            const AZ::Vector3 directionWorldSpace = relativeToWorldTM.TransformVector(velocity.m_direction * velocity.m_speed * scale);
            const AZ::Vector3 arrowPosition = jointPosition + directionWorldSpace;

            drawQueue->DrawCylinder(/*center=*/(arrowPosition + jointPosition) * 0.5f,
                /*direction=*/(arrowPosition - jointPosition).GetNormalizedSafe(),
                /*radius=*/0.003f,
                /*height=*/(arrowPosition - jointPosition).GetLength(),
                m_debugColor,
                AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                AZ::RPI::AuxGeomDraw::DepthTest::Off);

            drawQueue->DrawCone(jointPosition + directionWorldSpace, directionWorldSpace, 0.1f * scale, scale * 0.5f, m_debugColor, AZ::RPI::AuxGeomDraw::DrawStyle::Solid);
        }

        float FeatureVelocity::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            const Velocity& frameVelocity = GetFeatureData(context.m_featureMatrix, frameIndex);
            const float dotResult = frameVelocity.m_direction.Dot(context.m_direction);
            //const float speedDiff = AZ::GetAbs(frameVelocity.m_speed - context.m_speed);

            float totalCost = 0.0f;
            totalCost += 2.0f - (1.0f + dotResult);
            //totalCost *= speedDiff;
            return AZ::GetAbs(totalCost);
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

        size_t FeatureVelocity::GetNumDimensions() const
        {
            return 4;
        }

        AZStd::string FeatureVelocity::GetDimensionName(size_t index, Skeleton* skeleton) const
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
                case 0: { result += "Velocity.DirX"; break; }
                case 1: { result += "Velocity.DirY"; break; }
                case 2: { result += "Velocity.DirZ"; break; }
                case 3: { result += "Velocity.Speed"; break; }
                default: { result += Feature::GetDimensionName(index, skeleton); }
            }

            return result;
        }

        EMotionFX::MotionMatching::FeatureVelocity::Velocity FeatureVelocity::GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex) const
        {
            Velocity result;
            result.m_direction = featureMatrix.GetVector3(frameIndex, m_featureColumnOffset);
            result.m_speed = featureMatrix(frameIndex, m_featureColumnOffset + 3);
            return result;
        }

        void FeatureVelocity::SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, const Velocity& velocity)
        {
            featureMatrix.SetVector3(frameIndex, m_featureColumnOffset, velocity.m_direction);
            featureMatrix(frameIndex, m_featureColumnOffset + 3) = velocity.m_speed;
        }
    } // namespace MotionMatching
} // namespace EMotionFX
