/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Process/TestImpactProcessException.h>
#include <Process/TestImpactProcessInfo.h>

namespace TestImpact
{
    ProcessInfo::ProcessInfo(ProcessId id, const RepoPath& processPath, const AZStd::string& startupArgs)
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
        const RepoPath& processPath,
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

    const RepoPath& ProcessInfo::GetProcessPath() const
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
