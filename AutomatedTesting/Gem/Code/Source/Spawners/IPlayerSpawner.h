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

namespace AZ
{
    class Transform;
}

namespace AutomatedTesting
{
    class NetworkPlayerSpawnerComponent;

    //! @class IPlayerSpawner
    //! @brief IPlayerSpawner coordinate NetworkPlayerSpawners
    //!
    //! IPlayerSpawner is an AZ::Interface<T> that provides a registry for
    //! NetworkPlayerSpawners which can then be queried when IMultiplayerSpawner
    //! events fire.

    class IPlayerSpawner
    {
    public:
        AZ_RTTI(IPlayerSpawner, "{48CE4CFF-594B-4C6F-B5E0-8290A72CFEF9}");
        virtual ~IPlayerSpawner() = default;

        virtual bool RegisterPlayerSpawner(NetworkPlayerSpawnerComponent* spawner) = 0;
        virtual AZStd::pair<Multiplayer::PrefabEntityId, AZ::Transform> GetNextPlayerSpawn() = 0;
        virtual bool UnregisterPlayerSpawner(NetworkPlayerSpawnerComponent* spawner) = 0;
    };
} // namespace AutomatedTesting