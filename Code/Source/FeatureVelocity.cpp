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
            queryFeatureValues[startIndex + 0] = context.m_velocity.GetX();
            queryFeatureValues[startIndex + 1] = context.m_velocity.GetY();
            queryFeatureValues[startIndex + 2] = context.m_velocity.GetZ();
        }

        void FeatureVelocity::ExtractFeatureValues(const ExtractFeatureContext& context)
        {
            AZ::Vector3 velocity;
            CalculateVelocity(context.m_actorInstance, m_nodeIndex, m_relativeToNodeIndex, context.m_frameDatabase->GetFrame(context.m_frameIndex), velocity);
            
            SetFeatureData(context.m_featureMatrix, context.m_frameIndex, velocity);
        }

        void FeatureVelocity::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
            BehaviorInstance* behaviorInstance,
            const AZ::Vector3& velocity,
            size_t jointIndex,
            size_t relativeToJointIndex,
            const AZ::Color& color)
        {
            // Don't visualize joints that remain motionless (zero velocity).
            if (velocity.GetLength() < AZ::Constants::FloatEpsilon)
            {
                return;
            }

            const float scale = 0.15f;
            const ActorInstance* actorInstance = behaviorInstance->GetActorInstance();
            const Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();
            const Transform jointModelTM = pose->GetModelSpaceTransform(jointIndex);
            const Transform relativeToWorldTM = pose->GetWorldSpaceTransform(relativeToJointIndex);
            const AZ::Vector3 jointPosition = relativeToWorldTM.TransformPoint(jointModelTM.m_position);
            const AZ::Vector3 velocityWorldSpace = relativeToWorldTM.TransformVector(velocity * scale);
            const AZ::Vector3 arrowPosition = jointPosition + velocityWorldSpace;

            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(color);

            debugDisplay.DrawSolidCylinder(/*center=*/(arrowPosition + jointPosition) * 0.5f,
                /*direction=*/(arrowPosition - jointPosition).GetNormalizedSafe(),
                /*radius=*/0.003f,
                /*height=*/(arrowPosition - jointPosition).GetLength(),
                /*drawShaded=*/false);

            debugDisplay.DrawSolidCone(jointPosition + velocityWorldSpace,
                velocityWorldSpace ,
                0.1f * scale,
                scale * 0.5f,
                /*drawShaded=*/false);
        }

        void FeatureVelocity::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
            BehaviorInstance* behaviorInstance,
            size_t frameIndex)
        {
            if (m_nodeIndex == InvalidIndex)
            {
                return;
            }

            const Behavior* behavior = behaviorInstance->GetBehavior();
            const AZ::Vector3 velocity = GetFeatureData(behavior->GetFeatures().GetFeatureMatrix(), frameIndex);
            DebugDraw(debugDisplay, behaviorInstance, velocity, m_nodeIndex, m_relativeToNodeIndex, m_debugColor);
        }

        float FeatureVelocity::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
        {
            const AZ::Vector3 frameVelocity = GetFeatureData(context.m_featureMatrix, frameIndex);

            // Direction difference
            const float directionDifferenceCost = GetNormalizedDirectionDifference(frameVelocity.GetNormalized(), context.m_velocity.GetNormalized());

            // Speed difference
            // TODO: This needs to be normalized later on, else wise it could be that the direction difference is weights
            // too heavily or too less compared to what the speed values are
            const float speedDifferenceCost = AZ::GetAbs(frameVelocity.GetLength() - context.m_velocity.GetLength());

            return directionDifferenceCost + speedDifferenceCost;
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

            editContext->Class<FeatureVelocity>("FeatureVelocity", "Joint velocity data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }

        size_t FeatureVelocity::GetNumDimensions() const
        {
            return 3;
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
                case 0: { result += "VelocityX"; break; }
                case 1: { result += "VelocityY"; break; }
                case 2: { result += "VelocityZ"; break; }
                default: { result += Feature::GetDimensionName(index, skeleton); }
            }

            return result;
        }

        AZ::Vector3 FeatureVelocity::GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex) const
        {
            return featureMatrix.GetVector3(frameIndex, m_featureColumnOffset);
        }

        void FeatureVelocity::SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, const AZ::Vector3& velocity)
        {
            featureMatrix.SetVector3(frameIndex, m_featureColumnOffset, velocity);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
