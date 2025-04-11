/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Common/PhysicsSimulatedBodyAutomation.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

namespace AzPhysics::Automation
{
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedBodyCollisionAutomationHandler, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedBodyTriggerAutomationHandler, AZ::SystemAllocator);

    /*static*/ void SimulatedBodyCollisionAutomationHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AzPhysics::Automation::AutomationCollisionNotificationsBus>("CollisionNotificationBus")
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Handler<SimulatedBodyCollisionAutomationHandler>()
                ;
        }
    }

    SimulatedBodyCollisionAutomationHandler::SimulatedBodyCollisionAutomationHandler()
    {
        m_collisionBeginHandler = SimulatedBodyEvents::OnCollisionBegin::Handler(
            [this]([[maybe_unused]] SimulatedBodyHandle bodyHandle,
                const CollisionEvent& event)
            {
                OnCollisionBeginEvent(event);
            });
        m_collisionPersistHandler = SimulatedBodyEvents::OnCollisionPersist::Handler(
            [this]([[maybe_unused]] SimulatedBodyHandle bodyHandle,
                const CollisionEvent& event)
            {
                OnCollisionPersistEvent(event);
            });
        m_collisionEndHandler = SimulatedBodyEvents::OnCollisionEnd::Handler(
            [this]([[maybe_unused]] SimulatedBodyHandle bodyHandle,
                const CollisionEvent& event)
            {
                OnCollisionEndEvent(event);
            });

        SetEvent(&SimulatedBodyCollisionAutomationHandler::OnCollisionBegin, "OnCollisionBegin");
        SetEvent(&SimulatedBodyCollisionAutomationHandler::OnCollisionPersist, "OnCollisionPersist");
        SetEvent(&SimulatedBodyCollisionAutomationHandler::OnCollisionEnd, "OnCollisionEnd");
    }

    void SimulatedBodyCollisionAutomationHandler::Disconnect(AZ::BehaviorArgument* id [[maybe_unused]])
    {
        m_collisionBeginHandler.Disconnect();
        m_collisionPersistHandler.Disconnect();
        m_collisionEndHandler.Disconnect();
    }

    bool SimulatedBodyCollisionAutomationHandler::Connect(AZ::BehaviorArgument* id /*= nullptr*/)
    {
        if (id && id->ConvertTo<typename AZ::EntityId>())
        {
            m_connectedEntityId = *id->GetAsUnsafe<typename AZ::EntityId>();
            auto* physicsSystem = AZ::Interface<SystemInterface>::Get();
            auto* sceneInterface = AZ::Interface<SceneInterface>::Get();
            if (physicsSystem != nullptr && sceneInterface != nullptr)
            {
                auto [sceneHandle, bodyHandle] = physicsSystem->FindAttachedBodyHandleFromEntityId(m_connectedEntityId);
                if (SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, bodyHandle))
                {
                    bool connected = false;
                    if (auto* collisionBeginEvent = body->GetOnCollisionBeginEvent())
                    {
                        m_collisionBeginHandler.Connect(*collisionBeginEvent);
                        connected = true;
                    }
                    if (auto* collisionPersistEvent = body->GetOnCollisionPersistEvent())
                    {
                        m_collisionPersistHandler.Connect(*collisionPersistEvent);
                        connected = true;
                    }
                    if (auto* collisionEndEvent = body->GetOnCollisionEndEvent())
                    {
                        m_collisionEndHandler.Connect(*collisionEndEvent);
                        connected = true;
                    }
                    return connected;
                }
            }
        }

        return false;
    }

    bool SimulatedBodyCollisionAutomationHandler::IsConnected()
    {
        return m_collisionBeginHandler.IsConnected() || m_collisionPersistHandler.IsConnected() || m_collisionEndHandler.IsConnected();
    }

    bool SimulatedBodyCollisionAutomationHandler::IsConnectedId(AZ::BehaviorArgument* id)
    {
        if (id && id->ConvertTo<typename AZ::EntityId>())
        {
            return m_connectedEntityId == *id->GetAsUnsafe<typename AZ::EntityId>() && IsConnected();
        }
        return false;
    }

    int SimulatedBodyCollisionAutomationHandler::GetFunctionIndex(const char* functionName) const
    {
        if (azstricmp(functionName, "OnCollisionBegin") == 0)
        {
            return FN_OnCollisionBegin;
        }
        if (azstricmp(functionName, "OnCollisionPersist") == 0)
        {
            return FN_OnCollisionPersist;
        }
        if (azstricmp(functionName, "OnCollisionEnd") == 0)
        {
            return FN_OnCollisionEnd;
        }
        return -1;
    }

    void SimulatedBodyCollisionAutomationHandler::OnCollisionBeginEvent(const CollisionEvent& event)
    {
        Call(FN_OnCollisionBegin, event.m_body2->GetEntityId(), event.m_contacts); //send m_body2 entity id as that is the other body involved in the collision.
    }

    void SimulatedBodyCollisionAutomationHandler::OnCollisionPersistEvent(const CollisionEvent& event)
    {
        Call(FN_OnCollisionPersist, event.m_body2->GetEntityId(), event.m_contacts); //send m_body2 entity id as that is the other body involved in the collision.
    }

    void SimulatedBodyCollisionAutomationHandler::OnCollisionEndEvent(const CollisionEvent& event)
    {
        Call(FN_OnCollisionEnd, event.m_body2->GetEntityId()); //send m_body2 entity id as that is the other body involved in the collision.
    }

    /*static*/ void SimulatedBodyTriggerAutomationHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AzPhysics::Automation::AutomationTriggerNotificationsBus>("TriggerNotificationBus")
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Handler<SimulatedBodyTriggerAutomationHandler>()
                ;
        }
    }

    SimulatedBodyTriggerAutomationHandler::SimulatedBodyTriggerAutomationHandler()
    {
        m_triggerEnterHandler = SimulatedBodyEvents::OnTriggerEnter::Handler(
            [this]([[maybe_unused]] SimulatedBodyHandle bodyHandle,
                const TriggerEvent& event)
            {
                OnTriggerEnterEvent(event);
            });
        m_triggerExitHandler = SimulatedBodyEvents::OnTriggerExit::Handler(
            [this]([[maybe_unused]] SimulatedBodyHandle bodyHandle,
                const TriggerEvent& event)
            {
                OnTriggerExitEvent(event);
            });

        SetEvent(&SimulatedBodyTriggerAutomationHandler::OnTriggerEnter, "OnTriggerEnter");
        SetEvent(&SimulatedBodyTriggerAutomationHandler::OnTriggerExit, "OnTriggerExit");
    }

    void SimulatedBodyTriggerAutomationHandler::Disconnect(AZ::BehaviorArgument* id [[maybe_unused]])
    {
        m_triggerEnterHandler.Disconnect();
        m_triggerExitHandler.Disconnect();
    }

    bool SimulatedBodyTriggerAutomationHandler::Connect(AZ::BehaviorArgument* id /*= nullptr*/)
    {
        if (id && id->ConvertTo<typename AZ::EntityId>())
        {
            m_connectedEntityId = *id->GetAsUnsafe<typename AZ::EntityId>();
            auto* physicsSystem = AZ::Interface<SystemInterface>::Get();
            auto* sceneInterface = AZ::Interface<SceneInterface>::Get();
            if (physicsSystem != nullptr && sceneInterface != nullptr)
            {
                auto [sceneHandle, bodyHandle] = physicsSystem->FindAttachedBodyHandleFromEntityId(m_connectedEntityId);
                if (SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, bodyHandle))
                {
                    bool connected = false;
                    if (auto* triggerEnterEvent = body->GetOnTriggerEnterEvent())
                    {
                        m_triggerEnterHandler.Connect(*triggerEnterEvent);
                        connected = true;
                    }
                    if (auto* triggerExitEvent = body->GetOnTriggerExitEvent())
                    {
                        m_triggerExitHandler.Connect(*triggerExitEvent);
                        connected = true;
                    }
                    return connected;
                }
            }
        }

        return false;
    }

    bool SimulatedBodyTriggerAutomationHandler::IsConnected()
    {
        return m_triggerEnterHandler.IsConnected() || m_triggerExitHandler.IsConnected();
    }

    bool SimulatedBodyTriggerAutomationHandler::IsConnectedId(AZ::BehaviorArgument* id)
    {
        if (id && id->ConvertTo<typename AZ::EntityId>())
        {
            return m_connectedEntityId == *id->GetAsUnsafe<typename AZ::EntityId>() && IsConnected();
        }
        return false;
    }

    int SimulatedBodyTriggerAutomationHandler::GetFunctionIndex(const char* functionName) const
    {
        if (azstricmp(functionName, "OnTriggerEnter") == 0)
        {
            return FN_OnTriggerEnter;
        }
        if (azstricmp(functionName, "OnTriggerExit") == 0)
        {
            return FN_OnTriggerExit;
        }
        return -1;
    }

    void SimulatedBodyTriggerAutomationHandler::OnTriggerEnterEvent(const TriggerEvent& event)
    {
        Call(FN_OnTriggerEnter, event.m_otherBody->GetEntityId());
    }

    void SimulatedBodyTriggerAutomationHandler::OnTriggerExitEvent(const TriggerEvent& event)
    {
        Call(FN_OnTriggerExit, event.m_otherBody->GetEntityId());
    }
}
