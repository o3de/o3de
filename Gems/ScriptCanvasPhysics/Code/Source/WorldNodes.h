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

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/Data.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzCore/Component/TransformBus.h>

namespace ScriptCanvasPhysics
{
    namespace WorldNodes
    {
        using Result = AZStd::tuple<
            bool /*true if an object was hit*/,
            AZ::Vector3 /*world space position*/,
            AZ::Vector3 /*surface normal*/,
            float /*distance to the hit*/,
            AZ::EntityId /*entity hit, if any*/,
            AZ::Crc32 /*tag of material surface hit, if any*/
        >;

        using OverlapResult = AZStd::tuple<
            bool, /*true if the overlap returned hits*/
            AZStd::vector<AZ::EntityId> /*list of entityIds*/
        >;

        AZ_INLINE Result RayCastWorldSpaceWithGroup(const AZ::Vector3& start,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::RayCastRequest request;
            request.m_start = start;
            request.m_direction = direction.GetNormalized();
            request.m_distance = distance;
            request.m_filterCallback = [ignore](const Physics::WorldBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore ? Physics::QueryHitType::Block : Physics::QueryHitType::None;
            };

            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            Physics::RayCastHit hit;
            Physics::WorldRequestBus::BroadcastResult(hit, &Physics::World::RayCast, request);

            AZ::EntityId id;
            AZ::Crc32 surfaceType;
            if (hit.m_body)
            {
                id = hit.m_body->GetEntityId();
            }
            if (hit.m_material)
            {
                surfaceType = hit.m_material->GetSurfaceType();
            }
            return AZStd::make_tuple(hit.m_body != nullptr, hit.m_position, hit.m_normal, hit.m_distance, id, surfaceType);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RayCastWorldSpaceWithGroup,
            "PhysX/World",
            "{695EE108-68C1-40E3-ADA5-8ED9AB74D054}",
            "Returns the first entity hit by a ray cast in world space from the start position in the specified direction.",
            "Start",
            "Direction",
            "Distance",
            "Collision group",
            "Ignore",
            "Object hit",
            "Position",
            "Normal",
            "Distance",
            "EntityId",
            "Surface");

        AZ_INLINE Result RayCastLocalSpaceWithGroup(const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            Physics::RayCastRequest request;
            request.m_start = worldSpaceTransform.GetTranslation();
            request.m_direction = worldSpaceTransform.TransformVector(direction.GetNormalized());
            request.m_distance = distance;
            request.m_filterCallback = [ignore](const Physics::WorldBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore ? Physics::QueryHitType::Block : Physics::QueryHitType::None;
            };
            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            Physics::RayCastHit hit;
            Physics::WorldRequestBus::BroadcastResult(hit, &Physics::World::RayCast, request);

            AZ::EntityId id;
            AZ::Crc32 surfaceType;
            if (hit.m_body)
            {
                id = hit.m_body->GetEntityId();
            }
            if (hit.m_material)
            {
                surfaceType = hit.m_material->GetSurfaceType();
            }
            return AZStd::make_tuple(hit.m_body != nullptr, hit.m_position, hit.m_normal, hit.m_distance, id, surfaceType);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RayCastLocalSpaceWithGroup,
            "PhysX/World",
            "{938E0C6E-C6A3-4716-9233-941EFA70241A}",
            "Returns the first entity hit by a ray cast in local space from the source entity in the specified direction.",
            "Source",
            "Direction",
            "Distance",
            "Collision group",
            "Ignore",
            "Object hit",
            "Position",
            "Normal",
            "Distance",
            "EntityId",
            "Surface");

        AZ_INLINE AZStd::vector<Physics::RayCastHit> RayCastMultipleLocalSpaceWithGroup(const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            Physics::RayCastRequest request;
            request.m_start = worldSpaceTransform.GetTranslation();
            request.m_direction = worldSpaceTransform.TransformVector(direction.GetNormalized());
            request.m_distance = distance;
            request.m_filterCallback = [ignore](const Physics::WorldBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore ? Physics::QueryHitType::Touch : Physics::QueryHitType::None;
            };
            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            AZStd::vector<Physics::RayCastHit> hits;
            Physics::WorldRequestBus::BroadcastResult(hits, &Physics::World::RayCastMultiple, request);
            return hits;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RayCastMultipleLocalSpaceWithGroup,
            "PhysX/World",
            "{A867FC55-6610-42C2-97E8-C614450CAE92}",
            "Returns all entities hit by a ray cast in local space from the source entity in the specified direction.",
            "Source",
            "Direction",
            "Distance",
            "Collision group",
            "Ignore",
            "Objects hit");

        OverlapResult OverlapQuery(const AZ::Transform& pose,
            Physics::ShapeConfiguration& shape,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::OverlapRequest request;
            request.m_pose = pose;
            request.m_shapeConfiguration = &shape;
            request.m_filterCallback = [ignore](const Physics::WorldBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore;
            };

            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            AZStd::vector<Physics::OverlapHit> overlaps;
            Physics::WorldRequestBus::BroadcastResult(overlaps, &Physics::World::Overlap, request);

            AZStd::vector<AZ::EntityId> overlapIds;
            overlapIds.reserve(overlaps.size());
            AZStd::transform(overlaps.begin(), overlaps.end(), AZStd::back_inserter(overlapIds), [](const Physics::OverlapHit& overlap)
            {
                return overlap.m_body->GetEntityId();
            });
            return AZStd::make_tuple(!overlapIds.empty(), overlapIds);
        }

        AZ_INLINE OverlapResult OverlapSphereWithGroup(const AZ::Vector3& position,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::SphereShapeConfiguration sphere(radius);
            return OverlapQuery(AZ::Transform::CreateTranslation(position), sphere, collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(OverlapSphereWithGroup,
            "PhysX/World",
            "{0A2831AB-E994-4533-8E64-700631994E64}",
            "Returns the objects overlapping a sphere at a position",
            "Position",
            "Radius",
            "Collision group",
            "Ignore"
        );

        AZ_INLINE OverlapResult OverlapBoxWithGroup(const AZ::Transform& pose,
            const AZ::Vector3& dimensions,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::BoxShapeConfiguration box(dimensions);
            return OverlapQuery(pose, box, collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(OverlapBoxWithGroup,
            "PhysX/World",
            "{1991BA3D-3848-4BF0-B696-C39C42CFE49A}",
            "Returns the objects overlapping a box at a position",
            "Pose",
            "Dimensions",
            "Collision group",
            "Ignore"
        );

        AZ_INLINE OverlapResult OverlapCapsuleWithGroup(const AZ::Transform& pose,
            float height,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::CapsuleShapeConfiguration capsule(height, radius);
            return OverlapQuery(pose, capsule, collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(OverlapCapsuleWithGroup,
            "PhysX/World",
            "{1DD49D7A-348A-4CB1-82C0-D93FE01FEFA1}",
            "Returns the objects overlapping a capsule at a position",
            "Pose",
            "Height",
            "Radius",
            "Collision group",
            "Ignore"
        );

        Result ShapecastQuery(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            Physics::ShapeConfiguration& shape,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::ShapeCastRequest request;
            request.m_distance = distance;
            request.m_start = pose;
            request.m_direction = direction;
            request.m_shapeConfiguration = &shape;
            request.m_filterCallback = [ignore](const Physics::WorldBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore ? Physics::QueryHitType::Block : Physics::QueryHitType::None;
            };

            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            Physics::RayCastHit hit;
            Physics::WorldRequestBus::BroadcastResult(hit, &Physics::World::ShapeCast, request);

            AZ::EntityId id;
            AZ::Crc32 surfaceType;
            if (hit.m_body)
            {
                id = hit.m_body->GetEntityId();
            }
            if (hit.m_material)
            {
                surfaceType = hit.m_material->GetSurfaceType();
            }
            return AZStd::make_tuple(hit.m_body != nullptr, hit.m_position, hit.m_normal, hit.m_distance, id, surfaceType);
        }

        AZ_INLINE Result SphereCastWithGroup(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::SphereShapeConfiguration sphere(radius);
            return ShapecastQuery(distance, pose, direction, sphere, collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SphereCastWithGroup,
            "PhysX/World",
            "{7A4D8893-51F5-444F-9C77-64D179F9C9BB}", "SphereCast",
            "Distance", "Pose", "Direction", "Radius", "Collision group", "Ignore", // In Params
            "Object Hit", "Position", "Normal", "Distance", "EntityId", "Surface" // Out Params

        );

        AZ_INLINE Result BoxCastWithGroup(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            const AZ::Vector3& dimensions,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::BoxShapeConfiguration box(dimensions);
            return ShapecastQuery(distance, pose, direction, box, collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(BoxCastWithGroup,
            "PhysX/World",
            "{E7C2CFE0-3FB9-438B-9A8A-A5D333AB0791}", "BoxCast",
            "Distance", "Pose", "Direction", "Dimensions", "Collision group", "Ignore", // In Params
            "Object Hit", "Position", "Normal", "Distance", "EntityId", "Surface" // Out Params
        );

        AZ_INLINE Result CapsuleCastWithGroup(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            float height,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            Physics::CapsuleShapeConfiguration capsule(height, radius);
            return ShapecastQuery(distance, pose, direction, capsule, collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(CapsuleCastWithGroup,
            "PhysX/World",
            "{938B047C-6282-4510-8AFE-21D58426061D}", "CapsuleCast",
            "Distance", "Pose", "Direction", "Height", "Radius", "Collision group", "Ignore", // In Params
            "Object Hit", "Position", "Normal", "Distance", "EntityId", "Surface" // Out Params
        );

        ///////////////////////////////////////////////////////////////
        // Deprecated nodes

        using DeprecatedResult = AZStd::tuple<
            bool /*true if an object was hit*/,
            AZ::Vector3 /*world space position*/,
            AZ::Vector3 /*surface normal*/,
            float /*distance to the hit*/,
            AZ::EntityId /*entity hit, if any*/
        >;

        DeprecatedResult DeprecatedResultFunc(bool arg1, const AZ::Vector3& arg2, const AZ::Vector3& arg3, float arg4, AZ::EntityId arg5, AZ::Crc32)
        {
            return DeprecatedResult(arg1, arg2, arg3, arg4, arg5);
        };

        AZ_INLINE DeprecatedResult RayCastWorldSpace(const AZ::Vector3& start, const AZ::Vector3& direction, float distance, AZ::EntityId ignore)
        {
            return AZStd::apply(DeprecatedResultFunc, RayCastWorldSpaceWithGroup(start, direction, distance, "", ignore));
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(RayCastWorldSpace,
            "PhysX/World",
            "{F75EF071-6755-40A3-8D5D-0603964774AE}",
            "This node is deprecated, use RayCastWorldSpaceWithGroup",
            "Start",
            "Direction",
            "Distance",
            "Ignore",
            "Object hit",
            "Position",
            "Normal",
            "Distance",
            "EntityId");

        AZ_INLINE DeprecatedResult RayCastLocalSpace(const AZ::EntityId& fromEntityId, const AZ::Vector3& direction, float distance, AZ::EntityId ignore)
        {
            return AZStd::apply(DeprecatedResultFunc, RayCastLocalSpaceWithGroup(fromEntityId, direction, distance, "", ignore));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(RayCastLocalSpace,
            "PhysX/World",
            "{1D6935AF-8EE9-4636-9F0C-E0485D935800}",
            "This node is deprecated, use RayCastLocalSpaceWithGroup",
            "Source",
            "Direction",
            "Distance",
            "Ignore",
            "Object hit",
            "Position",
            "Normal",
            "Distance",
            "EntityId");

        AZ_INLINE AZStd::vector<Physics::RayCastHit> RayCastMultipleLocalSpace(const AZ::EntityId& fromEntityId, const AZ::Vector3& direction, float distance, AZ::EntityId ignore)
        {
            return RayCastMultipleLocalSpaceWithGroup(fromEntityId, direction, distance, "", ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(RayCastMultipleLocalSpace,
            "PhysX/World",
            "{AB48C54A-0E0D-41F1-B73E-F689B9570446}",
            "This node is deprecated, use RayCastMultipleLocalSpaceWithGroup",
            "Source",
            "Direction",
            "Distance",
            "Ignore",
            "Objects hit");

        AZ_INLINE OverlapResult OverlapSphere(const AZ::Vector3& position, float radius, AZ::EntityId ignore)
        {
            return OverlapSphereWithGroup(position, radius, "", ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(OverlapSphere,
            "PhysX/World",
            "{ED1CE1E1-5BF1-46BA-8DFF-78966FFDBF75}",
            "This node is deprecated, use OverlapSphereWithGroup",
            "Position",
            "Radius",
            "Ignore"
        );

        AZ_INLINE OverlapResult OverlapBox(const AZ::Transform& pose, const AZ::Vector3& dimensions, AZ::EntityId ignore)
        {
            return OverlapBoxWithGroup(pose, dimensions, "", ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(OverlapBox,
            "PhysX/World",
            "{8D60E5FA-6101-4D3B-BC60-28449CC93AC2}",
            "This node is deprecated, use OverlapBoxWithGroup",
            "Pose",
            "Dimensions",
            "Ignore"
        );

        AZ_INLINE OverlapResult OverlapCapsule(const AZ::Transform& pose, float height, float radius, AZ::EntityId ignore)
        {
            return OverlapCapsuleWithGroup(pose, height, radius, "", ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(OverlapCapsule,
            "PhysX/World",
            "{600D8F16-2F18-4FA3-858B-D7B59484BEF1}",
            "This node is deprecated, use OverlapCapsuleWithGroup",
            "Pose",
            "Height",
            "Radius",
            "Ignore"
        );

        AZ_INLINE DeprecatedResult SphereCast(float distance, const AZ::Transform& pose, const AZ::Vector3& direction, float radius, AZ::EntityId ignore)
        {
            return AZStd::apply(DeprecatedResultFunc, SphereCastWithGroup(distance, pose, direction, radius, "", ignore));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(SphereCast,
            "PhysX/World",
            "{0236AC59-190C-4A41-99ED-5AFA3AD6C500}", "This node is deprecated, use SphereCastWithGroup",
            "Distance", "Pose", "Direction", "Radius", "Ignore", // In Params
            "Object Hit", "Position", "Normal", "Distance", "EntityId" // Out Params
        );

        AZ_INLINE DeprecatedResult BoxCast(float distance, const AZ::Transform& pose, const AZ::Vector3& direction, const AZ::Vector3& dimensions, AZ::EntityId ignore)
        {
            return AZStd::apply(DeprecatedResultFunc, BoxCastWithGroup(distance, pose, direction, dimensions, "", ignore));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(BoxCast,
            "PhysX/World",
            "{E9DB2310-21FE-4D58-BA13-512CA9BAA305}", "This node is deprecated, use BoxCastWithGroup",
            "Distance", "Pose", "Direction", "Dimensions", "Ignore", // In Params
            "Object Hit", "Position", "Normal", "Distance", "EntityId" // Out Params
        );

        AZ_INLINE DeprecatedResult CapsuleCast(float distance, const AZ::Transform& pose, const AZ::Vector3& direction, float height, float radius, AZ::EntityId ignore)
        {
            return AZStd::apply(DeprecatedResultFunc, CapsuleCastWithGroup(distance, pose, direction, height, radius, "", ignore));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(CapsuleCast,
            "PhysX/World",
            "{FC9F0631-4D63-453C-A876-EC87BC4F2A23}", "This node is deprecated, use CapsuleCastWithGroup",
            "Distance", "Pose", "Direction", "Height", "Radius", "Ignore", // In Params
            "Object Hit", "Position", "Normal", "Distance", "EntityId" // Out Params
        );

        ///////////////////////////////////////////////////////////////
        using Registrar = ScriptCanvas::RegistrarGeneric
            < 
            RayCastWorldSpaceNode,
            RayCastWorldSpaceWithGroupNode,
            RayCastLocalSpaceNode,
            RayCastLocalSpaceWithGroupNode,
            RayCastMultipleLocalSpaceNode,
            RayCastMultipleLocalSpaceWithGroupNode,
            OverlapSphereNode,
            OverlapSphereWithGroupNode,
            OverlapBoxNode,
            OverlapBoxWithGroupNode,
            OverlapCapsuleNode,
            OverlapCapsuleWithGroupNode,
            SphereCastNode,
            SphereCastWithGroupNode,
            BoxCastNode,
            BoxCastWithGroupNode,
            CapsuleCastNode,
            CapsuleCastWithGroupNode
            >;
    }
}
