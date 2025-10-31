/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBusTraits.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

namespace AZ::RHI
{
    //! This bus is called first to enumerate platform memory heaps.
    class MemoryStatisticsHeapEventInterface
        : public DeviceBusTraits
    {
    public:


        virtual void ReportMemoryHeaps(MemoryStatisticsBuilder& builder) const = 0;
    };

    using MemoryStatisticsHeapEventBus = AZ::EBus<MemoryStatisticsHeapEventInterface>;

    //! This bus is called after heaps have been enumerated to populate memory usage within each heap.
    class MemoryStatisticsEventInterface
        : public DeviceBusTraits
    {
    public:
        virtual void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const = 0;
    };

    using MemoryStatisticsEventBus = AZ::EBus<MemoryStatisticsEventInterface>;
}
