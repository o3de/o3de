/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>

namespace Multiplayer
{
    struct EntityReplicationData;
    struct MultiplayerAgentDatum;

    // It's important that this be an ordered associative container
    // as we'll walk corresponding replication sets to compute differences
    using ReplicationSet = AZStd::map<ConstNetworkEntityHandle, EntityReplicationData>;

    //! @class IMultiplayerSpawner
    //! @brief IMultiplayerSpawner routes spawning requests for connecting players from
    //! the Muliplayer Gem to game logic utilizing it
    //!
    //! IMultiplayerSpawner is an AZ::Interface<T> that provides a mechanism to
    //! tell the Multiplayer Gem what to spawn on player connection.
    //! IMultiplayerSpawner is intended to be implemented on games utilizing the
    //! Multiplayer Gem. The Multiplayer Gem then calls the implementation via
    //! AZ::Interface.
    class IMultiplayerSpawner
    {
    public:
        AZ_RTTI(IMultiplayerSpawner, "{E5525317-A476-4209-BE45-477FB9D96083}");

        virtual ~IMultiplayerSpawner() = default;

        //! Invoked when a Client connects/ClientHost starts a session to determine what autonomous Prefab should be spawned where
        //! @param userId User ID of joining player
        //! @param agentDatum Datum containing connection data that can be used to inform join logic
        //! @return A NetworkEntityHandle of the entity the player will have autonomy over
        virtual Multiplayer::NetworkEntityHandle OnPlayerJoin(uint64_t userId, const Multiplayer::MultiplayerAgentDatum& agentDatum) = 0;

        //! Invoked when a Client disconnects from the session to determine how the autonomous prefab should be cleaned up
        //! @param entityHandle The entity handle to consider on leaving, generally the connection's primary player entity
        //! @param replicationSet The replication set of the related connection
        //! @param reason The cause of disconnection
        virtual void OnPlayerLeave(
            Multiplayer::ConstNetworkEntityHandle entityHandle, const Multiplayer::ReplicationSet& replicationSet, AzNetworking::DisconnectReason reason) = 0;
    };
}
