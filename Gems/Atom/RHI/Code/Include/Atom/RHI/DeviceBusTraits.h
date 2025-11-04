/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ::RHI
{
    class Device;

    //! Describes the common traits used by buses addressed by a device instance.
    class DeviceBusTraits
        : public EBusTraits
    {
    public:
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
        using MutexType = AZStd::mutex;
        using BusIdType = Device*;
    };
}
