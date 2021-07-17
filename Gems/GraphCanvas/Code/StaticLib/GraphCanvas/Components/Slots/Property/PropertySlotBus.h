/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QColor>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

namespace GraphCanvas
{
    class PropertySlotRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual const AZ::Crc32& GetPropertyId() const = 0;
    };

    using PropertySlotRequestBus = AZ::EBus<PropertySlotRequests>;    
    
    class PropertySlotNotifications
        : public AZ::EBusTraits
    {
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
    };
    
    using PropertySlotNotificationBus = AZ::EBus<PropertySlotNotifications>;
}
