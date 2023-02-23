/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

namespace Multiplayer
{
    //! @class SimplePlayerSpawnerRequests
    //! @brief The SimplePlayerSpawnerRequest event-bus exposes helper methods regarding network player spawners.
    //! Although the Multiplayer System automatically spawns in players, it's common for game specific server logic to retrieve valid spawn locations when respawning a player.
    class SimplePlayerSpawnerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        //! Returns and increments the next player spawn point.
        //! Method is only valid if called from the multiplayer host/authority; clients are not given information regarding the spawn point index.
        //! @return AZ::Transform The location of the next spawn point
        virtual AZ::Transform RoundRobinNextSpawnPoint() = 0;

        //! Returns the next player spawn point. Unlike RoundRobinNextSpawnPoint(), this will not cause the current spawn point index to increment.
        //! Method is only valid if called from the multiplayer host/authority; clients are not given information regarding the spawn point index.
        //! @return AZ::Transform The location of the next spawn point
        virtual AZ::Transform GetNextSpawnPoint() const = 0;


        //! Returns a mutable list of all the spawn points.
        //! Only use this list on multiplayer hosts; clients aren't in charge of spawn players so this list might not actually represent where the players spawn.
        //! @return AZStd::vector<AZ::EntityId> List of spawn points.
        virtual AZStd::vector<AZ::EntityId>& GetSpawnPoints() = 0;

        //! Returns the number of spawn points.
        //! @return Number of spawn points.
        virtual uint32_t GetSpawnPointCount() const = 0;

        //! Returns the index of the player spawn point.
        //! @return AZ::Outcome<uint32_t, AZStd::string> If success, the value present a valid index that can be used to lookup into the spawn points array.
        virtual AZ::Outcome<uint32_t, AZStd::string> GetNextSpawnPointIndex() const = 0;

        //! Overwrites the next player's spawn index. The spawn index provided must be a valid in-bounds index into the array of available spawn points.
        //! @return AZ::Outcome<void, AZStd::string> Success means a valid index was provided; otherwise the index is out-of-bounds.
        virtual AZ::Outcome<void, AZStd::string> SetNextSpawnPointIndex(uint32_t index) = 0;
    };
    using SimplePlayerSpawnerRequestBus = AZ::EBus<SimplePlayerSpawnerRequests>;
} // namespace Multiplayer
