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

#include <Target/TestImpactTestTarget.h>
#include <TestEngine/TestImpactTestEngineJob.h>

namespace TestImpact
{
    TestEngineJob::TestEngineJob(const TestTarget* testTarget, const AZStd::string& commandString, const JobMeta& jobMeta, Client::TestRunResult testResult)
        : m_testTarget(testTarget)
        , m_commandString(commandString)
        , JobMetaWrapper(jobMeta)
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
