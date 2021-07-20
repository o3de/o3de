/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactClientFailureReport.h>

namespace TestImpact
{
    namespace Client
    {
        TargetFailure::TargetFailure(const AZStd::string& targetName)
            : m_targetName(targetName)
        {
        }

        const AZStd::string& TargetFailure::GetTargetName() const
        {
            return m_targetName;
        }

        ExecutionFailure::ExecutionFailure(const AZStd::string& targetName, const AZStd::string& command)
            : TargetFailure(targetName)
            , m_commandString(command)
        {
        }

        const AZStd::string& ExecutionFailure::GetCommandString() const
        {
            return m_commandString;
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

        TestRunFailure::TestRunFailure(const AZStd::string& targetName, AZStd::vector<TestCaseFailure>&& testFailures)
            : TargetFailure(targetName)
            , m_testCaseFailures(AZStd::move(testFailures))
        {
            for (const auto& testCase : m_testCaseFailures)
            {
                m_numTestFailures += testCase.GetTestFailures().size();
            }
        }

        size_t TestRunFailure::GetNumTestFailures() const
        {
            return m_numTestFailures;
        }

        const AZStd::vector<TestCaseFailure>& TestRunFailure::GetTestCaseFailures() const
        {
            return m_testCaseFailures;
        }

        SequenceFailure::SequenceFailure(
            AZStd::vector<ExecutionFailure>&& executionFailures,
            AZStd::vector<TestRunFailure>&& testRunFailures,
            AZStd::vector<TargetFailure>&& timedOutTests,
            AZStd::vector<TargetFailure>&& unexecutionTests)
            : m_executionFailures(AZStd::move(executionFailures))
            , m_testRunFailures(testRunFailures)
            , m_timedOutTests(AZStd::move(timedOutTests))
            , m_unexecutedTests(AZStd::move(unexecutionTests))
        {
        }

        const AZStd::vector<ExecutionFailure>& SequenceFailure::GetExecutionFailures() const
        {
            return m_executionFailures;
        }

        const AZStd::vector<TestRunFailure>& SequenceFailure::GetTestRunFailures() const
        {
            return m_testRunFailures;
        }

        const AZStd::vector<TargetFailure>& SequenceFailure::GetTimedOutTests() const
        {
            return m_timedOutTests;
        }

        const AZStd::vector<TargetFailure>& SequenceFailure::GetUnexecutedTests() const
        {
            return m_unexecutedTests;
        }
    } // namespace Client
} // namespace TestImpact
