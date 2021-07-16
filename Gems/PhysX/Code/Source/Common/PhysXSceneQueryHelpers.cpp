/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/limits.h>
#include <Common/PhysXSceneQueryHelpers.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>
#include <Collision.h>
#include <Shape.h>

namespace PhysX
{
    namespace SceneQueryHelpers
    {
        physx::PxQueryFlags GetPxQueryFlags(const AzPhysics::SceneQuery::QueryType& queryType)
        {
            physx::PxQueryFlags queryFlags = physx::PxQueryFlag::ePREFILTER;
            switch (queryType)
            {
            case AzPhysics::SceneQuery::QueryType::StaticAndDynamic:
                queryFlags |= physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC;
                break;
            case AzPhysics::SceneQuery::QueryType::Dynamic:
                queryFlags |= physx::PxQueryFlag::eDYNAMIC;
                break;
            case AzPhysics::SceneQuery::QueryType::Static:
                queryFlags |= physx::PxQueryFlag::eSTATIC;
                break;
            default:
                AZ_Warning("Physics::World", false, "Unhandled queryType");
                break;
            }
            return queryFlags;
        }

        AzPhysics::SceneQueryHit GetHitFromPxHit(const physx::PxLocationHit& pxHit)
        {
            AzPhysics::SceneQueryHit hit;

            hit.m_distance = pxHit.distance;
            hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Distance;

            if (pxHit.flags & physx::PxHitFlag::ePOSITION)
            {
                hit.m_position = PxMathConvert(pxHit.position);
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Position;
            }

            if (pxHit.flags & physx::PxHitFlag::eNORMAL)
            {
                hit.m_normal = PxMathConvert(pxHit.normal);
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Normal;
            }

            const ActorData* actorData = Utils::GetUserData(pxHit.actor);
            hit.m_bodyHandle = actorData->GetBodyHandle();
            if (hit.m_bodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::BodyHandle;
            }
            hit.m_entityId = actorData->GetEntityId();
            if (hit.m_entityId.IsValid())
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::EntityId;
            }
            hit.m_shape = Utils::GetUserData(pxHit.shape);
            if (hit.m_shape != nullptr)
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Shape;
            }

            if (pxHit.faceIndex != 0xFFFFffff)
            {
                PHYSX_SCENE_READ_LOCK(pxHit.actor->getScene());
                hit.m_material = Utils::GetUserData(pxHit.shape->getMaterialFromInternalFaceIndex(pxHit.faceIndex));
            }
            else if (hit.m_shape != nullptr)
            {
                hit.m_material = hit.m_shape->GetMaterial().get();
            }
            if (hit.m_material != nullptr)
            {
                hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Material;
            }

            return hit;
        }

        AzPhysics::SceneQueryHit GetHitFromPxOverlapHit(const physx::PxOverlapHit& pxHit)
        {
            AzPhysics::SceneQueryHit hit;
            if (ActorData* actorData = Utils::GetUserData(pxHit.actor))
            {
                hit.m_entityId = actorData->GetEntityId();
                if (hit.m_entityId.IsValid())
                {
                    hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::EntityId;
                }

                hit.m_bodyHandle = actorData->GetBodyHandle();
                if (hit.m_bodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
                {
                    hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::BodyHandle;
                }

                if (pxHit.shape != nullptr)
                {
                    hit.m_shape = static_cast<PhysX::Shape*>(pxHit.shape->userData);
                    hit.m_resultFlags |= AzPhysics::SceneQuery::ResultFlags::Shape;
                }
            }
            return hit;
        }

        physx::PxHitFlags GetPxHitFlags(AzPhysics::SceneQuery::HitFlags hitFlags)
        {
            static_assert(
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::Position) == static_cast<AZ::u16>(physx::PxHitFlag::ePOSITION) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::Normal) == static_cast<AZ::u16>(physx::PxHitFlag::eNORMAL) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::UV) == static_cast<AZ::u16>(physx::PxHitFlag::eUV) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::AssumeNoInitialOverlap) == static_cast<AZ::u16>(physx::PxHitFlag::eASSUME_NO_INITIAL_OVERLAP) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::MeshMultiple) == static_cast<AZ::u16>(physx::PxHitFlag::eMESH_MULTIPLE) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::MeshAny) == static_cast<AZ::u16>(physx::PxHitFlag::eMESH_ANY) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::MeshBothSides) == static_cast<AZ::u16>(physx::PxHitFlag::eMESH_BOTH_SIDES) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::PreciseSweep) == static_cast<AZ::u16>(physx::PxHitFlag::ePRECISE_SWEEP) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::MTD) == static_cast<AZ::u16>(physx::PxHitFlag::eMTD) &&
                static_cast<AZ::u16>(AzPhysics::SceneQuery::HitFlags::FaceIndex) == static_cast<AZ::u16>(physx::PxHitFlag::eFACE_INDEX),
                "Physics::HitFlags values do not match the corresponding PhysX ones.");

            return static_cast<physx::PxHitFlags>(static_cast<AZ::u16>(hitFlags));
        }

        physx::PxQueryHitType::Enum GetPxHitType(AzPhysics::SceneQuery::QueryHitType hitType)
        {
            static_assert(static_cast<int>(AzPhysics::SceneQuery::QueryHitType::None) == static_cast<int>(physx::PxQueryHitType::eNONE) &&
                static_cast<int>(AzPhysics::SceneQuery::QueryHitType::Touch) == static_cast<int>(physx::PxQueryHitType::eTOUCH) &&
                static_cast<int>(AzPhysics::SceneQuery::QueryHitType::Block) == static_cast<int>(physx::PxQueryHitType::eBLOCK),
                "PhysX hit types do not match QueryHitTypes");
            return static_cast<physx::PxQueryHitType::Enum>(hitType);
        }

        AzPhysics::SceneQueryHit ClosestRayHitAgainstPxRigidActor(
            const AzPhysics::RayCastRequest& worldSpaceRequest, physx::PxRigidActor* actor)
        {
            const physx::PxVec3 start = PxMathConvert(worldSpaceRequest.m_start);
            const physx::PxVec3 unitDir = PxMathConvert(worldSpaceRequest.m_direction.GetNormalized());
            const physx::PxU32 maxHits = 1;
            const physx::PxHitFlags hitFlags = GetPxHitFlags(worldSpaceRequest.m_hitFlags);

            AzPhysics::SceneQueryHit closestHit;
            float closestHitDistance = FLT_MAX;

            int numShapes = actor->getNbShapes();
            AZStd::vector<physx::PxShape*> shapes(numShapes);
            actor->getShapes(shapes.data(), numShapes);

            {
                physx::PxScene* scene = actor->getScene();
                PHYSX_SCENE_READ_LOCK(scene);

                const physx::PxTransform actorTransform = actor->getGlobalPose();
                for (physx::PxShape* shape : shapes)
                {
                    const physx::PxTransform shapeTransform = actorTransform * shape->getLocalPose();

                    physx::PxRaycastHit pxHitInfo;
                    const bool hit = physx::PxGeometryQuery::raycast(start, unitDir, shape->getGeometry().any(), shapeTransform,
                        worldSpaceRequest.m_distance, hitFlags, maxHits, &pxHitInfo);

                    if (hit && pxHitInfo.distance < closestHitDistance)
                    {
                        // Fill actor and shape, as they won't be filled from PxGeometryQuery
                        pxHitInfo.actor = actor;
                        pxHitInfo.shape = shape;
                        closestHit = GetHitFromPxHit(pxHitInfo);
                        closestHitDistance = pxHitInfo.distance;
                    }
                }
            }
            return closestHit;
        }

        AzPhysics::SceneQueryHit ClosestRayHitAgainstShapes(const AzPhysics::RayCastRequest& request,
            const AZStd::vector<AZStd::shared_ptr<PhysX::Shape>>& shapes, const AZ::Transform& parentTransform)
        {
            AzPhysics::SceneQueryHit closestHit;
            float closestHitDist = AZStd::numeric_limits<float>::max();
            for (auto& shape : shapes)
            {
                AzPhysics::SceneQueryHit hit = shape->RayCast(request, parentTransform);
                if (hit && hit.m_distance < closestHitDist)
                {
                    closestHit = hit;
                    closestHitDist = hit.m_distance;
                }
            }
            return closestHit;
        }

        AzPhysics::SceneQuery::FilterCallback GetSceneQueryBlockFilterCallback(const AzPhysics::SceneQuery::FilterCallback& filterCallback)
        {
            if (!filterCallback)
            {
                return nullptr;
            }

            return [filterCallback](const AzPhysics::SimulatedBody* body, const Physics::Shape* shape)
            {
                if (filterCallback(body, shape) != AzPhysics::SceneQuery::QueryHitType::None)
                {
                    return AzPhysics::SceneQuery::QueryHitType::Block;
                }
                return AzPhysics::SceneQuery::QueryHitType::None;
            };
        }

        AzPhysics::SceneQuery::FilterCallback GetFilterCallbackFromOverlap(const AzPhysics::SceneQuery::OverlapFilterCallback& overlapFilterCallback)
        {
            if (!overlapFilterCallback)
            {
                return nullptr;
            }

            return [overlapFilterCallback](const AzPhysics::SimulatedBody* body, const Physics::Shape* shape)
            {
                if (overlapFilterCallback(body, shape))
                {
                    return AzPhysics::SceneQuery::QueryHitType::Touch;
                }
                return AzPhysics::SceneQuery::QueryHitType::None;
            };
        }

        PhysXQueryFilterCallback::PhysXQueryFilterCallback(
            const AzPhysics::CollisionGroup& collisionGroup,
            AzPhysics::SceneQuery::FilterCallback filterCallback,
            physx::PxQueryHitType::Enum hitType)
            : m_filterCallback(AZStd::move(filterCallback))
            , m_collisionGroup(collisionGroup)
            , m_hitType(hitType)
        {
        }

        //Performs game specific entity filtering
        physx::PxQueryHitType::Enum PhysXQueryFilterCallback::preFilter(
            [[maybe_unused]] const physx::PxFilterData& queryFilterData, const physx::PxShape* pxShape,
            const physx::PxRigidActor* actor, [[maybe_unused]] physx::PxHitFlags& queryTypes)
        {
            auto shapeFilterData = pxShape->getQueryFilterData();

            if (m_collisionGroup.GetMask() & PhysX::Collision::Combine(shapeFilterData.word0, shapeFilterData.word1))
            {
                if (m_filterCallback)
                {
                    ActorData* userData = Utils::GetUserData(actor);
                    Physics::Shape* shape = Utils::GetUserData(pxShape);
                    if (userData != nullptr && userData->GetSimulatedBody())
                    {
                        return GetPxHitType(m_filterCallback(userData->GetSimulatedBody(), shape));
                    }
                }
                return m_hitType;
            }
            return physx::PxQueryHitType::eNONE;
        }

        // Unused, we're only pre-filtering at this time
        physx::PxQueryHitType::Enum PhysXQueryFilterCallback::postFilter(
            [[maybe_unused]] const physx::PxFilterData&, [[maybe_unused]] const physx::PxQueryHit&)
        {
            return physx::PxQueryHitType::eNONE;
        }

        UnboundedOverlapCallback::UnboundedOverlapCallback(const AzPhysics::SceneQuery::UnboundedOverlapHitCallback& hitCallback,
            AZStd::vector<physx::PxOverlapHit>& hitBuffer)
            : m_hitCallback(hitCallback)
            , physx::PxHitCallback<physx::PxOverlapHit>(hitBuffer.begin(), static_cast<physx::PxU32>(hitBuffer.size()))
        {}

        physx::PxAgain UnboundedOverlapCallback::processTouches(const physx::PxOverlapHit* buffer, physx::PxU32 numHits)
        {
            for (auto it = buffer; it != buffer + numHits; ++it)
            {
                const AzPhysics::SceneQueryHit hit = SceneQueryHelpers::GetHitFromPxOverlapHit(*it);
                if (hit.IsValid() && !m_hitCallback(AZStd::optional<AzPhysics::SceneQueryHit>(hit)))
                {
                    return false;
                }
                m_results.m_hits.emplace_back(hit);
            }
            return true;
        };

        void UnboundedOverlapCallback::finalizeQuery()
        {
            m_hitCallback({});
        }
    } // namespace SceneQuery
}
