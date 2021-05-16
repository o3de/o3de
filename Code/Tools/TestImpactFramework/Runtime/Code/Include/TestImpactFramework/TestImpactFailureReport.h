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

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    class TargetFailure
    {
    public:
        TargetFailure(const AZStd::string& targetName);

        const AZStd::string& GetTargetName() const;
    };

    class ExecutionFailure
        : public TargetFailure
    {
    public:
        ExecutionFailure(const AZStd::string& targetName, const AZStd::string& command);

        const AZStd::string& GetCommandString() const;
    };

    class LauncherFailure
        : public ExecutionFailure
    {
    public:
        LauncherFailure(const AZStd::string& targetName, const AZStd::string& command, int returnCode);

        int GetReturnCode() const;
    };

    class TestFailure
    {
    public:
        TestFailure(const AZStd::string& testName, const AZStd::string& errorMessage);

        const AZStd::string& GetName() const;
        const AZStd::string& GetErrorMessage() const;
    };

    class TestCaseFailure
    {
    public:
        TestCaseFailure(const AZStd::string& testCaseName, AZStd::vector<TestFailure>&& testFailures);

        const AZStd::string& GetName() const;
        const AZStd::vector<TestFailure>& GetTestFailures() const;
    };

    class TestRunFailure
        : public TargetFailure
    {
    public:
        TestRunFailure(const AZStd::string& targetName, AZStd::vector<TestCaseFailure>&& testFailures);

        size_t GetNumTestFailures() const;
        const AZStd::vector<TestCaseFailure>& GetTestCaseFailures() const;
    };

    class FailureReport
    {
    public:
        FailureReport(
            AZStd::vector<ExecutionFailure>&& executionFailures,
            AZStd::vector<LauncherFailure>&& launcherFailures,
            AZStd::vector<TestRunFailure>&& testRunFailures,
            AZStd::vector<TargetFailure>&& unexecutionTests);

        const AZStd::vector<ExecutionFailure>& GetExecutionFailures() const;
        const AZStd::vector<LauncherFailure>& GetLauncherFailures() const;
        const AZStd::vector<TestRunFailure>& GetTestRunFailures() const;
        const AZStd::vector<TargetFailure>& GetUnexecutedTest() const;
    };

    class ImpactAnalysisFailureReport
        : public FailureReport
    {
    public:
        ImpactAnalysisFailureReport(
            AZStd::vector<ExecutionFailure>&& executionFailures,
            AZStd::vector<LauncherFailure>&& launcherFailures,
            AZStd::vector<TestRunFailure>&& selectedTestRunFailures,
            AZStd::vector<TestRunFailure>&& discardedTestRunFailures,
            AZStd::vector<TargetFailure>&& unexecutionTests);

        const AZStd::vector<TestRunFailure>& GetSelectedTestRunFailures() const;
        const AZStd::vector<TestRunFailure>& GetDiscardedTestRunFailures() const;
    };
}
