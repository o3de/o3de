/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Source/Components/NetworkPlayerSpawnerComponent.h>
#include <Source/Spawners/IPlayerSpawner.h>

namespace AutomatedTesting
{
    void NetworkPlayerSpawnerComponent::NetworkPlayerSpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkPlayerSpawnerComponent, NetworkPlayerSpawnerComponentBase>()->Version(1);
        }
        NetworkPlayerSpawnerComponentBase::Reflect(context);
    }

    void NetworkPlayerSpawnerComponent::OnInit()
    {
        ;
    }

    void NetworkPlayerSpawnerComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        AZ::Interface<IPlayerSpawner>::Get()->RegisterPlayerSpawner(this);
    }

    void NetworkPlayerSpawnerComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        AZ::Interface<IPlayerSpawner>::Get()->UnregisterPlayerSpawner(this);
    }
} // namespace AutomatedTesting
