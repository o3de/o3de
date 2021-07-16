/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PxPhysicsAPI.h>
#include <AzCore/base.h>

namespace AzPhysics
{
    class CollisionGroup;
    class CollisionLayer;
}

namespace PhysX
{
    namespace Collision
    {
        physx::PxFilterFlags DefaultFilterShader(
            physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);

        physx::PxFilterFlags DefaultFilterShaderCCD(
            physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);

        AZ::u64 Combine(AZ::u32 word0, AZ::u32 word1);
        physx::PxFilterData CreateFilterData(const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group);
        void SetLayer(const AzPhysics::CollisionLayer& layer, physx::PxFilterData& filterData);
        void SetGroup(const AzPhysics::CollisionGroup& group, physx::PxFilterData& filterData);
        bool ShouldCollide(const physx::PxFilterData& filterData0, const physx::PxFilterData& filterData1);
    }
}
