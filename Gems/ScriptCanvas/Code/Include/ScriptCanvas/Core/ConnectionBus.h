/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include "Core.h"
#include "Endpoint.h"

#include <AzCore/EBus/EBus.h>

namespace ScriptCanvas
{
    class ConnectionRequests :
        public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual const SlotId& GetTargetSlot() const = 0;
        virtual const SlotId& GetSourceSlot() const = 0;

        virtual const AZ::EntityId& GetTargetNode() const = 0;
        virtual const AZ::EntityId& GetSourceNode() const = 0;

        virtual const Endpoint& GetTargetEndpoint() const = 0;
        virtual const Endpoint& GetSourceEndpoint() const = 0;
    };

    using ConnectionRequestBus = AZ::EBus<ConnectionRequests>;
    
}
