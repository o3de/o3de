/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
