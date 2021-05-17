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

#include <TestImpactFramework/TestImpactFailureReport.h>

namespace TestImpact
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

    LauncherFailure::LauncherFailure(const AZStd::string& targetName, const AZStd::string& command, int returnCode)
        : ExecutionFailure(targetName, command)
        , m_returnCode(returnCode)
    {
    }

    int LauncherFailure::GetReturnCode() const
    {
        return m_returnCode;
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
    }

    size_t TestRunFailure::GetNumTestFailures() const
    {
        // TODO
        return 1;
    }

    const AZStd::vector<TestCaseFailure>& TestRunFailure::GetTestCaseFailures() const
    {
        return m_testCaseFailures;
    }

    FailureReport::FailureReport(
        AZStd::vector<ExecutionFailure>&& executionFailures,
        AZStd::vector<LauncherFailure>&& launcherFailures,
        AZStd::vector<TestRunFailure>&& testRunFailures,
        AZStd::vector<TargetFailure>&& unexecutionTests)
        : m_executionFailures(AZStd::move(executionFailures))
        , m_launcherFailures(AZStd::move(launcherFailures))
        , m_testRunFailures(AZStd::move(testRunFailures))
        , m_unexecutionTests(AZStd::move(unexecutionTests))
    {
    }

    const AZStd::vector<ExecutionFailure>& FailureReport::GetExecutionFailures() const
    {
        return m_executionFailures;
    }

    const AZStd::vector<LauncherFailure>& FailureReport::GetLauncherFailures() const
    {
        return m_launcherFailures;
    }

    const AZStd::vector<TestRunFailure>& FailureReport::GetTestRunFailures() const
    {
        return m_testRunFailures;
    }

    const AZStd::vector<TargetFailure>& FailureReport::GetUnexecutedTest() const
    {
        return m_unexecutionTests;
    }

    ImpactAnalysisFailureReport::ImpactAnalysisFailureReport(
        AZStd::vector<ExecutionFailure>&& executionFailures,
        AZStd::vector<LauncherFailure>&& launcherFailures,
        AZStd::vector<TestRunFailure>&& selectedTestRunFailures,
        AZStd::vector<TestRunFailure>&& discardedTestRunFailures,
        AZStd::vector<TargetFailure>&& unexecutionTests)
        : m_executionFailures(AZStd::move(executionFailures))
        , m_launcherFailures(AZStd::move(launcherFailures))
        , m_selectedTestRunFailures(AZStd::move(selectedTestRunFailures))
        , m_discardedTestRunFailures(AZStd::move(discardedTestRunFailures))
        , m_unexecutionTests(AZStd::move(unexecutionTests))
    {
    }

    const AZStd::vector<ExecutionFailure> ImpactAnalysisFailureReport::GetExecutionFailures() const
    {
        return m_executionFailures;
    }

    const AZStd::vector<LauncherFailure> ImpactAnalysisFailureReport::GetLauncherFailures() const
    {
        return m_launcherFailures;
    }

    const AZStd::vector<TestRunFailure> ImpactAnalysisFailureReport::GetSelectedTestRunFailures() const
    {
        return m_selectedTestRunFailures;
    }

    const AZStd::vector<TestRunFailure> ImpactAnalysisFailureReport::GetDiscardedTestRunFailures() const
    {
        return m_discardedTestRunFailures;
    }

    const AZStd::vector<TargetFailure> ImpactAnalysisFailureReport::GetUnexecutedTest() const
    {
        return m_unexecutionTests;
    }
}
