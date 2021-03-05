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

#include <PhysX_precompiled.h>

#include "PhysXTestUtil.h"

namespace PhysX
{
    CollisionCallbacksListener::CollisionCallbacksListener(AZ::EntityId entityId)
    {
        Physics::CollisionNotificationBus::Handler::BusConnect(entityId);
    }

    CollisionCallbacksListener::~CollisionCallbacksListener()
    {
        Physics::CollisionNotificationBus::Handler::BusDisconnect();
    }

    void CollisionCallbacksListener::OnCollisionBegin(const Physics::CollisionEvent& collision)
    {
        if (m_onCollisionBegin)
        {
            m_onCollisionBegin(collision);
        }
        m_beginCollisions.push_back(collision);
    }

    void CollisionCallbacksListener::OnCollisionPersist(const Physics::CollisionEvent& collision)
    {
        if (m_onCollisionPersist)
        {
            m_onCollisionPersist(collision);
        }
        m_persistCollisions.push_back(collision);
    }

    void CollisionCallbacksListener::OnCollisionEnd(const Physics::CollisionEvent& collision)
    {
        if (m_onCollisionEnd)
        {
            m_onCollisionEnd(collision);
        }
        m_endCollisions.push_back(collision);
    }

    TestTriggerAreaNotificationListener::TestTriggerAreaNotificationListener(AZ::EntityId triggerAreaEntityId)
    {
        Physics::TriggerNotificationBus::Handler::BusConnect(triggerAreaEntityId);
    }

    TestTriggerAreaNotificationListener::~TestTriggerAreaNotificationListener()
    {
        Physics::TriggerNotificationBus::Handler::BusDisconnect();
    }

    void TestTriggerAreaNotificationListener::OnTriggerEnter(const Physics::TriggerEvent& event)
    {
        if (m_onTriggerEnter)
        {
            m_onTriggerEnter(event);
        }
        m_enteredEvents.push_back(event);
    }

    void TestTriggerAreaNotificationListener::OnTriggerExit(const Physics::TriggerEvent& event)
    {
        if (m_onTriggerExit)
        {
            m_onTriggerExit(event);
        }
        m_exitedEvents.push_back(event);
    }
}
