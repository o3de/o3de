/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <PxPhysicsAPI.h>

namespace PhysX
{
    inline ActorData* Utils::GetUserData(const physx::PxActor* actor)
    {
        if (actor == nullptr || actor->userData == nullptr)
        {
            return nullptr;
        }

        ActorData* actorData = static_cast<ActorData*>(actor->userData);
        if (!actorData || !actorData->IsValid())
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

    inline AzPhysics::Scene* Utils::GetUserData(physx::PxScene* scene)
    {
        return scene ? static_cast<AzPhysics::Scene*>(scene->userData) : nullptr;
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
}
