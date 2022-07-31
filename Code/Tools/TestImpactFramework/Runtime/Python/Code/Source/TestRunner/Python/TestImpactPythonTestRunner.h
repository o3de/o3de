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
    class PythonTestRunner
        : public TestRunnerWithCoverage<TestRunWithCoverageJobData, TestCaseCoverage>
    {
    public:
        using TestRunnerWithCoverage = TestRunnerWithCoverage<TestRunWithCoverageJobData, TestCaseCoverage>;
        PythonTestRunner();

    protected:
        JobPayloadOutcome PayloadFactory(const JobInfo& jobData, const JobMeta& jobMeta) override;
    };
} // namespace TestImpact
