/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/TestImpactProcessInfo.h>

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
