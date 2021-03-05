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
#ifndef AZCORE_DRILLER_BUS_H
#define AZCORE_DRILLER_BUS_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/OSAllocator.h>

namespace AZStd
{
    class mutex;
}

namespace AZ
{
    namespace Debug
    {
        class DrillerEBusMutex
        {
        public:
            typedef AZStd::recursive_mutex MutexType;

            static MutexType& GetMutex();
            void lock();
            bool try_lock();
            void unlock();
        };

        /**
         * Specialization of the EBusTraits for a driller bus. We make sure
         * all allocation are made using DebugAllocation (so no engine systems are involved).
         * In addition we make sure all driller buses use the same Mutex to synchronize data across
         * threads (so all events came in order all the time), they are still executed in the context of
         * the thread.
         */
        struct DrillerEBusTraits
            : public AZ::EBusTraits
        {
            typedef DrillerEBusMutex    MutexType;
            typedef OSStdAllocator  AllocatorType;
        };
    }
}

#endif // AZCORE_DRILLER_BUS_H
#pragma once
