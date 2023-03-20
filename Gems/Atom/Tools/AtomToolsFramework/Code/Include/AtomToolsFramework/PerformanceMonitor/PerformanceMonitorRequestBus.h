/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AtomToolsFramework/PerformanceMonitor/PerformanceMetrics.h>

namespace AtomToolsFramework
{
    //! Provides communication with Performance Monitor
    class PerformanceMonitorRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Enable or disable CPU and GPU monitoring
        //! @param enabled whether performance monitoring should be enabled
        virtual void SetProfilerEnabled(bool enabled) = 0;

        //! Get current metrics
        virtual const PerformanceMetrics& GetMetrics() = 0;
    };

    using PerformanceMonitorRequestBus = AZ::EBus<PerformanceMonitorRequests>;
} // namespace AtomToolsFramework
