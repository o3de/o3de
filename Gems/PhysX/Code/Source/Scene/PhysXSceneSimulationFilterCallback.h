/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>

#include <PxFiltering.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace PhysX
{
    //! Helper class to handle the filtering of collision pairs reported from PhysX.
    class SceneSimulationFilterCallback
        : public physx::PxSimulationFilterCallback
    {
    public:
        SceneSimulationFilterCallback() = default;
        ~SceneSimulationFilterCallback() = default;

        //! Registers a pair of simulated bodies for which collisions should be suppressed.
        void RegisterSuppressedCollision(
            const AzPhysics::SimulatedBody* body0,
            const AzPhysics::SimulatedBody* body1);

        //! Unregisters a pair of simulated bodies for which collisions should be suppressed.
        void UnregisterSuppressedCollision(
            const AzPhysics::SimulatedBody* body0,
            const AzPhysics::SimulatedBody* body1);

        // physx::PxSimulationFilterCallback Interface
        physx::PxFilterFlags pairFound(physx::PxU32 pairId, physx::PxFilterObjectAttributes attributes0,
            physx::PxFilterData filterData0, const physx::PxActor* actor0, const physx::PxShape* shape0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, const physx::PxActor* actor1,
            const physx::PxShape* shape1, physx::PxPairFlags& pairFlags) override;
        void pairLost(physx::PxU32 pairId, physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, bool objectRemoved) override;
        bool statusChange(physx::PxU32& pairId, physx::PxPairFlags& pairFlags, physx::PxFilterFlags& filterFlags) override;

    private:
        struct CollisionActorPair
        {
            CollisionActorPair() = default;
            CollisionActorPair(const physx::PxActor* actorA, const physx::PxActor* actorB);
            ~CollisionActorPair() = default;

            const bool operator==(const CollisionActorPair& other) const;

            const physx::PxActor* m_actorA = nullptr;
            const physx::PxActor* m_actorB = nullptr;
        };

        struct CollisionPairHasher
        {
            size_t operator()(const CollisionActorPair& collisionPair) const;
        };
        using CollisionPairSet = AZStd::unordered_set<CollisionActorPair, CollisionPairHasher>;

        CollisionPairSet::const_iterator FindSuppressedPair(const physx::PxActor* actor0, const physx::PxActor* actor1) const;

        CollisionPairSet m_suppressedCollisionPairs; //!< Actor pairs with collision suppressed.
    };
}
