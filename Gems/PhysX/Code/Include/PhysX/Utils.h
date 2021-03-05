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

#include <PxActor.h>
#include <PxMaterial.h>
#include <PxShape.h>
#include <PhysX/UserDataTypes.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/Shape.h>

namespace AzPhysics
{
    class CollisionLayer;
    class CollisionGroup;
}

namespace Physics
{
    class Material;
    class Shape;
}

namespace PhysX
{
    class Shape;

    namespace Utils
    {
        ActorData* GetUserData(const physx::PxActor* actor);
        Physics::Material* GetUserData(const physx::PxMaterial* material);
        Physics::Shape* GetUserData(const physx::PxShape* shape);
        Physics::World* GetUserData(physx::PxScene* scene);

        namespace Collision
        {
            AZ::u64 Combine(AZ::u32 word0, AZ::u32 word1);
            void SetLayer(const AzPhysics::CollisionLayer& layer, physx::PxFilterData& filterData);
            void SetGroup(const AzPhysics::CollisionGroup& group, physx::PxFilterData& filterData);
            void SetCollisionLayerAndGroup(physx::PxShape* shape, const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group);
            bool ShouldCollide(const physx::PxFilterData& filterData0, const physx::PxFilterData& filterData1);
        }

        namespace RayCast
        {
            Physics::RayCastHit GetHitFromPxHit(const physx::PxLocationHit& pxHit);
            physx::PxHitFlags GetPxHitFlags(Physics::HitFlags hitFlags);
            Physics::RayCastHit ClosestRayHitAgainstPxRigidActor(const Physics::RayCastRequest& request, physx::PxRigidActor* actor);
        }
    }
}

#include <PhysX/Utils.inl>
