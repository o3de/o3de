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
    //! @class IMultiplayerSpawner
    //! @brief IMultiplayerSpawner routes spawning requests for connecting players from
    //! the Muliplayer Gem to game logic utilizing it
    //!
    //! IMultiplayerSpawner is an AZ::Interface<T> that provides applications a means to
    //! let games tell the Muliplayer Gem what to spawn on player connection.
    //! IMultiplayerSpawner is intended to be implemented on games utilizing the
    //! Multiplayer Gem. The Multiplayer Gem then calls the implementation via
    //! AZ::Interface.

    class IMultiplayerSpawner
    {
    public:
        AZ_RTTI(IMultiplayerSpawner, "{E5525317-A476-4209-BE45-477FB9D96083}");

        virtual ~IMultiplayerSpawner() = default;

        virtual AZStd::pair<Multiplayer::PrefabEntityId, AZ::Transform> SpawnPlayerPrefab(uint64_t userId) = 0;
    };
}
