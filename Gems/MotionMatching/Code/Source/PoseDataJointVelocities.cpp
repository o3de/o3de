/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Velocity.h>
#include <Allocators.h>
#include <Feature.h>
#include <PoseDataJointVelocities.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(PoseDataJointVelocities, MotionMatchAllocator)

    PoseDataJointVelocities::PoseDataJointVelocities()
        : PoseData()
    {
    }

    PoseDataJointVelocities::~PoseDataJointVelocities()
    {
        Clear();
    }

    void PoseDataJointVelocities::Clear()
    {
        m_velocities.clear();
        m_angularVelocities.clear();
    }

    void PoseDataJointVelocities::LinkToActorInstance(const ActorInstance* actorInstance)
    {
        m_velocities.resize(actorInstance->GetNumNodes());
        m_angularVelocities.resize(actorInstance->GetNumNodes());

        SetRelativeToJointIndex(actorInstance->GetActor()->GetMotionExtractionNodeIndex());
    }

    void PoseDataJointVelocities::SetRelativeToJointIndex(size_t relativeToJointIndex)
    {
        if (relativeToJointIndex == InvalidIndex)
        {
            m_relativeToJointIndex = 0;
        }
        else
        {
            m_relativeToJointIndex = relativeToJointIndex;
        }
    }

    void PoseDataJointVelocities::LinkToActor(const Actor* actor)
    {
        AZ_UNUSED(actor);
        Clear();
    }

    void PoseDataJointVelocities::Reset()
    {
        const size_t numJoints = m_velocities.size();
        for (size_t i = 0; i < numJoints; ++i)
        {
            m_velocities[i] = AZ::Vector3::CreateZero();
            m_angularVelocities[i] = AZ::Vector3::CreateZero();
        }
    }

    void PoseDataJointVelocities::CopyFrom(const PoseData* from)
    {
        AZ_Assert(from->RTTI_GetType() == azrtti_typeid<PoseDataJointVelocities>(), "Cannot copy from pose data other than joint velocity pose data.");
        const PoseDataJointVelocities* fromVelocityPoseData = static_cast<const PoseDataJointVelocities*>(from);

        m_isUsed = fromVelocityPoseData->m_isUsed;
        m_velocities = fromVelocityPoseData->m_velocities;
        m_angularVelocities = fromVelocityPoseData->m_angularVelocities;
        m_relativeToJointIndex = fromVelocityPoseData->m_relativeToJointIndex;
    }

    void PoseDataJointVelocities::Blend(const Pose* destPose, float weight)
    {
        PoseDataJointVelocities* destPoseData = destPose->GetPoseData<PoseDataJointVelocities>();

        if (destPoseData && destPoseData->IsUsed())
        {
            AZ_Assert(m_velocities.size() == destPoseData->m_velocities.size(), "Expected the same number of joints and velocities in the destination pose data.");

            if (m_isUsed)
            {
                // Blend while both, the destination pose as well as the current pose hold joint velocities.
                for (size_t i = 0; i < m_velocities.size(); ++i)
                {
                    m_velocities[i] = m_velocities[i].Lerp(destPoseData->m_velocities[i], weight);
                    m_angularVelocities[i] = m_angularVelocities[i].Lerp(destPoseData->m_angularVelocities[i], weight);
                }
            }
            else
            {
                // The destination pose data is used while the current one is not. Just copy over the velocities from the destination.
                m_velocities = destPoseData->m_velocities;
                m_angularVelocities = destPoseData->m_angularVelocities;
            }
        }
        else
        {
            // Destination pose either doesn't contain velocity pose data or it is unused.
            // Don't do anything and keep the current velocities.
        }
    }

    void PoseDataJointVelocities::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color) const
    {
        AZ_Assert(m_pose->GetNumTransforms() == m_velocities.size(), "Expected a joint velocity for each joint in the pose.");

        const Pose* pose = m_pose;
        for (size_t i = 0; i < m_velocities.size(); ++i)
        {
            const size_t jointIndex = i;

            // draw linear velocity
            {
                const Transform jointModelTM = pose->GetModelSpaceTransform(jointIndex);
                const Transform relativeToWorldTM = pose->GetWorldSpaceTransform(m_relativeToJointIndex);
                const AZ::Vector3 jointPosition = relativeToWorldTM.TransformPoint(jointModelTM.m_position);

                const AZ::Vector3& velocity = m_velocities[i];
                const AZ::Vector3 velocityWorldSpace = relativeToWorldTM.TransformVector(velocity);

                const float scale = 0.1f;
                DebugDrawVelocity(debugDisplay, jointPosition, velocityWorldSpace * scale, color);
            }
        }
    }

    void PoseDataJointVelocities::CalculateVelocity(const ActorInstance* actorInstance, AnimGraphPosePool& posePool, Motion* motion, float requestedSampleTime, size_t relativeToJointIndex)
    {
        MotionDataSampleSettings sampleSettings;
        sampleSettings.m_actorInstance = actorInstance;
        sampleSettings.m_inPlace = false;
        sampleSettings.m_mirror = false;
        sampleSettings.m_retarget = false;
        sampleSettings.m_inputPose = sampleSettings.m_actorInstance->GetTransformData()->GetBindPose();

        const size_t numJoints = m_velocities.size();
        SetRelativeToJointIndex(relativeToJointIndex);
        m_velocities.resize(numJoints);
        m_angularVelocities.resize(numJoints);

        // Prepare for sampling.
        AnimGraphPose* prevPose = posePool.RequestPose(actorInstance);
        AnimGraphPose* currentPose = posePool.RequestPose(actorInstance);

        const size_t numSamples = 3;
        const float timeRange = 0.05f; // secs

        const float halfTimeRange = timeRange * 0.5f;
        const float startTime = requestedSampleTime - halfTimeRange;
        const float numInbetweens = aznumeric_cast<float>(numSamples - 1); // Number of elements or windows between two keyframes.
        const float frameDelta = timeRange / numInbetweens;
        const float motionDuration = motion->GetDuration();

        // Zero all linear and angular velocities.
        Reset();

        for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            float sampleTime = startTime + sampleIndex * frameDelta;
            sampleTime = AZ::GetClamp(sampleTime, 0.0f, motionDuration);

            if (sampleIndex == 0)
            {
                sampleSettings.m_sampleTime = sampleTime;
                motion->SamplePose(&prevPose->GetPose(), sampleSettings);
                continue;
            }

            sampleSettings.m_sampleTime = sampleTime;
            motion->SamplePose(&currentPose->GetPose(), sampleSettings);

            const Transform inverseJointWorldTransform = currentPose->GetPose().GetWorldSpaceTransform(relativeToJointIndex).Inversed();

            for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
            {
                const Transform prevWorldTransform = prevPose->GetPose().GetWorldSpaceTransform(jointIndex);
                const Transform currentWorldTransform = currentPose->GetPose().GetWorldSpaceTransform(jointIndex);

                // Calculate the linear velocity.
                const AZ::Vector3 prevPosition = inverseJointWorldTransform.TransformPoint(prevWorldTransform.m_position);
                const AZ::Vector3 currentPosition = inverseJointWorldTransform.TransformPoint(currentWorldTransform.m_position);

                const AZ::Vector3 linearVelocity = CalculateLinearVelocity(prevPosition, currentPosition, frameDelta);
                m_velocities[jointIndex] += linearVelocity;

                // Calculate the angular velocity.
                const AZ::Quaternion prevRotation = inverseJointWorldTransform.m_rotation * prevWorldTransform.m_rotation;
                const AZ::Quaternion currentRotation = inverseJointWorldTransform.m_rotation * currentWorldTransform.m_rotation;

                const AZ::Vector3 angularVelocity = CalculateAngularVelocity(prevRotation, currentRotation, frameDelta);
                m_angularVelocities[jointIndex] += angularVelocity;
            }

            *prevPose = *currentPose;
        }

        for (size_t i = 0; i < numJoints; ++i)
        {
            m_velocities[i] /= numInbetweens;
            m_angularVelocities[i] /= numInbetweens;
        }

        posePool.FreePose(prevPose);
        posePool.FreePose(currentPose);
    }

    void PoseDataJointVelocities::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PoseDataJointVelocities, PoseData>()->Version(1);
        }
    }
} // namespace EMotionFX::MotionMatching
