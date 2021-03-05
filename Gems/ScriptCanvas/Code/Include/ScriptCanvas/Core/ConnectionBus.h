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