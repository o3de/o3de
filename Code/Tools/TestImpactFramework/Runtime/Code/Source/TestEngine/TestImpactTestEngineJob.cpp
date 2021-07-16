/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/TestImpactTestTarget.h>
#include <TestEngine/TestImpactTestEngineJob.h>

namespace TestImpact
{
    TestEngineJob::TestEngineJob(const TestTarget* testTarget, const AZStd::string& commandString, const JobMeta& jobMeta, Client::TestRunResult testResult)
        : JobMetaWrapper(jobMeta)
        , m_testTarget(testTarget)
        , m_commandString(commandString)
        , m_testResult(testResult)
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
} // namespace TestImpact
