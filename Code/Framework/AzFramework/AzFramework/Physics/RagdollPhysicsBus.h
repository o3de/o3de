/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>

namespace Physics
{
    enum class SimulationType
    {
        Kinematic, ///< For ragdoll nodes controlled directly by animation.
        Simulated ///< For ragdoll nodes driven by the physics simulation.
    };

    /// Contains pose and velocity information, simulation type and joint strength properties for a node in the ragdoll
    /// for data transfer between physics and other systems.
    struct RagdollNodeState
    {
        RagdollNodeState() {}

        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); ///< Position in world space.
        AZ::Quaternion m_orientation = AZ::Quaternion::CreateIdentity(); ///< Orientation in world space.
        AZ::Vector3 m_linearVelocity = AZ::Vector3::CreateZero(); ///< Linear velocity in world space.
        AZ::Vector3 m_angularVelocity = AZ::Vector3::CreateZero(); ///< Angular velocity in world space.
        SimulationType m_simulationType = SimulationType::Kinematic; ///< Whether the node is kinematic or simulated.
        float m_strength = 0.0f; ///< Controls how powerfully the joint attempts to reach a target orientation.
        float m_dampingRatio = 1.0f; ///< Whether the joint is underdamped (below 1.0), critically damped (1.0) or overdamped (above 1.0).
    };

    using RagdollState = AZStd::vector<RagdollNodeState>;
    class RagdollNode;
    class Ragdoll;
}

namespace AzFramework
{
    /// Messages serviced by character physics ragdolls.
    class RagdollPhysicsRequests
        : public AZ::ComponentBus
    {
    public:

        virtual ~RagdollPhysicsRequests() {}

        /// Inserts the ragdoll into the physics simulation.
        /// @param initialState Contains pose and velocity information for initializing the ragdoll.
        virtual void EnableSimulation(const Physics::RagdollState& initialState) = 0;

        /// Queues inserting the ragdoll into the physics simulation, to be executed before the next physics update.
        /// @param initialState State for initializing the ragdoll positions, orientations and velocities.
        virtual void EnableSimulationQueued(const Physics::RagdollState& initialState) = 0;

        /// Removes the ragdoll from physics simulation.
        virtual void DisableSimulation() = 0;

        /// Queues removing the ragdoll from the physics simulation, to be executed before the next physics update.
        virtual void DisableSimulationQueued() = 0;

        /// Gets a pointer to the underlying generic ragdoll object.
        virtual Physics::Ragdoll* GetRagdoll() = 0;

        /// Writes the state for all of the nodes in the ragdoll to the provided output.
        /// The caller owns the output state and can safely manipulate it without affecting the physics simulation.
        /// @param[out] ragdollState Output parameter for writing the ragdoll state to.
        virtual void GetState(Physics::RagdollState& ragdollState) const = 0;

        /// Updates the state for all of the nodes in the ragdoll based on the input state.
        /// @param ragdollState Contains the state with which to update the ragdoll.
        virtual void SetState(const Physics::RagdollState& ragdollState) = 0;

        /// Queues updating the state for all of the bodies in the ragdoll using the input ragdoll state.
        /// The new state is applied before the next physics update.
        /// @param ragdollState The state with which to update the ragdoll.
        virtual void SetStateQueued(const Physics::RagdollState& ragdollState) = 0;

        /// Writes the state for an individual node in the ragdoll to the provided output.
        /// The caller owns the output state and can safely manipulate it without affecting the physics simulation.
        /// @param nodeIndex Index in the physics representation of the character.  Note this does not necessarily
        /// correspond to indices used in other systems.
        /// @param[out] nodeState Output parameter for writing the state for the specified node to.
        virtual void GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const = 0;

        /// Updates the state for an individual body in the ragdoll based on the input state.
        /// @param nodeIndex Index in the physics representation of the character.  Note this does not necessarily
        /// correspond to indices used in other systems.
        /// @param nodeState Contains the state with which to update the individual node.
        virtual void SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState) = 0;

        /// Gets a pointer to an individual rigid body in the ragdoll.
        /// @param nodeIndex Index in the physics representation of the character.  Note this does not necessarily
        /// correspond to indices used in other systems.
        virtual Physics::RagdollNode* GetNode(size_t nodeIndex) const = 0;

        // deprecated Cry functions

        /// @cond EXCLUDE_DOCS
        /// @deprecated Please use generic ragdoll functions instead of legacy cry ragdoll functions.
        /// Causes an entity with a skinned mesh component to disable its current physics and enable ragdoll physics.
        virtual void EnterRagdoll() = 0;

        /// @cond EXCLUDE_DOCS
        /// @deprecated Please use generic ragdoll functions instead of legacy cry ragdoll functions.
        /// This will cause the ragdoll component to deactivate itself and re-enable the entity physics component.
        virtual void ExitRagdoll() = 0;
    };

    using RagdollPhysicsRequestBus = AZ::EBus<RagdollPhysicsRequests>;

    class RagdollPhysicsNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~RagdollPhysicsNotifications() = default;

        virtual void OnRagdollActivated() = 0;
        virtual void OnRagdollDeactivated() = 0;
    };

    using RagdollPhysicsNotificationBus = AZ::EBus<RagdollPhysicsNotifications>;
} // namespace AzFramework
