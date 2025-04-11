/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <PhysXCharacters/API/RagdollNode.h>

namespace PhysX
{
    /// PhysX specific implementation of generic physics API Ragdoll class.
    class Ragdoll
        : public Physics::Ragdoll
    {
    public:
        friend class RagdollComponent;

        AZ_CLASS_ALLOCATOR(Ragdoll, AZ::SystemAllocator);
        AZ_RTTI(PhysX::Ragdoll, "{55D477B5-B922-4D3E-89FE-7FB7B9FDD635}", Physics::Ragdoll);
        static void Reflect(AZ::ReflectContext* context);

        Ragdoll() = default;
        explicit Ragdoll(AzPhysics::SceneHandle sceneHandle);
        Ragdoll(const Ragdoll&) = delete;
        ~Ragdoll();

        void AddNode(AZStd::unique_ptr<RagdollNode> node);
        void SetParentIndices(const Physics::ParentIndices& parentIndices);
        void SetRootIndex(size_t nodeIndex);
        physx::PxRigidDynamic* GetPxRigidDynamic(size_t nodeIndex) const;
        physx::PxTransform GetRootPxTransform() const;

        // Physics::Ragdoll
        void EnableSimulation(const Physics::RagdollState& initialState) override;
        void EnableSimulationQueued(const Physics::RagdollState& initialState) override;
        void DisableSimulation() override;
        void DisableSimulationQueued() override;
        bool IsSimulated() const override;
        void GetState(Physics::RagdollState& ragdollState) const override;
        void SetState(const Physics::RagdollState& ragdollState) override;
        void SetStateQueued(const Physics::RagdollState& ragdollState) override;
        void GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const override;
        void SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState) override;
        Physics::RagdollNode* GetNode(size_t nodeIndex) const override;
        size_t GetNumNodes() const override;

        // AzPhysics::SimulatedBody
        AZ::EntityId GetEntityId() const override;
        AzPhysics::Scene* GetScene() override;
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;

    private:
        void ApplyQueuedEnableSimulation();
        void ApplyQueuedSetState();
        void ApplyQueuedDisableSimulation();

        AZStd::vector<AZStd::unique_ptr<RagdollNode>> m_nodes;
        Physics::ParentIndices m_parentIndices;
        AZ::Outcome<size_t> m_rootIndex = AZ::Failure();
        
        /// Queued initial state for the ragdoll, for EnableSimulationQueued, to be applied prior to the world update.
        Physics::RagdollState m_queuedInitialState;
        /// Holds a queued state for SetState, to be applied prior to the physics world update.
        Physics::RagdollState m_queuedState;
        /// Used to track whether a call to DisableSimulation has been queued.
        bool m_queuedDisableSimulation = false;

        AzPhysics::SceneEvents::OnSceneSimulationStartHandler m_sceneStartSimHandler;
    };
} // namespace PhysX
