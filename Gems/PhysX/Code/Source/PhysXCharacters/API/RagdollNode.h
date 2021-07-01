/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <PhysX/UserDataTypes.h>

namespace PhysX
{
    /// PhysX specific implementation of generic physics API RagdollNode class.
    class RagdollNode
        : public Physics::RagdollNode
    {
    public:
        AZ_CLASS_ALLOCATOR(RagdollNode, AZ::SystemAllocator, 0);
        AZ_RTTI(RagdollNode, "{6AB5AB45-6DE3-4F97-B7C7-CEEB1FEEE721}", Physics::RagdollNode);
        static void Reflect(AZ::ReflectContext* context);

        RagdollNode() = default;
        explicit RagdollNode(AzPhysics::SceneHandle sceneHandle, Physics::RagdollNodeConfiguration& nodeConfig);
        ~RagdollNode();

        void SetJoint(const AZStd::shared_ptr<Physics::Joint>& joint);

        // Physics::RagdollNode
        AzPhysics::RigidBody& GetRigidBody() override;
        const AZStd::shared_ptr<Physics::Joint>& GetJoint() const override;
        bool IsSimulating() const override;

        // AzPhysics::SimulatedBody
        AzPhysics::Scene* GetScene() override;
        AZ::EntityId GetEntityId() const override;

        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;

        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;

        AZ::Aabb GetAabb() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;

        AzPhysics::SimulatedBodyHandle GetRigidBodyHandle() const;

    private:
        void CreatePhysicsBody(AzPhysics::SceneHandle sceneHandle, Physics::RagdollNodeConfiguration& nodeConfig);
        void DestroyPhysicsBody();

        AZStd::shared_ptr<Physics::Joint> m_joint;
        AzPhysics::RigidBody* m_rigidBody;
        AzPhysics::SimulatedBodyHandle m_rigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SceneHandle m_sceneOwner = AzPhysics::InvalidSceneHandle;
        PhysX::ActorData m_actorUserData;
    };
} // namespace PhysX
