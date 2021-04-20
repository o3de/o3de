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

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Identifier to distinguish between processes.
    using ProcessId = size_t;

    //! Return code of successfully launched process.
    using ReturnCode = int;

    //! Error code for processes that are forcefully terminated whilst in-flight by the client.
    inline constexpr const ReturnCode ProcessTerminateErrorCode = 0xF10BAD;

    //! Error code for processes that are forcefully terminated whilst in-flight by the scheduler due to timing out.
    inline constexpr const ReturnCode ProcessTimeoutErrorCode = 0xBADF10;

    //! Specifier for how the process's standard out willt be routed
    enum class StdOutputRouting
    {
        ToParent,
        None
    };

    enum class StdErrorRouting
    {
        ToParent,
        None
    };

    //! Container for process standard output and standard error.
    struct StdContent
    {
        AZStd::optional<AZStd::string> m_out;
        AZStd::optional<AZStd::string> m_err;
    };

    //! Information about a process the arguments used to launch it.
    class ProcessInfo
    {
    public:
        //! Provides the information required to launch a process.
        //! @param processId Client-supplied id to diffrentiate between processes.
        //! @param stdOut Routing of process standard output.
        //! @param stdErr Routing of process standard error.
        //! @param processPath Path to executable binary to launch.
        //! @param startupArgs Arguments to launch the process with.
        ProcessInfo(
            ProcessId processId,
            StdOutputRouting stdOut,
            StdErrorRouting stdErr,
            const AZ::IO::Path& processPath,
            const AZStd::string& startupArgs = "");
        ProcessInfo(ProcessId processId, const AZ::IO::Path& processPath, const AZStd::string& startupArgs = "");

        //! Returns the identifier of this process.
        ProcessId GetId() const;

        //! Returns whether or not stdoutput is routed to the parent process.
        bool ParentHasStdOutput() const;

        //! Returns whether or not stderror is routed to the parent process.
        bool ParentHasStdError() const;

        // Returns the path to the process binary.
        const AZ::IO::Path& GetProcessPath() const;

        //! Returns the command line arguments used to launch the process.
        const AZStd::string& GetStartupArgs() const;

    private:
        const ProcessId m_id;
        const bool m_parentHasStdOutput;
        const bool m_parentHasStdErr;
        const AZ::IO::Path m_processPath;
        const AZStd::string m_startupArgs;
    };
} // namespace TestImpact
