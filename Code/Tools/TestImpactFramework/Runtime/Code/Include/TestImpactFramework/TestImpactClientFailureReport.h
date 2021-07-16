/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    namespace Client
    {
        //! Represents a test target that failed, either due to failing to execute, completing in an abnormal state or completing with failing tests.
        class TargetFailure
        {
        public:
            TargetFailure(const AZStd::string& targetName);

            //! Returns the name of the test target this failure pertains to.
            const AZStd::string& GetTargetName() const;
        private:
            AZStd::string m_targetName;
        };

        //! Represents a test target that failed to execute.
        class ExecutionFailure
            : public TargetFailure
        {
        public:
            ExecutionFailure(const AZStd::string& targetName, const AZStd::string& command);

            //! Returns the command string used to execute this test target.
            const AZStd::string& GetCommandString() const;
        private:
            AZStd::string m_commandString;
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

        //! Represents a test target that launched successfully but contains failing tests.
        class TestRunFailure
            : public TargetFailure
        {
        public:
            TestRunFailure(const AZStd::string& targetName, AZStd::vector<TestCaseFailure>&& testFailures);

            //! Returns the total number of failing tests in this run.
            size_t GetNumTestFailures() const;

            //! Returns the test cases in this run containing failing tests.
            const AZStd::vector<TestCaseFailure>& GetTestCaseFailures() const;

        private:
            AZStd::vector<TestCaseFailure> m_testCaseFailures;
            size_t m_numTestFailures = 0;
        };

        //! Base class for reporting failing test sequences.
        class SequenceFailure
        {
        public:
            SequenceFailure(
                AZStd::vector<ExecutionFailure>&& executionFailures,
                AZStd::vector<TestRunFailure>&& testRunFailures,
                AZStd::vector<TargetFailure>&& timedOutTests,
                AZStd::vector<TargetFailure>&& unexecutedTests);

            //! Returns the test targets in this sequence that failed to execute.
            const AZStd::vector<ExecutionFailure>& GetExecutionFailures() const;

            //! Returns the test targets that contain failing tests.
            const AZStd::vector<TestRunFailure>& GetTestRunFailures() const;

            //! Returns the test targets in this sequence that were terminated for exceeding their allotted runtime.
            const AZStd::vector<TargetFailure>& GetTimedOutTests() const;

            //! Returns the test targets in this sequence that were not executed due to the sequence terminating prematurely.
            const AZStd::vector<TargetFailure>& GetUnexecutedTests() const;

        private:
            AZStd::vector<ExecutionFailure> m_executionFailures;
            AZStd::vector<TestRunFailure> m_testRunFailures;
            AZStd::vector<TargetFailure> m_timedOutTests;
            AZStd::vector<TargetFailure> m_unexecutedTests;
        };
    } // namespace Client
} // namespace TestImpact
