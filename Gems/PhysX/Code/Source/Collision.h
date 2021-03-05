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

#include <PxPhysicsAPI.h>

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
