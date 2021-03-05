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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

namespace ScriptCanvas
{
    class Datum;
    
    class DatumNotifications : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual void OnDatumEdited(const Datum* datum) = 0;
    };
    
    using DatumNotificationBus = AZ::EBus<DatumNotifications>;

    class DatumSystemNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void OnDatumChanged(Datum&) = 0;
    };

    using DatumSystemNotificationBus = AZ::EBus<DatumSystemNotifications>;
}
