/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <MotionMatchingData.h>
#include <MotionMatchingInstance.h>
#include <FrameDatabase.h>
#include <FeatureVelocity.h>
#include <PoseDataJointVelocities.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FeatureVelocity, MotionMatchAllocator, 0)

    void FeatureVelocity::FillQueryFeatureValues(size_t startIndex, AZStd::vector<float>& queryFeatureValues, const FrameCostContext& context)
    {
        PoseDataJointVelocities* velocityPoseData = static_cast<PoseDataJointVelocities*>(context.m_currentPose.GetPoseDataByType(azrtti_typeid<PoseDataJointVelocities>()));
        AZ_Assert(velocityPoseData, "Cannot calculate velocity feature cost without joint velocity pose data.");
        const AZ::Vector3 currentVelocity = velocityPoseData->GetVelocity(m_jointIndex);

        queryFeatureValues[startIndex + 0] = currentVelocity.GetX();
        queryFeatureValues[startIndex + 1] = currentVelocity.GetY();
        queryFeatureValues[startIndex + 2] = currentVelocity.GetZ();
    }

    void FeatureVelocity::ExtractFeatureValues(const ExtractFeatureContext& context)
    {
        AZ::Vector3 velocity;
        CalculateVelocity(context.m_actorInstance, m_jointIndex, m_relativeToNodeIndex, context.m_frameDatabase->GetFrame(context.m_frameIndex), velocity);
            
        SetFeatureData(context.m_featureMatrix, context.m_frameIndex, velocity);
    }

    void FeatureVelocity::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
        MotionMatchingInstance* instance,
        const AZ::Vector3& velocity,
        size_t jointIndex,
        size_t relativeToJointIndex,
        const AZ::Color& color)
    {
        const ActorInstance* actorInstance = instance->GetActorInstance();
        const Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();
        const Transform jointModelTM = pose->GetModelSpaceTransform(jointIndex);
        const Transform relativeToWorldTM = pose->GetWorldSpaceTransform(relativeToJointIndex);

        const AZ::Vector3 jointPosition = relativeToWorldTM.TransformPoint(jointModelTM.m_position);
        const float scale = 0.15f;
        const AZ::Vector3 velocityWorldSpace = relativeToWorldTM.TransformVector(velocity * scale);

        DebugDrawVelocity(debugDisplay, jointPosition, velocityWorldSpace, color);
    }

    void FeatureVelocity::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
        MotionMatchingInstance* instance,
        size_t frameIndex)
    {
        if (m_jointIndex == InvalidIndex)
        {
            return;
        }

        const MotionMatchingData* data = instance->GetData();
        const AZ::Vector3 velocity = GetFeatureData(data->GetFeatureMatrix(), frameIndex);
        DebugDraw(debugDisplay, instance, velocity, m_jointIndex, m_relativeToNodeIndex, m_debugColor);
    }

    float FeatureVelocity::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
    {
        PoseDataJointVelocities* velocityPoseData = static_cast<PoseDataJointVelocities*>(context.m_currentPose.GetPoseDataByType(azrtti_typeid<PoseDataJointVelocities>()));
        AZ_Assert(velocityPoseData, "Cannot calculate velocity feature cost without joint velocity pose data.");
        const AZ::Vector3 currentVelocity = velocityPoseData->GetVelocity(m_jointIndex);

        const AZ::Vector3 frameVelocity = GetFeatureData(context.m_featureMatrix, frameIndex);

        // Direction difference
        const float directionDifferenceCost = GetNormalizedDirectionDifference(frameVelocity.GetNormalized(), currentVelocity.GetNormalized());

        // Speed difference
        // TODO: This needs to be normalized later on, else wise it could be that the direction difference is weights
        // too heavily or too less compared to what the speed values are
        const float speedDifferenceCost = frameVelocity.GetLength() - currentVelocity.GetLength();

        return CalcResidual(directionDifferenceCost) + CalcResidual(speedDifferenceCost);
    }

    void FeatureVelocity::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<FeatureVelocity, Feature>()
            ->Version(1)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<FeatureVelocity>("FeatureVelocity", "Matches joint velocities.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ;
    }

    size_t FeatureVelocity::GetNumDimensions() const
    {
        return 3;
    }

    AZStd::string FeatureVelocity::GetDimensionName(size_t index) const
    {
        AZStd::string result = m_jointName;
        result += '.';

        switch (index)
        {
            case 0: { result += "VelocityX"; break; }
            case 1: { result += "VelocityY"; break; }
            case 2: { result += "VelocityZ"; break; }
            default: { result += Feature::GetDimensionName(index); }
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
} // namespace EMotionFX::MotionMatching
