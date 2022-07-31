/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <Artifact/Factory/TestImpactModuleCoverageFactory.h>
#include <TestRunner/Common/TestImpactTestRunnerException.h>
#include <TestRunner/Common/Job/TestImpactTestRunWithCoverageJobData.h>
#include <TestRunner/Common/Run/TestImpactTestCoverage.h>
#include <TestRunner/Common/TestImpactTestRunnerWithCoverage.h>

namespace TestImpact
{
    class PythonNullTestRunner
        : public TestRunnerWithCoverage<TestRunWithCoverageJobData, TestCaseCoverage>
    {
    public:
        using TestRunnerWithCoverage = TestRunnerWithCoverage<TestRunWithCoverageJobData, TestCaseCoverage>;

        PythonNullTestRunner();

        AZStd::pair<ProcessSchedulerResult, AZStd::vector<TestJobRunner::Job>> RunTests(
            const AZStd::vector<TestJobRunner::JobInfo>& jobInfos,
            [[maybe_unused]] StdOutputRouting stdOutRouting,
            [[maybe_unused]] StdErrorRouting stdErrRouting,
            [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
            AZStd::optional<TestJobRunner::JobCallback> clientCallback,
            [[maybe_unused]] AZStd::optional<TestJobRunner::StdContentCallback> stdContentCallback);

    protected:
        JobPayloadOutcome PayloadFactory(const JobInfo& jobData, const JobMeta& jobMeta) override;
    };
} // namespace TestImpact
