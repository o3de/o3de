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

#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <PhysX/UserDataTypes.h>

namespace PhysX
{
    class Shape;

    class RigidBodyStatic
        : public Physics::RigidBodyStatic
    {
    public:
        AZ_CLASS_ALLOCATOR(RigidBodyStatic, AZ::SystemAllocator, 0);
        AZ_RTTI(RigidBodyStatic, "{06E960EF-E1F3-466F-B34F-800E32775092}", Physics::RigidBodyStatic);

        RigidBodyStatic() = default;
        RigidBodyStatic(const Physics::WorldBodyConfiguration& configuration);
        ~RigidBodyStatic();

        // Physics::RigidBodyStatic
        void AddShape(const AZStd::shared_ptr<Physics::Shape>& shape) override;
        AZStd::shared_ptr<Physics::Shape> GetShape(AZ::u32 index) override;
        AZ::u32 GetShapeCount() override;

        // Physics::WorldBody
        AZ::EntityId GetEntityId() const override;

        Physics::World* GetWorld() const override;

        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;

        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;

        AZ::Aabb GetAabb() const override;
        Physics::RayCastHit RayCast(const Physics::RayCastRequest& request) override;

        virtual AZ::Crc32 GetNativeType() const override;
        virtual void* GetNativePointer() const override;

        void AddToWorld(Physics::World&) override;
        void RemoveFromWorld(Physics::World&) override;

    private:
        void CreatePxActor(const Physics::WorldBodyConfiguration& configuration);
        AZStd::shared_ptr<physx::PxRigidStatic> m_staticRigidBody;
        AZStd::vector<AZStd::shared_ptr<PhysX::Shape>> m_shapes;
        PhysX::ActorData m_actorUserData;
        AZStd::string m_debugName;
    };
} // namespace PhysX
