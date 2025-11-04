/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Collision.h>
#include <PhysX/Utils.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace PhysX
{
    namespace Collision
    {
        // Combines two 32 bit values into 1 64 bit
        AZ::u64 Combine(AZ::u32 word0, AZ::u32 word1)
        {
            return Utils::Collision::Combine(word0, word1);
        }

        physx::PxFilterFlags DefaultFilterShader(
            physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags, [[maybe_unused]] const void* constantBlock, [[maybe_unused]] physx::PxU32 constantBlockSize)
        {
            if (!ShouldCollide(filterData0, filterData1))
            {
                return physx::PxFilterFlag::eSUPPRESS;
            }

            // let triggers through
            if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
            {
                pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
                return physx::PxFilterFlag::eDEFAULT;
            }

            // generate contacts for all that were not filtered above
            pairFlags =
                physx::PxPairFlag::eCONTACT_DEFAULT |
                physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
                physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
                physx::PxPairFlag::eNOTIFY_TOUCH_LOST |
                physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

            // resolving contacts between two kinematic objects is invalid
            if (physx::PxFilterObjectIsKinematic(attributes0) && physx::PxFilterObjectIsKinematic(attributes1))
            {
                pairFlags.clear(physx::PxPairFlag::eSOLVE_CONTACT);
            }

            // generate callbacks for collisions between kinematic and dynamic objects
            if (physx::PxFilterObjectIsKinematic(attributes0) != physx::PxFilterObjectIsKinematic(attributes1))
            {
                return physx::PxFilterFlag::eCALLBACK;
            }

            return physx::PxFilterFlag::eDEFAULT;

        }

        physx::PxFilterFlags DefaultFilterShaderCCD(
            physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags, [[maybe_unused]] const void* constantBlock, [[maybe_unused]] physx::PxU32 constantBlockSize)
        {
            if (!ShouldCollide(filterData0, filterData1))
            {
                return physx::PxFilterFlag::eSUPPRESS;
            }

            // let triggers through
            if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
            {
                pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT |
                    physx::PxPairFlag::eNOTIFY_TOUCH_CCD;
                return physx::PxFilterFlag::eDEFAULT;
            }

            // generate contacts for all that were not filtered above
            pairFlags =
                physx::PxPairFlag::eCONTACT_DEFAULT |
                physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
                physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
                physx::PxPairFlag::eNOTIFY_TOUCH_LOST |
                physx::PxPairFlag::eNOTIFY_TOUCH_CCD |
                physx::PxPairFlag::eNOTIFY_CONTACT_POINTS |
                physx::PxPairFlag::eDETECT_CCD_CONTACT |
                physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

            // resolving contacts between two kinematic objects is invalid
            if (physx::PxFilterObjectIsKinematic(attributes0) && physx::PxFilterObjectIsKinematic(attributes1))
            {
                pairFlags.clear(physx::PxPairFlag::eSOLVE_CONTACT);
            }

            // generate callbacks for collisions between kinematic and dynamic objects
            if (physx::PxFilterObjectIsKinematic(attributes0) != physx::PxFilterObjectIsKinematic(attributes1))
            {
                return physx::PxFilterFlag::eCALLBACK;
            }

            return physx::PxFilterFlag::eDEFAULT;
        }

        physx::PxFilterData CreateFilterData(const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group)
        {
            physx::PxFilterData data;
            SetLayer(layer, data);
            SetGroup(group, data);
            return data;
        }

        void SetLayer(const AzPhysics::CollisionLayer& layer, physx::PxFilterData& filterData)
        {
            Utils::Collision::SetLayer(layer, filterData);
        }

        void SetGroup(const AzPhysics::CollisionGroup& group, physx::PxFilterData& filterData)
        {
            Utils::Collision::SetGroup(group, filterData);
        }

        bool ShouldCollide(const physx::PxFilterData& filterData0, const physx::PxFilterData& filterData1)
        {
            return Utils::Collision::ShouldCollide(filterData0, filterData1);
        }
    }
}
