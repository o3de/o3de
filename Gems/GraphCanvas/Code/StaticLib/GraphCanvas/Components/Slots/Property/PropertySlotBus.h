/*
* All or Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
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
