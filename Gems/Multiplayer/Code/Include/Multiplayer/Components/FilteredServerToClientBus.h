/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Multiplayer/Components/FilteredReplicationInterface.h>

namespace Multiplayer
{
    using FilteredReplicationHandlerChanged = AZ::Event<FilteredReplicationInterface*>;

    class FilteredServerToClientRequests
        : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(FilteredServerToClientRequests, "{A5DE9343-5E3E-4FFC-A0D5-EDDCE57AFD48}");

        virtual void SetFilteredInterface(FilteredReplicationInterface* filteredReplication) = 0;
        virtual FilteredReplicationInterface* GetFilteredInterface() = 0;

        virtual void SetFilteredReplicationHandlerChanged(FilteredReplicationHandlerChanged::Handler handler) = 0;
    };

    //! The EBus for requests to position and parent an entity.
    using FilteredServerToClientRequestBus = AZ::EBus<FilteredServerToClientRequests>;

    class FilteredServerToClientNotifications
        : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(FilteredServerToClientNotifications, "{2306596C-D5C3-4AC4-B9B5-D2588340C5FD}");

        virtual void OnFilteredServerToClientActivated(AZ::EntityId controllerEntity) = 0;
    };

    //! The EBus for requests to position and parent an entity.
    using FilteredServerToClientNotificationBus = AZ::EBus<FilteredServerToClientNotifications>;
}
