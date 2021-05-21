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

#include <Process/Scheduler/TestImpactProcessScheduler.h>
#include <Process/TestImpactProcess.h>
#include <Process/TestImpactProcessException.h>
#include <Process/TestImpactProcessInfo.h>
#include <Process/TestImpactProcessLauncher.h>

namespace TestImpact
{
    struct ProcessScheduler::ProcessInFlight
    {
        AZStd::unique_ptr<Process> m_process;
        AZStd::optional<AZStd::chrono::high_resolution_clock::time_point> m_startTime;
        AZStd::string m_stdOutput;
        AZStd::string m_stdError;
    };

    ProcessScheduler::ProcessScheduler(
        const AZStd::vector<ProcessInfo>& processes,
        const ProcessLaunchCallback& processLaunchCallback,
        const ProcessExitCallback& processExitCallback,
        size_t maxConcurrentProcesses,
        AZStd::optional<AZStd::chrono::milliseconds> processTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> scheduleTimeout)
        : m_processCreateCallback(processLaunchCallback)
        , m_processExitCallback(processExitCallback)
        , m_processTimeout(processTimeout)
        , m_scheduleTimeout(scheduleTimeout)
        , m_startTime(AZStd::chrono::high_resolution_clock::now())
    {
        AZ_TestImpact_Eval(maxConcurrentProcesses != 0, ProcessException, "Max Number of concurrent processes in flight cannot be 0");
        AZ_TestImpact_Eval(!processes.empty(), ProcessException, "Number of processes to launch cannot be 0");
        AZ_TestImpact_Eval(
            !m_processTimeout.has_value() || m_processTimeout->count() > 0, ProcessException,
            "Process timeout must be empty or non-zero value");
        AZ_TestImpact_Eval(
            !m_scheduleTimeout.has_value() || m_scheduleTimeout->count() > 0, ProcessException,
            "Scheduler timeout must be empty or non-zero value");

        const size_t numConcurrentProcesses = AZStd::min(processes.size(), maxConcurrentProcesses);
        m_processPool.resize(numConcurrentProcesses);

        for (const auto& process : processes)
        {
            m_processQueue.emplace(process);
        }

        for (auto& process : m_processPool)
        {
            if (PopAndLaunch(process) == ProcessCallbackResult::Abort)
            {
                TerminateAllProcesses(ExitCondition::Terminated);
                return;
            }
        }

        MonitorProcesses();
    }

    ProcessScheduler::~ProcessScheduler()
    {
        TerminateAllProcesses(ExitCondition::Terminated);
    }

    void ProcessScheduler::MonitorProcesses()
    {
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
                    return;
                }
            }

            // Flag to determine whether or not there are currently any processes in-flight
            bool processesInFlight = false;

            // Loop round the process pool and visit round robin queued up processes for launch
            for (auto& processInFlight : m_processPool)
            {
                if (processInFlight.m_process)
                {
                    // Process is alive (note: not necessarilly currently running)
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
                            return;
                        }
                        else if (!m_processQueue.empty())
                        {
                            // This slot in the pool is free so launch one of the processes waiting in the queue
                            if (PopAndLaunch(processInFlight) == ProcessCallbackResult::Abort)
                            {
                                // Client chose to abort the scheduler
                                TerminateAllProcesses(ExitCondition::Terminated);
                                return;
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
                                // Flight time exceeded, terminate this process
                                TerminateAllProcesses(ExitCondition::Terminated);
                                return;
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
                            return;
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
    }

    ProcessCallbackResult ProcessScheduler::PopAndLaunch(ProcessInFlight& processInFlight)
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
        catch (ProcessException& e)
        {
            AZ_Warning("ProcessScheduler", false, e.what());
            createResult = LaunchResult::Failure;
        }

        return m_processCreateCallback(processInfo.GetId(), createResult, createTime);
    }

    void ProcessScheduler::AccumulateProcessStdContent(ProcessInFlight& processInFlight)
    {
        // Accumulate the stdout/stderr so we don't deadlock with the process waiting for the pipe to empty before finishing
        processInFlight.m_stdOutput += processInFlight.m_process->ConsumeStdOut().value_or("");
        processInFlight.m_stdError += processInFlight.m_process->ConsumeStdErr().value_or("");
    }

    StdContent ProcessScheduler::ConsumeProcessStdContent(ProcessInFlight& processInFlight)
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

    void ProcessScheduler::TerminateAllProcesses(ExitCondition exitStatus)
    {
        bool isCallingBackToClient = true;
        const ReturnCode returnCode = static_cast<ReturnCode>(exitStatus);

        for (auto& processInFlight : m_processPool)
        {
            if (processInFlight.m_process)
            {
                processInFlight.m_process->Terminate(ProcessTerminateErrorCode);
                AccumulateProcessStdContent(processInFlight);
                const ProcessId processId = processInFlight.m_process->GetProcessInfo().GetId();

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
} // namespace TestImpact
