/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <functional>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Material.h>

namespace Physics
{
    class WorldBody;
    class Shape;
    class ShapeConfiguration;

    /// Enum to specify the hit type returned by the filter callback.
    enum QueryHitType
    {
        None, ///< The hit should not be reported.
        Touch, ///< The hit should be reported but it should not block the query
        Block ///< The hit should be reported and it should block the query
    };

    /// Callback used for directed scene queries: RayCasts and ShapeCasts
    using FilterCallback = AZStd::function<QueryHitType(const Physics::WorldBody* body, const Physics::Shape* shape)>;

    /// Enum to specify which shapes are included in the query.
    enum class QueryType : int
    {
        Static, ///< Only test against static shapes
        Dynamic, ///< Only test against dynamic shapes
        StaticAndDynamic ///< Test against both static and dynamic shapes
    };

    //! Scene query and geometry query behavior flags.
    //! 
    //! HitFlags are used for 3 different purposes:
    //! 
    //! 1) To request hit fields to be filled in by scene queries (such as hit position, normal, face index or UVs).
    //! 2) Once query is completed, to indicate which fields are valid (note that a query may produce more valid fields than requested).
    //! 3) To specify additional options for the narrow phase and mid-phase intersection routines.
    enum class HitFlags : AZ::u16
    {
        Position = (1 << 0), //!< "position" member of the hit is valid
        Normal = (1 << 1), //!< "normal" member of the hit is valid
        UV = (1 << 3), //!< "u" and "v" barycentric coordinates of the hit are valid. Not applicable to ShapeCast queries.
        //! Performance hint flag for ShapeCasts when it is known upfront there's no initial overlap.
        //! NOTE: using this flag may cause undefined results if shapes are initially overlapping.
        AssumeNoInitialOverlap = (1 << 4), 
        MeshMultiple = (1 << 5), //!< Report all hits for meshes rather than just the first. Not applicable to ShapeCast queries.
        //! Report any first hit for meshes. If neither MeshMultiple nor MeshAny is specified,
        //! a single closest hit will be reported for meshes.
        MeshAny = (1 << 6), 
        //! Report hits with back faces of mesh triangles. Also report hits for raycast
        //! originating on mesh surface and facing away from the surface normal. Not applicable to ShapeCast queries.
        MeshBothSides = (1 << 7), 
        PreciseSweep = (1 << 8), //!< Use more accurate but slower narrow phase sweep tests.
        MTD = (1 << 9), //!< Report the minimum translation depth, normal and contact point.
        FaceIndex = (1 << 10), //!< "face index" member of the hit is valid. Required to get the per-face material data.
        Default = Position | Normal | FaceIndex
    };

    /// Casts a ray from a starting pose along a direction returning objects that intersected with the ray.
    struct RayCastRequest
    {
        AZ_CLASS_ALLOCATOR(RayCastRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(RayCastRequest, "{53EAD088-A391-48F1-8370-2A1DBA31512F}");

        float m_distance = 500.0f; ///< The distance along m_dir direction.
        AZ::Vector3 m_start = AZ::Vector3::CreateZero(); ///< World space point where ray starts from.
        AZ::Vector3 m_direction = AZ::Vector3::CreateZero(); ///< World space direction (normalized).
        AzPhysics::CollisionGroup m_collisionGroup = AzPhysics::CollisionGroup::All; ///< The layers to include in the query
        FilterCallback m_filterCallback = nullptr; ///< Hit filtering function
        QueryType m_queryType = QueryType::StaticAndDynamic; ///< Object types to include in the query
        HitFlags m_hitFlags = HitFlags::Default; ///< Query behavior flags
        AZ::u64 m_maxResults = 32; ///< The Maximum results for this request to return, this is limited by the value set in WorldConfiguration
    };
    
    /// Sweeps a shape from a starting pose along a direction returning objects that intersected with the shape.
    struct ShapeCastRequest
    {
        AZ_CLASS_ALLOCATOR(ShapeCastRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ShapeCastRequest, "{52F6C536-92F6-4C05-983D-0A74800AE56D}");

        float m_distance = 500.0f; /// The distance to cast along m_dir direction.
        AZ::Transform m_start = AZ::Transform::CreateIdentity(); ///< World space start position. Assumes only rotation + translation (no scaling).
        AZ::Vector3 m_direction = AZ::Vector3::CreateZero(); ///< World space direction (Should be normalized)
        ShapeConfiguration* m_shapeConfiguration = nullptr; ///< Shape information.
        AzPhysics::CollisionGroup m_collisionGroup = AzPhysics::CollisionGroup::All; ///< Collision filter for the query.
        FilterCallback m_filterCallback = nullptr; ///< Hit filtering function
        QueryType m_queryType = QueryType::StaticAndDynamic; ///< Object types to include in the query
        HitFlags m_hitFlags = HitFlags::Default; ///< Query behavior flags
        AZ::u64 m_maxResults = 32; ///< The Maximum results for this request to return, this is limited by the value set in WorldConfiguration
    };

    /// Callback used for undirected scene queries: Overlaps
    using OverlapFilterCallback = AZStd::function<bool(const Physics::WorldBody* body, const Physics::Shape* shape)>;

    /// Searches a region enclosed by a specified shape for any overlapping objects in the scene.
    struct OverlapRequest
    {
        AZ_CLASS_ALLOCATOR(OverlapRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(OverlapRequest, "{3DC986C2-316B-4C54-A0A6-8ABBB8ABCC4A}");

        AZ::Transform m_pose = AZ::Transform::CreateIdentity(); ///< Initial shape pose
        ShapeConfiguration* m_shapeConfiguration = nullptr; ///< Shape information.
        AzPhysics::CollisionGroup m_collisionGroup = AzPhysics::CollisionGroup::All; ///< Collision filter for the query.
        OverlapFilterCallback m_filterCallback = nullptr; ///< Hit filtering function
        QueryType m_queryType = QueryType::StaticAndDynamic; ///< Object types to include in the query
        AZ::u64 m_maxResults = 32; ///< The Maximum results for this request to return, this is limited by the value set in WorldConfiguration
    };

    /// Structure used to store the result from either a raycast or a shape cast.
    struct RayCastHit
    {
        AZ_CLASS_ALLOCATOR(RayCastHit, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(RayCastHit, "{A46CBEA6-6B92-4809-9363-9DDF0F74F296}");
        static void Reflect(AZ::ReflectContext* context);

        inline operator bool() const { return m_body != nullptr; }

        float m_distance = 0.0f; ///< The distance along the cast at which the hit occurred as given by Dot(m_normal, startPoint) - Dot(m_normal, m_point).
        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); ///< The position of the hit in world space
        AZ::Vector3 m_normal = AZ::Vector3::CreateZero(); ///< The normal of the surface hit
        WorldBody* m_body = nullptr; ///< World body that was hit.
        Shape* m_shape = nullptr; ///< The shape on the body that was hit
        Material* m_material = nullptr; ///< The material on the shape (or face) that was hit
    };

    /// Overlap hit.
    struct OverlapHit
    {
        AZ_CLASS_ALLOCATOR(OverlapHit, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(OverlapHit, "{7A7201B9-67B5-438B-B4EB-F3EEBB78C617}");

        inline operator bool() const { return m_body != nullptr; }

        WorldBody* m_body = nullptr; ///< World body that was hit.
        Shape* m_shape = nullptr; ///< The shape on the body that was hit
        Material* m_material = nullptr; ///< The material on the shape (or face) that was hit
    };

    /// Bitwise operators for HitFlags
    inline HitFlags operator|(HitFlags lhs, HitFlags rhs)
    {
        return static_cast<HitFlags>(static_cast<AZ::u16>(lhs) | static_cast<AZ::u16>(rhs));
    }

    inline HitFlags operator&(HitFlags lhs, HitFlags rhs)
    {
        return static_cast<HitFlags>(static_cast<AZ::u16>(lhs) & static_cast<AZ::u16>(rhs));
    }
} // namespace Physics

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Physics::QueryType, "{0E0E56A8-73A8-40B4-B438-B19FC852E3C0}");
}
