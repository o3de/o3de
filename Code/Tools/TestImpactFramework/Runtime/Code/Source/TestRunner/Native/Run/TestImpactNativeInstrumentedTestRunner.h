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
#include <TestRunner/Common/Run/TestImpactTestRunnerWithCoverage.h>
#include <TestRunner/Common/Run/TestImpactTestCoverage.h>
#include <TestRunner/Common/Job/TestImpactTestRunWithCoverageJobData.h>
#include <TestRunner/Native/Job/TestImpactNativeTestRunJobData.h>

namespace TestImpact
{
    class NativeInstrumentedTestRunner
        : public TestRunnerWithCoverage<NativeTestRunJobData<TestRunWithCoverageJobData>, TestCoverage>
    {
    public:
        using TestImpact::TestRunnerWithCoverage<NativeTestRunJobData<TestRunWithCoverageJobData>, TestCoverage>::TestRunnerWithCoverage;
    };

    template<>
    inline NativeInstrumentedTestRunner::JobPayloadOutcome PayloadFactory(
        const NativeInstrumentedTestRunner::JobInfo& jobData, const JobMeta& jobMeta)
    {
        AZStd::optional<TestRun> run;
        try
        {
            run = TestRun(
                GTest::TestRunSuitesFactory(ReadFileContents<TestRunnerException>(jobData.GetRunArtifactPath())),
                jobMeta.m_duration.value());
        }
        catch (const Exception& e)
        {
            // No run result is not necessarily a failure (e.g. test targets not using gtest)
            AZ_Printf("NativeInstrumentedTestRunJobData", AZStd::string::format("%s\n.", e.what()).c_str());
        }

        try
        {
            AZStd::vector<ModuleCoverage> moduleCoverages = Cobertura::ModuleCoveragesFactory(
                ReadFileContents<TestRunnerException>(jobData.GetCoverageArtifactPath()));
            return AZ::Success(NativeInstrumentedTestRunner::JobPayload{ run, TestCoverage(AZStd::move(moduleCoverages)) });
        }
        catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string(e.what()));
        }
    };

} // namespace TestImpact
