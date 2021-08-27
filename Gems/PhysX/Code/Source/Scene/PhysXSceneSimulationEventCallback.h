/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Collision/CollisionEvents.h>

#include <PxSimulationEventCallback.h>
namespace PhysX
{
    //! Helper class that receives the collision events reported from PhysX.
    //! This will convert physx collision and trigger events in TriggerEvent and CollisionEvent to be forwarded to the Scene.
    class SceneSimulationEventCallback
        : public physx::PxSimulationEventCallback
    {
    public:
        SceneSimulationEventCallback() = default;
        ~SceneSimulationEventCallback() = default;

        //! Accessor to the queued Collision / trigger Events.
        AzPhysics::CollisionEventList& GetQueuedCollisionEvents();
        AzPhysics::TriggerEventList& GetQueuedTriggerEvents();

        //! Clear all queued collision / trigger events.
        void FlushQueuedCollisionEvents();
        void FlushQueuedTriggerEvents();

        // physx::PxSimulationEventCallback Interface
        void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
        void onWake(physx::PxActor** actors, physx::PxU32 count) override;
        void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
        void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
        void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
        void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;

    private:
        AzPhysics::CollisionEventList m_queuedCollisionEvents; //!< Holds all the collision events the happened until the next call to FlushCollisionEvents;
        AzPhysics::TriggerEventList m_queuedTriggerEvents; //!< Holds all the trigger events the happened until the next call to FlushTriggerEvents;
    };
}
