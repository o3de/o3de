/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <PhysX/UserDataTypes.h>

namespace AzPhysics
{
    struct StaticRigidBodyConfiguration;
}

namespace PhysX
{
    class Shape;

    class StaticRigidBody
        : public AzPhysics::StaticRigidBody
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(PhysX::StaticRigidBody, "{06E960EF-E1F3-466F-B34F-800E32775092}", AzPhysics::StaticRigidBody);

        StaticRigidBody() = default;
        StaticRigidBody(const AzPhysics::StaticRigidBodyConfiguration& configuration);
        ~StaticRigidBody();

        // AzPhysics::StaticRigidBody
        void AddShape(AZStd::shared_ptr<Physics::Shape> shape) override;
        AZStd::shared_ptr<Physics::Shape> GetShape(AZ::u32 index) override;
        AZStd::shared_ptr<const Physics::Shape> GetShape(AZ::u32 index) const override;
        AZ::u32 GetShapeCount() const override;

        // AzPhysics::SimulatedBody
        AZ::EntityId GetEntityId() const override;

        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;

        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;

        AZ::Aabb GetAabb() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        virtual AZ::Crc32 GetNativeType() const override;
        virtual void* GetNativePointer() const override;

    private:
        void CreatePhysXActor(const AzPhysics::StaticRigidBodyConfiguration& configuration);

        AZStd::shared_ptr<physx::PxRigidStatic> m_pxStaticRigidBody;
        AZStd::vector<AZStd::shared_ptr<PhysX::Shape>> m_shapes;
        PhysX::ActorData m_actorUserData;
        AZStd::string m_debugName;
    };
} // namespace PhysX
