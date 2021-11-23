/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/TestImpactTestEngineJob.h>

namespace TestImpact
{
    TestEngineJob::TestEngineJob(
        const TestTarget* testTarget,
        const AZStd::string& commandString,
        const JobMeta& jobMeta,
        Client::TestRunResult testResult,
        AZStd::string&& stdOut,
        AZStd::string&& stdErr)
        : JobMetaWrapper(jobMeta)
        , m_testTarget(testTarget)
        , m_commandString(commandString)
        , m_testResult(testResult)
        , m_stdOut(AZStd::move(stdOut))
        , m_stdErr(AZStd::move(stdErr))
    {
    }

    const TestTarget* TestEngineJob::GetTestTarget() const
    {
        return m_testTarget;
    }

    const AZStd::string& TestEngineJob::GetCommandString() const
    {
        return m_commandString;
    }

    Client::TestRunResult TestEngineJob::GetTestResult() const
    {
        return m_testResult;
    }

    const AZStd::string& TestEngineJob::GetStdOutput() const
    {
        return m_stdOut;
    }

    const AZStd::string& TestEngineJob::GetStdError() const
    {
        return m_stdErr;
    }
} // namespace TestImpact
