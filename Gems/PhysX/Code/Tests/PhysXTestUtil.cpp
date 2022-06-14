/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhysXTestUtil.h"
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzCore/Interface/Interface.h>

namespace PhysX
{
    CollisionCallbacksListener::CollisionCallbacksListener(AZ::EntityId entityId)
    {
        InitCollisionHandlers();

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            auto [sceneHandle, bodyHandle] = physicsSystem->FindAttachedBodyHandleFromEntityId(entityId);
            if (sceneHandle != AzPhysics::InvalidSceneHandle)
            {
                AzPhysics::SimulatedBodyEvents::RegisterOnCollisionBeginHandler(sceneHandle, bodyHandle, m_onCollisionBeginHandler);
                AzPhysics::SimulatedBodyEvents::RegisterOnCollisionBeginHandler(sceneHandle, bodyHandle, m_onCollisionPersistHandler);
                AzPhysics::SimulatedBodyEvents::RegisterOnCollisionBeginHandler(sceneHandle, bodyHandle, m_onCollisionEndHandler);
            }
        }
    }

    CollisionCallbacksListener::CollisionCallbacksListener(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        InitCollisionHandlers();

        AzPhysics::SimulatedBodyEvents::RegisterOnCollisionBeginHandler(sceneHandle, bodyHandle, m_onCollisionBeginHandler);
        AzPhysics::SimulatedBodyEvents::RegisterOnCollisionBeginHandler(sceneHandle, bodyHandle, m_onCollisionPersistHandler);
        AzPhysics::SimulatedBodyEvents::RegisterOnCollisionBeginHandler(sceneHandle, bodyHandle, m_onCollisionEndHandler);
    }

    CollisionCallbacksListener::~CollisionCallbacksListener()
    {
        m_onCollisionBeginHandler.Disconnect();
        m_onCollisionPersistHandler.Disconnect();
        m_onCollisionEndHandler.Disconnect();
    }

    void CollisionCallbacksListener::InitCollisionHandlers()
    {
        m_onCollisionBeginHandler = AzPhysics::SimulatedBodyEvents::OnCollisionBegin::Handler(
            [this]([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
                const AzPhysics::CollisionEvent& event)
                {
                    if (m_onCollisionBegin)
                    {
                        m_onCollisionBegin(event);
                    }
                    m_beginCollisions.push_back(event);
                });
        m_onCollisionPersistHandler = AzPhysics::SimulatedBodyEvents::OnCollisionPersist::Handler(
            [this]([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
                const AzPhysics::CollisionEvent& event)
                {
                    if (m_onCollisionPersist)
                    {
                        m_onCollisionPersist(event);
                    }
                    m_persistCollisions.push_back(event);
                });
        m_onCollisionEndHandler = AzPhysics::SimulatedBodyEvents::OnCollisionEnd::Handler(
            [this]([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
            const AzPhysics::CollisionEvent& event)
            {
                if (m_onCollisionEnd)
                {
                    m_onCollisionEnd(event);
                }
                m_endCollisions.push_back(event);
            });
    }

    TestTriggerAreaNotificationListener::TestTriggerAreaNotificationListener(AZ::EntityId entityId)
    {
        InitTriggerHandlers();
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            auto [sceneHandle, bodyHandle] = physicsSystem->FindAttachedBodyHandleFromEntityId(entityId);
            if (sceneHandle != AzPhysics::InvalidSceneHandle)
            {
                AzPhysics::SimulatedBodyEvents::RegisterOnTriggerEnterHandler(sceneHandle, bodyHandle, m_onTriggerEnterHandler);
                AzPhysics::SimulatedBodyEvents::RegisterOnTriggerExitHandler(sceneHandle, bodyHandle, m_onTriggerExitHandler);
            }
        }
    }

    TestTriggerAreaNotificationListener::TestTriggerAreaNotificationListener(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        InitTriggerHandlers();
        AzPhysics::SimulatedBodyEvents::RegisterOnTriggerEnterHandler(sceneHandle, bodyHandle, m_onTriggerEnterHandler);
        AzPhysics::SimulatedBodyEvents::RegisterOnTriggerEnterHandler(sceneHandle, bodyHandle, m_onTriggerExitHandler);
    }

    TestTriggerAreaNotificationListener::~TestTriggerAreaNotificationListener()
    {
        m_onTriggerEnterHandler.Disconnect();
        m_onTriggerExitHandler.Disconnect();
    }

    void TestTriggerAreaNotificationListener::InitTriggerHandlers()
    {
        m_onTriggerEnterHandler = AzPhysics::SimulatedBodyEvents::OnTriggerEnter::Handler(
            [this]([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
                const  AzPhysics::TriggerEvent& event)
                {
                    if (m_onTriggerEnter)
                    {
                        m_onTriggerEnter(event);
                    }
                    m_enteredEvents.push_back(event);
                });

        m_onTriggerExitHandler = AzPhysics::SimulatedBodyEvents::OnTriggerExit::Handler(
            [this]([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
                const  AzPhysics::TriggerEvent& event)
                {
                    if (m_onTriggerExit)
                    {
                        m_onTriggerExit(event);
                    }
                    m_exitedEvents.push_back(event);
                });
    }
}
