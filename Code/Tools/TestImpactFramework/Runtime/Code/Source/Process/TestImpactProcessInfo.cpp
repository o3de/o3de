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

#include <Process/TestImpactProcessException.h>
#include <Process/TestImpactProcessInfo.h>

namespace TestImpact
{
    ProcessInfo::ProcessInfo(ProcessId id, const AZ::IO::Path& processPath, const AZStd::string& startupArgs)
        : m_id(id)
        , m_parentHasStdOutput(false)
        , m_parentHasStdErr(false)
        , m_processPath(processPath)
        , m_startupArgs(startupArgs)
    {
        AZ_TestImpact_Eval(processPath.String().length() > 0, ProcessException, "Process path cannot be empty");
    }

    ProcessInfo::ProcessInfo(
        ProcessId id,
        StdOutputRouting stdOut,
        StdErrorRouting stdErr,
        const AZ::IO::Path& processPath,
        const AZStd::string& startupArgs)
        : m_id(id)
        , m_processPath(processPath)
        , m_startupArgs(startupArgs)
        , m_parentHasStdOutput(stdOut == StdOutputRouting::ToParent ? true : false)
        , m_parentHasStdErr(stdErr == StdErrorRouting::ToParent ? true : false)
    {
        AZ_TestImpact_Eval(processPath.String().length() > 0, ProcessException, "Process path cannot be empty");
    }

    ProcessId ProcessInfo::GetId() const
    {
        return m_id;
    }

    const AZ::IO::Path& ProcessInfo::GetProcessPath() const
    {
        return m_processPath;
    }

    const AZStd::string& ProcessInfo::GetStartupArgs() const
    {
        return m_startupArgs;
    }

    bool ProcessInfo::ParentHasStdOutput() const
    {
        return m_parentHasStdOutput;
    }

    bool ProcessInfo::ParentHasStdError() const
    {
        return m_parentHasStdErr;
    }
} // namespace TestImpact
