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

#include <Atom/RHI/DeviceBusTraits.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * This bus is called first to enumerate platform memory heaps.
         */
        class MemoryStatisticsHeapEventInterface
            : public DeviceBusTraits
        {
        public:


            virtual void ReportMemoryHeaps(MemoryStatisticsBuilder& builder) const = 0;
        };

        using MemoryStatisticsHeapEventBus = AZ::EBus<MemoryStatisticsHeapEventInterface>;

        /**
         * This bus is called after heaps have been enumerated to populate memory usage within each heap.
         */
        class MemoryStatisticsEventInterface
            : public DeviceBusTraits
        {
        public:
            virtual void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const = 0;
        };

        using MemoryStatisticsEventBus = AZ::EBus<MemoryStatisticsEventInterface>;
    }
}
