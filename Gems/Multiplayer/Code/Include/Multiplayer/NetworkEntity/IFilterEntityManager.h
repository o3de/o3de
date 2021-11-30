/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>

namespace Multiplayer
{
    //! @class IFilterEntityManager
    //! @brief IFilterEntityManager provides an interface for filtering entities out from replication down to clients.
    //!
    //! By default, all entities with NetBindComponent on them are replicated to all clients.
    //! (There is a built-in distance filtering, where only entities within vicinity of a player are sent to that player.
    //! This is controlled by sv_ClientAwarenessRadius AZ_CVAR variable.)
    //!
    //! There are use cases where you want to limit the entities sent to a client, for example "fog of war" or
    //! "out of line of sight" anti-cheating mechanic by omitting information clients should not have access to.
    //!
    //! By implementing IFilterEntityManager interface and setting it on GetMultiplayer()->SetFilterEntityManager()
    //! entities can be filtered by IsEntityFiltered(...) returning true.
    //!
    //! Note: one cannot filter out entities in Level prefab (spawned by LoadLevel console command). Level prefabs are fully
    //! spawned on each client. Filtering of entities is applied to dynamically spawned prefabs, and specifically
    //! entities must have NetBindComponent on them.
    class IFilterEntityManager
    {
    public:
        AZ_RTTI(IFilterEntityManager, "{91F879F2-3DAF-43B8-B474-B312D26C0F48}");

        virtual ~IFilterEntityManager() = default;

        //! Return true if a given entity should be filtered out, false otherwise.
        //! Important: this method is a hot code path, it will be called over all entities around each player frequently.
        //! Ideally, this method should be implemented as a quick look up.
        //!
        //! @param entity the entity to be considered for filtering
        //! @param controllerEntity player's entity for the associated connection
        //! @param connectionId the affected connection should the entity be filtered out.
        //! @return if false the given entity will be not be replicated to the connection
        virtual bool IsEntityFiltered(AZ::Entity* entity, ConstNetworkEntityHandle controllerEntity, AzNetworking::ConnectionId connectionId) = 0;
    };
}
