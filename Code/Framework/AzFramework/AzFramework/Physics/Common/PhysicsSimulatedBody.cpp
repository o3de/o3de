/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyAutomation.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedBody, AZ::SystemAllocator);

    namespace Internal
    {
        template<class Event, class Function>
        Event* GetEvent(AZ::EntityId entityid, Function getEventFunc)
        {
            auto* physicsSystem = AZ::Interface<SystemInterface>::Get();
            auto* sceneInterface = AZ::Interface<SceneInterface>::Get();
            if (physicsSystem != nullptr && sceneInterface != nullptr)
            {
                auto [sceneHandle, bodyHandle] = physicsSystem->FindAttachedBodyHandleFromEntityId(entityid);
                if (SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, bodyHandle))
                {
                    auto func = AZStd::bind(getEventFunc, body);
                    return func();
                }
            }
            return nullptr;
        }
    }

    /*static*/ void SimulatedBody::Reflect(AZ::ReflectContext* context)
    {
        Automation::SimulatedBodyCollisionAutomationHandler::Reflect(context);
        Automation::SimulatedBodyTriggerAutomationHandler::Reflect(context);

        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AzPhysics::SimulatedBody>()
                ->Version(1)
                ->Field("SceneOwner", &SimulatedBody::m_sceneOwner)
                ->Field("BodyHandle", &SimulatedBody::m_bodyHandle)
                ;
        }

        // reflect the collision and trigger AZ::Events
        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            const AZStd::vector<AZStd::string> collisionEventParams = {
                "Simulated Body Handle",
                "Collision Event"
            };

            const AZ::BehaviorAzEventDescription onCollisionBeginEventDescription =
            {
                "On Collision Begin event",
                 collisionEventParams // Parameters
            };
            const AZ::BehaviorAzEventDescription onCollisionPersistDescription =
            {
                "On Collision Persist event",
                collisionEventParams // Parameters
            };
            const AZ::BehaviorAzEventDescription onCollisionEndEventDescription =
            {
                "On Collision End event",
                collisionEventParams // Parameters
            };

            const AZStd::vector<AZStd::string> triggerEventParams = {
                "Simulated Body Handle",
                "Trigger Event"
            };
            const AZ::BehaviorAzEventDescription onTriggerEnterDescription =
            {
                "On Trigger Enter event",
                triggerEventParams // Parameters
            };
            const AZ::BehaviorAzEventDescription onTriggerExitDescription =
            {
                "On Trigger Exit event",
                triggerEventParams // Parameters
            };

            const auto getOnCollisionBegin = [](AZ::EntityId id) -> SimulatedBodyEvents::OnCollisionBegin*
            {
                return Internal::GetEvent<SimulatedBodyEvents::OnCollisionBegin>(id, &SimulatedBody::GetOnCollisionBeginEvent);
            };
            const auto getOnCollisionPersist = [](AZ::EntityId id) -> SimulatedBodyEvents::OnCollisionPersist*
            {
                return Internal::GetEvent<SimulatedBodyEvents::OnCollisionPersist>(id, &SimulatedBody::GetOnCollisionPersistEvent);
            };
            const auto getOnCollisionEnd = [](AZ::EntityId id) -> SimulatedBodyEvents::OnCollisionEnd*
            {
                return Internal::GetEvent<SimulatedBodyEvents::OnCollisionEnd>(id, &SimulatedBody::GetOnCollisionEndEvent);
            };
            const auto getOnTriggerEnter = [](AZ::EntityId id) -> SimulatedBodyEvents::OnTriggerEnter*
            {
                return Internal::GetEvent<SimulatedBodyEvents::OnTriggerEnter>(id, &SimulatedBody::GetOnTriggerEnterEvent);
            };
            const auto getOnTriggerExit = [](AZ::EntityId id) -> SimulatedBodyEvents::OnTriggerExit*
            {
                return Internal::GetEvent<SimulatedBodyEvents::OnTriggerExit>(id, &SimulatedBody::GetOnTriggerExitEvent);
            };

            behaviorContext->Class<SimulatedBody>("SimulatedBody")
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Method("GetOnCollisionBeginEvent", getOnCollisionBegin)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, AZStd::move(onCollisionBeginEventDescription))
                ->Method("GetOnCollisionPersistEvent", getOnCollisionPersist)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, AZStd::move(onCollisionPersistDescription))
                ->Method("GetOnCollisionEndEvent", getOnCollisionEnd)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, AZStd::move(onCollisionEndEventDescription))
                ->Method("GetOnTriggerEnterEvent", getOnTriggerEnter)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, AZStd::move(onTriggerEnterDescription))
                ->Method("GetOnTriggerExitEvent", getOnTriggerExit)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, AZStd::move(onTriggerExitDescription))
                ;
        }
    }

    void SimulatedBody::ProcessCollisionEvent(const CollisionEvent& collision) const
    {
        switch (collision.m_type)
        {
        case CollisionEvent::Type::Begin:
            m_collisionBeginEvent.Signal(m_bodyHandle, collision);
            break;
        case CollisionEvent::Type::Persist:
            m_collisionPersistEvent.Signal(m_bodyHandle, collision);
            break;
        case CollisionEvent::Type::End:
            m_collisionEndEvent.Signal(m_bodyHandle, collision);
            break;
        default:
            AZ_Warning("Physics", false, "[SimulatedBody::ProcessCollisionEvent] Unexpected collison type.");
            break;
        }
    }

    void SimulatedBody::ProcessTriggerEvent(const TriggerEvent& trigger) const
    {
        switch (trigger.m_type)
        {
        case AzPhysics::TriggerEvent::Type::Enter:
            m_triggerEnterEvent.Signal(m_bodyHandle, trigger);
            break;
        case AzPhysics::TriggerEvent::Type::Exit:
            m_triggerExitEvent.Signal(m_bodyHandle, trigger);
            break;
        default:
            AZ_Warning("Physics", false, "[SimulatedBody::ProcessTriggerEvent] Unexpected trigger type.");
            break;
        }
    }

    void SimulatedBody::SyncTransform(float deltaTime) const
    {
        m_syncTransformEvent.Signal(deltaTime);
    }

    Scene* SimulatedBody::GetScene()
    {
        if (auto* physicsSystem = AZ::Interface<SystemInterface>::Get())
        {
            return physicsSystem->GetScene(m_sceneOwner);
        }
        return nullptr;
    }

    SimulatedBodyEvents::OnCollisionBegin* SimulatedBody::GetOnCollisionBeginEvent()
    {
        return &m_collisionBeginEvent;
    }

    SimulatedBodyEvents::OnCollisionPersist* SimulatedBody::GetOnCollisionPersistEvent()
    {
        return &m_collisionPersistEvent;
    }

    SimulatedBodyEvents::OnCollisionEnd* SimulatedBody::GetOnCollisionEndEvent()
    {
        return &m_collisionEndEvent;
    }

    SimulatedBodyEvents::OnTriggerEnter* SimulatedBody::GetOnTriggerEnterEvent()
    {
        return &m_triggerEnterEvent;
    }

    SimulatedBodyEvents::OnTriggerExit* SimulatedBody::GetOnTriggerExitEvent()
    {
        return &m_triggerExitEvent;
    }

    SimulatedBodyEvents::OnSyncTransform* SimulatedBody::GetOnSyncTransformEvent()
    {
        return &m_syncTransformEvent;
    }
}
