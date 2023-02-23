/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>


namespace Multiplayer
{
    //! @class SimplePlayerSpawnerRequests
    //! @brief The SimplePlayerSpawnerRequest event-bus exposes helper methods regarding network player spawners.
    //! For example, it's common for game servers want information regarding valid spawn locations in order to reset a player's position after the player dies or the round restarts (respawning).
    class SimplePlayerSpawnerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        //! Visits and returns the next player spawn point, causing the current spawn point index to increment.
        //! Method is only valid if called from the multiplayer host/authority; clients are not given information regarding spawn points.
        //! @return AZ::Transform The location of the next spawn point
        virtual AZ::Transform VisitNextSpawnPoint() = 0;

        //! Returns the next player spawn point. Unlike VisitNextSpawnPoint, this will not cause the current spawn point index to increment.
        //! Method is only valid if called from the multiplayer host/authority; clients are not given information regarding spawn points.
        //! @return AZ::Transform The location of the next spawn point
        virtual AZ::Transform GetNextSpawnPoint() const = 0;
    };
    using SimplePlayerSpawnerRequestBus = AZ::EBus<SimplePlayerSpawnerRequests>;
} // namespace Multiplayer
