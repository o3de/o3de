/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Scene/PhysXSceneSimulationFilterCallback.h>

#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <PhysX/NativeTypeIdentifiers.h>

namespace PhysX
{
    namespace Internal
    {
        // get the PxActor if the simulated body is one.
        const physx::PxActor* GetPxActor(const AzPhysics::SimulatedBody* simBody)
        {
            const AZ::Crc32& nativeType = simBody->GetNativeType();
            if (nativeType != NativeTypeIdentifiers::RigidBody &&
                nativeType != NativeTypeIdentifiers::RigidBodyStatic)
            {
                return nullptr;
            }

            return static_cast<physx::PxActor*>(simBody->GetNativePointer());
        }
    } // namespace Internal

    // SceneSimulationFilterCallback::CollisionActorPair -------------

    SceneSimulationFilterCallback::CollisionActorPair::CollisionActorPair(const physx::PxActor* actorA, const physx::PxActor* actorB)
        : m_actorA(actorA)
        , m_actorB(actorB)
    {

    }

    const bool SceneSimulationFilterCallback::CollisionActorPair::operator==(const CollisionActorPair& other) const
    {
        //the order of m_actorA and m_actorB do not matter thus {1,2} == {2,1} and {1,2} == {1,2} should both return true.
        if (m_actorA == other.m_actorA)
        {
            return m_actorB == other.m_actorB;
        }
        else if (m_actorA == other.m_actorB)
        {
            return m_actorB == other.m_actorA;
        }
        return false;
    }

    size_t SceneSimulationFilterCallback::CollisionPairHasher::operator()(const CollisionActorPair& collisionPair) const
    {
        size_t hash{ 0 };
        // Order elements so {1,2} and {2,1} would generate the same hash
        auto [smallerVal, biggerVal] = AZStd::minmax(collisionPair.m_actorA, collisionPair.m_actorB);
        AZStd::hash_combine(hash, smallerVal, biggerVal);
        return hash;
    }

    // SceneSimulationFilterCallback -------------

    void SceneSimulationFilterCallback::RegisterSuppressedCollision(const AzPhysics::SimulatedBody* body0,
        const AzPhysics::SimulatedBody* body1)
    {
        const physx::PxActor* actor0 = Internal::GetPxActor(body0);
        const physx::PxActor* actor1 = Internal::GetPxActor(body1);
        if (actor0 && actor1)
        {
            if (FindSuppressedPair(actor0, actor1) == m_suppressedCollisionPairs.end())
            {
                m_suppressedCollisionPairs.insert(CollisionActorPair(actor0, actor1));
            }
        }
    }

    void SceneSimulationFilterCallback::UnregisterSuppressedCollision(const AzPhysics::SimulatedBody* body0,
        const AzPhysics::SimulatedBody* body1)
    {
        const physx::PxActor* actor0 = Internal::GetPxActor(body0);
        const physx::PxActor* actor1 = Internal::GetPxActor(body1);
        if (actor0 && actor1)
        {
            if (auto iterator = FindSuppressedPair(actor0, actor1);
                iterator != m_suppressedCollisionPairs.end())
            {
                m_suppressedCollisionPairs.erase(*iterator);
            }
        }
    }

    physx::PxFilterFlags SceneSimulationFilterCallback::pairFound(
        [[maybe_unused]] physx::PxU32 pairId, [[maybe_unused]] physx::PxFilterObjectAttributes attributes0,
        [[maybe_unused]] physx::PxFilterData filterData0, const physx::PxActor* actor0,
        [[maybe_unused]] const physx::PxShape* shape0, [[maybe_unused]] physx::PxFilterObjectAttributes attributes1,
        [[maybe_unused]] physx::PxFilterData filterData1, const physx::PxActor* actor1,
        [[maybe_unused]] const physx::PxShape* shape1, [[maybe_unused]] physx::PxPairFlags& pairFlags)
    {
        if (FindSuppressedPair(actor0, actor1) != m_suppressedCollisionPairs.end())
        {
            return physx::PxFilterFlag::eSUPPRESS;
        }

        return physx::PxFilterFlag::eDEFAULT;
    }

    void SceneSimulationFilterCallback::pairLost(
        [[maybe_unused]] physx::PxU32 pairId, [[maybe_unused]] physx::PxFilterObjectAttributes attributes0,
        [[maybe_unused]] physx::PxFilterData filterData0, [[maybe_unused]] physx::PxFilterObjectAttributes attributes1,
        [[maybe_unused]] physx::PxFilterData filterData1, [[maybe_unused]] bool objectRemoved)
    {

    }

    bool SceneSimulationFilterCallback::statusChange([[maybe_unused]] physx::PxU32& pairId, [[maybe_unused]] physx::PxPairFlags& pairFlags,
        [[maybe_unused]] physx::PxFilterFlags& filterFlags)
    {
        return false;
    }

    SceneSimulationFilterCallback::CollisionPairSet::const_iterator SceneSimulationFilterCallback::FindSuppressedPair(
        const physx::PxActor* actor0, const physx::PxActor* actor1) const
    {
        return m_suppressedCollisionPairs.find(CollisionActorPair(actor0, actor1));
    }
}
