/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

namespace Multiplayer
{
    //! @class ISimplePlayerSpawner
    //! @brief The SimplePlayerSpawnerRequest event-bus exposes helper methods regarding network player spawners.
    //! Although the Multiplayer System automatically spawns in players, it's common for game specific server logic to retrieve valid spawn locations when respawning a player.
    class ISimplePlayerSpawner
    {
    public:
        AZ_RTTI(ISimplePlayerSpawner, "{6DAC5CDE-5D63-4C0B-9F7B-9F5B09079DBB}");

        virtual ~ISimplePlayerSpawner() = default;

        //! Returns the location where the next joining player will be spawned.
        //! Method is only valid if called from the multiplayer host/authority; clients are not given information regarding the spawn point index.
        //! @return AZ::Transform The location of the next spawn point
        virtual AZ::Transform GetNextSpawnPoint() const = 0;

        //! Returns an immutable list of all the spawn points.
        //! Only access this list on the multiplayer host; spawn points are not synced across the network and only the host is responsible for spawning players.
        //! @return const AZStd::vector<AZ::EntityId> List of spawn points.
        virtual const AZStd::vector<AZ::EntityId>& GetSpawnPoints() const = 0;

        //! Returns the number of spawn points.
        //! @return Number of spawn points.
        virtual uint32_t GetSpawnPointCount() const = 0;

        //! Returns the spawn point index where the next joining player will be spawned.
        //! @return The index of the next spawn point. Index will be valid except when the spawn point count is zero.
        virtual uint32_t GetNextSpawnPointIndex() const = 0;

        //! Overwrites the next joining player's spawn index. The spawn index provided must be a valid (in-bounds) index into the array of available spawn points.
        virtual void SetNextSpawnPointIndex(uint32_t index) = 0;
    };
} // namespace Multiplayer
