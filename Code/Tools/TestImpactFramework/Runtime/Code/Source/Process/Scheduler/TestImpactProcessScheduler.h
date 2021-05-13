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

#include <Process/TestImpactProcessInfo.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

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

    //! Callback for process launch attempt.
    //! @param processId The id of the process that attempted to launch.
    //! @param launchResult The result of the process launch attempt.
    //! @param createTime The timestamp of the process launch attempt.
    using ProcessLaunchCallback =
        AZStd::function<ProcessCallbackResult(
            ProcessId processId,
            LaunchResult launchResult,
            AZStd::chrono::high_resolution_clock::time_point createTime)>;

    //! Callback for process exit of successfully launched process.
    //! @param processId The id of the process that attempted to launch.
    //! @param exitStatus The circumstances upon which the processes exited.
    //! @param returnCode The return code of the exited process.
    //! @param std The standard output and standard error of the process.
    //! @param createTime The timestamp of the process exit.
    using ProcessExitCallback =
        AZStd::function<ProcessCallbackResult(
            ProcessId processId,
            ExitCondition exitStatus,
            ReturnCode returnCode,
            StdContent&& std,
            AZStd::chrono::high_resolution_clock::time_point exitTime)>;

    //! Schedules a batch of processes for launch using a round robin approach to distribute the in-flight processes over
    //! the specified number of concurrent process slots.
    class ProcessScheduler
    {
    public:
        //! Constructs the scheduler with the specified batch of processes.
        //! @param processes The batch of processes to schedule.
        //! @param processLaunchCallback The process launch callback function.
        //! @param processExitCallback The process exit callback function.
        //! @param maxConcurrentProcesses The maximum number of concurrent processes in-flight.
        //! @param processTimeout The maximum duration a process may be in-flight for before being forcefully terminated.
        //! @param scheduleTimeout The maximum duration the scheduler may run before forcefully terminating all in-flight processes.
        //! processes and abandoning any queued processes.
        ProcessScheduler(
            const AZStd::vector<ProcessInfo>& processes,
            const ProcessLaunchCallback& processLaunchCallback,
            const ProcessExitCallback& processExitCallback,
            size_t maxConcurrentProcesses,
            AZStd::optional<AZStd::chrono::milliseconds> processTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> scheduleTimeout);

        ~ProcessScheduler();

    private:
        struct ProcessInFlight;

        void MonitorProcesses();
        ProcessCallbackResult PopAndLaunch(ProcessInFlight& processInFlight);
        void TerminateAllProcesses(ExitCondition exitStatus);
        StdContent ConsumeProcessStdContent(ProcessInFlight& processInFlight);
        void AccumulateProcessStdContent(ProcessInFlight& processInFlight);

        const ProcessLaunchCallback m_processCreateCallback;
        const ProcessExitCallback m_processExitCallback;
        const AZStd::optional<AZStd::chrono::milliseconds> m_processTimeout;
        const AZStd::optional<AZStd::chrono::milliseconds> m_scheduleTimeout;
        const AZStd::chrono::high_resolution_clock::time_point m_startTime;
        AZStd::vector<ProcessInFlight> m_processPool;
        AZStd::queue<ProcessInfo> m_processQueue;
    };
} // namespace TestImpact
