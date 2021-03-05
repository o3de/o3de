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
#include "PhysXLocks.h"

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace PhysX
{
    inline ActorData* Utils::GetUserData(const physx::PxActor* actor)
    {
        if (actor == nullptr || actor->userData == nullptr)
        {
            return nullptr;
        }

        ActorData* actorData = static_cast<ActorData*>(actor->userData);
        if (!actorData->IsValid())
        {
            AZ_Warning("PhysX::Utils::GetUserData", false, "The actor data does not look valid and is not safe to use");
            return nullptr;
        }

        return actorData;
    }

    inline Physics::Material* Utils::GetUserData(const physx::PxMaterial* material)
    {
        return (material == nullptr) ? nullptr : static_cast<Physics::Material*>(material->userData);
    }

    inline Physics::Shape* Utils::GetUserData(const physx::PxShape* pxShape)
    {
        return (pxShape == nullptr) ? nullptr : static_cast<Physics::Shape*>(pxShape->userData);
    }

    inline Physics::World* Utils::GetUserData(physx::PxScene* scene)
    {
        return scene ? static_cast<Physics::World*>(scene->userData) : nullptr;
    }

    inline AZ::u64 Utils::Collision::Combine(AZ::u32 word0, AZ::u32 word1)
    {
        return (static_cast<AZ::u64>(word0) << 32) | word1;
    }

    inline void Utils::Collision::SetLayer(const AzPhysics::CollisionLayer& layer, physx::PxFilterData& filterData)
    {
        filterData.word0 = (physx::PxU32)(layer.GetMask() >> 32);
        filterData.word1 = (physx::PxU32)(layer.GetMask());
    }

    inline void Utils::Collision::SetGroup(const AzPhysics::CollisionGroup& group, physx::PxFilterData& filterData)
    {
        filterData.word2 = (physx::PxU32)(group.GetMask() >> 32);
        filterData.word3 = (physx::PxU32)(group.GetMask());
    }

    inline void Utils::Collision::SetCollisionLayerAndGroup(physx::PxShape* shape, const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup&  group)
    {
        physx::PxFilterData filterData;
        SetLayer(layer, filterData);
        SetGroup(group, filterData);
        shape->setSimulationFilterData(filterData);
    }

    inline bool Utils::Collision::ShouldCollide(const physx::PxFilterData& filterData0, const physx::PxFilterData& filterData1)
    {
        AZ::u64 layer0 = Combine(filterData0.word0, filterData0.word1);
        AZ::u64 layer1 = Combine(filterData1.word0, filterData1.word1);
        AZ::u64 group0 = Combine(filterData0.word2, filterData0.word3);
        AZ::u64 group1 = Combine(filterData1.word2, filterData1.word3);
        return (layer0 & group1) && (layer1 & group0);
    }

    inline Physics::RayCastHit Utils::RayCast::GetHitFromPxHit(const physx::PxLocationHit& pxHit)
    {
        Physics::RayCastHit hit;

        hit.m_distance = pxHit.distance;
        hit.m_position = PxMathConvert(pxHit.position);
        hit.m_normal = PxMathConvert(pxHit.normal);
        hit.m_body = Utils::GetUserData(pxHit.actor)->GetWorldBody();
        hit.m_shape = Utils::GetUserData(pxHit.shape);

        if (pxHit.faceIndex != 0xFFFFffff)
        {
            PHYSX_SCENE_READ_LOCK(pxHit.actor->getScene());
            hit.m_material = Utils::GetUserData(pxHit.shape->getMaterialFromInternalFaceIndex(pxHit.faceIndex));
        }
        else
        {
            hit.m_material = hit.m_shape->GetMaterial().get();
        }

        return hit;
    }

    inline Physics::RayCastHit Utils::RayCast::ClosestRayHitAgainstPxRigidActor(const Physics::RayCastRequest& worldSpaceRequest, physx::PxRigidActor* actor)
    {
        const physx::PxVec3 start = PxMathConvert(worldSpaceRequest.m_start);
        const physx::PxVec3 unitDir = PxMathConvert(worldSpaceRequest.m_direction);
        const physx::PxU32 maxHits = 1;
        const physx::PxHitFlags hitFlags = Utils::RayCast::GetPxHitFlags(worldSpaceRequest.m_hitFlags);

        physx::PxScene* scene = actor->getScene();
        physx::PxTransform actorTransform;
        {
            PHYSX_SCENE_READ_LOCK(scene);
            actorTransform = actor->getGlobalPose();
        }

        Physics::RayCastHit closestHit;
        float closestHitDistance = FLT_MAX;

        int numShapes = actor->getNbShapes();
        AZStd::vector<physx::PxShape*> shapes(numShapes);
        actor->getShapes(shapes.data(), numShapes);
        for (physx::PxShape* shape : shapes)
        {
            physx::PxTransform shapeTransform;
            {
                PHYSX_SCENE_READ_LOCK(scene);
                shapeTransform = actorTransform * shape->getLocalPose();
            }

            physx::PxRaycastHit pxHitInfo;
            const bool hit = physx::PxGeometryQuery::raycast(start, unitDir, shape->getGeometry().any(), shapeTransform,
                worldSpaceRequest.m_distance, hitFlags, maxHits, &pxHitInfo);

            if (hit && pxHitInfo.distance < closestHitDistance)
            {
                // Fill actor and shape, as they won't be filled from PxGeometryQuery
                pxHitInfo.actor = actor;
                pxHitInfo.shape = shape;
                closestHit = Utils::RayCast::GetHitFromPxHit(pxHitInfo);
                closestHitDistance = pxHitInfo.distance;
            }
        }
        return closestHit;
    }

    inline physx::PxHitFlags Utils::RayCast::GetPxHitFlags(Physics::HitFlags hitFlags)
    {
        static_assert(
            static_cast<AZ::u16>(Physics::HitFlags::Position) == static_cast<AZ::u16>(physx::PxHitFlag::ePOSITION) &&
            static_cast<AZ::u16>(Physics::HitFlags::Normal) == static_cast<AZ::u16>(physx::PxHitFlag::eNORMAL) &&
            static_cast<AZ::u16>(Physics::HitFlags::UV) == static_cast<AZ::u16>(physx::PxHitFlag::eUV) &&
            static_cast<AZ::u16>(Physics::HitFlags::AssumeNoInitialOverlap) == static_cast<AZ::u16>(physx::PxHitFlag::eASSUME_NO_INITIAL_OVERLAP) &&
            static_cast<AZ::u16>(Physics::HitFlags::MeshMultiple) == static_cast<AZ::u16>(physx::PxHitFlag::eMESH_MULTIPLE) &&
            static_cast<AZ::u16>(Physics::HitFlags::MeshAny) == static_cast<AZ::u16>(physx::PxHitFlag::eMESH_ANY) &&
            static_cast<AZ::u16>(Physics::HitFlags::MeshBothSides) == static_cast<AZ::u16>(physx::PxHitFlag::eMESH_BOTH_SIDES) &&
            static_cast<AZ::u16>(Physics::HitFlags::PreciseSweep) == static_cast<AZ::u16>(physx::PxHitFlag::ePRECISE_SWEEP) &&
            static_cast<AZ::u16>(Physics::HitFlags::MTD) == static_cast<AZ::u16>(physx::PxHitFlag::eMTD) &&
            static_cast<AZ::u16>(Physics::HitFlags::FaceIndex) == static_cast<AZ::u16>(physx::PxHitFlag::eFACE_INDEX),
            "Physics::HitFlags values do not match the corresponding PhysX ones.");

        return static_cast<physx::PxHitFlags>(static_cast<AZ::u16>(hitFlags));
    }
}
