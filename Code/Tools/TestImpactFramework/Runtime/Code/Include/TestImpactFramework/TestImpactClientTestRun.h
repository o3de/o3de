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

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#pragma once

namespace TestImpact
{
    namespace Client
    {
        //! Result of a test run.
        enum class TestRunResult
        {
            NotRun, //!< The test run was not executed due to the test sequence terminating prematurely.
            FailedToExecute, //!< The test run failed to execute either due to the target binary missing or incorrect arguments.
            Timeout, //!< The test run timed out whilst in flight before being able to complete its run.
            TestFailures, //!< The test run completed its run but there were failing tests.
            AllTestsPass //!< The test run completed its run and all tests passed.
        };

        //! Representation of a completed test run.
        class TestRun
        {
        public:
            //! Constructs the client facing representation of a given test target's run.
            //! @param name The name of the test target.
            //! @param commandString The command string used to execute this test target.
            //! @param startTime The start time, relative to the sequence start, that this run started.
            //! @param duration The duration that this test run took to complete.
            //! @param result The result of the run.
            TestRun(
                const AZStd::string& name,
                const AZStd::string& commandString,
                AZStd::chrono::high_resolution_clock::time_point startTime,
                AZStd::chrono::milliseconds duration,
                TestRunResult result);

            virtual ~TestRun() = default;

            //! Returns the test target name.
            const AZStd::string& GetTargetName() const;
            TestRunResult GetResult() const;
            AZStd::chrono::milliseconds GetDuration() const;

            //! Returns the command string used to execute this test target.
            const AZStd::string& GetCommandString() const;

        private:
            AZStd::string m_targetName;
            AZStd::string m_commandString;
            TestRunResult m_result;
            AZStd::chrono::milliseconds m_duration;
        };

        class TestRunWithExecutonFailure
            : public TestRun
        {
        public:
            using TestRun::TestRun;
            TestRunWithExecutonFailure(TestRun&& testRun);
        };

        class TimedOutTestRun : public TestRun
        {
        public:
            using TestRun::TestRun;
            TimedOutTestRun(TestRun&& testRun);
        };

        class UnexecutedTestRun : public TestRun
        {
        public:
            using TestRun::TestRun;
            UnexecutedTestRun(TestRun&& testRun);
        };

        enum class TestCaseResult : AZ::u8
        {
            Passed,
            Failed,
            NotRun
        };

        class TestCase
        {
        public:
            TestCase(const AZStd::string& testName, TestCaseResult result);

            const AZStd::string& GetName() const;

            TestCaseResult GetResult() const;

        private:
            AZStd::string m_name;
            TestCaseResult m_result;
        };

        class TestSuite
        {
        public:
            TestSuite(const AZStd::string& testCaseName, AZStd::vector<TestCase>&& testCases);

            const AZStd::string& GetName() const;

            const AZStd::vector<TestCase>& GetTestCases() const;

            size_t GetNumPassingTests() const;

            size_t GetNumFailingTests() const;

        private:
            AZStd::string m_name;
            AZStd::vector<TestCase> m_testCases;
            size_t m_numPassingTests = 0;
            size_t m_numFailingTests = 0;
        };

        class CompletedTestRun
            : public TestRun
        {
        public:
            CompletedTestRun(
                const AZStd::string& name,
                const AZStd::string& commandString,
                AZStd::chrono::high_resolution_clock::time_point startTime,
                AZStd::chrono::milliseconds duration,
                TestRunResult result,
                AZStd::vector<TestSuite>&& testSuites);

            CompletedTestRun(TestRun&& testRun, AZStd::vector<TestSuite>&& testSuites);

            size_t GetTotalNumPassingTests() const;

            size_t GetTotalNumFailingTests() const;

            const AZStd::vector<TestSuite>& GetTestSuites() const;

        private:
            AZStd::vector<TestSuite> m_testSuites;
            size_t m_totalNumPassingTests = 0;
            size_t m_totalNumFailingTests = 0;
        };
    } // namespace Client
} // namespace TestImpact
