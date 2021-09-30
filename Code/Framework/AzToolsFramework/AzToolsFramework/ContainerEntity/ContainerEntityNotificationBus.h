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
    //! Used to notify changes of state for Container Entities.
    class ContainerEntityNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AzFramework::EntityContextId;
        //////////////////////////////////////////////////////////////////////////
        
        //! Triggered when a container entity status changes.
        //! @param entityId The entity whose status has changed.
        virtual void OnContainerEntityStatusChanged(AZ::EntityId entityId) = 0;

    protected:
        ~ContainerEntityNotifications() = default;
    };

    using ContainerEntityNotificationBus = AZ::EBus<ContainerEntityNotifications>;

} // namespace AzToolsFramework
