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

#include <AzCore/std/tuple.h>

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

        TestRunWithExecutonFailure::TestRunWithExecutonFailure(TestRun&& testRun)
            : TestRun(AZStd::move(testRun))
        {
        }

        TimedOutTestRun::TimedOutTestRun(TestRun&& testRun)
            : TestRun(AZStd::move(testRun))
        {
        }

        UnexecutedTestRun::UnexecutedTestRun(TestRun&& testRun)
            : TestRun(AZStd::move(testRun))
        {
        }

        TestCase::TestCase(const AZStd::string& testName, TestCaseResult result)
            : m_name(testName)
            , m_result(result)
        {
        }

        const AZStd::string& TestCase::GetName() const
        {
            return m_name;
        }

        TestCaseResult TestCase::GetResult() const
        {
            return m_result;
        }

        TestSuite::TestSuite(const AZStd::string& testCaseName, AZStd::vector<TestCase>&& testCases)
            : m_name(testCaseName)
            , m_testCases(AZStd::move(testCases))
        {
            for (const auto& testCase : m_testCases)
            {
                if (auto result = testCase.GetResult();
                    result == TestCaseResult::Passed)
                {
                    m_numPassingTests++;
                }
                else if (result == TestCaseResult::Failed)
                {
                    m_numFailingTests++;
                }
            }
        }

        const AZStd::string& TestSuite::GetName() const
        {
            return m_name;
        }

        const AZStd::vector<TestCase>& TestSuite::GetTestCases() const
        {
            return m_testCases;
        }

        size_t TestSuite::GetNumPassingTests() const
        {
            return m_numPassingTests;
        }

        size_t TestSuite::GetNumFailingTests() const
        {
            return m_numFailingTests;
        }

        AZStd::tuple<size_t, size_t> CalculateNumPassingAndFailingTestCases(const AZStd::vector<TestSuite>& testSuites)
        {
            size_t totalNumPassingTests = 0;
            size_t totalNumFailingTests = 0;

            for (const auto& testSuite : testSuites)
            {
                totalNumPassingTests += testSuite.GetNumPassingTests();
                totalNumPassingTests += testSuite.GetNumFailingTests();
            }

            return { totalNumPassingTests, totalNumFailingTests };
        }

        CompletedTestRun::CompletedTestRun(
            const AZStd::string& name,
            const AZStd::string& commandString,
            AZStd::chrono::high_resolution_clock::time_point startTime,
            AZStd::chrono::milliseconds duration,
            TestRunResult result,
            AZStd::vector<TestSuite>&& testSuites)
            : TestRun(name, commandString, startTime, duration, result)
            , m_testSuites(AZStd::move(testSuites))
        {
            AZStd::tie(m_totalNumPassingTests, m_totalNumFailingTests) = CalculateNumPassingAndFailingTestCases(m_testSuites);
        }

        CompletedTestRun::CompletedTestRun(TestRun&& testRun, AZStd::vector<TestSuite>&& testSuites)
            : TestRun(AZStd::move(testRun))
            , m_testSuites(AZStd::move(testSuites))
        {
            AZStd::tie(m_totalNumPassingTests, m_totalNumFailingTests) = CalculateNumPassingAndFailingTestCases(m_testSuites);
        }

        size_t CompletedTestRun::GetTotalNumPassingTests() const
        {
            return m_totalNumPassingTests;
        }

        size_t CompletedTestRun::GetTotalNumFailingTests() const
        {
            return m_totalNumFailingTests;
        }

        const AZStd::vector<TestSuite>& CompletedTestRun::GetTestSuites() const
        {
            return m_testSuites;
        }
    } // namespace Client
} // namespace TestImpact
