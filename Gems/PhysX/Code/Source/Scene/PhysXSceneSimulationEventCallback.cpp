/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Scene/PhysXSceneSimulationEventCallback.h>

#include <PhysX/MathConversion.h>
#include <PhysX/Utils.h>

namespace PhysX
{
    AzPhysics::CollisionEventList& SceneSimulationEventCallback::GetQueuedCollisionEvents()
    {
        return m_queuedCollisionEvents;
    }

    AzPhysics::TriggerEventList& SceneSimulationEventCallback::GetQueuedTriggerEvents()
    {
        return m_queuedTriggerEvents;
    }

    void SceneSimulationEventCallback::FlushQueuedCollisionEvents()
    {
        m_queuedCollisionEvents.clear();
    }

    void SceneSimulationEventCallback::FlushQueuedTriggerEvents()
    {
        m_queuedTriggerEvents.clear();
    }

    void SceneSimulationEventCallback::onConstraintBreak(
        [[maybe_unused]] physx::PxConstraintInfo* constraints, [[maybe_unused]] physx::PxU32 count)
    {
    }

    void SceneSimulationEventCallback::onWake([[maybe_unused]] physx::PxActor** actors, [[maybe_unused]] physx::PxU32 count)
    {
    }

    void SceneSimulationEventCallback::onSleep([[maybe_unused]] physx::PxActor** actors, [[maybe_unused]] physx::PxU32 count)
    {
    }

    void SceneSimulationEventCallback::onContact(
        const physx::PxContactPairHeader& pairHeader,
        const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
    {
        const bool body1Destroyed = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0;
        const bool body2Destroyed = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1;
        if (body1Destroyed || body2Destroyed)
        {
            // We can't report destroyed bodies at the moment.
            return;
        }

        const auto flagsToNotify =
            physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
            physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
            physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
        static constexpr const physx::PxU32 MaxPointsToReport = 10;
        for (physx::PxU32 i = 0; i < nbPairs; i++)
        {
            const physx::PxContactPair& contactPair = pairs[i];

            if (contactPair.events & flagsToNotify)
            {
                ActorData* actorData1 = Utils::GetUserData(pairHeader.actors[0]);
                ActorData* actorData2 = Utils::GetUserData(pairHeader.actors[1]);

                // Missing user data, or user data was invalid
                if (!actorData1 || !actorData2)
                {
                    AZ_Warning("PhysX", false, "Invalid user data set for objects Obj0:%p Obj1:%p", actorData1, actorData2);
                    continue;
                }

                AzPhysics::SimulatedBody* body1 = actorData1->GetSimulatedBody();
                AzPhysics::SimulatedBody* body2 = actorData2->GetSimulatedBody();

                if (!body1 || !body2)
                {
                    AZ_Warning("PhysX", false, "Invalid body data set for objects Obj0:%p Obj1:%p", body1, body2);
                    continue;
                }

                Physics::Shape* shape1 = Utils::GetUserData(contactPair.shapes[0]);
                Physics::Shape* shape2 = Utils::GetUserData(contactPair.shapes[1]);

                if (!shape1 || !shape2)
                {
                    AZ_Warning("PhysX", false, "Invalid shape userdata set for objects Obj0:%p Obj1:%p", shape1, shape2);
                    continue;
                }

                // Collision Event
                AzPhysics::CollisionEvent collision;
                collision.m_bodyHandle1 = actorData1->GetBodyHandle();
                collision.m_body1 = body1;
                collision.m_bodyHandle2 = actorData2->GetBodyHandle();
                collision.m_body2 = body2;
                collision.m_shape1 = shape1;
                collision.m_shape2 = shape2;

                // Extract contacts for collision event
                physx::PxContactPairPoint extractedPoints[MaxPointsToReport];
                physx::PxU32 contactPointCount = contactPair.extractContacts(extractedPoints, MaxPointsToReport);
                collision.m_contacts.resize(contactPointCount);
                for (physx::PxU8 j = 0; j < contactPointCount; ++j)
                {
                    const physx::PxContactPairPoint& point = extractedPoints[j];

                    AzPhysics::Contact& contact = collision.m_contacts[j];
                    contact.m_position = PxMathConvert(point.position);
                    contact.m_normal = PxMathConvert(point.normal);
                    contact.m_impulse = PxMathConvert(point.impulse);
                    contact.m_separation = point.separation;
                    contact.m_internalFaceIndex01 = point.internalFaceIndex0;
                    contact.m_internalFaceIndex02 = point.internalFaceIndex1;
                }

                if (contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
                {
                    collision.m_type = AzPhysics::CollisionEvent::Type::Begin;
                    m_queuedCollisionEvents.emplace_back(AZStd::move(collision));
                }
                else if (contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
                {
                    collision.m_type = AzPhysics::CollisionEvent::Type::Persist;
                    m_queuedCollisionEvents.emplace_back(AZStd::move(collision));
                }
                else if (contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
                {
                    collision.m_type = AzPhysics::CollisionEvent::Type::End;
                    m_queuedCollisionEvents.emplace_back(AZStd::move(collision));
                }
            }
        }
    }

    void SceneSimulationEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
    {
        for (physx::PxU32 i = 0; i < count; ++i)
        {
            const physx::PxTriggerPair& triggerPair = pairs[i];

            if (triggerPair.triggerActor == nullptr ||
                triggerPair.triggerActor->userData == nullptr ||
                triggerPair.otherActor == nullptr ||
                triggerPair.otherActor->userData == nullptr)
            {
                continue;
            }

            ActorData* triggerBodyActorData = Utils::GetUserData(triggerPair.triggerActor);
            AzPhysics::SimulatedBody* triggerBody = triggerBodyActorData->GetSimulatedBody();
            if (!triggerBody)
            {
                AZ_Error("PhysX", false, "onTrigger:: trigger body was invalid");
                continue;
            }
            if (!triggerBody->GetEntityId().IsValid())
            {
                AZ_Warning("PhysX", false, "onTrigger received invalid actors.");
                continue;
            }

            ActorData* otherActorData = Utils::GetUserData(triggerPair.otherActor);
            AzPhysics::SimulatedBody* otherBody = otherActorData->GetSimulatedBody();
            if (!otherBody)
            {
                AZ_Error("PhysX", false, "onTrigger:: otherBody was invalid");
                continue;
            }
            if (!otherBody->GetEntityId().IsValid())
            {
                AZ_Warning("PhysX", false, "onTrigger received invalid actors.");
                continue;
            }

            AzPhysics::TriggerEvent trigger;
            trigger.m_triggerBodyHandle = triggerBodyActorData->GetBodyHandle();
            trigger.m_triggerBody = triggerBody;
            trigger.m_triggerShape = static_cast<Physics::Shape*>(triggerPair.triggerShape->userData);
            trigger.m_otherBodyHandle = otherActorData->GetBodyHandle();
            trigger.m_otherBody = otherBody;
            trigger.m_otherShape = static_cast<Physics::Shape*>(triggerPair.otherShape->userData);

            if (triggerPair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
            {
                trigger.m_type = AzPhysics::TriggerEvent::Type::Enter;
                m_queuedTriggerEvents.emplace_back(AZStd::move(trigger));
            }
            else if (triggerPair.status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
            {
                trigger.m_type = AzPhysics::TriggerEvent::Type::Exit;
                m_queuedTriggerEvents.emplace_back(AZStd::move(trigger));
            }
            else
            {
                AZ_Warning("PhysX", false, "onTrigger with status different from TOUCH_FOUND and TOUCH_LOST.");
            }
        }
    }

    void SceneSimulationEventCallback::onAdvance(
        [[maybe_unused]] const physx::PxRigidBody* const* bodyBuffer,
        [[maybe_unused]] const physx::PxTransform* poseBuffer, [[maybe_unused]] const physx::PxU32 count)
    {
    }
}
