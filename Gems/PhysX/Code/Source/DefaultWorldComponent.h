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
