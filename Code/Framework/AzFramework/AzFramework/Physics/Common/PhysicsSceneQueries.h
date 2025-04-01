/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Material/PhysicsMaterialId.h>

namespace AZ
{
    class ReflectContext;
}

namespace Physics
{
    class Material;
    class Shape;
    class ShapeConfiguration;
}

namespace AzPhysics
{
    struct SimulatedBody;
    struct SceneQueryHit;
    struct SceneQueryHits;
    using SceneQueryHitsList = AZStd::vector<SceneQueryHits>;

    namespace SceneQuery
    {
        //! Enum to specify the hit type returned by the filter callback.
        enum class QueryHitType : AZ::u8
        {
            None, //!< The hit should not be reported.
            Touch, //!< The hit should be reported but it should not block the query
            Block //!< The hit should be reported and it should block the query
        };

        //! Enum to specify which shapes are included in the query.
        enum class QueryType : AZ::u8
        {
            Static, //!< Only test against static shapes
            Dynamic, //!< Only test against dynamic shapes
            StaticAndDynamic //!< Test against both static and dynamic shapes
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
            AnyHit = (1 << 5), //!< Report any first hit. Used for geometries that contain more than one primitive. For meshes,
                               //!< if neither MeshMultiple nor AnyHit is specified, a single closest hit will be reported.
            MeshMultiple = (1 << 6), //!< Report all hits for meshes rather than just the first. Not applicable to ShapeCast queries.
            //! Report any first hit for meshes. If neither MeshMultiple nor MeshAny is specified,
            //! a single closest hit will be reported for meshes.
            MeshAny = AnyHit, //!< @deprecated Deprecated, please use AnyHit instead.
            MeshBothSides = (1 << 7),
            PreciseSweep = (1 << 8), //!< Use more accurate but slower narrow phase sweep tests.
            MTD = (1 << 9), //!< Report the minimum translation depth, normal and contact point.
            FaceIndex = (1 << 10), //!< "face index" member of the hit is valid. Required to get the per-face material data.
            Default = Position | Normal | FaceIndex
        };

        //! Bitwise operators for HitFlags
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(HitFlags)

        //! Flag used to mark which members are valid in a SceneQueryHit object.
        //! Example: if SceneQueryHit::m_resultFlags & ResultFlags::Distance is true,
        //! then the SceneQueryHit::m_distance member would have a valid value.
        enum ResultFlags : AZ::u8
        {
            Invalid = 0,

            Distance = (1 << 0),
            BodyHandle = (1 << 1),
            EntityId = (1 << 2),
            Shape = (1 << 3),
            Material = (1 << 4),
            Position = (1 << 5),
            Normal = (1 << 6)
        };

        //! Bitwise operators for ResultFlags
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(ResultFlags)

        //! Callback used for directed scene queries: RayCasts and ShapeCasts
        using FilterCallback = AZStd::function<QueryHitType(const SimulatedBody* body, const Physics::Shape* shape)>;

        //! Callback used for undirected scene queries: Overlaps
        using OverlapFilterCallback = AZStd::function<bool(const SimulatedBody* body, const Physics::Shape* shape)>;

        //! Callback for unbounded world queries. These are queries which don't require
        //! building the entire result vector, and so saves memory for very large numbers of hits.
        //! Called with '{ hit }' repeatedly until there are no more hits, then called with '{}', then never called again.
        //! Returns 'true' to continue processing more hits, or 'false' otherwise. If the function ever returns
        //! 'false', it is unspecified if the finalizing call '{}' occurs.
        using UnboundedOverlapHitCallback = AZStd::function<bool(AZStd::optional<SceneQueryHit>&&)>;

        using AsyncRequestId = int;
        using AsyncCallback = AZStd::function<void(AsyncRequestId requestId, SceneQueryHits hits)>;
        using AsyncBatchCallback = AZStd::function<void(AsyncRequestId requestId, SceneQueryHitsList hits)>;

        //! Helper used to reflect all required objects in PhysicsSceneQuery.h
        void ReflectSceneQueryObjects(AZ::ReflectContext* context);

    } // namespace SceneQuery

    //! Base Scene Query request.
    //! Not valid to be used with Scene::QueryScene functions
    struct SceneQueryRequest
    {
        enum class RequestType : AZ::u8
        {
            Undefined = 0,
            Raycast,
            Shapecast,
            Overlap
        };

        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(SceneQueryRequest, "{76ECAB7D-42BA-461F-82E6-DCED8E1BDCB9}");

        explicit SceneQueryRequest(RequestType requestType)
            : m_requestType(requestType)
        {
        }
        SceneQueryRequest() = default;
        static void Reflect(AZ::ReflectContext* context);
        virtual ~SceneQueryRequest() = default;

        RequestType m_requestType = RequestType::Undefined;
        AZ::u32 m_maxResults = 32; //!< The Maximum results for this request to return, this is limited by the value set in the SceneConfiguration
        CollisionGroup m_collisionGroup = CollisionGroup::All; //!< Collision filter for the query.
        SceneQuery::QueryType m_queryType = SceneQuery::QueryType::StaticAndDynamic; //!< Object types to include in the query
    };
    using SceneQueryRequests = AZStd::vector<AZStd::shared_ptr<SceneQueryRequest>>;

    //! Casts a ray from a starting pose along a direction returning objects that intersected with the ray.
    struct RayCastRequest final :
        public SceneQueryRequest
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(RayCastRequest, "{53EAD088-A391-48F1-8370-2A1DBA31512F}", SceneQueryRequest);

        RayCastRequest()
            : SceneQueryRequest(RequestType::Raycast)
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        float m_distance = 500.0f; //!< The distance to cast along the direction.
        AZ::Vector3 m_start = AZ::Vector3::CreateZero(); //!< World space point where ray starts from.
        AZ::Vector3 m_direction = AZ::Vector3::CreateZero(); //!< World space direction (Should be normalized)
        SceneQuery::HitFlags m_hitFlags = SceneQuery::HitFlags::Default; //!< Query behavior flags
        SceneQuery::FilterCallback m_filterCallback = nullptr; //!< Hit filtering function
        bool m_reportMultipleHits = false; //!< flag to have the cast stop after the first hit or return all hits along the query.
    };

    //! Sweeps a shape from a starting pose along a direction returning objects that intersected with the shape.
    struct ShapeCastRequest final :
        public SceneQueryRequest
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(ShapeCastRequest, "{52F6C536-92F6-4C05-983D-0A74800AE56D}", SceneQueryRequest);

        ShapeCastRequest()
            : SceneQueryRequest(RequestType::Shapecast)
        {
        }
        static void Reflect(AZ::ReflectContext* context);

        float m_distance = 500.0f; //! The distance to cast along the direction.
        AZ::Transform m_start = AZ::Transform::CreateIdentity(); //!< World space start position. Assumes only rotation + translation (no scaling).
        AZ::Vector3 m_direction = AZ::Vector3::CreateZero(); //!< World space direction (Should be normalized)
        AZStd::shared_ptr<Physics::ShapeConfiguration> m_shapeConfiguration; //!< Shape information.
        SceneQuery::HitFlags m_hitFlags = SceneQuery::HitFlags::Default | SceneQuery::HitFlags::MTD; //!< Query behavior flags. MTD Is On by default to correctly report objects that are initially in contact with the start pose.
        SceneQuery::FilterCallback m_filterCallback = nullptr; //!< Hit filtering function
        bool m_reportMultipleHits = false; //!< flag to have the cast stop after the first hit or return all hits along the query.
    };

    namespace ShapeCastRequestHelpers
    {
        //! Helper to create a ShapeCastRequest with a SphereShapeConfiguration as its shape configuration.
        ShapeCastRequest CreateSphereCastRequest(float radius,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance, 
            SceneQuery::QueryType queryType = SceneQuery::QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All,
            SceneQuery::FilterCallback filterCallback = nullptr);

        //! Helper to create a ShapeCastRequest with a BoxShapeConfiguration as its shape configuration.
        ShapeCastRequest CreateBoxCastRequest(const AZ::Vector3& boxDimensions,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            SceneQuery::QueryType queryType = SceneQuery::QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All,
            SceneQuery::FilterCallback filterCallback = nullptr);

        //! Helper to create a ShapeCastRequest with a CapsuleShapeConfiguration as its shape configuration.
        ShapeCastRequest CreateCapsuleCastRequest(float capsuleRadius, float capsuleHeight,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            SceneQuery::QueryType queryType = SceneQuery::QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All,
            SceneQuery::FilterCallback filterCallback = nullptr);

    } // namespace ShapeCastRequestHelpers

    //! Searches a region enclosed by a specified shape for any overlapping objects in the scene.
    struct OverlapRequest final :
        public SceneQueryRequest
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(OverlapRequest, "{3DC986C2-316B-4C54-A0A6-8ABBB8ABCC4A}", SceneQueryRequest);

        OverlapRequest()
            : SceneQueryRequest(RequestType::Overlap)
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        AZ::Transform m_pose = AZ::Transform::CreateIdentity(); //!< Initial shape pose
        AZStd::shared_ptr<Physics::ShapeConfiguration> m_shapeConfiguration; //!< Shape information.
        SceneQuery::OverlapFilterCallback m_filterCallback = nullptr; //!< Hit filtering function
        SceneQuery::UnboundedOverlapHitCallback m_unboundedOverlapHitCallback = nullptr; //!< When not nullptr the request will perform an unbounded overlap query.
    };

    namespace OverlapRequestHelpers
    {
        //! Helper to create a OverlapRequest with a SphereShapeConfiguration as its shape configuration.
        OverlapRequest CreateSphereOverlapRequest(float radius,const AZ::Transform& pose,
            SceneQuery::OverlapFilterCallback filterCallback = nullptr);

        //! Helper to create a OverlapRequest with a BoxShapeConfiguration as its shape configuration.
        OverlapRequest CreateBoxOverlapRequest(const AZ::Vector3& dimensions, const AZ::Transform& pose,
            SceneQuery::OverlapFilterCallback filterCallback = nullptr);

        //! Helper to create a OverlapRequest with a CapsuleShapeConfiguration as its shape configuration.
        OverlapRequest CreateCapsuleOverlapRequest(float height, float radius, const AZ::Transform& pose,
            SceneQuery::OverlapFilterCallback filterCallback = nullptr);

    } // namespace OverlapRequestHelpers

    //! Structure that contains information of an individual hit related to a SceneQuery.
    struct SceneQueryHit
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(SceneQueryHit, "{7A7201B9-67B5-438B-B4EB-F3EEBB78C617}");
        static void Reflect(AZ::ReflectContext* context);

        explicit operator bool() const { return IsValid(); }
        bool IsValid() const { return m_resultFlags != SceneQuery::ResultFlags::Invalid; }

        //! Flags used to determine what members are valid.
        //! If the flag is true, the member will have a valid value.
        SceneQuery::ResultFlags m_resultFlags = SceneQuery::ResultFlags::Invalid;

        //! The distance along the cast at which the hit occurred as given by Dot(m_normal, startPoint) - Dot(m_normal, m_position).
        //! Valid if SceneQuery::ResultFlags::Distance is set.
        float m_distance = 0.0f; 
        //! Handler to the simulated body that was hit.
        //! Valid if SceneQuery::ResultFlags::BodyHandle is set.
        AzPhysics::SimulatedBodyHandle m_bodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        //! The Entity Id of the body that was hit.
        //! Valid if SceneQuery::ResultFlags::EntityId is set.
        AZ::EntityId m_entityId;
        //! The shape on the body that was hit.
        //! Valid if SceneQuery::ResultFlags::Shape is set.
        Physics::Shape* m_shape = nullptr;
        //! The physics material id on the shape (or face) that was hit.
        //! Valid if SceneQuery::ResultFlags::Material is set.
        Physics::MaterialId m_physicsMaterialId;
        //! The position of the hit in world space.
        //! Valid if SceneQuery::ResultFlags::Position is set.
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        //! The normal of the surface hit.
        //! Valid if SceneQuery::ResultFlags::Normal is set.
        AZ::Vector3 m_normal = AZ::Vector3::CreateZero();
    };

    //! Structure that contains all hits related to a SceneQuery.
    struct SceneQueryHits
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(SceneQueryHits, "{BAFCC4E7-A06B-4909-B2AE-C89D9E84FE4E}");
        static void Reflect(AZ::ReflectContext* context);

        explicit operator bool() const { return !m_hits.empty(); }

        AZStd::vector<SceneQueryHit> m_hits;
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzPhysics::SceneQuery::QueryType, "{0E0E56A8-73A8-40B4-B438-B19FC852E3C0}");
    AZ_TYPE_INFO_SPECIALIZE(AzPhysics::SceneQuery::ResultFlags, "{E081DB48-CFC8-4480-BB69-AA5BFC8C5FEE}");
}
