/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/Scheduler/TestImpactProcessScheduler.h>

#include <AzCore/EBus/EBus.h>

namespace TestImpact
{
    //! Bus for process scheduler notifications.
    class ProcessSchedulerNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides ...
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Callback for process launch attempt.
        //! @param processId The id of the process that attempted to launch.
        //! @param launchResult The result of the process launch attempt.
        //! @param createTime The timestamp of the process launch attempt.
        virtual ProcessCallbackResult OnProcessLaunch(
            [[maybe_unused]] ProcessId processId,
            [[maybe_unused]] LaunchResult launchResult,
            [[maybe_unused]] AZStd::chrono::steady_clock::time_point createTime)
        {
            return ProcessCallbackResult::Continue;
        }

        //! Callback for process exit of successfully launched process.
        //! @param processId The id of the process that attempted to launch.
        //! @param exitCondition The circumstances upon which the processes exited.
        //! @param returnCode The return code of the exited process.
        //! @param std The standard output and standard error of the process.
        //! @param createTime The timestamp of the process exit.
        virtual ProcessCallbackResult OnProcessExit(
            [[maybe_unused]] ProcessId processId,
            [[maybe_unused]] ExitCondition exitCondition,
            [[maybe_unused]] ReturnCode returnCode,
            [[maybe_unused]] const StdContent& std,
            [[maybe_unused]] AZStd::chrono::steady_clock::time_point exitTime)
        {
            return ProcessCallbackResult::Continue;
        }

        //! Callback for process standard output/error buffer consumption in real-time.
        //! @note The full standard output/error data is available to all capturing processes at their end of life regardless of this
        //! callback.
        //! @param processId The id of the process that attempted to launch.
        //! @param stdOutput The total accumulated standard output buffer.
        //! @param stdError The total accumulated standard error buffer.
        //! @param stdOutputDelta The standard output buffer data since the last callback.
        //! @param stdErrorDelta The standard error buffer data since the last callback.
        virtual void OnRealtimeStdContent(
            [[maybe_unused]] ProcessId processId,
            [[maybe_unused]] const AZStd::string& stdOutput,
            [[maybe_unused]] const AZStd::string& stdError,
            [[maybe_unused]] const AZStd::string& stdOutputDelta,
            [[maybe_unused]] const AZStd::string& stdErrorDelta)
        {
        }
    };

    using ProcessSchedulerNotificationBus = AZ::EBus<ProcessSchedulerNotifications>;

    //! Returns the aggregate process callback result where if one or more aborts exists, the result is abort, otherwise continue.
    ProcessCallbackResult GetAggregateProcessCallbackResult(const AZ::EBusAggregateResults<ProcessCallbackResult>& results);
} // namespace TestImpact
