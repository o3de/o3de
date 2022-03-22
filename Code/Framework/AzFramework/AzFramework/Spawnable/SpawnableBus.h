/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Spawnable/SpawnableAssetRef.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace AzFramework
{
    // Allows EBus calls to Spawnable API
    class SpawnableRequests
        : public AZ::EBusTraits
    {
    public:
        // Only a single handler is allowed
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Creates EntitySpawnTicket using provided prefab asset
        virtual EntitySpawnTicket CreateSpawnTicket(const SpawnableAssetRef& spawnableAsset) = 0;

        //! Spawns a prefab and places it relative to provided parent,
        //! if no parentId is specified then places it in world coordinates
        virtual bool Spawn(
            EntitySpawnTicket spawnTicket,
            AZ::EntityId parentId,
            AZ::Vector3 translation,
            AZ::Vector3 rotation,
            float scale) = 0;

        //! Despawns a prefab
        virtual bool Despawn(EntitySpawnTicket spawnTicket) = 0;
    };

    using SpawnableRequestsBus = AZ::EBus<SpawnableRequests>;

    // Provides spawn notifications from Spawnable API
    class SpawnableNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        using BusIdType = EntitySpawnTicket::Id;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        //! Notifies that spawn operation was completed
        virtual void OnSpawn(
            [[maybe_unused]] EntitySpawnTicket spawnTicket,
            [[maybe_unused]] AZStd::vector<AZ::EntityId> entityList) {}

        //! Notifies that despawn operation was completed
        virtual void OnDespawn([[maybe_unused]] EntitySpawnTicket spawnTicket) {}
    };
    
    using SpawnableNotificationsBus = AZ::EBus<SpawnableNotifications>;
} // namespace AzFramework
