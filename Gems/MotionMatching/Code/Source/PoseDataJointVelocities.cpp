/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/Velocity.h>
#include <Allocators.h>
#include <Feature.h>
#include <PoseDataJointVelocities.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(PoseDataJointVelocities, MotionMatchAllocator, 0)

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

                const float scale = 0.15f;
                const AZ::Vector3 velocityWorldSpace = relativeToWorldTM.TransformVector(velocity * scale);

                DebugDrawVelocity(debugDisplay, jointPosition, velocityWorldSpace, color);
            }
        }
    }

    void PoseDataJointVelocities::CalculateVelocity(MotionInstance* motionInstance, size_t relativeToJointIndex)
    {
        SetRelativeToJointIndex(relativeToJointIndex);
        ActorInstance* actorInstance = motionInstance->GetActorInstance();
        m_velocities.resize(actorInstance->GetNumNodes());
        m_angularVelocities.resize(actorInstance->GetNumNodes());
        for (size_t i = 0; i < m_velocities.size(); ++i)
        {
            Feature::CalculateVelocity(i, m_relativeToJointIndex, motionInstance, m_velocities[i]);
            // TODO: Angular velocity not used yet.
        }
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
