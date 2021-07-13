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

#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <AzCore/std/containers/array.h>

namespace TestImpact
{
    namespace Client
    {
        TestRun::TestRun(
            const AZStd::string& name,
            const AZStd::string& commandString,
            AZStd::chrono::high_resolution_clock::time_point startTime,
            AZStd::chrono::milliseconds duration,
            TestRunResult result)
            : m_targetName(name)
            , m_commandString(commandString)
            , m_startTime(startTime)
            , m_duration(duration)
            , m_result(result)
            , m_duration(duration)
        {
        }

        const AZStd::string& TestRun::GetTargetName() const
        {
            return m_targetName;
        }

        const AZStd::string& TestRun::GetCommandString() const
        {
            return m_commandString;
        }

        AZStd::chrono::high_resolution_clock::time_point TestRun::GetStartTime() const
        {
            return m_startTime;
        }

        AZStd::chrono::high_resolution_clock::time_point TestRun::GetEndTime() const
        {
            return m_startTime + m_duration;
        }

        AZStd::chrono::milliseconds TestRun::GetDuration() const
        {
            return m_duration;
        }

        TestRunResult TestRun::GetResult() const
        {
            return m_result;
        }

        TestFailure::TestFailure(const AZStd::string& testName, const AZStd::string& errorMessage)
            : m_name(testName)
            , m_errorMessage(errorMessage)
        {
        }

        const AZStd::string& TestFailure::GetName() const
        {
            return m_name;
        }

        const AZStd::string& TestFailure::GetErrorMessage() const
        {
            return m_errorMessage;
        }

        TestCaseFailure::TestCaseFailure(const AZStd::string& testCaseName, AZStd::vector<TestFailure>&& testFailures)
            : m_name(testCaseName)
            , m_testFailures(AZStd::move(testFailures))
        {
        }

        const AZStd::string& TestCaseFailure::GetName() const
        {
            return m_name;
        }

        const AZStd::vector<TestFailure>& TestCaseFailure::GetTestFailures() const
        {
            return m_testFailures;
        }

        static size_t CalculateNumTestRunFailures(const AZStd::vector<TestCaseFailure>& testFailures)
        {
            size_t numTestFailures = 0;
            for (const auto& testCase : testFailures)
            {
                numTestFailures += testCase.GetTestFailures().size();
            }

            return numTestFailures;
        }

        TestRunWithTestFailures::TestRunWithTestFailures(
            const AZStd::string& name,
            const AZStd::string& commandString,
            AZStd::chrono::high_resolution_clock::time_point startTime,
            AZStd::chrono::milliseconds duration,
            TestRunResult result,
            AZStd::vector<TestCaseFailure>&& testFailures)
            : TestRun(name, commandString, startTime, duration, result)
            , m_testCaseFailures(AZStd::move(testFailures))
        {
            m_numTestFailures = CalculateNumTestRunFailures(m_testCaseFailures);
        }

        TestRunWithTestFailures::TestRunWithTestFailures(TestRun&& testRun, AZStd::vector<TestCaseFailure>&& testFailures)
            : TestRun(AZStd::move(testRun))
            , m_testCaseFailures(AZStd::move(testFailures))
        {
            m_numTestFailures = CalculateNumTestRunFailures(m_testCaseFailures);
        }

        size_t TestRunWithTestFailures::GetNumTestFailures() const
        {
            return m_numTestFailures;
        }

        const AZStd::vector<TestCaseFailure>& TestRunWithTestFailures::GetTestCaseFailures() const
        {
            return m_testCaseFailures;
        }
    } // namespace Client
} // namespace TestImpact
