/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

#include <AzFramework/Entity/EntityContext.h>

namespace AzToolsFramework
{
    //! Used to notify changes of state for read-only entities.
    class ReadOnlyEntityPublicNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AzFramework::EntityContextId;
        //////////////////////////////////////////////////////////////////////////
        
        //! Triggered when an entity's read-only status changes.
        //! @param entityId The entity whose status has changed.
        //! @param readOnly The read-only state the container was changed to.
        virtual void OnReadOnlyEntityStatusChanged([[maybe_unused]] const AZ::EntityId& entityId, [[maybe_unused]] bool readOnly) {}

    protected:
        ~ReadOnlyEntityPublicNotifications() = default;
    };
    using ReadOnlyEntityPublicNotificationBus = AZ::EBus<ReadOnlyEntityPublicNotifications>;

    //! Used by the ReadOnlyEntitySystemComponent to query the read-only state of entities as set by systems using the API.
    class ReadOnlyEntityQueryRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AzFramework::EntityContextId;
        //////////////////////////////////////////////////////////////////////////
        
        //! Triggered when an entity's read-only status is queried.
        //! Allows multiple systems to weigh in on the read-only status of an entity.
        //! @param entityId The entity whose status has changed.
        //! @param[out] isReadOnly The output of the query. Should only be changed to true, and left untouched if false.
        virtual void IsReadOnly(const AZ::EntityId& entityId, bool& isReadOnly) = 0;

    protected:
        ~ReadOnlyEntityQueryRequests() = default;
    };
    using ReadOnlyEntityQueryRequestBus = AZ::EBus<ReadOnlyEntityQueryRequests>;

} // namespace AzToolsFramework
