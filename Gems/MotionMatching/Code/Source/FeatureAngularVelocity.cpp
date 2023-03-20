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

#include <Allocators.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <FeatureAngularVelocity.h>
#include <FeatureMatrixTransformer.h>
#include <FrameDatabase.h>
#include <MotionMatchingInstance.h>
#include <PoseDataJointVelocities.h>

namespace EMotionFX::MotionMatching
{
    AZ_CVAR_EXTERNED(float, mm_debugDrawVelocityScale);

    AZ_CLASS_ALLOCATOR_IMPL(FeatureAngularVelocity, MotionMatchAllocator)

    void FeatureAngularVelocity::ExtractFeatureValues(const ExtractFeatureContext& context)
    {
        const ActorInstance* actorInstance = context.m_actorInstance;
        const Frame& frame = context.m_frameDatabase->GetFrame(context.m_frameIndex);

        AnimGraphPose* tempPose = context.m_posePool.RequestPose(actorInstance);
        {
            // Calculate the joint velocities for the sampled pose using the same method as we do for the frame database.
            PoseDataJointVelocities* velocityPoseData = tempPose->GetPose().GetAndPreparePoseData<PoseDataJointVelocities>(actorInstance);
            velocityPoseData->CalculateVelocity(
                actorInstance, context.m_posePool, frame.GetSourceMotion(), frame.GetSampleTime(), m_relativeToNodeIndex);

            const AZ::Vector3& angularVelocity = velocityPoseData->GetAngularVelocities()[m_jointIndex];
            context.m_featureMatrix.SetVector3(context.m_frameIndex, m_featureColumnOffset, angularVelocity);
        }
        context.m_posePool.FreePose(tempPose);
    }

    void FeatureAngularVelocity::FillQueryVector(QueryVector& queryVector, const QueryVectorContext& context)
    {
        PoseDataJointVelocities* velocityPoseData = context.m_currentPose.GetPoseData<PoseDataJointVelocities>();
        AZ_Assert(velocityPoseData, "Cannot calculate velocity feature cost without joint velocity pose data.");
        const AZ::Vector3 currentVelocity = velocityPoseData->GetAngularVelocity(m_jointIndex);

        queryVector.SetVector3(currentVelocity, m_featureColumnOffset);
    }

    float FeatureAngularVelocity::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
    {
        const AZ::Vector3 queryVelocity = context.m_queryVector.GetVector3(m_featureColumnOffset);
        const AZ::Vector3 frameVelocity = context.m_featureMatrix.GetVector3(frameIndex, m_featureColumnOffset);

        return CalcResidual(queryVelocity, frameVelocity);
    }

    void FeatureAngularVelocity::DebugDraw(
        AzFramework::DebugDisplayRequests& debugDisplay,
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

    void FeatureAngularVelocity::DebugDraw(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const Pose& currentPose,
        const FeatureMatrix& featureMatrix,
        const FeatureMatrixTransformer* featureTransformer,
        size_t frameIndex)
    {
        if (m_jointIndex == InvalidIndex)
        {
            return;
        }

        AZ::Vector3 angularVelocity = featureMatrix.GetVector3(frameIndex, m_featureColumnOffset);
        if (featureTransformer)
        {
            angularVelocity = featureTransformer->InverseTransform(angularVelocity, m_featureColumnOffset);
        }
        DebugDraw(debugDisplay, currentPose, angularVelocity, m_jointIndex, m_relativeToNodeIndex, m_debugColor);
    }

    void FeatureAngularVelocity::Reflect(AZ::ReflectContext* context)
    {
        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<FeatureAngularVelocity, Feature>()->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<FeatureAngularVelocity>("FeatureAngularVelocity", "Matches joint angular velocities.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "");
    }

    size_t FeatureAngularVelocity::GetNumDimensions() const
    {
        return 3;
    }

    AZStd::string FeatureAngularVelocity::GetDimensionName(size_t index) const
    {
        AZStd::string result = m_jointName;
        result += '.';

        switch (index)
        {
        case 0:
            {
                result += "AngularVelocityX";
                break;
            }
        case 1:
            {
                result += "AngularVelocityY";
                break;
            }
        case 2:
            {
                result += "AngularVelocityZ";
                break;
            }
        default:
            {
                result += Feature::GetDimensionName(index);
            }
        }

        return result;
    }
} // namespace EMotionFX::MotionMatching
