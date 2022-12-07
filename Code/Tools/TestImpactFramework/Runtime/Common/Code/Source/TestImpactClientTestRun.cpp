/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <AzCore/std/tuple.h>

namespace TestImpact
{
    namespace Client
    {
        TestRunBase::TestRunBase(
            const AZStd::string& testNamespace,
            const AZStd::string& name,
            const AZStd::string& commandString,
            const AZStd::string& stdOutput,
            const AZStd::string& stdError,
            AZStd::chrono::steady_clock::time_point startTime,
            AZStd::chrono::milliseconds duration,
            TestRunResult result)
            : m_targetName(name)
            , m_commandString(commandString)
            , m_stdOutput(stdOutput)
            , m_stdError(stdError)
            , m_startTime(startTime)
            , m_duration(duration)
            , m_result(result)
            , m_testNamespace(testNamespace)
        {
        }

        const AZStd::string& TestRunBase::GetTargetName() const
        {
            return m_targetName;
        }

        const AZStd::string& TestRunBase::GetCommandString() const
        {
            return m_commandString;
        }

        AZStd::chrono::steady_clock::time_point TestRunBase::GetStartTime() const
        {
            return m_startTime;
        }

        AZStd::chrono::steady_clock::time_point TestRunBase::GetEndTime() const
        {
            return m_startTime + m_duration;
        }

        AZStd::chrono::milliseconds TestRunBase::GetDuration() const
        {
            return m_duration;
        }

        const AZStd::string& TestRunBase::GetTestNamespace() const
        {
            return m_testNamespace;
        }

        TestRunResult TestRunBase::GetResult() const
        {
            return m_result;
        }

        const AZStd::string& TestRunBase::GetStdOutput() const
        {
            return m_stdOutput;
        }

        const AZStd::string& TestRunBase::GetStdError() const
        {
            return m_stdError;
        }

        TestRunWithExecutionFailure::TestRunWithExecutionFailure(TestRunBase&& testRun)
            : TestRunBase(AZStd::move(testRun))
        {
        }

        TimedOutTestRun::TimedOutTestRun(TestRunBase&& testRun)
            : TestRunBase(AZStd::move(testRun))
        {
        }

        UnexecutedTestRun::UnexecutedTestRun(TestRunBase&& testRun)
            : TestRunBase(AZStd::move(testRun))
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

        AZStd::tuple<size_t, size_t, size_t> CalculateTestCaseMetrics(const AZStd::vector<Test>& tests)
        {
            size_t totalNumPassingTests = 0;
            size_t totalNumFailingTests = 0;
            size_t totalNumDisabledTests = 0;

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
                else
                {
                    totalNumDisabledTests++;
                }
            }

            return { totalNumPassingTests, totalNumFailingTests, totalNumDisabledTests };
        }

        CompletedTestRun::CompletedTestRun(
            const AZStd::string& name,
            const AZStd::string& commandString,
            const AZStd::string& stdOutput,
            const AZStd::string& stdError,
            AZStd::chrono::steady_clock::time_point startTime,
            AZStd::chrono::milliseconds duration,
            TestRunResult result,
            AZStd::vector<Test>&& tests,
            const AZStd::string& testNamespace)
            : TestRunBase(testNamespace, name, commandString, stdOutput, stdError, startTime, duration, result)
            , m_tests(AZStd::move(tests))
        {
            AZStd::tie(m_totalNumPassingTests, m_totalNumFailingTests, m_totalNumDisabledTests) = CalculateTestCaseMetrics(m_tests);
        }

        CompletedTestRun::CompletedTestRun(TestRunBase&& testRun, AZStd::vector<Test>&& tests)
            : TestRunBase(AZStd::move(testRun))
            , m_tests(AZStd::move(tests))
        {
            AZStd::tie(m_totalNumPassingTests, m_totalNumFailingTests, m_totalNumDisabledTests) = CalculateTestCaseMetrics(m_tests);
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

        size_t CompletedTestRun::GetTotalNumDisabledTests() const
        {
            return m_totalNumDisabledTests;
        }

        const AZStd::vector<Test>& CompletedTestRun::GetTests() const
        {
            return m_tests;
        }

        PassingTestRun::PassingTestRun(TestRunBase&& testRun, AZStd::vector<Test>&& tests)
            : CompletedTestRun(AZStd::move(testRun), AZStd::move(tests))
        {
        }

        FailingTestRun::FailingTestRun(TestRunBase&& testRun, AZStd::vector<Test>&& tests)
            : CompletedTestRun(AZStd::move(testRun), AZStd::move(tests))
        {
        }
    } // namespace Client
} // namespace TestImpact
