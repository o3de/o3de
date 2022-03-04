/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/Spawners/IPlayerSpawner.h>

namespace AzFramework
{
    struct PlayerConnectionConfig;
}

namespace Multiplayer
{
    struct EntityReplicationData;
    using ReplicationSet = AZStd::map<ConstNetworkEntityHandle, EntityReplicationData>;
} // namespace Multiplayer

namespace AutomatedTesting
{
    class RoundRobinSpawner
        : public IPlayerSpawner
    {
    public:
        AZ_RTTI(RoundRobinSpawner, "{C934A204-D6F8-4A44-870B-DFE5B8C7BA6B}");

        ////////////////////////////////////////////////////////////////////////
        // IPlayerSpawner overrides
        bool RegisterPlayerSpawner(NetworkPlayerSpawnerComponent* spawner) override;
        AZStd::pair<Multiplayer::PrefabEntityId, AZ::Transform> GetNextPlayerSpawn() override;
        bool UnregisterPlayerSpawner(NetworkPlayerSpawnerComponent* spawner) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZStd::vector<NetworkPlayerSpawnerComponent*> m_spawners;
        uint8_t m_spawnIndex = 0;
    };
} // namespace AutomatedTesting
