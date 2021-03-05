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
        RagdollNode(AZStd::unique_ptr<Physics::RigidBody> rigidBody);
        ~RagdollNode() = default;

        void SetJoint(const AZStd::shared_ptr<Physics::Joint>& joint);

        // Physics::RagdollNode
        Physics::RigidBody& GetRigidBody() override;
        const AZStd::shared_ptr<Physics::Joint>& GetJoint() const override;

        // Physics::WorldBody
        Physics::World* GetWorld() const override;
        AZ::EntityId GetEntityId() const override;

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
        AZStd::shared_ptr<Physics::Joint> m_joint;
        AZStd::unique_ptr<Physics::RigidBody> m_rigidBody;
        PhysX::ActorData m_actorUserData;
    };
} // namespace PhysX
