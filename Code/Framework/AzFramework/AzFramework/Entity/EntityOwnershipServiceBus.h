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

#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    using EntityContextId = AZ::Uuid;
    using EntityList = AZStd::vector<AZ::Entity*>;

    class EntityOwnershipService;   

    class EntityOwnershipServiceInterface
    {
    public:
        AZ_RTTI(EntityOwnershipServiceInterface, "{6490E958-5DF5-45CF-9A25-D857DB0C67DB}");

        EntityOwnershipServiceInterface() = default;
        virtual ~EntityOwnershipServiceInterface() = default;

        virtual AZStd::unique_ptr<EntityOwnershipService> CreateEntityOwnershipService() = 0;
    };

    class EntityOwnershipServiceNotifications
        : public AZ::EBusTraits
    {
    public:

        // We don't want anybody other than entity contexts to listen to these events.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EntityContextId;

        /**
         * Sends a notification before resetting the Entity Ownership Service.
         */
        virtual void PrepareForEntityOwnershipServiceReset() = 0;

        /**
         * Sends a notification indicating that the Entity Ownership Service has been reset.
         */
        virtual void OnEntityOwnershipServiceReset() = 0;

        /**
         * Signals that entities from a given stream have been reloaded.
         * @param entities A reference to a list of entities that are reloaded from the given stream.
         */
        virtual void OnEntitiesReloadedFromStream(const EntityList& entities) = 0;
    };

    using EntityOwnershipServiceNotificationBus = AZ::EBus<EntityOwnershipServiceNotifications>;
}
