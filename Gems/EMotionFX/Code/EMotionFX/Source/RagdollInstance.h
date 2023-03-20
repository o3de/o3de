/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Ragdoll.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/RagdollVelocityEvaluators.h>


namespace EMotionFX
{
    class ActorInstance;
    class Node;
    class PoseDataRagdoll;

    class RagdollInstance
    {
    public:
        AZ_RTTI(RagdollInstance, "{B11169A1-2090-41A3-BA42-6B2E8E6AD191}")
        AZ_CLASS_ALLOCATOR_DECL

        RagdollInstance();
        RagdollInstance(Physics::Ragdoll* ragdoll, ActorInstance* actorInstance);
        virtual ~RagdollInstance() = default;

        /**
        * Accumulate motion extraction delta position and rotation.
        * As the physics system updates with fixed time steps, we sometimes update the animation system multiple times without
        * updating physics while when framerate is low it could happen that we update physics multiple times within one frame.
        * Any time the physics system updates, this function gets called, calculates the delta position and rotation of the ragdoll
        * between the last and the current physics update and store the accumulated delta.
        */
        void PostPhysicsUpdate(float timeDelta);

        void PostAnimGraphUpdate(float timeDelta);

        void SetRagdollRootNode(Node* node)                     { m_ragdollRootJoint = node; }
        Node* GetRagdollRootNode() const                        { return m_ragdollRootJoint; }
        const AZ::Outcome<size_t> GetRootRagdollNodeIndex() const;

        Physics::Ragdoll* GetRagdoll() const;
        AzPhysics::SceneHandle GetRagdollSceneHandle() const;
        void SetRagdollUsed()                                   { m_ragdollUsedThisFrame = true; }

        /*
         * Direct look-up for ragdoll node indices based on animation skeleton joint indices.
         * @param[in] jointIndex The index of the joint in the animation skeleton [0, Actor::GetNumNodes()-1].
         * @result The index of the ragdoll node in case the given joint is part of the ragdoll. AZ::Failure() in case the joint is not simulated.
         */
        const AZ::Outcome<size_t> GetRagdollNodeIndex(size_t jointIndex) const;

        /*
         * Direct look-up for animation skeleton joint indices based on ragdoll node indices.
         * @param[in] ragdollNodeIndex The index of the ragdoll node [0, Physics::Ragdoll::GetNumNodes()-1].
         * @result The index of the joint in the animation skeleton.
         */
        size_t GetJointIndex(size_t ragdollNodeIndex) const;

        const AZ::Vector3& GetCurrentPos() const;
        const AZ::Vector3& GetLastPos() const;

        const AZ::Quaternion& GetCurrentRot() const;
        const AZ::Quaternion& GetLastRot() const;

        const Physics::RagdollState& GetCurrentState() const;
        const Physics::RagdollState& GetLastState() const;
        const Physics::RagdollState& GetTargetState() const;

        const AZ::Vector3& GetTrajectoryDeltaPos() const;
        const AZ::Quaternion& GetTrajectoryDeltaRot() const;

        /**
         * Reset the accumulated position and rotation motion extraction deltas.
         */
        void ResetTrajectoryDelta();

        void SetVelocityEvaluator(RagdollVelocityEvaluator* evaluator);
        RagdollVelocityEvaluator* GetVelocityEvaluator() const;

        void GetWorldSpaceTransform(const Pose* pose, size_t jointIndex, AZ::Vector3& outPosition, AZ::Quaternion& outRotation);
        void FindNextRagdollParentForJoint(Node* joint, Node*& outParentJoint, AZ::Outcome<size_t>& outRagdollParentNodeIndex) const;

        typedef const AZStd::function<void(const AZ::Vector3&, const AZ::Color&, const AZ::Vector3&, const AZ::Color&, float)>& DrawLineFunction;
        void DebugDraw(DrawLineFunction drawLine) const;

    private:
        void ReadRagdollStateFromActorInstance(Physics::RagdollState& outRagdollState, AZ::Vector3& outRagdollPos, AZ::Quaternion& outRagdollRot);
        void ReadRagdollState(Physics::RagdollState& outRagdollState, AZ::Vector3& outRagdollPos, AZ::Quaternion& outRagdollRot);

        AZStd::vector<size_t>                       m_ragdollNodeIndices; /**< Stores the ragdoll node indices for each joint in the animation skeleton, InvalidIndex in case a given joint is not part of the ragdoll. [0, Actor::GetNumNodes()-1] */
        AZStd::vector<size_t>                      m_jointIndicesByRagdollNodeIndices; /**< Stores the animation skeleton joint indices for each ragdoll node. [0, Physics::Ragdoll::GetNumNodes()-1] */
        ActorInstance*                              m_actorInstance;
        Node*                                       m_ragdollRootJoint;
        Physics::Ragdoll*                           m_ragdoll;
        AZStd::unique_ptr<RagdollVelocityEvaluator> m_velocityEvaluator;

        Physics::RagdollState                       m_lastState;
        AZ::Vector3                                 m_lastPos;
        AZ::Quaternion                              m_lastRot;
        Physics::RagdollState                       m_currentState;
        AZ::Vector3                                 m_currentPos;
        AZ::Quaternion                              m_currentRot;
        AZ::Quaternion                              m_trajectoryDeltaRot;
        AZ::Vector3                                 m_trajectoryDeltaPos;

        Physics::RagdollState                       m_targetState;

        bool                                        m_ragdollUsedLastFrame;
        bool                                        m_ragdollUsedThisFrame;
    };
} // namespace EMotionFX
