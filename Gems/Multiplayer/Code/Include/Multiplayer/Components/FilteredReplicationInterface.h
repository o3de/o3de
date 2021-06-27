/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>

namespace Multiplayer
{
    //! @class FilteredReplicationInterface
    //! @brief Allows filtering of entities when replicating from a server to clients.
    //!        This filtering can be done on per connection basis.
    class FilteredReplicationInterface
    {
    public:
        virtual ~FilteredReplicationInterface() = default;

        //! Return true if a given entity should be filtered out, false otherwise.
        //!
        //! @param entity the entity to be considered for filtering
        //! @param controllerEntity player's entity for the associated connection
        //! @param connectionId the affected connection should the entity be filtered out.
        //! @return if false the given entity will be not be replicated to the connection
        virtual bool IsEntityFiltered(AZ::Entity* entity, ConstNetworkEntityHandle controllerEntity, const AzNetworking::ConnectionId connectionId) = 0;
    };
}
