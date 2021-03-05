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

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/Joint.h>

namespace Physics
{
    class RagdollNodeConfiguration
        : public RigidBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(RagdollNodeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(RagdollNodeConfiguration, "{A1796586-85AB-496E-93C9-C5841F03B1AD}", RigidBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        RagdollNodeConfiguration();
        RagdollNodeConfiguration(const RagdollNodeConfiguration& settings) = default;

        AZStd::shared_ptr<JointLimitConfiguration> m_jointLimit;
    };

    class RagdollConfiguration
        : public WorldBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(RagdollConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(RagdollConfiguration, "{7C96D332-61D8-4C58-A2BF-707716D38D14}", WorldBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        RagdollConfiguration() = default;
        explicit RagdollConfiguration(const RagdollConfiguration& settings) = default;

        RagdollNodeConfiguration* FindNodeConfigByName(const AZStd::string& nodeName) const;
        AZ::Outcome<size_t> FindNodeConfigIndexByName(const AZStd::string& nodeName) const;

        void RemoveNodeConfigByName(const AZStd::string& nodeName);

        AZStd::vector<RagdollNodeConfiguration> m_nodes;
        CharacterColliderConfiguration m_colliders;
    };

    /// Represents a single rigid part of a ragdoll.
    class RagdollNode
        : public WorldBody
    {
    public:
        AZ_CLASS_ALLOCATOR(RagdollNode, AZ::SystemAllocator, 0);
        AZ_RTTI(RagdollNode, "{226D02B7-6138-4F6B-9870-DE5A1C3C5077}", WorldBody);

        virtual RigidBody& GetRigidBody() = 0;
        virtual ~RagdollNode() = default;

        virtual const AZStd::shared_ptr<Physics::Joint>& GetJoint() const = 0;
    };

    /// A hierarchical collection of rigid bodies connected by joints typically used to physically simulate a character.
    class Ragdoll
        : public WorldBody
    {
    public:
        AZ_CLASS_ALLOCATOR(Ragdoll, AZ::SystemAllocator, 0);
        AZ_RTTI(Ragdoll, "{01F09602-80EC-4693-A0E7-C2719239044B}", WorldBody);
        virtual ~Ragdoll() = default;

        /// Inserts the ragdoll into the physics simulation.
        /// @param initialState State for initializing the ragdoll positions, orientations and velocities.
        virtual void EnableSimulation(const RagdollState& initialState) = 0;

        /// Queues inserting the ragdoll into the physics simulation, to be executed before the next physics update.
        /// @param initialState State for initializing the ragdoll positions, orientations and velocities.
        virtual void EnableSimulationQueued(const RagdollState& initialState) = 0;

        /// Removes the ragdoll from physics simulation.
        virtual void DisableSimulation() = 0;

        /// Queues removing the ragdoll from the physics simulation, to be executed before the next physics update.
        virtual void DisableSimulationQueued() = 0;

        /// Is the ragdoll currently simulated?
        /// @result True in case the ragdoll is simulated, false if not.
        virtual bool IsSimulated() = 0;

        /// Writes the state for all of the bodies in the ragdoll to the provided output.
        /// The caller owns the output state and can safely manipulate it without affecting the physics simulation.
        /// @param[out] ragdollState Output parameter to write ragdoll state to.
        virtual void GetState(RagdollState& ragdollState) const = 0;

        /// Updates the state for all of the bodies in the ragdoll using the input ragdoll state.
        /// @param ragdollState The state with which to update the ragdoll.
        virtual void SetState(const RagdollState& ragdollState) = 0;

        /// Queues updating the state for all of the bodies in the ragdoll using the input ragdoll state.
        /// The new state is applied before the next physics update.
        /// @param ragdollState The state with which to update the ragdoll.
        virtual void SetStateQueued(const RagdollState& ragdollState) = 0;

        /// Writes the state for an individual body in the ragdoll to the provided output.
        /// The caller owns the output state and can safely manipulate it without affecting the physics simulation.
        /// @param nodeIndex Index in the physics representation of the character.  Note this does not necessarily
        /// correspond to indices used in other systems.
        /// @param[out] nodeState Output parameter to write the node state to.
        virtual void GetNodeState(size_t nodeIndex, RagdollNodeState& nodeState) const = 0;

        /// Updates the state for an individual body in the ragdoll using the input node state.
        /// @param nodeIndex Index in the physics representation of the character.  Note this does not necessarily
        /// correspond to indices used in other systems.
        /// @param nodeState Contains the state with which to update the individual node.
        virtual void SetNodeState(size_t nodeIndex, const RagdollNodeState& nodeState) = 0;

        /// Gets a pointer to an individual rigid body in the ragdoll.
        /// @param nodeIndex Index in the physics representation of the character.  Note this does not necessarily
        /// correspond to indices used in other systems.
        virtual RagdollNode* GetNode(size_t nodeIndex) const = 0;

        /// Returns the number of ragdoll nodes in the ragdoll.
        virtual size_t GetNumNodes() const = 0;

        /// Returns the id of the world the ragdoll exists in.
        virtual AZ::Crc32 GetWorldId() const = 0;
    };
}
