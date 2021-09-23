/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/JobRunner/TestImpactTestJobRunner.h>
#include <TestEngine/Run/TestImpactTestCoverage.h>
#include <TestEngine/Run/TestImpactTestRun.h>
#include <TestEngine/Run/TestImpactTestRunJobData.h>







#include <Artifact/Factory/TestImpactModuleCoverageFactory.h>
#include <TestEngine/Run/TestImpactTestRunner.h>

namespace TestImpact
{
    //! Per-job data for instrumented test runs.
    class InstrumentedTestRunJobData
        : public TestRunJobData
    {
    public:
        InstrumentedTestRunJobData(const RepoPath& resultsArtifact, const RepoPath& coverageArtifact);

        //! Returns the path to the coverage artifact produced by the test target.
        const RepoPath& GetCoverageArtifactPath() const;

    private:
        RepoPath m_coverageArtifact; //!< Path to coverage data.
    };

    class InstrumentedTestRunner
        : public TestRunner<InstrumentedTestRunJobData, AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>
    {
    public:
        using TestRunner<InstrumentedTestRunJobData, AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>::TestRunner;
    };

    template<>
    inline PayloadOutcome<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>> PayloadFactory(
        const JobInfo<InstrumentedTestRunJobData>& jobData, const JobMeta& jobMeta)
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
