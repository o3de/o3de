/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestImpactWin32_Process.h"

#include <Process/TestImpactProcessException.h>

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/string/conversions.h>

namespace TestImpact
{
    // Note: this is called from an OS thread
    VOID ProcessWin32::ProcessExitCallback(PVOID processPtr, [[maybe_unused]] BOOLEAN EventSignalled)
    {
        // Lock the process destructor from being entered from the client thread
        AZStd::lock_guard lifeLock(m_lifeCycleMutex);

        ProcessId id = reinterpret_cast<ProcessId>(processPtr);
        auto process = m_masterProcessList[id];

        // Check that the process hasn't already been destructed from the client thread
        if (process && process->m_isRunning)
        {
            // Lock state access and/or mutation from the client thread
            AZStd::lock_guard stateLock(process->m_stateMutex);
            process->RetrieveOSReturnCodeAndCleanUpProcess();
        }
    }

    ProcessWin32::ProcessWin32(const ProcessInfo& processInfo)
        : Process(processInfo)
    {
        AZStd::string args(m_processInfo.GetProcessPath().String());

        if (m_processInfo.GetStartupArgs().length())
        {
            args = AZStd::string::format("%s %s", args.c_str(), m_processInfo.GetStartupArgs().c_str());
        }

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = nullptr;
        sa.bInheritHandle = IsPiping();

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        CreatePipes(sa, si);

        AZStd::wstring argsW;
        AZStd::to_wstring(argsW, args.c_str());
        if (!CreateProcessW(
            NULL,
            argsW.data(),
            NULL,
            NULL,
            IsPiping(),
            CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW,
            NULL, NULL,
            &si, &pi))
        {
            throw ProcessException(AZStd::string::format("Couldn't create process with args: %s", args.c_str()));
        }

        ReleaseChildPipes();

        m_process = pi.hProcess;
        m_thread = pi.hThread;
        m_isRunning = true;

        {
            // Lock reading of the master process list from the OS thread
            AZStd::lock_guard lock(m_lifeCycleMutex);

            // Register this process with a unique id in the master process list
            m_uniqueId = m_uniqueIdCounter++;
            m_masterProcessList[m_uniqueId] = this;
        }

        // Register the process exit signal callback
        if (!RegisterWaitForSingleObject(
            &m_waitCallback,
            pi.hProcess,
            ProcessExitCallback,
            reinterpret_cast<PVOID>(m_uniqueId),
            INFINITE,
            WT_EXECUTEONLYONCE))
        {
            throw ProcessException("Couldn't register wait object for process exit event");
        }
    }

    bool ProcessWin32::IsPiping() const
    {
        return m_processInfo.ParentHasStdOutput() || m_processInfo.ParentHasStdError();
    }

    void ProcessWin32::CreatePipes(SECURITY_ATTRIBUTES& sa, STARTUPINFO& si)
    {
        if (IsPiping())
        {
            si.dwFlags = STARTF_USESTDHANDLES;

            if (m_processInfo.ParentHasStdOutput())
            {
                m_stdOutPipe.emplace(sa, si.hStdOutput);
            }

            if (m_processInfo.ParentHasStdError())
            {
                m_stdErrPipe.emplace(sa, si.hStdError);
            }
        }
    }

    void ProcessWin32::ReleaseChildPipes()
    {
        if (m_stdOutPipe)
        {
            m_stdOutPipe->ReleaseChild();
        }

        if (m_stdErrPipe)
        {
            m_stdErrPipe->ReleaseChild();
        }
    }

    void ProcessWin32::EmptyPipes()
    {
        if (m_stdOutPipe)
        {
            m_stdOutPipe->EmptyPipe();
        }

        if (m_stdErrPipe)
        {
            m_stdErrPipe->EmptyPipe();
        }
    }

    AZStd::optional<AZStd::string> ProcessWin32::ConsumeStdOut()
    {
        if (m_stdOutPipe)
        {
            AZStd::string contents = m_stdOutPipe->GetContentsAndClearInternalBuffer();
            if (!contents.empty())
            {
                return contents;
            }
        }

        return AZStd::nullopt;
    }

    AZStd::optional<AZStd::string> ProcessWin32::ConsumeStdErr()
    {
        if (m_stdErrPipe)
        {
            AZStd::string contents = m_stdErrPipe->GetContentsAndClearInternalBuffer();
            if (!contents.empty())
            {
                return contents;
            }
        }

        return AZStd::nullopt;
    }

    void ProcessWin32::Terminate(ReturnCode returnCode)
    {
        // Lock process cleanup from the OS thread
        AZStd::lock_guard lock(m_stateMutex);

        if (m_isRunning)
        {
            // Cancel the callback so we can wait for the signal ourselves
            // Note: we keep the state mutex locked as closing the callback is not guaranteed to be instantaneous
            m_waitCallback.Close();

            // Terminate the process and set the error code to the terminate code
            TerminateProcess(m_process, returnCode);
            SetReturnCodeAndCleanUpProcesses(returnCode);
        }
    }

    bool ProcessWin32::IsRunning() const
    {
        return m_isRunning;
    }

    void ProcessWin32::BlockUntilExit()
    {
        // Lock process cleanup from the OS thread
        AZStd::lock_guard lock(m_stateMutex);

        if (m_isRunning)
        {
            // Cancel the callback so we can wait for the signal ourselves
            // Note: we keep the state mutex locked as closing the callback is not guaranteed to be instantaneous
            m_waitCallback.Close();

            if (IsPiping())
            {
                // This process will be blocked from exiting if pipe not emptied so will deadlock if we wait
                // indefintely whilst there is still output in the pipes so instead keep waiting and checking
                // if the pipes need emptying until the process exits
                while (WAIT_OBJECT_0 != WaitForSingleObject(m_process, 1))
                {
                    EmptyPipes();
                }
            }
            else
            {
                // No possibility of pipe deadlocking, safe to wait indefinitely for process exit
                WaitForSingleObject(m_process, INFINITE);
            }

            // Now that the this process has definately exited we are safe to clean up
            RetrieveOSReturnCodeAndCleanUpProcess();
        }
    }

    void ProcessWin32::RetrieveOSReturnCodeAndCleanUpProcess()
    {
        DWORD returnCode;
        GetExitCodeProcess(m_process, &returnCode);
        SetReturnCodeAndCleanUpProcesses(returnCode);
    }

    void ProcessWin32::SetReturnCodeAndCleanUpProcesses(ReturnCode returnCode)
    {
        m_returnCode = returnCode;
        m_process.Close();
        m_thread.Close();
        m_waitCallback.Close();
        m_isRunning = false;
    }

    ProcessWin32::~ProcessWin32()
    {
        // Lock the process exit signal callback from being entered OS thread
        AZStd::lock_guard lock(m_lifeCycleMutex);

        // Remove this process from the master list so the process exit signal doesn't attempt to cleanup
        // this process if it is deleted client side
        m_masterProcessList[m_uniqueId] = nullptr;
    }
} // namespace TestImpact
