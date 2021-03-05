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

#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/World.h>
#include <PhysXCharacters/API/RagdollNode.h>

namespace PhysX
{
    using ParentIndices = AZStd::vector<size_t>;

    /// PhysX specific implementation of generic physics API Ragdoll class.
    class Ragdoll
        : public Physics::Ragdoll
        , private Physics::WorldNotificationBus::Handler
    {
    public:
        friend class RagdollComponent;

        AZ_CLASS_ALLOCATOR(Ragdoll, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO_LEGACY(Ragdoll, "{55D477B5-B922-4D3E-89FE-7FB7B9FDD635}", Physics::Ragdoll);
        static void Reflect(AZ::ReflectContext* context);

        Ragdoll();
        Ragdoll(const Ragdoll&) = delete;
        ~Ragdoll();

        void AddNode(AZStd::unique_ptr<RagdollNode> node);
        void SetParentIndices(const ParentIndices& parentIndices);
        void SetRootIndex(size_t nodeIndex);
        physx::PxRigidDynamic* GetPxRigidDynamic(size_t nodeIndex) const;
        physx::PxTransform GetRootPxTransform() const;

        // Physics::Ragdoll
        void EnableSimulation(const Physics::RagdollState& initialState) override;
        void EnableSimulationQueued(const Physics::RagdollState& initialState) override;
        void DisableSimulation() override;
        void DisableSimulationQueued() override;
        bool IsSimulated() override;
        void GetState(Physics::RagdollState& ragdollState) const override;
        void SetState(const Physics::RagdollState& ragdollState) override;
        void SetStateQueued(const Physics::RagdollState& ragdollState) override;
        void GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const override;
        void SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState) override;
        Physics::RagdollNode* GetNode(size_t nodeIndex) const override;
        size_t GetNumNodes() const override;
        AZ::Crc32 GetWorldId() const override;

        // Physics::WorldBody
        AZ::EntityId GetEntityId() const override;
        Physics::World* GetWorld() const override;
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override;
        Physics::RayCastHit RayCast(const Physics::RayCastRequest& request) override;
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;
        void AddToWorld(Physics::World&) override;
        void RemoveFromWorld(Physics::World&) override;

    private:
        void ApplyQueuedEnableSimulation();
        void ApplyQueuedSetState();
        void ApplyQueuedDisableSimulation();

        // Physics::WorldNotificationBus
        void OnPrePhysicsSubtick(float fixedDeltaTime) override;

        AZStd::vector<AZStd::unique_ptr<RagdollNode>> m_nodes;
        ParentIndices m_parentIndices;
        AZ::Outcome<size_t> m_rootIndex = AZ::Failure();
        bool m_isSimulated;
        /// Queued initial state for the ragdoll, for EnableSimulationQueued, to be applied prior to the world update.
        Physics::RagdollState m_queuedInitialState;
        /// Holds a queued state for SetState, to be applied prior to the physics world update.
        Physics::RagdollState m_queuedState;
        /// Used to track whether a call to DisableSimulation has been queued.
        bool m_queuedDisableSimulation = false;
    };
} // namespace PhysX
