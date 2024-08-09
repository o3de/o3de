/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>

namespace AzPhysics
{
    struct TriggerEvent;
}

namespace PhysX
{
    // Sub Component to be conveniently used for spawning and ticking the default world
    // Creates world and enables ticking when the game context activates (before game entities start)
    class DefaultWorldComponent
        : private Physics::DefaultWorldBus::Handler
        , private AzFramework::GameEntityContextEventBus::Handler
    {
    public:
        DefaultWorldComponent();
        ~DefaultWorldComponent() override = default;

        void Activate();
        void Deactivate();

    private:

        // DefaultWorldBus
        AzPhysics::SceneHandle GetDefaultSceneHandle() const override;

        // GameEntityContextEventBus
        void OnPreGameEntitiesStarted() override;
        void OnGameEntitiesReset() override;

        void UpdateDefaultConfiguration(const AzPhysics::SceneConfiguration& config);

        AzPhysics::SceneHandle m_sceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::SystemEvents::OnDefaultSceneConfigurationChangedEvent::Handler m_onDefaultSceneConfigChangedHandler;
    };
}
