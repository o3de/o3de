/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    class BehaviorMethod;
    class BehaviorProperty;

    class BehaviorObjectSignalsInterface
        : public EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        // Keyed off of the void* pointer of the object that is being listened to.
        typedef const void* BusIdType;

        virtual void OnMemberMethodCalled(const BehaviorMethod* method) = 0;
    };

    using BehaviorObjectSignals =  AZ::EBus<BehaviorObjectSignalsInterface>;
}

DECLARE_EBUS_EXTERN_DLL_MULTI_ADDRESS(BehaviorObjectSignalsInterface);
