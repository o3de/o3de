/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace Physics
{
    class Material;
}

namespace PhysX
{
    class Material;

    class Shape
        : public Physics::Shape
        , public AZStd::enable_shared_from_this<Shape>
    {
    public:
        AZ_CLASS_ALLOCATOR(Shape, AZ::SystemAllocator);
        AZ_RTTI(Shape, "{A84BCCA2-7F29-4E17-830F-911E7BB3E80C}", Physics::Shape);

        Shape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration);
        Shape(physx::PxShape* nativeShape);
        virtual ~Shape();

        Shape(Shape&& shape);
        Shape& operator=(Shape&& shape);
        Shape(const Shape& shape) = delete;
        Shape& operator=(const Shape& shape) = delete;

        // Physics::Shape overrides...
        void SetMaterial(const AZStd::shared_ptr<Physics::Material>& material) override;
        AZStd::shared_ptr<Physics::Material> GetMaterial() const override;
        Physics::MaterialId GetMaterialId() const override;
        void SetCollisionLayer(const AzPhysics::CollisionLayer& layer) override;
        AzPhysics::CollisionLayer GetCollisionLayer() const override;
        void SetCollisionGroup(const AzPhysics::CollisionGroup& group) override;
        AzPhysics::CollisionGroup GetCollisionGroup() const override;
        void SetName(const char* name) override;
        void SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation) override;
        AZStd::pair<AZ::Vector3, AZ::Quaternion> GetLocalPose() const override;
        float GetRestOffset() const override;
        float GetContactOffset() const override;
        void SetRestOffset(float restOffset) override;
        void SetContactOffset(float contactOffset) override;
        void* GetNativePointer() override;
        const void* GetNativePointer() const override;
        AZ::Crc32 GetTag() const override;
        void AttachedToActor(void* actor) override;
        void DetachedFromActor() override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform) override;
        AzPhysics::SceneQueryHit RayCastLocal(const AzPhysics::RayCastRequest& localSpaceRequest) override;
        AZ::Aabb GetAabb(const AZ::Transform& worldTransform) const override;
        AZ::Aabb GetAabbLocal() const override;
        void GetGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices,
            const AZ::Aabb* optionalBounds = nullptr) const override;

        physx::PxShape* GetPxShape();

        void SetPhysXMaterials(const AZStd::vector<AZStd::shared_ptr<PhysX::Material>>& materials);
        const AZStd::vector<AZStd::shared_ptr<PhysX::Material>>& GetPhysXMaterials();

        bool IsTrigger() const;

    private:
        void BindMaterialsWithPxShape();
        void ExtractMaterialsFromPxShape();
        physx::PxScene* GetScene() const;
        void ReleasePxShape(physx::PxShape* shape);
        AzPhysics::SceneQueryHit RayCastInternal(const AzPhysics::RayCastRequest& worldSpaceRequest, const physx::PxTransform& pose);

        using PxShapeUniquePtr = AZStd::unique_ptr<physx::PxShape, AZStd::function<void(physx::PxShape*)>>;
        Shape() = default;

        PxShapeUniquePtr m_pxShape;
        AZStd::vector<AZStd::shared_ptr<PhysX::Material>> m_materials;
        AzPhysics::CollisionLayer m_collisionLayer;
        AzPhysics::CollisionGroup m_collisionGroup;
        AZ::Crc32 m_tag;
        physx::PxActor* m_attachedActor = nullptr;
    };
}
