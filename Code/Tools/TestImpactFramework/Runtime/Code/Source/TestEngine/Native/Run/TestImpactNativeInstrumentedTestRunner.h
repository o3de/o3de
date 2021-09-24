/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <Artifact/Factory/TestImpactModuleCoverageFactory.h>
#include <TestEngine/Common/Run/TestImpactInstrumentedTestRunner.h>
#include <TestEngine/Common/Job/TestImpactInstrumentedTestRunJobData.h>

namespace TestImpact
{
    struct NativeInstrumentedTestRunJobData
        : public InstrumentedTestRunJobData
    {
        using InstrumentedTestRunJobData::InstrumentedTestRunJobData;
    };

    class NativeInstrumentedTestRunner
        : public InstrumentedTestRunner<NativeInstrumentedTestRunJobData>
    {
    public:
        using TestImpact::InstrumentedTestRunner<NativeInstrumentedTestRunJobData>::InstrumentedTestRunner;
    };

    template<>
    inline PayloadOutcome<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>> PayloadFactory(
        const JobInfo<NativeInstrumentedTestRunJobData>& jobData, const JobMeta& jobMeta)
    {
        AZStd::optional<TestRun> run;
        try
        {
            run = TestRun(
                GTest::TestRunSuitesFactory(ReadFileContents<TestEngineException>(jobData.GetRunArtifactPath())),
                jobMeta.m_duration.value());
        }
        catch (const Exception& e)
        {
            // No run result is not necessarily a failure (e.g. test targets not using gtest)
            AZ_Printf("RunInstrumentedTests", AZStd::string::format("%s\n.", e.what()).c_str());
        }

        try
        {
            AZStd::vector<ModuleCoverage> moduleCoverages = Cobertura::ModuleCoveragesFactory(
                ReadFileContents<TestEngineException>(jobData.GetCoverageArtifactPath()));
            return AZ::Success(AZStd::pair<AZStd::optional<TestRun>, TestCoverage>{ run, TestCoverage(AZStd::move(moduleCoverages)) });
        }
        catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string(e.what()));
        }
    };

} // namespace TestImpact
