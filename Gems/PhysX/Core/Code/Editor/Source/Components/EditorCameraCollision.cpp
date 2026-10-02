/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/Components/EditorCameraCollision.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <Shape.h>
#include <PhysX/MathConversion.h>
#include <extensions/PxShapeExt.h>

namespace PhysX
{
    namespace
    {
        AZ::Vector3 FindCameraPenetration(
            AzPhysics::SceneInterface& sceneInterface, const AzPhysics::SceneHandle sceneHandle,
            const AZ::Vector3& position, const float radius, const float contactGap)
        {
            AZ::Vector3 separation = AZ::Vector3::CreateZero();
            float deepestPenetration = 0.0f;
            const physx::PxSphereGeometry sphere(radius);
            const physx::PxTransform pose(PxMathConvert(position));
            auto overlap = AzPhysics::OverlapRequestHelpers::CreateSphereOverlapRequest(
                radius, AZ::Transform::CreateTranslation(position),
                [&](const AzPhysics::SimulatedBody*, const Physics::Shape* shape)
                {
                    const auto* physxShape = azrtti_cast<const PhysX::Shape*>(shape);
                    if (physxShape && !physxShape->IsTrigger())
                    {
                        const auto* nativeShape = static_cast<const physx::PxShape*>(physxShape->GetNativePointer());
                        const auto* actor = nativeShape->getActor();
                        physx::PxVec3 direction;
                        float depth = 0.0f;
                        // The query invokes this callback under the scene read lock. Compute the actual overlap here
                        // and retain no native pointers after the query. This also avoids a bounded hit buffer.
                        if (actor && physx::PxGeometryQuery::computePenetration(
                                direction, depth, sphere, pose, nativeShape->getGeometry(),
                                physx::PxShapeExt::getGlobalPose(*nativeShape, *actor))
                            && direction.isFinite() && AZ::IsFiniteFloat(depth) && depth > deepestPenetration)
                        {
                            deepestPenetration = depth;
                            separation = PxMathConvert(direction) * (depth + contactGap);
                        }
                    }
                    return false;
                });
            sceneInterface.QueryScene(sceneHandle, &overlap);
            return separation;
        }
    } // namespace

    AZ::Vector3 ResolveCameraCollision(
        const AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& previousPosition, const AZ::Vector3& desiredPosition)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (!sceneInterface || sceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return desiredPosition;
        }

        constexpr float CameraRadius = 0.25f;
        constexpr float ContactGap = 0.001f;
        constexpr float MovementTolerance = 1.0e-5f;
        constexpr int MaxIterations = 4;

        auto request = AzPhysics::ShapeCastRequestHelpers::CreateSphereCastRequest(
            CameraRadius, AZ::Transform::CreateTranslation(previousPosition), AZ::Vector3::CreateAxisZ(), 0.0f,
            AzPhysics::SceneQuery::QueryType::StaticAndDynamic, AzPhysics::CollisionGroup::All,
            []([[maybe_unused]] const AzPhysics::SimulatedBody* body, const Physics::Shape* shape)
            {
                const auto* physxShape = azrtti_cast<const PhysX::Shape*>(shape);
                return physxShape && !physxShape->IsTrigger()
                    ? AzPhysics::SceneQuery::QueryHitType::Block : AzPhysics::SceneQuery::QueryHitType::None;
            });
        request.m_hitFlags |= AzPhysics::SceneQuery::HitFlags::MeshBothSides;

        AZ::Vector3 position = previousPosition;
        AZ::Vector3 remaining = desiredPosition - previousPosition;
        AZStd::fixed_vector<AZ::Vector3, MaxIterations> contactNormals;
        for (int iteration = 0; iteration < MaxIterations; ++iteration)
        {
            // A zero-length sweep can miss an initial heightfield overlap. Recover penetration independently
            // of movement direction, including when collision is enabled while the camera is stationary.
            const AZ::Vector3 separation = FindCameraPenetration(*sceneInterface, sceneHandle, position, CameraRadius, ContactGap);
            if (!separation.IsZero(MovementTolerance))
            {
                position += separation;
                continue;
            }
            const float distance = remaining.GetLength();
            if (distance <= MovementTolerance)
            {
                return position;
            }
            request.m_start.SetTranslation(position);
            request.m_distance = distance;
            request.m_direction = remaining / distance;
            const auto hits = sceneInterface->QueryScene(sceneHandle, &request);
            if (hits.m_hits.empty())
            {
                return position + remaining;
            }

            const auto& hit = hits.m_hits.front();
            const AZ::Vector3 normal = hit.m_normal.GetNormalizedSafe();
            if (!normal.IsFinite() || normal.IsZero() || !AZ::IsFiniteFloat(hit.m_distance))
            {
                return position;
            }

            if (hit.m_distance <= 0.0f)
            {
                // MTD provides a separation direction and negative penetration depth. Also separate exact contacts
                // so a camera placed on a surface can move away without repeatedly hitting it at zero distance.
                position += normal * (-hit.m_distance + ContactGap);
            }
            else if (distance > MovementTolerance)
            {
                const float travel = AZ::GetClamp(hit.m_distance - ContactGap, 0.0f, distance);
                position += request.m_direction * travel;
                remaining -= request.m_direction * travel;
            }
            else
            {
                return position;
            }

            contactNormals.push_back(normal);
            const float intoSurface = remaining.Dot(normal);
            if (intoSurface < 0.0f)
            {
                remaining -= normal * intoSurface;
            }

            // In a corner, slide along the intersection of the contact planes instead of re-entering an earlier surface.
            for (const auto& previousNormal : contactNormals)
            {
                if (remaining.Dot(previousNormal) < -MovementTolerance)
                {
                    const AZ::Vector3 crease = normal.Cross(previousNormal).GetNormalizedSafe();
                    remaining = crease * remaining.Dot(crease);
                    for (const auto& contactNormal : contactNormals)
                    {
                        if (remaining.Dot(contactNormal) < -MovementTolerance)
                        {
                            remaining = AZ::Vector3::CreateZero();
                            break;
                        }
                    }
                }
            }
        }

        // Never apply untested displacement if the iteration budget was exhausted.
        return position;
    }
} // namespace PhysX
