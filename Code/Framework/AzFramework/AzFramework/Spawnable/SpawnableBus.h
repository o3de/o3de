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

namespace AzFramework
{
    // Provides spawn notifications from Spawnable API
    class SpawnableNotifications
        : public AZ::EBusTraits
    {
    public:

        //! Notifies that a spawn entity ticket has been created
        virtual void OnSpawnEntityTicketCreated([[maybe_unused]] EntitySpawnTicket spawnTicket) {}

        //! Sent when the cached spawn entity tickets should be cleared
        virtual void ClearEntityTickets() {}

    };

    using SpawnableNotificationBus = AZ::EBus<SpawnableNotifications>;
} // namespace AzFramework
