/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Process/Scheduler/TestImpactProcessScheduler.h>
#include <Process/TestImpactProcess.h>
#include <Process/TestImpactProcessException.h>
#include <Process/TestImpactProcessInfo.h>
#include <Process/TestImpactProcessLauncher.h>

namespace TestImpact
{
    struct ProcessInFlight
    {
        AZStd::unique_ptr<Process> m_process;
        AZStd::optional<AZStd::chrono::high_resolution_clock::time_point> m_startTime;
        AZStd::string m_stdOutput;
        AZStd::string m_stdError;
    };

    class ProcessScheduler::ExecutionState
    {
    public:
        ExecutionState(
            size_t maxConcurrentProcesses,
            AZStd::optional<AZStd::chrono::milliseconds> processTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> scheduleTimeout,
            ProcessLaunchCallback& processLaunchCallback,
            ProcessExitCallback& processExitCallback);
        ~ExecutionState();

        ProcessSchedulerResult MonitorProcesses(const AZStd::vector<ProcessInfo>& processes);
        void TerminateAllProcesses(ExitCondition exitStatus);
    private:
        ProcessCallbackResult PopAndLaunch(ProcessInFlight& processInFlight);
        StdContent ConsumeProcessStdContent(ProcessInFlight& processInFlight);
        void AccumulateProcessStdContent(ProcessInFlight& processInFlight);

        size_t m_maxConcurrentProcesses = 0;
        ProcessLaunchCallback m_processLaunchCallback;
        ProcessExitCallback m_processExitCallback;
        AZStd::optional<AZStd::chrono::milliseconds> m_processTimeout;
        AZStd::optional<AZStd::chrono::milliseconds> m_scheduleTimeout;
        AZStd::chrono::high_resolution_clock::time_point m_startTime;
        AZStd::vector<ProcessInFlight> m_processPool;
        AZStd::queue<ProcessInfo> m_processQueue;
    };

    ProcessScheduler::ExecutionState::ExecutionState(
        size_t maxConcurrentProcesses,
        AZStd::optional<AZStd::chrono::milliseconds> processTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> scheduleTimeout,
        ProcessLaunchCallback& processLaunchCallback,
        ProcessExitCallback& processExitCallback)
        : m_maxConcurrentProcesses(maxConcurrentProcesses)
        , m_processLaunchCallback(processLaunchCallback)
        , m_processExitCallback(processExitCallback)
        , m_processTimeout(processTimeout)
        , m_scheduleTimeout(scheduleTimeout)
    {
        AZ_TestImpact_Eval(
            !m_processTimeout.has_value() || m_processTimeout->count() > 0, ProcessException,
            "Process timeout must be empty or non-zero value");
        AZ_TestImpact_Eval(
            !m_scheduleTimeout.has_value() || m_scheduleTimeout->count() > 0, ProcessException,
            "Scheduler timeout must be empty or non-zero value");
    }

    ProcessScheduler::ExecutionState::~ExecutionState()
    {
        TerminateAllProcesses(ExitCondition::Terminated);
    }

    ProcessSchedulerResult ProcessScheduler::ExecutionState::MonitorProcesses(const AZStd::vector<ProcessInfo>& processes)
    {
        AZ_TestImpact_Eval(!processes.empty(), ProcessException, "Number of processes to launch cannot be 0");
        m_startTime = AZStd::chrono::high_resolution_clock::now();
        const size_t numConcurrentProcesses = AZStd::min(processes.size(), m_maxConcurrentProcesses);
        m_processPool.resize(numConcurrentProcesses);

        for (const auto& process : processes)
        {
            m_processQueue.emplace(process);
        }

        for (auto& process : m_processPool)
        {
            if (PopAndLaunch(process) == ProcessCallbackResult::Abort)
            {
                // Client chose to abort the scheduler
                TerminateAllProcesses(ExitCondition::Terminated);
                return ProcessSchedulerResult::UserAborted;
            }
        }

        while (true)
        {
            // Check to see whether or not the scheduling has exceeded its specified runtime
            if (m_scheduleTimeout.has_value())
            {
                const auto shedulerRunTime = AZStd::chrono::milliseconds(AZStd::chrono::high_resolution_clock::now() - m_startTime);

                if (shedulerRunTime > m_scheduleTimeout)
                {
                    // Runtime exceeded, terminate all proccesses and schedule no further
                    TerminateAllProcesses(ExitCondition::Timeout);
                    return ProcessSchedulerResult::Timeout;
                }
            }

            // Flag to determine whether or not there are currently any processes in-flight
            bool processesInFlight = false;

            // Loop round the process pool and visit round robin queued up processes for launch
            for (auto& processInFlight : m_processPool)
            {
                if (processInFlight.m_process)
                {
                    // Process is alive (note: not necessarily currently running)
                    AccumulateProcessStdContent(processInFlight);
                    const ProcessId processId = processInFlight.m_process->GetProcessInfo().GetId();

                    if (!processInFlight.m_process->IsRunning())
                    {
                        // Process has exited of its own accord
                        const ReturnCode returnCode = processInFlight.m_process->GetReturnCode().value();
                        processInFlight.m_process.reset();
                        const auto exitTime = AZStd::chrono::high_resolution_clock::now();

                        // Inform the client that the processes has exited
                        if (ProcessCallbackResult::Abort == m_processExitCallback(
                            processId,
                            ExitCondition::Gracefull,
                            returnCode,
                            ConsumeProcessStdContent(processInFlight),
                            exitTime))
                        {
                            // Client chose to abort the scheduler
                            TerminateAllProcesses(ExitCondition::Terminated);
                            return ProcessSchedulerResult::UserAborted;
                        }
                        else if (!m_processQueue.empty())
                        {
                            // This slot in the pool is free so launch one of the processes waiting in the queue
                            if (PopAndLaunch(processInFlight) == ProcessCallbackResult::Abort)
                            {
                                // Client chose to abort the scheduler
                                TerminateAllProcesses(ExitCondition::Terminated);
                                return ProcessSchedulerResult::UserAborted;
                            }
                            else
                            {
                                // We know from the above PopAndLaunch there is at least one process in-flight this iteration
                                processesInFlight = true;
                            }
                        }
                    }
                    else
                    {
                        // Process is still in-flight
                        const auto exitTime = AZStd::chrono::high_resolution_clock::now();
                        const auto runTime = AZStd::chrono::milliseconds(exitTime - processInFlight.m_startTime.value());

                        // Check to see whether or not the processes has exceeded its specified flight time
                        if (m_processTimeout.has_value() && runTime > m_processTimeout)
                        {
                            processInFlight.m_process->Terminate(ProcessTimeoutErrorCode);
                            const ReturnCode returnCode = processInFlight.m_process->GetReturnCode().value();
                            processInFlight.m_process.reset();

                            if (ProcessCallbackResult::Abort == m_processExitCallback(
                                processId,
                                ExitCondition::Timeout,
                                returnCode,
                                ConsumeProcessStdContent(processInFlight),
                                exitTime))
                            {
                                // Client chose to abort the scheduler
                                TerminateAllProcesses(ExitCondition::Terminated);
                                return ProcessSchedulerResult::UserAborted;
                            }
                        }

                        // We know that at least this process is in-flight this iteration
                        processesInFlight = true;
                    }
                }
                else
                {
                    // Queue is empty, no more processes to launch
                    if (!m_processQueue.empty())
                    {
                        if (PopAndLaunch(processInFlight) == ProcessCallbackResult::Abort)
                        {
                            // Client chose to abort the scheduler
                            TerminateAllProcesses(ExitCondition::Terminated);
                            return ProcessSchedulerResult::UserAborted;
                        }
                        else
                        {
                            // We know from the above PopAndLaunch there is at least one process in-flight this iteration
                            processesInFlight = true;
                        }
                    }
                }
            }

            if (!processesInFlight)
            {
                break;
            }
        }

        return ProcessSchedulerResult::Graceful;
    }

    ProcessCallbackResult ProcessScheduler::ExecutionState::PopAndLaunch(ProcessInFlight& processInFlight)
    {
        auto processInfo = m_processQueue.front();
        m_processQueue.pop();
        const auto createTime = AZStd::chrono::high_resolution_clock::now();
        LaunchResult createResult = LaunchResult::Success;

        try
        {
            processInFlight.m_process = LaunchProcess(AZStd::move(processInfo));
            processInFlight.m_startTime = createTime;
        }
        catch ([[maybe_unused]] ProcessException& e)
        {
            AZ_Warning("ProcessScheduler", false, e.what());
            createResult = LaunchResult::Failure;
        }

        return m_processLaunchCallback(processInfo.GetId(), createResult, createTime);
    }

    void ProcessScheduler::ExecutionState::AccumulateProcessStdContent(ProcessInFlight& processInFlight)
    {
        // Accumulate the stdout/stderr so we don't deadlock with the process waiting for the pipe to empty before finishing
        processInFlight.m_stdOutput += processInFlight.m_process->ConsumeStdOut().value_or("");
        processInFlight.m_stdError += processInFlight.m_process->ConsumeStdErr().value_or("");
    }

    StdContent ProcessScheduler::ExecutionState::ConsumeProcessStdContent(ProcessInFlight& processInFlight)
    {
        return
        {
            !processInFlight.m_stdOutput.empty()
                ? AZStd::optional<AZStd::string>{AZStd::move(processInFlight.m_stdOutput)}
                : AZStd::nullopt,
            !processInFlight.m_stdError.empty()
                ? AZStd::optional<AZStd::string>{AZStd::move(processInFlight.m_stdError)}
                : AZStd::nullopt
        };
    }

    void ProcessScheduler::ExecutionState::TerminateAllProcesses(ExitCondition exitStatus)
    {
        bool isCallingBackToClient = true;
        const ReturnCode returnCode = static_cast<ReturnCode>(exitStatus);

        for (auto& processInFlight : m_processPool)
        {
            if (processInFlight.m_process)
            {
                processInFlight.m_process->Terminate(ProcessTerminateErrorCode);
                AccumulateProcessStdContent(processInFlight);

                if (isCallingBackToClient)
                {
                    const auto exitTime = AZStd::chrono::high_resolution_clock::now();
                    if (ProcessCallbackResult::Abort == m_processExitCallback(
                        processInFlight.m_process->GetProcessInfo().GetId(),
                        exitStatus,
                        returnCode,
                        ConsumeProcessStdContent(processInFlight),
                        exitTime))
                    {
                        // Client chose to abort the scheduler, do not make any further callbacks
                        isCallingBackToClient = false;
                    }
                }

                processInFlight.m_process.reset();
            }
        }
    }

    ProcessScheduler::ProcessScheduler(size_t maxConcurrentProcesses)
        : m_maxConcurrentProcesses(maxConcurrentProcesses)
    {
        AZ_TestImpact_Eval(maxConcurrentProcesses != 0, ProcessException, "Max Number of concurrent processes in flight cannot be 0");
    }

    ProcessScheduler::~ProcessScheduler() = default;

    ProcessSchedulerResult ProcessScheduler::Execute(
        const AZStd::vector<ProcessInfo>& processes,
        AZStd::optional<AZStd::chrono::milliseconds> processTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> scheduleTimeout,
        ProcessLaunchCallback processLaunchCallback,
        ProcessExitCallback processExitCallback)
    {
        AZ_TestImpact_Eval(!m_executionState, ProcessException, "Couldn't execute schedule, schedule already in progress");
        m_executionState = AZStd::make_unique<ExecutionState>(
            m_maxConcurrentProcesses, processTimeout, scheduleTimeout, processLaunchCallback, processExitCallback);
        const auto result = m_executionState->MonitorProcesses(processes);
        m_executionState.reset();
        return result;
    }
} // namespace TestImpact
