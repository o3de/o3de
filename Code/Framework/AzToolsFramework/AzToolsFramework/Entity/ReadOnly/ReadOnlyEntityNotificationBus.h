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
    class ReadOnlyEntityNotifications
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
        //! @param open The read-only state the container was changed to.
        virtual void OnReadOnlyEntityStatusChanged([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] bool readOnly) {}

    protected:
        ~ReadOnlyEntityNotifications() = default;
    };

    using ReadOnlyEntityNotificationBus = AZ::EBus<ReadOnlyEntityNotifications>;

} // namespace AzToolsFramework
