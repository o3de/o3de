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

            //! Returns the test target name.
            const AZStd::string& GetTargetName() const;

            //! Returns the test run result.
            TestRunResult GetResult() const;

            //! Returns the test run start time.
            AZStd::chrono::high_resolution_clock::time_point GetStartTime() const;

            //! Returns the end time, relative to the sequence start, that this run ended.
            AZStd::chrono::high_resolution_clock::time_point GetEndTime() const;

            //! Returns the duration that this test run took to complete.
            AZStd::chrono::milliseconds GetDuration() const;

            //! Returns the command string used to execute this test target.
            const AZStd::string& GetCommandString() const;

        private:
            AZStd::string m_targetName;
            AZStd::string m_commandString;
            TestRunResult m_result;
            AZStd::chrono::high_resolution_clock::time_point m_startTime;
            AZStd::chrono::milliseconds m_duration;
        };

        //! Represents an individual test of a test target that failed.
        class TestFailure
        {
        public:
            TestFailure(const AZStd::string& testName, const AZStd::string& errorMessage);

            //! Returns the name of the test that failed.
            const AZStd::string& GetName() const;

            //! Returns the error message of the test that failed.
            const AZStd::string& GetErrorMessage() const;

        private:
            AZStd::string m_name;
            AZStd::string m_errorMessage;
        };

        //! Represents a collection of tests that failed.
        //! @note Only the failing tests are included in the collection.
        class TestCaseFailure
        {
        public:
            TestCaseFailure(const AZStd::string& testCaseName, AZStd::vector<TestFailure>&& testFailures);

            //! Returns the name of the test case containing the failing tests.
            const AZStd::string& GetName() const;

            //! Returns the collection of tests in this test case that failed.
            const AZStd::vector<TestFailure>& GetTestFailures() const;

        private:
            AZStd::string m_name;
            AZStd::vector<TestFailure> m_testFailures;
        };

        //! Representation of a test run's failing tests.
        class TestRunWithTestFailures
            : public TestRun
        {
        public:
            //! Constructs the client facing representation of a given test target's run.
            //! @param name The name of the test target.
            //! @param commandString The command string used to execute this test target.
            //! @param startTime The start time, relative to the sequence start, that this run started.
            //! @param duration The duration that this test run took to complete.
            //! @param result The result of the run.
            //! @param testFailures The failing tests for this test run.
            TestRunWithTestFailures(
                const AZStd::string& name,
                const AZStd::string& commandString,
                AZStd::chrono::high_resolution_clock::time_point startTime,
                AZStd::chrono::milliseconds duration,
                TestRunResult result,
                AZStd::vector<TestCaseFailure>&& testFailures);

            //! Constructs the client facing representation of a given test target's run.
            //! @param testRun The test run this run is to be derived from.
            //! @param testFailures The failing tests for this run.
            TestRunWithTestFailures(TestRun&& testRun, AZStd::vector<TestCaseFailure>&& testFailures);

            //! Returns the total number of failing tests in this run.
            size_t GetNumTestFailures() const;

            //! Returns the test cases in this run containing failing tests.
            const AZStd::vector<TestCaseFailure>& GetTestCaseFailures() const;

        private:
            AZStd::vector<TestCaseFailure> m_testCaseFailures;
            size_t m_numTestFailures = 0;
        };
    } // namespace Client
} // namespace TestImpact
