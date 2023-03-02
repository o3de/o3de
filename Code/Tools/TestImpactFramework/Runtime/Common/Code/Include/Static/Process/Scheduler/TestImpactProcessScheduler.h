/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/TestImpactProcessInfo.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Result of the attempt to launch a process.
    enum class LaunchResult : bool
    {
        Failure,
        Success
    };

    //! The condition under which the processes exited.
    //! @note For convinience, the terminate and timeout condition values are set to the corresponding return value sent to the
    //! process.
    enum class ExitCondition : ReturnCode
    {
        Gracefull, //!< Process has exited of its own accord.
        Terminated = ProcessTerminateErrorCode, //!< The process was terminated by the client/scheduler.
        Timeout = ProcessTimeoutErrorCode //!< The process was terminated by the scheduler due to exceeding runtime limit.
    };

    //! Client result for process scheduler callbacks.
    enum class ProcessCallbackResult : bool
    {
        Continue, //!< Continune scheduling.
        Abort //!< Abort scheduling immediately.
    };

    //! Result of the process scheduling sequence.
    enum class ProcessSchedulerResult : AZ::u8
    {
        Graceful, //!< The scheduler completed its run without incident or was terminated gracefully in response to a client callback result.
        UserAborted, //!< The scheduler aborted prematurely due to the user returning an abort value from thier callback handler.
        Timeout //!< The scheduler aborted its run prematurely due to its runtime exceeding the scheduler timeout value.
    };

    //!
    class ProcessSchedulerNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

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

    using ProcessSchedulerNotificationsBus = AZ::EBus<ProcessSchedulerNotifications>;

    //! Returns the aggregate process callback result where if one or more aborts exists, the result is abort, otherwise continue. 
    ProcessCallbackResult GetAggregateProcessCallbackResult(const AZ::EBusAggregateResults<ProcessCallbackResult>& results);

    //! Schedules a batch of processes for launch using a round robin approach to distribute the in-flight processes over
    //! the specified number of concurrent process slots.
    class ProcessScheduler
    {
    public:
        //! Constructs the scheduler with the specified batch of processes.
        //! @param maxConcurrentProcesses The maximum number of concurrent processes in-flight.
        explicit ProcessScheduler(size_t maxConcurrentProcesses);
        ~ProcessScheduler();

        //! Executes the specified processes and calls the client callbacks (if any) as each process progresses in its life cycle.
        //! @note Multiple subsequent calls to Execute are permitted.
        //! @param processes The batch of processes to schedule.
        //! @param processTimeout The maximum duration a process may be in-flight for before being forcefully terminated.
        //! @param scheduleTimeout The maximum duration the scheduler may run before forcefully terminating all in-flight processes.
        //! @returns The state that triggered the end of the schedule sequence.
        ProcessSchedulerResult Execute(
            const AZStd::vector<ProcessInfo>& processes,
            AZStd::optional<AZStd::chrono::milliseconds> processTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> scheduleTimeout);

    private:
        class ExecutionState;
        AZStd::unique_ptr<ExecutionState> m_executionState;
        size_t m_maxConcurrentProcesses = 0;
    };
} // namespace TestImpact
