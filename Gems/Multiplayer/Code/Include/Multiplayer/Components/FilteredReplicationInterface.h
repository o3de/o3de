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
