/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "TestImpactWin32_Handle.h"
#include "TestImpactWin32_Pipe.h"

#include <Process/TestImpactProcess.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Platform-specific implementation of Process.
    class ProcessWin32
        : public Process
    {
    public:
        explicit ProcessWin32(const ProcessInfo& processInfo);
        ProcessWin32(ProcessWin32&& other) = delete;
        ProcessWin32(ProcessWin32& other) = delete;
        ProcessWin32& operator=(ProcessWin32& other) = delete;
        ProcessWin32& operator=(ProcessWin32&& other) = delete;
        ~ProcessWin32();

        // Process overrides...
        void Terminate(ReturnCode returnCode) override;
        void BlockUntilExit() override;
        bool IsRunning() const override;
        AZStd::optional<AZStd::string> ConsumeStdOut() override;
        AZStd::optional<AZStd::string> ConsumeStdErr() override;

    private:
        //! Callback for process exit signal.
        static VOID ProcessExitCallback(PVOID processPtr, BOOLEAN EventSignalled);

        //! Retrieves the return code and cleans up the OS handles.
        void RetrieveOSReturnCodeAndCleanUpProcess();

        //! Sets the return code and cleans up the OS handles
        void SetReturnCodeAndCleanUpProcesses(ReturnCode returnCode);

        //! Returns true if either stdout or stderr is beign redirected.
        bool IsPiping() const;

        //! Creates the parent and child pipes for stdout and/or stderr.
        void CreatePipes(SECURITY_ATTRIBUTES& sa, STARTUPINFO& si);

        //! Empties all pipes so the process can exit without deadlocking.
        void EmptyPipes();

        //! Releases the child end of the stdout and/or stderr pipes/
        void ReleaseChildPipes();

        // Flag to determine whether or not the process is in flight
        AZStd::atomic_bool m_isRunning = false;

        // Unique id assigned to this process (not the same as the id assigned by the client in the ProcessInfo class)
        // as used in the master process list
        size_t m_uniqueId = 0;

        // Handles to OS process
        ObjectHandle m_process;
        ObjectHandle m_thread;

        // Handle to process exit signal callback
        WaitHandle m_waitCallback;

        // Process to parent standard output piping
        AZStd::optional<Pipe> m_stdOutPipe;
        AZStd::optional<Pipe> m_stdErrPipe;

        // Mutex protecting process state access/mutation from the OS thread and client thread
        mutable AZStd::mutex m_stateMutex;

        // Mutex keeping the process life cycles in sync between the OS thread and client thread
        inline static AZStd::mutex m_lifeCycleMutex;

        // Unique counter to give each launched process a unique id
        inline static size_t m_uniqueIdCounter = 1;

        // Master process list used to ensure consistency of process lifecycles between OS thread and client thread
        inline static std::unordered_map<ProcessId, ProcessWin32*> m_masterProcessList;
    };
} // namespace TestImpact
