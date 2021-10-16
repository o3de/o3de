/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>

namespace Profiler
{
    class ProfilerRequests
    {
    public:
        AZ_RTTI(ProfilerRequests, "{3757c4e5-1941-457c-85ae-16305e17a4c6}");
        virtual ~ProfilerRequests() = default;

        //! Enable/Disable the CpuProfiler
        virtual void SetProfilerEnabled(bool enabled) = 0;

        //! Dump a single frame of Cpu profiling data
        virtual bool CaptureCpuProfilingStatistics(const AZStd::string& outputFilePath) = 0;

        //! Start a multiframe capture of CPU profiling data.
        virtual bool BeginContinuousCpuProfilingCapture() = 0;

        //! End and dump an in-progress continuous capture.
        virtual bool EndContinuousCpuProfilingCapture(const AZStd::string& outputFilePath) = 0;
    };

    class ProfilerBusTraits
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    class ProfilerNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~ProfilerNotifications() = default;

        //! Notify when the current CpuProfilingStatistics capture is finished
        //! @param result Set to true if it's finished successfully
        //! @param info The output file path or error information which depends on the return.
        virtual void OnCaptureCpuProfilingStatisticsFinished(bool result, const AZStd::string& info) = 0;
    };

    using ProfilerInterface = AZ::Interface<ProfilerRequests>;
    using ProfilerRequestBus = AZ::EBus<ProfilerRequests, ProfilerBusTraits>;
    using ProfilerNotificationBus = AZ::EBus<ProfilerNotifications>;
} // namespace Profiler
