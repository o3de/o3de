/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

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

namespace EMotionFX::MotionMatching
{
    AZ_CVAR_EXTERNED(float, mm_debugDrawVelocityScale);

    AZ_CLASS_ALLOCATOR_IMPL(FeatureVelocity, MotionMatchAllocator, 0)

    void FeatureVelocity::FillQueryFeatureValues(size_t startIndex, AZStd::vector<float>& queryFeatureValues, const FrameCostContext& context)
    {
        PoseDataJointVelocities* velocityPoseData = context.m_currentPose.GetPoseData<PoseDataJointVelocities>();
        AZ_Assert(velocityPoseData, "Cannot calculate velocity feature cost without joint velocity pose data.");
        const AZ::Vector3 currentVelocity = velocityPoseData->GetVelocity(m_jointIndex);

        queryFeatureValues[startIndex + 0] = currentVelocity.GetX();
        queryFeatureValues[startIndex + 1] = currentVelocity.GetY();
        queryFeatureValues[startIndex + 2] = currentVelocity.GetZ();
    }

    void FeatureVelocity::ExtractFeatureValues(const ExtractFeatureContext& context)
    {
        const ActorInstance* actorInstance = context.m_actorInstance;
        const Frame& frame = context.m_frameDatabase->GetFrame(context.m_frameIndex);

        AnimGraphPose* tempPose = context.m_posePool.RequestPose(actorInstance);
        {
            // Calculate the joint velocities for the sampled pose using the same method as we do for the frame database.
            PoseDataJointVelocities* velocityPoseData = tempPose->GetPose().GetAndPreparePoseData<PoseDataJointVelocities>(actorInstance);
            velocityPoseData->CalculateVelocity(actorInstance,
                context.m_posePool,
                frame.GetSourceMotion(),
                frame.GetSampleTime(),
                m_relativeToNodeIndex);

            const AZ::Vector3& velocity = velocityPoseData->GetVelocities()[m_jointIndex];
            SetFeatureData(context.m_featureMatrix, context.m_frameIndex, velocity);
        }
        context.m_posePool.FreePose(tempPose);
    }

    void FeatureVelocity::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
        const Pose& pose,
        const AZ::Vector3& velocity,
        size_t jointIndex,
        size_t relativeToJointIndex,
        const AZ::Color& color)
    {
        const Transform jointModelTM = pose.GetModelSpaceTransform(jointIndex);
        const Transform relativeToWorldTM = pose.GetWorldSpaceTransform(relativeToJointIndex);

        const AZ::Vector3 jointPosition = relativeToWorldTM.TransformPoint(jointModelTM.m_position);
        const AZ::Vector3 velocityWorldSpace = relativeToWorldTM.TransformVector(velocity);

        DebugDrawVelocity(debugDisplay, jointPosition, velocityWorldSpace * mm_debugDrawVelocityScale, color);
    }

    void FeatureVelocity::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
        const Pose& currentPose,
        const FeatureMatrix& featureMatrix,
        size_t frameIndex)
    {
        if (m_jointIndex == InvalidIndex)
        {
            return;
        }

        const AZ::Vector3 velocity = GetFeatureData(featureMatrix, frameIndex);
        DebugDraw(debugDisplay, currentPose, velocity, m_jointIndex, m_relativeToNodeIndex, m_debugColor);
    }

    float FeatureVelocity::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
    {
        PoseDataJointVelocities* velocityPoseData = context.m_currentPose.GetPoseData<PoseDataJointVelocities>();
        AZ_Assert(velocityPoseData, "Cannot calculate velocity feature cost without joint velocity pose data.");

        const AZ::Vector3 currentVelocity = velocityPoseData->GetVelocity(m_jointIndex);
        const AZ::Vector3 frameVelocity = GetFeatureData(context.m_featureMatrix, frameIndex);

        const float speedDifferenceCost = (currentVelocity - frameVelocity).GetLength();
        return CalcResidual(speedDifferenceCost);
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
