/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace AzFramework::Scripts
{
    // Provides spawn notifications from Spawnable API
    class SpawnableScriptNotifications
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
    
    using SpawnableScriptNotificationsBus = AZ::EBus<SpawnableScriptNotifications>;
} // namespace AzFramework
