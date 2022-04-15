/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptBus.h>

namespace AzFramework::Scripts
{
    //! Behavior Context forwarder for SpawnableScriptNotificationsBus
    struct SpawnableScriptNotificationsHandler final
        : public SpawnableScriptNotificationsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
        AZ_EBUS_BEHAVIOR_BINDER(
            SpawnableScriptNotificationsHandler,
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
