/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/Data.h>
#include <AzFramework/Physics/Material/PhysicsMaterialId.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>

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

        static constexpr const char* k_categoryName = "PhysX/World";

        AZ_INLINE Result RayCastWorldSpaceWithGroup(const AZ::Vector3& start,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AzPhysics::RayCastRequest request;
            request.m_start = start;
            request.m_direction = direction.GetNormalized();
            request.m_distance = distance;
            request.m_filterCallback = [ignore](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore ? AzPhysics::SceneQuery::QueryHitType::Block : AzPhysics::SceneQuery::QueryHitType::None;
            };

            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            AzPhysics::SceneQueryHits result;
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                if (AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                    sceneHandle != AzPhysics::InvalidSceneHandle)
                {
                    result = sceneInterface->QueryScene(sceneHandle, &request);
                }
            }
            if (result)
            {
                const AZ::Crc32 surfaceType(result.m_hits[0].m_physicsMaterialId.ToString<AZStd::string>());
                return AZStd::make_tuple(result.m_hits[0].IsValid(), result.m_hits[0].m_position,
                    result.m_hits[0].m_normal, result.m_hits[0].m_distance,
                    result.m_hits[0].m_entityId, surfaceType);
            }
            return AZStd::make_tuple(false, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), 0.0f, AZ::EntityId(), AZ::Crc32());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RayCastWorldSpaceWithGroup,
            k_categoryName,
            "{695EE108-68C1-40E3-ADA5-8ED9AB74D054}", "ScriptCanvasPhysics_WorldFunctions_RayCastWorldSpaceWithGroup");

        AZ_INLINE Result RayCastFromScreenWithGroup(
            const AZ::Vector2& screenPosition,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AZ::EntityId camera;
            Camera::CameraSystemRequestBus::BroadcastResult(camera, &Camera::CameraSystemRequestBus::Events::GetActiveCamera);
            if (camera.IsValid())
            {
                AZ::Vector3 origin = AZ::Vector3::CreateZero();
                Camera::CameraRequestBus::EventResult(origin, camera, &Camera::CameraRequestBus::Events::ScreenToWorld, screenPosition, 0.f);
                AZ::Vector3 offset = AZ::Vector3::CreateZero();
                Camera::CameraRequestBus::EventResult(offset, camera, &Camera::CameraRequestBus::Events::ScreenToWorld, screenPosition, 1.f);
                const AZ::Vector3 direction = (offset - origin).GetNormalized();
                return RayCastWorldSpaceWithGroup(origin, direction, distance, collisionGroup, ignore);
            }

            // fallback in the rare case there is no active camera
            return AZStd::make_tuple(false, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), 0.0f, AZ::EntityId(), AZ::Crc32());
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RayCastFromScreenWithGroup,
            k_categoryName,
            "{8F98A766-A93F-4DA7-B281-482C3DB20649}", "ScriptCanvasPhysics_WorldFunctions_RayCastFromScreenWithGroup");

        AZ_INLINE Result RayCastLocalSpaceWithGroup(const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            return RayCastWorldSpaceWithGroup(worldSpaceTransform.GetTranslation(), worldSpaceTransform.TransformVector(direction.GetNormalized()),
                distance, collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RayCastLocalSpaceWithGroup,
            k_categoryName,
            "{938E0C6E-C6A3-4716-9233-941EFA70241A}", "ScriptCanvasPhysics_WorldFunctions_RayCastLocalSpaceWithGroup");

        AZ_INLINE AZStd::vector<AzPhysics::SceneQueryHit> RayCastMultipleLocalSpaceWithGroup(const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            AzPhysics::RayCastRequest request;
            request.m_start = worldSpaceTransform.GetTranslation();
            request.m_direction = worldSpaceTransform.TransformVector(direction.GetNormalized());
            request.m_distance = distance;
            request.m_reportMultipleHits = true;
            request.m_filterCallback = [ignore](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore ? AzPhysics::SceneQuery::QueryHitType::Touch : AzPhysics::SceneQuery::QueryHitType::None;
            };
            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            AzPhysics::SceneQueryHits result;
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                if (AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                    sceneHandle != AzPhysics::InvalidSceneHandle)
                {
                    result = sceneInterface->QueryScene(sceneHandle, &request);
                }
            }
            return result.m_hits;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            RayCastMultipleLocalSpaceWithGroup,
            k_categoryName,
            "{A867FC55-6610-42C2-97E8-C614450CAE92}", "ScriptCanvasPhysics_WorldFunctions_RayCastMultipleLocalSpaceWithGroup");

        OverlapResult OverlapQuery(const AZ::Transform& pose,
            AZStd::shared_ptr<Physics::ShapeConfiguration> shape,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AzPhysics::OverlapRequest request;
            request.m_pose = pose;
            request.m_shapeConfiguration = shape;
            request.m_filterCallback = [ignore](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore;
            };

            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            AzPhysics::SceneQueryHits results;
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                if (AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                    sceneHandle != AzPhysics::InvalidSceneHandle)
                {
                    results = sceneInterface->QueryScene(sceneHandle, &request);
                }
            }

            AZStd::vector<AZ::EntityId> overlapIds;
            overlapIds.reserve(results.m_hits.size());
            AZStd::transform(results.m_hits.begin(), results.m_hits.end(), AZStd::back_inserter(overlapIds),
                [](const AzPhysics::SceneQueryHit& overlap)
                {
                    return overlap.m_entityId;
                });
            return AZStd::make_tuple(!overlapIds.empty(), overlapIds);
        }

        AZ_INLINE OverlapResult OverlapSphereWithGroup(const AZ::Vector3& position,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return OverlapQuery(AZ::Transform::CreateTranslation(position),
                AZStd::make_shared<Physics::SphereShapeConfiguration>(radius), collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            OverlapSphereWithGroup,
            k_categoryName,
            "{0A2831AB-E994-4533-8E64-700631994E64}", "ScriptCanvasPhysics_WorldFunctions_OverlapSphereWithGroup");

        AZ_INLINE OverlapResult OverlapBoxWithGroup(const AZ::Transform& pose,
            const AZ::Vector3& dimensions,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return OverlapQuery(pose,
                AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions), collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            OverlapBoxWithGroup,
            k_categoryName,
            "{1991BA3D-3848-4BF0-B696-C39C42CFE49A}", "ScriptCanvasPhysics_WorldFunctions_OverlapBoxWithGroup");

        AZ_INLINE OverlapResult OverlapCapsuleWithGroup(const AZ::Transform& pose,
            float height,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return OverlapQuery(pose,
                AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius), collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            OverlapCapsuleWithGroup,
            k_categoryName,
            "{1DD49D7A-348A-4CB1-82C0-D93FE01FEFA1}", "ScriptCanvasPhysics_WorldFunctions_OverlapCapsuleWithGroup");

        Result ShapecastQuery(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            AZStd::shared_ptr<Physics::ShapeConfiguration> shape,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AzPhysics::ShapeCastRequest request;
            request.m_distance = distance;
            request.m_start = pose;
            request.m_direction = direction;
            request.m_shapeConfiguration = shape;
            request.m_filterCallback = [ignore](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore ? AzPhysics::SceneQuery::QueryHitType::Block : AzPhysics::SceneQuery::QueryHitType::None;
            };

            if (!collisionGroup.empty())
            {
                request.m_collisionGroup = AzPhysics::CollisionGroup(collisionGroup);
            }

            AzPhysics::SceneQueryHits result;
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                if (AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                    sceneHandle != AzPhysics::InvalidSceneHandle)
                {
                    result = sceneInterface->QueryScene(sceneHandle, &request);
                }
            }
            if (result)
            {
                const AZ::Crc32 surfaceType(result.m_hits[0].m_physicsMaterialId.ToString<AZStd::string>());
                return AZStd::make_tuple(result.m_hits[0].IsValid(), result.m_hits[0].m_position,
                    result.m_hits[0].m_normal, result.m_hits[0].m_distance,
                    result.m_hits[0].m_entityId, surfaceType);
            }
            return AZStd::make_tuple(false, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), 0.0f, AZ::EntityId(), AZ::Crc32());
        }

        AZ_INLINE Result SphereCastWithGroup(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return ShapecastQuery(distance, pose, direction,
                AZStd::make_shared<Physics::SphereShapeConfiguration>(radius), collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            SphereCastWithGroup,
            k_categoryName,
            "{7A4D8893-51F5-444F-9C77-64D179F9C9BB}", "ScriptCanvasPhysics_WorldFunctions_SphereCastWithGroup");

        AZ_INLINE Result BoxCastWithGroup(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            const AZ::Vector3& dimensions,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return ShapecastQuery(distance, pose, direction,
                AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions), collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            BoxCastWithGroup,
            k_categoryName,
            "{E7C2CFE0-3FB9-438B-9A8A-A5D333AB0791}", "ScriptCanvasPhysics_WorldFunctions_BoxCastWithGroup");

        AZ_INLINE Result CapsuleCastWithGroup(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            float height,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return ShapecastQuery(distance, pose, direction,
                AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius), collisionGroup, ignore);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            CapsuleCastWithGroup,
            k_categoryName,
            "{938B047C-6282-4510-8AFE-21D58426061D}", "ScriptCanvasPhysics_WorldFunctions_CapsuleCastWithGroup");

        ///////////////////////////////////////////////////////////////
        using Registrar = ScriptCanvas::RegistrarGeneric
            <RayCastWorldSpaceWithGroupNode,
            RayCastLocalSpaceWithGroupNode,
            RayCastMultipleLocalSpaceWithGroupNode,
            RayCastFromScreenWithGroupNode,
            OverlapSphereWithGroupNode,
            OverlapBoxWithGroupNode,
            OverlapCapsuleWithGroupNode,
            BoxCastWithGroupNode,
            SphereCastWithGroupNode,
            CapsuleCastWithGroupNode>;
    }
}
