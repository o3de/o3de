/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

namespace AZ
{
    class Aabb;
}

namespace Physics
{
    class Material;

    class ColliderConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(ColliderConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(ColliderConfiguration, "{16206828-F867-4DA9-9E4E-549B7B2C6174}");
        static void Reflect(AZ::ReflectContext* context);

        enum PropertyVisibility : AZ::u8
        {
            CollisionLayer = 1 << 0,
            MaterialSelection = 1 << 1,
            IsTrigger = 1 << 2,
            IsVisible = 1 << 3, ///< @deprecated This property will be removed in a future release.
            Offset = 1 << 4 ///< Whether the rotation and position offsets should be visible.
        };

        // Delta to ensure that contact offset is slightly larger than rest offset.
        static const float ContactOffsetDelta;

        ColliderConfiguration() = default;
        ColliderConfiguration(const ColliderConfiguration&) = default;
        virtual ~ColliderConfiguration() = default;

        AZ::Crc32 GetPropertyVisibility(PropertyVisibility property) const;
        void SetPropertyVisibility(PropertyVisibility property, bool isVisible);

        AZ::Crc32 GetIsTriggerVisibility() const;
        AZ::Crc32 GetCollisionLayerVisibility() const;
        AZ::Crc32 GetMaterialSelectionVisibility() const;
        AZ::Crc32 GetOffsetVisibility() const;

        AzPhysics::CollisionLayer m_collisionLayer; ///< Which collision layer is this collider on.
        AzPhysics::CollisionGroups::Id m_collisionGroupId; ///< Which layers does this collider collide with.
        bool m_isTrigger = false; ///< Should this shape act as a trigger shape.
        bool m_isSimulated = true; ///< Should this shape partake in collision in the physical simulation.
        bool m_isInSceneQueries = true; ///< Should this shape partake in scene queries (ray casts, overlap tests, sweeps).
        bool m_isExclusive = true; ///< Can this collider be shared between multiple bodies?
        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); /// Shape offset relative to the connected rigid body.
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity(); ///< Shape rotation relative to the connected rigid body.
        Physics::MaterialSelection m_materialSelection; ///< Materials for the collider.
        AZ::u8 m_propertyVisibilityFlags = (std::numeric_limits<AZ::u8>::max)(); ///< Visibility flags for collider.
                                                                                 ///< Note: added parenthesis for std::numeric_limits is
                                                                                 ///< to avoid collision with `max` macro in uber builds.
        bool m_visible = false; ///< @deprecated This property will be removed in a future release. Display the collider in editor view.
        AZStd::string m_tag; ///< Identification tag for the collider.
        float m_restOffset = 0.0f; ///< Bodies will come to rest separated by the sum of their rest offsets.
        float m_contactOffset = 0.02f; ///< Bodies will start to generate contacts when closer than the sum of their contact offsets.
    private:
        void OnRestOffsetChanged();
        void OnContactOffsetChanged();
    };

    struct RayCastRequest;

    class Shape
    {
    public:
        AZ_CLASS_ALLOCATOR(Shape, AZ::SystemAllocator, 0);
        AZ_RTTI(Shape, "{0A47DDD6-2BD7-43B3-BF0D-2E12CC395C13}");
        virtual ~Shape() = default;

        virtual void SetMaterial(const AZStd::shared_ptr<Material>& material) = 0;
        virtual AZStd::shared_ptr<Material> GetMaterial() const = 0;

        virtual void SetCollisionLayer(const AzPhysics::CollisionLayer& layer) = 0;
        virtual AzPhysics::CollisionLayer GetCollisionLayer() const = 0;

        virtual void SetCollisionGroup(const AzPhysics::CollisionGroup& group) = 0;
        virtual AzPhysics::CollisionGroup GetCollisionGroup() const = 0;

        virtual void SetName(const char* name) = 0;

        virtual void SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation) = 0;
        virtual AZStd::pair<AZ::Vector3, AZ::Quaternion> GetLocalPose() const = 0;

        virtual float GetRestOffset() const = 0;
        virtual float GetContactOffset() const = 0;
        virtual void SetRestOffset(float restOffset) = 0;
        virtual void SetContactOffset(float contactOffset) = 0;

        virtual void* GetNativePointer() = 0;

        virtual AZ::Crc32 GetTag() const = 0;

        virtual void AttachedToActor(void* actor) = 0;
        virtual void DetachedFromActor() = 0;

        //! Raycast against this shape.
        //! @param request Ray parameters in world space.
        //! @param worldTransform World transform of this shape.
        virtual AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform) = 0;

        //! Raycast against this shape using local coordinates.
        //! @param request Ray parameters in local space.
        virtual AzPhysics::SceneQueryHit RayCastLocal(const AzPhysics::RayCastRequest& localSpaceRequest) = 0;

        //! Retrieve this shape AABB.
        //! @param worldTransform World transform of this shape.
        virtual AZ::Aabb GetAabb(const AZ::Transform& worldTransform) const = 0;

        //! Retrieve this shape AABB using local coordinates
        virtual AZ::Aabb GetAabbLocal() const = 0;

        //! Fills in the vertices and indices buffers representing this shape.
        //! If vertices are returned but not indices you may assume the vertices are in triangle list format.
        //! @param vertices A buffer to be filled with vertices
        //! @param indices A buffer to be filled with indices
        //! @param optionalBounds Optional AABB that, if provided, will limit the mesh returned to that AABB.  
        //!                       Currently only supported by the heightfield shape.
        virtual void GetGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb* optionalBounds = nullptr) = 0;

    };
} // namespace Physics
