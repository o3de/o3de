/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "World.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Material/PhysicsMaterialId.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace ScriptCanvasPhysics
{
    namespace WorldFunctions
    {
        Result RayCastWorldSpaceWithGroup(
            const AZ::Vector3& start,
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
                return body->GetEntityId() != ignore ? AzPhysics::SceneQuery::QueryHitType::Block
                                                     : AzPhysics::SceneQuery::QueryHitType::None;
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
                return AZStd::make_tuple(
                    result.m_hits[0].IsValid(), result.m_hits[0].m_position, result.m_hits[0].m_normal, result.m_hits[0].m_distance,
                    result.m_hits[0].m_entityId, surfaceType);
            }
            return AZStd::make_tuple(false, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), 0.0f, AZ::EntityId(), AZ::Crc32());
        }

        Result RayCastFromScreenWithGroup(
            const AZ::Vector2& screenPosition, float distance, const AZStd::string& collisionGroup, AZ::EntityId ignore)
        {
            AZ::EntityId camera;
            Camera::CameraSystemRequestBus::BroadcastResult(camera, &Camera::CameraSystemRequestBus::Events::GetActiveCamera);
            if (camera.IsValid())
            {
                AZ::Vector3 origin = AZ::Vector3::CreateZero();
                Camera::CameraRequestBus::EventResult(
                    origin, camera, &Camera::CameraRequestBus::Events::ScreenToWorld, screenPosition, 0.f);
                AZ::Vector3 offset = AZ::Vector3::CreateZero();
                Camera::CameraRequestBus::EventResult(
                    offset, camera, &Camera::CameraRequestBus::Events::ScreenToWorld, screenPosition, 1.f);
                const AZ::Vector3 direction = (offset - origin).GetNormalized();
                return RayCastWorldSpaceWithGroup(origin, direction, distance, collisionGroup, ignore);
            }

            // fallback in the rare case there is no active camera
            return AZStd::make_tuple(false, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), 0.0f, AZ::EntityId(), AZ::Crc32());
        }

        Result RayCastLocalSpaceWithGroup(
            const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            return RayCastWorldSpaceWithGroup(
                worldSpaceTransform.GetTranslation(), worldSpaceTransform.TransformVector(direction.GetNormalized()), distance,
                collisionGroup, ignore);
        }

        AZStd::vector<AzPhysics::SceneQueryHit> RayCastMultipleLocalSpaceWithGroup(
            const AZ::EntityId& fromEntityId,
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
                return body->GetEntityId() != ignore ? AzPhysics::SceneQuery::QueryHitType::Touch
                                                     : AzPhysics::SceneQuery::QueryHitType::None;
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

        OverlapResult OverlapQuery(
            const AZ::Transform& pose,
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
            AZStd::transform(
                results.m_hits.begin(), results.m_hits.end(), AZStd::back_inserter(overlapIds),
                [](const AzPhysics::SceneQueryHit& overlap)
                {
                    return overlap.m_entityId;
                });
            return AZStd::make_tuple(!overlapIds.empty(), overlapIds);
        }

        OverlapResult OverlapSphereWithGroup(
            const AZ::Vector3& position, float radius, const AZStd::string& collisionGroup, AZ::EntityId ignore)
        {
            return OverlapQuery(
                AZ::Transform::CreateTranslation(position), AZStd::make_shared<Physics::SphereShapeConfiguration>(radius), collisionGroup,
                ignore);
        }

        OverlapResult OverlapBoxWithGroup(
            const AZ::Transform& pose, const AZ::Vector3& dimensions, const AZStd::string& collisionGroup, AZ::EntityId ignore)
        {
            return OverlapQuery(pose, AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions), collisionGroup, ignore);
        }

        OverlapResult OverlapCapsuleWithGroup(
            const AZ::Transform& pose, float height, float radius, const AZStd::string& collisionGroup, AZ::EntityId ignore)
        {
            return OverlapQuery(pose, AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius), collisionGroup, ignore);
        }

        Result ShapecastQuery(
            float distance,
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
                return body->GetEntityId() != ignore ? AzPhysics::SceneQuery::QueryHitType::Block
                                                     : AzPhysics::SceneQuery::QueryHitType::None;
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
                return AZStd::make_tuple(
                    result.m_hits[0].IsValid(), result.m_hits[0].m_position, result.m_hits[0].m_normal, result.m_hits[0].m_distance,
                    result.m_hits[0].m_entityId, surfaceType);
            }
            return AZStd::make_tuple(false, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), 0.0f, AZ::EntityId(), AZ::Crc32());
        }

        Result SphereCastWithGroup(
            float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return ShapecastQuery(
                distance, pose, direction, AZStd::make_shared<Physics::SphereShapeConfiguration>(radius), collisionGroup, ignore);
        }

        Result BoxCastWithGroup(
            float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            const AZ::Vector3& dimensions,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return ShapecastQuery(
                distance, pose, direction, AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions), collisionGroup, ignore);
        }

        Result CapsuleCastWithGroup(
            float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            float height,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore)
        {
            return ShapecastQuery(
                distance, pose, direction, AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius), collisionGroup, ignore);
        }
    } // namespace WorldFunctions
} // namespace ScriptCanvasPhysics
