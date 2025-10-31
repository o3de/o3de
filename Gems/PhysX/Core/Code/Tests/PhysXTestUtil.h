/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AzCore/Component/Component.h"
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>


namespace PhysX
{
    // transform for a floor centred at x = 0, y = 0, with top at level z = 0
    inline const AZ::Transform DefaultFloorTransform = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisZ(-0.5f));

    //! CollisionCallbacksListener listens to collision events for a particular sceneHandle and simulatedBodyHandle
    class CollisionCallbacksListener
    {
    public:
        explicit CollisionCallbacksListener(AZ::EntityId entityId);
        CollisionCallbacksListener(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle);
        ~CollisionCallbacksListener();

        AZStd::vector<AzPhysics::CollisionEvent> m_beginCollisions;
        AZStd::vector<AzPhysics::CollisionEvent> m_persistCollisions;
        AZStd::vector<AzPhysics::CollisionEvent> m_endCollisions;

        AZStd::function<void(const AzPhysics::CollisionEvent& collisionEvent)> m_onCollisionBegin;
        AZStd::function<void(const AzPhysics::CollisionEvent& collisionEvent)> m_onCollisionPersist;
        AZStd::function<void(const AzPhysics::CollisionEvent& collisionEvent)> m_onCollisionEnd;

    private:
        void InitCollisionHandlers();

        AzPhysics::SimulatedBodyEvents::OnCollisionBegin::Handler m_onCollisionBeginHandler;
        AzPhysics::SimulatedBodyEvents::OnCollisionPersist::Handler m_onCollisionPersistHandler;
        AzPhysics::SimulatedBodyEvents::OnCollisionEnd::Handler m_onCollisionEndHandler;
    };

    //! TestTriggerAreaNotificationListener listens to trigger events for a particular sceneHandle and simulatedBodyHandle
    class TestTriggerAreaNotificationListener
    {
    public:
        explicit TestTriggerAreaNotificationListener(AZ::EntityId entityId);
        TestTriggerAreaNotificationListener(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle);
        ~TestTriggerAreaNotificationListener();

        const AZStd::vector<AzPhysics::TriggerEvent>& GetEnteredEvents() const { return m_enteredEvents; }
        const AZStd::vector<AzPhysics::TriggerEvent>& GetExitedEvents() const { return m_exitedEvents; }

        AZStd::function<void(const AzPhysics::TriggerEvent& event)> m_onTriggerEnter;
        AZStd::function<void(const AzPhysics::TriggerEvent& event)> m_onTriggerExit;

    private:
        void InitTriggerHandlers();

        AZStd::vector<AzPhysics::TriggerEvent> m_enteredEvents;
        AZStd::vector<AzPhysics::TriggerEvent> m_exitedEvents;

        AzPhysics::SimulatedBodyEvents::OnTriggerEnter::Handler m_onTriggerEnterHandler;
        AzPhysics::SimulatedBodyEvents::OnTriggerExit::Handler m_onTriggerExitHandler;
    };

} // namespace PhysX
