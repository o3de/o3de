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

        Test::Test(const AZStd::string& testName, TestResult result)
            : m_name(testName)
            , m_result(result)
        {
        }

        const AZStd::string& Test::GetName() const
        {
            return m_name;
        }

        TestResult Test::GetResult() const
        {
            return m_result;
        }

        AZStd::tuple<size_t, size_t> CalculateNumPassingAndFailingTestCases(const AZStd::vector<Test>& tests)
        {
            size_t totalNumPassingTests = 0;
            size_t totalNumFailingTests = 0;

            for (const auto& test : tests)
            {
                if (test.GetResult() == Client::TestResult::Passed)
                {
                    totalNumPassingTests++;
                }
                else if (test.GetResult() == Client::TestResult::Failed)
                {
                    totalNumFailingTests++;
                }
            }

            return { totalNumPassingTests, totalNumFailingTests };
        }

        CompletedTestRun::CompletedTestRun(
            const AZStd::string& name,
            const AZStd::string& commandString,
            AZStd::chrono::high_resolution_clock::time_point startTime,
            AZStd::chrono::milliseconds duration,
            TestRunResult result,
            AZStd::vector<Test>&& tests)
            : TestRun(name, commandString, startTime, duration, result)
            , m_tests(AZStd::move(tests))
        {
            AZStd::tie(m_totalNumPassingTests, m_totalNumFailingTests) = CalculateNumPassingAndFailingTestCases(m_tests);
        }

        CompletedTestRun::CompletedTestRun(TestRun&& testRun, AZStd::vector<Test>&& tests)
            : TestRun(AZStd::move(testRun))
            , m_tests(AZStd::move(tests))
        {
            AZStd::tie(m_totalNumPassingTests, m_totalNumFailingTests) = CalculateNumPassingAndFailingTestCases(m_tests);
        }

        size_t CompletedTestRun::GetTotalNumTests() const
        {
            return m_tests.size();
        }

        size_t CompletedTestRun::GetTotalNumPassingTests() const
        {
            return m_totalNumPassingTests;
        }

        size_t CompletedTestRun::GetTotalNumFailingTests() const
        {
            return m_totalNumFailingTests;
        }

        const AZStd::vector<Test>& CompletedTestRun::GetTests() const
        {
            return m_tests;
        }
    } // namespace Client
} // namespace TestImpact
