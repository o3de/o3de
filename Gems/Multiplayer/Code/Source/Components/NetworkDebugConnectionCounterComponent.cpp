/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Multiplayer/Components/NetworkDebugConnectionCounterComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzNetworking/Framework/INetworking.h>
#include <Multiplayer/MultiplayerConstants.h>

namespace Multiplayer
{
    NetworkDebugConnectionCounterComponentController::NetworkDebugConnectionCounterComponentController(NetworkDebugConnectionCounterComponent& parent)
        : NetworkDebugConnectionCounterComponentControllerBase(parent)
    {
    }

    void NetworkDebugConnectionCounterComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void NetworkDebugConnectionCounterComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void NetworkDebugConnectionCounterComponentController::OnTick([[maybe_unused]]float deltaTime, [[maybe_unused]]AZ::ScriptTimePoint time)
    {
        if (IsNetEntityRoleAuthority())
        {
            if (const auto networkInterface =
                AZ::Interface<AzNetworking::INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpNetworkInterfaceName)))
            {
                SetConnectionCount(networkInterface->GetConnectionSet().GetConnectionCount());
            }            
        }
    }
}
