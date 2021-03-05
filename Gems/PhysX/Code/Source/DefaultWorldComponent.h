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
#pragma once

#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace Physics
{
    class World;
}

namespace PhysX
{
    class World;

    // Sub Component to be conviniently used for spawning and ticking the default world
    // Creates world and enables ticking when the game context activates (before game entities start)
    class DefaultWorldComponent
        : private Physics::DefaultWorldBus::Handler
        , private Physics::WorldEventHandler
        , private AzFramework::GameEntityContextEventBus::Handler
    {
    public:
        DefaultWorldComponent();
        ~DefaultWorldComponent() override = default;

        void Activate();
        void Deactivate();

    private:
        // WorldEventHandler
        void OnTriggerEnter(const Physics::TriggerEvent& event) override;
        void OnTriggerExit(const Physics::TriggerEvent& event) override;
        void OnCollisionBegin(const Physics::CollisionEvent& event) override;
        void OnCollisionPersist(const Physics::CollisionEvent& event) override;
        void OnCollisionEnd(const Physics::CollisionEvent& event) override;

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override;

        // GameEntityContextEventBus
        void OnPreGameEntitiesStarted() override;
        void OnGameEntitiesReset() override;

        void UpdateDefaultConfiguration(const AzPhysics::SceneConfiguration& config);

        AzPhysics::SceneHandle m_sceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::SystemEvents::OnDefaultSceneConfigurationChangedEvent::Handler m_onDefaultSceneConfigChangedHandler;
    };
}
