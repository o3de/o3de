/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/JointConfiguration.h>

namespace Physics
{
    using ParentIndices = AZStd::vector<size_t>;

    class RagdollNodeConfiguration
        : public AzPhysics::RigidBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(RagdollNodeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(RagdollNodeConfiguration, "{A1796586-85AB-496E-93C9-C5841F03B1AD}", AzPhysics::RigidBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        RagdollNodeConfiguration();
        RagdollNodeConfiguration(const RagdollNodeConfiguration& settings) = default;

        AZStd::shared_ptr<AzPhysics::JointConfiguration> m_jointConfig;
    };

    class RagdollConfiguration
        : public AzPhysics::SimulatedBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(RagdollConfiguration, AZ::SystemAllocator);
        AZ_RTTI(RagdollConfiguration, "{7C96D332-61D8-4C58-A2BF-707716D38D14}", AzPhysics::SimulatedBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        RagdollConfiguration();
        explicit RagdollConfiguration(const RagdollConfiguration& settings) = default;

        RagdollNodeConfiguration* FindNodeConfigByName(const AZStd::string& nodeName) const;
        AZ::Outcome<size_t> FindNodeConfigIndexByName(const AZStd::string& nodeName) const;

        void RemoveNodeConfigByName(const AZStd::string& nodeName);

        AZStd::vector<RagdollNodeConfiguration> m_nodes;
        CharacterColliderConfiguration m_colliders;
        RagdollState m_initialState;
        ParentIndices m_parentIndices;
    };

    /// Represents a single rigid part of a ragdoll.
    class RagdollNode
        : public AzPhysics::SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR(RagdollNode, AZ::SystemAllocator);
        AZ_RTTI(RagdollNode, "{226D02B7-6138-4F6B-9870-DE5A1C3C5077}", AzPhysics::SimulatedBody);

        virtual AzPhysics::RigidBody& GetRigidBody() = 0;
        virtual ~RagdollNode() = default;

        virtual AzPhysics::Joint* GetJoint() = 0;
        virtual bool IsSimulating() const = 0;
    };

    /// A hierarchical collection of rigid bodies connected by joints typically used to physically simulate a character.
    class Ragdoll
        : public AzPhysics::SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR(Ragdoll, AZ::SystemAllocator);
        AZ_RTTI(Physics::Ragdoll, "{01F09602-80EC-4693-A0E7-C2719239044B}", AzPhysics::SimulatedBody);
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
        virtual bool IsSimulated() const = 0;

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
    };
}
