/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/AutoGen/NetworkDebugConnectionCounterComponent.AutoComponent.h>
#include <AzCore/Component/TickBus.h>

namespace Multiplayer
{

    class NetworkDebugConnectionCounterComponentController
        : public NetworkDebugConnectionCounterComponentControllerBase,
            AZ::TickBus::Handler
    {
    public:
        explicit NetworkDebugConnectionCounterComponentController(NetworkDebugConnectionCounterComponent& parent);

        void OnActivate(EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(EntityIsMigrating entityIsMigrating) override;

    private:
        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    };
}
