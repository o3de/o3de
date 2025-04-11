/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Ragdoll.h>
#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/PoseDataRagdoll.h>
#include <EMotionFX/Source/RagdollInstance.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(PoseDataRagdoll, PoseAllocator)

    PoseDataRagdoll::PoseDataRagdoll()
        : PoseData()
    {
    }

    PoseDataRagdoll::~PoseDataRagdoll()
    {
        Clear();
    }

    void PoseDataRagdoll::Clear()
    {
        m_nodeStates.clear();
    }

    void PoseDataRagdoll::LinkToActorInstance(const ActorInstance* actorInstance)
    {
        const RagdollInstance* ragdollInstance = actorInstance->GetRagdollInstance();
        if (ragdollInstance)
        {
            const Physics::Ragdoll* ragdoll = ragdollInstance->GetRagdoll();
            if (ragdoll)
            {
                m_nodeStates.resize(ragdoll->GetNumNodes());
            }
        }
    }

    void PoseDataRagdoll::LinkToActor(const Actor* actor)
    {
        AZ_UNUSED(actor);
        Clear();
    }

    void PoseDataRagdoll::Reset()
    {
        const Physics::RagdollNodeState defaultNodeState;
        for (Physics::RagdollNodeState& nodeState : m_nodeStates)
        {
            nodeState = defaultNodeState;
            // TODO: Read default values from the ragdoll configuration for strength and damping here once they are implemented in AzFramework.
        }
    }

    void PoseDataRagdoll::FastCopyNodeStates(AZStd::vector<Physics::RagdollNodeState>& to, const AZStd::vector<Physics::RagdollNodeState>& from)
    {
        if (from.empty())
        {
            // operator= deallocates memory in case the vector we copy from is empty. Special case handling to prevent runtime de-/allocations.
            to.clear();
        }
        else
        {
            to = from;
        }
    }

    void PoseDataRagdoll::CopyFrom(const PoseData* from)
    {
        AZ_Assert(from->RTTI_GetType() == azrtti_typeid<PoseDataRagdoll>(), "Cannot copy from pose data other than ragdoll pose data.");
        const PoseDataRagdoll* fromRagdollPoseData = static_cast<const PoseDataRagdoll*>(from);

        m_isUsed = fromRagdollPoseData->m_isUsed;
        FastCopyNodeStates(m_nodeStates, fromRagdollPoseData->m_nodeStates);
    }

    void PoseDataRagdoll::BlendNodeState(Physics::RagdollNodeState& nodeState, const Physics::RagdollNodeState& destNodeState, const Transform& jointTransform, const Transform& destJointTransform, float weight)
    {
        const float strengthEpsilon = 0.01f;
        // TODO: We'll need to experiment with this. A high max strength will lead to instabilities in the physics simulation while a too low number will result in pops when switching simulation states.
        const float strengthMax = std::numeric_limits<float>::max();

        // Blending from kinematic to dynamic joint
        if (nodeState.m_simulationType == Physics::SimulationType::Kinematic &&
            destNodeState.m_simulationType == Physics::SimulationType::Simulated)
        {
            nodeState.m_position = jointTransform.m_position.Lerp(destNodeState.m_position, weight);
            nodeState.m_orientation = jointTransform.m_rotation.NLerp(destNodeState.m_orientation, weight);

            // We're blending from a kinematic to a dynamic joint, which means when starting the blend we know that the animation pose matches the ragdoll pose.
            // The closest a powered ragdoll joint can be to its target pose and thus matching the kinematic one is by using its maximum strength.
            // When blending from a kinematic to a dynamic joint, at the moment when the simulation state changes we use the maximum strength and blend towards
            // the destination strength.
            nodeState.m_strength = AZ::Lerp(strengthMax, destNodeState.m_strength, weight);

            // TODO: Experiment when we want to switch simulation state. At a weight of 0.5f or at the beginning of the blend?
            if (weight > strengthEpsilon)
            {
                // TODO: Evaluate if we should calculate the initial velocities here when we switch to dynamic. Or will we need to calculate that information every frame anyway additionally to the motors!?
                nodeState.m_simulationType = Physics::SimulationType::Simulated;
            }
        }
        // Blending from dynamic to kinematic joint
        else if (nodeState.m_simulationType == Physics::SimulationType::Simulated &&
                 destNodeState.m_simulationType == Physics::SimulationType::Kinematic)
        {
            nodeState.m_position = nodeState.m_position.Lerp(destJointTransform.m_position, weight);
            nodeState.m_orientation = nodeState.m_orientation.NLerp(destJointTransform.m_rotation, weight);

            // Inverse way here. Blending towards the maximum strength possible to make sure we're as close as possible to the target pose when switching simulation
            // state to kinematic.
            nodeState.m_strength = AZ::Lerp(nodeState.m_strength, strengthMax, weight);

            // TODO: Experiment when we want to switch simulation state. At a weight of 0.5f or at the end of the blend?
            if (weight > 1.0f - strengthEpsilon)
            {
                nodeState.m_simulationType = Physics::SimulationType::Kinematic;
            }
        }
        // Blending between two dynamic joints
        else if (nodeState.m_simulationType == Physics::SimulationType::Simulated &&
                 destNodeState.m_simulationType == Physics::SimulationType::Simulated)
        {
            nodeState.m_position = nodeState.m_position.Lerp(destNodeState.m_position, weight);
            nodeState.m_orientation = nodeState.m_orientation.NLerp(destNodeState.m_orientation, weight);
            nodeState.m_strength = AZ::Lerp(nodeState.m_strength, destNodeState.m_strength, weight);
        }

        nodeState.m_dampingRatio = AZ::Lerp(nodeState.m_dampingRatio, destNodeState.m_dampingRatio, weight);
    }

    void PoseDataRagdoll::Blend(const Pose* destPose, float weight)
    {
        const size_t nodeStateCount = m_nodeStates.size();
        PoseDataRagdoll* destRagdollPoseData = destPose->GetPoseData<PoseDataRagdoll>();

        if (destRagdollPoseData && destRagdollPoseData->IsUsed())
        {
            const AZStd::vector<Physics::RagdollNodeState>& destNodeStates = destRagdollPoseData->GetRagdollNodeStates();
            AZ_Assert(nodeStateCount == destNodeStates.size(), "Expected the same ragdoll node counts for the current and the destination pose datas.");

            if (m_isUsed)
            {
                const ActorInstance* actorInstance = m_pose->GetActorInstance();
                const RagdollInstance* ragdollInstance = actorInstance->GetRagdollInstance();
                AZ_Assert(actorInstance && ragdollInstance, "Expected a valid actor and ragdoll instance in case the ragdoll pose data is used.");

                // Blend node states. Both, the destination pose as well as the current pose hold used ragdoll pose datas.
                for (size_t i = 0; i < nodeStateCount; ++i)
                {
                    const size_t jointIndex = ragdollInstance->GetJointIndex(i);
                    const Transform& localTransform = m_pose->GetLocalSpaceTransform(jointIndex);
                    const Transform& destLocalTransform = destPose->GetLocalSpaceTransform(jointIndex);

                    BlendNodeState(m_nodeStates[i], destNodeStates[i], localTransform, destLocalTransform, weight);
                }
            }
            else
            {
                // The destination pose holds an active ragdoll pose data while source pose data not.
                // E.g. Transitioning from a motion node to a blend tree with a ragdoll node.
                // Animation poses and thus the visual result will be blended.
                // Just copy the node states from the target pose for the ragdoll in this case.
                FastCopyNodeStates(m_nodeStates, destNodeStates);
            }
        }
        else
        {
            // Destination pose either doesn't contain ragdoll pose data or it is unused.
            // E.g. Transitioning from a blend tree with a ragdoll node to a motion node.
            // Don't do anything.
        }
    }

    void PoseDataRagdoll::Log() const
    {
        const size_t nodeStateCount = m_nodeStates.size();
        AZ_Printf("EMotionFX", " - Pose Data Ragdoll (Nodes=%d)", nodeStateCount);

        for (size_t i = 0; i < nodeStateCount; i++)
        {
            const Physics::RagdollNodeState& nodeState = m_nodeStates[i];

            AZ_Printf("EMotionFX", "     - Ragdoll Node State %d:", i);
            AZ_Printf("EMotionFX", "         + Type %s:", nodeState.m_simulationType == Physics::SimulationType::Simulated ? "Simulated" : "Kinematic");
            AZ_Printf("EMotionFX", "         + Position: (%f, %f, %f)", static_cast<float>(nodeState.m_position.GetX()), static_cast<float>(nodeState.m_position.GetY()), static_cast<float>(nodeState.m_position.GetZ()));
            AZ_Printf("EMotionFX", "         + Rotation: (%f, %f, %f, %f)", static_cast<float>(nodeState.m_orientation.GetX()), static_cast<float>(nodeState.m_orientation.GetY()), static_cast<float>(nodeState.m_orientation.GetZ()), static_cast<float>(nodeState.m_orientation.GetW()));
            AZ_Printf("EMotionFX", "         + Linear Velocity: (%f, %f, %f)", static_cast<float>(nodeState.m_linearVelocity.GetX()), static_cast<float>(nodeState.m_linearVelocity.GetY()), static_cast<float>(nodeState.m_linearVelocity.GetZ()));
            AZ_Printf("EMotionFX", "         + Angular Velocity: (%f, %f, %f)", static_cast<float>(nodeState.m_angularVelocity.GetX()), static_cast<float>(nodeState.m_angularVelocity.GetY()), static_cast<float>(nodeState.m_angularVelocity.GetZ()));
            AZ_Printf("EMotionFX", "         + Strength: %f", nodeState.m_strength);
            AZ_Printf("EMotionFX", "         + Damping Ratio: %f", nodeState.m_dampingRatio);
        }
    }

    void PoseDataRagdoll::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PoseDataRagdoll, PoseData>()
                ->Version(1)
            ;
        }
    }
} // namespace EMotionFX
