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
