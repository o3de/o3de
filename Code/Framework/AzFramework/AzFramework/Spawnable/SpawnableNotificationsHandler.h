/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Spawnable/SpawnableBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace AzFramework
{
    //! Behavior Context forwarder for SpawnableNotificationsBus
    struct SpawnableNotificationsHandler final
        : public SpawnableNotificationsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
        AZ_EBUS_BEHAVIOR_BINDER(
            SpawnableNotificationsHandler,
            "{3403D10A-9541-46A1-97CD-F994FF9E52C2}",
            AZ::SystemAllocator,
            OnSpawn,
            OnDespawn);

        void OnSpawn(
            EntitySpawnTicket spawnTicket,
            AZStd::vector<AZ::EntityId> entityList) override
        {
            Call(FN_OnSpawn, spawnTicket, entityList);
        }

        virtual void OnDespawn(EntitySpawnTicket spawnTicket)
        {
            Call(FN_OnDespawn, spawnTicket);
        }
    };
} // namespace AzFramework
