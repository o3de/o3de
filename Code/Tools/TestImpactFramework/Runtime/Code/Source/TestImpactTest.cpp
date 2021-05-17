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

#include <TestImpactFramework/TestImpactTest.h>
namespace TestImpact
{
    Test::Test(const AZStd::string& name, TestResult testResult, AZStd::chrono::milliseconds duration)
        : m_targetName(name)
        , m_testResult(testResult)
        , m_duration(duration)
    {
    }

    const AZStd::string& Test::GetTargetName() const
    {
        return m_targetName;
    }

    AZStd::chrono::milliseconds Test::GetDuration() const
    {
        return m_duration;
    }

    TestResult Test::GetTestResult() const
    {
        return m_testResult;
    }
}
