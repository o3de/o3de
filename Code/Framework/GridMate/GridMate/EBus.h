/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_EBUS_H
#define GM_EBUS_H

#include <GridMate/Memory.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>

namespace GridMate
{
    class IGridMate;

    struct GridMateEBusTraits
        : public AZ::EBusTraits
    {
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; ///< Allow multiple instances of gridmate in an unordered_map.
        typedef AZStd::recursive_mutex MutexType;       ///< We do allow running multiple instances gridmate on different threads.
        typedef IGridMate* BusIdType;       ///< Use the GridMate instance as an ID
    };
}

#endif // GM_EBUS_H
