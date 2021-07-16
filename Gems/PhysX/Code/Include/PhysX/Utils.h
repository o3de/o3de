/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PxActor.h>
#include <PxMaterial.h>
#include <PxShape.h>
#include <PhysX/UserDataTypes.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

namespace AzPhysics
{
    class CollisionLayer;
    class CollisionGroup;
    class Scene;
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
        AzPhysics::Scene* GetUserData(physx::PxScene* scene);

        namespace Collision
        {
            AZ::u64 Combine(AZ::u32 word0, AZ::u32 word1);
            void SetLayer(const AzPhysics::CollisionLayer& layer, physx::PxFilterData& filterData);
            void SetGroup(const AzPhysics::CollisionGroup& group, physx::PxFilterData& filterData);
            void SetCollisionLayerAndGroup(physx::PxShape* shape, const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group);
            bool ShouldCollide(const physx::PxFilterData& filterData0, const physx::PxFilterData& filterData1);
        }
    }
}

#include <PhysX/Utils.inl>
