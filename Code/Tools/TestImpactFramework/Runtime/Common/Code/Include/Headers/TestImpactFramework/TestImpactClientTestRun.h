/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        //! Representation of a test run.
        class TestRunBase
        {
        public:
            //! Constructs the client facing representation of a given test target's run.
            //! @param name The name of the test target.
            //! @param commandString The command string used to execute this test target.
            //! @param startTime The start time, relative to the sequence start, that this run started.
            //! @param duration The duration that this test run took to complete.
            //! @param result The result of the run.
            TestRunBase(
                const AZStd::string& testNamespace,
                const AZStd::string& name,
                const AZStd::string& commandString,
                const AZStd::string& stdOutput,
                const AZStd::string& stdError,
                AZStd::chrono::steady_clock::time_point startTime,
                AZStd::chrono::milliseconds duration,
                TestRunResult result);

            virtual ~TestRunBase() = default;

            //! Returns the test target name.
            const AZStd::string& GetTargetName() const;

            //! Returns the test target namespace.
            const AZStd::string& GetTestNamespace() const;

            //! Returns the test run result.
            TestRunResult GetResult() const;

            //! Returns the standard output produced by this test run.
            const AZStd::string& GetStdOutput() const;

            //! Returns the standard error produced by this test run.
            const AZStd::string& GetStdError() const;

            //! Returns the test run start time.
            AZStd::chrono::steady_clock::time_point GetStartTime() const;

            //! Returns the end time, relative to the sequence start, that this run ended.
            AZStd::chrono::steady_clock::time_point GetEndTime() const;

            //! Returns the duration that this test run took to complete.
            AZStd::chrono::milliseconds GetDuration() const;

            //! Returns the command string used to execute this test target.
            const AZStd::string& GetCommandString() const;

        private:
            AZStd::string m_targetName;
            AZStd::string m_commandString;
            AZStd::string m_testNamespace;
            TestRunResult m_result;
            AZStd::string m_stdOutput;
            AZStd::string m_stdError;
            AZStd::chrono::steady_clock::time_point m_startTime;
            AZStd::chrono::milliseconds m_duration;
        };

        //! Representation of a test run that failed to execute.
        class TestRunWithExecutionFailure
            : public TestRunBase
        {
        public:
            using TestRunBase::TestRunBase;
            TestRunWithExecutionFailure(TestRunBase&& testRun);
        };

        //! Representation of a test run that was terminated in-flight due to timing out.
        class TimedOutTestRun
            : public TestRunBase
        {
        public:
            using TestRunBase::TestRunBase;
            TimedOutTestRun(TestRunBase&& testRun);
        };

        //! Representation of a test run that was not executed.
        class UnexecutedTestRun
            : public TestRunBase
        {
        public:
            using TestRunBase::TestRunBase;
            UnexecutedTestRun(TestRunBase&& testRun);
        };

        // Result of a test executed during a test run.
        enum class TestResult : AZ::u8
        {
            Passed,
            Failed,
            NotRun
        };

        //! Representation of a single test in a test target.
        class Test
        {
        public:
            //! Constructs the test with the specified name and result.
            Test(const AZStd::string& testName, TestResult result);

            //! Returns the name of this test.
            const AZStd::string& GetName() const;

            //! Returns the result of executing this test.
            TestResult GetResult() const;

        private:
            AZStd::string m_name;
            TestResult m_result;
        };

        //! Representation of a test run that completed with or without test failures.
        class CompletedTestRun
            : public TestRunBase
        {
        public:
            //! Constructs the test run from the specified test target executaion data.
            //! @param name The name of the test target for this run.
            //! @param commandString The command string used to execute the test target for this run.
            //! @param startTime The start time, offset from the sequence start time, that this test run started.
            //! @param duration The duration that this test run took to complete.
            //! @param result The result of this test run.
            //! @param tests The tests contained in the test target for this test run.
            CompletedTestRun(
                const AZStd::string& name,
                const AZStd::string& commandString,
                const AZStd::string& stdOutput,
                const AZStd::string& stdError,
                AZStd::chrono::steady_clock::time_point startTime,
                AZStd::chrono::milliseconds duration,
                TestRunResult result,
                AZStd::vector<Test>&& tests,
                const AZStd::string& testNamespace);

            //! Constructs the test run from the specified test target executaion data.
            CompletedTestRun(TestRunBase&& testRun, AZStd::vector<Test>&& tests);

            //! Returns the total number of tests in the run.
            size_t GetTotalNumTests() const;

            //! Returns the total number of passing tests in the run.
            size_t GetTotalNumPassingTests() const;

            //! Returns the total number of failing tests in the run.
            size_t GetTotalNumFailingTests() const;

            //! Returns the total number of disabled tests in the run.
            size_t GetTotalNumDisabledTests() const;

            //! Returns the tests in the run.
            const AZStd::vector<Test>& GetTests() const;

        private:
            AZStd::vector<Test> m_tests;
            size_t m_totalNumPassingTests = 0;
            size_t m_totalNumFailingTests = 0;
            size_t m_totalNumDisabledTests = 0;
        };

        //! Representation of a test run that completed with no test failures.
        class PassingTestRun
            : public CompletedTestRun
        {
        public:
            using CompletedTestRun::CompletedTestRun;
            PassingTestRun(TestRunBase&& testRun, AZStd::vector<Test>&& tests);
        };

        //! Representation of a test run that completed with one or more test failures.
        class FailingTestRun
            : public CompletedTestRun
        {
        public:
            using CompletedTestRun::CompletedTestRun;
            FailingTestRun(TestRunBase&& testRun, AZStd::vector<Test>&& tests);
        };
    } // namespace Client
} // namespace TestImpact
