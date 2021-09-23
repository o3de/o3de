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

    //! Runs a batch of test targets to determine the test coverage and passes/failures.
    //class InstrumentedTestRunner
    //    : public TestJobRunner<InstrumentedTestRunJobData, AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>
    //{
    //    using JobRunner = TestJobRunner<InstrumentedTestRunJobData, AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>;
    //    using JobRunner::JobRunner;
    //
    //public:
    //    //! Executes the specified instrumented test run jobs according to the specified job exception policies.
    //    //! @param jobInfos The test run jobs to execute.
    //    //! @param jobExceptionPolicy The test run job exception policy to be used for this run (use
    //    //! TestJobExceptionPolicy::OnFailedToExecute to throw on test failures).
    //    //! @param runTimeout The maximum duration a run may be in-flight for before being forcefully terminated.
    //    //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight runs.
    //    //! @param clientCallback The optional client callback to be called whenever a run job changes state.
    //    //! @return The result of the run sequence and the instrumented run jobs with their associated test run and coverage payloads.
    //    AZStd::pair<ProcessSchedulerResult, AZStd::vector<Job>> RunInstrumentedTests(
    //        const AZStd::vector<JobInfo>& jobInfos,
    //        AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
    //        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
    //        AZStd::optional<ClientJobCallback> clientCallback);
    //};

    //template<typename TestCoverageType>
    //TestCoverageType TestCoverageFactory(const InstrumentedTestRunJobData& jobData)
    //{
    //    static_assert(false, "Please specify a factory function for the test coverage type.");
    //};

    //! Runs a batch of test targets to determine the test coverage and passes/failures.
    //template<typename TestRunType, typename TestCoverageType>
    //class InstrumentedTestRunner_
    //    : public TestJobRunner<InstrumentedTestRunJobData, AZStd::pair<AZStd::optional<TestRunType>, TestCoverageType>>
    //{
    //    using JobRunner = TestJobRunner<InstrumentedTestRunJobData, AZStd::pair<AZStd::optional<TestRunType>, TestCoverageType>>;
    //    using JobRunner::JobRunner;
    //
    //public:
    //    //! Executes the specified instrumented test run jobs according to the specified job exception policies.
    //    //! @param jobInfos The test run jobs to execute.
    //    //! @param jobExceptionPolicy The test run job exception policy to be used for this run (use
    //    //! TestJobExceptionPolicy::OnFailedToExecute to throw on test failures).
    //    //! @param runTimeout The maximum duration a run may be in-flight for before being forcefully terminated.
    //    //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight runs.
    //    //! @param clientCallback The optional client callback to be called whenever a run job changes state.
    //    //! @return The result of the run sequence and the instrumented run jobs with their associated test run and coverage payloads.
    //    AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename JobRunner::Job>> RunInstrumentedTests(
    //        const AZStd::vector<typename JobRunner::JobInfo>& jobInfos,
    //        AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
    //        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
    //        AZStd::optional<typename JobRunner::ClientJobCallback> clientCallback)
    //    {
    //        const auto payloadGenerator = [](const typename JobRunner::JobDataMap& jobDataMap)
    //        {
    //            PayloadMap<JobRunner::Job> runs;
    //            for (const auto& [jobId, jobData] : jobDataMap)
    //            {
    //                const auto& [meta, jobInfo] = jobData;
    //                if (meta.m_result == JobResult::ExecutedWithSuccess || meta.m_result == JobResult::ExecutedWithFailure)
    //                {
    //                    const auto printException = [](const Exception& e)
    //                    {
    //                        AZ_Printf("RunInstrumentedTests", AZStd::string::format("%s\n.", e.what()).c_str());
    //                    };
    //
    //                    AZStd::optional<TestRunType> run;
    //                    try
    //                    {
    //                        run = TestRunFactory<TestRunType>(*jobInfo, meta);
    //                    }
    //                    catch (const Exception& e)
    //                    {
    //                        // No run result is not necessarily a failure (e.g. test targets not using gtest)
    //                        printException(e);
    //                    }
    //
    //                    try
    //                    {
    //                        runs[jobId] = { run, TestCoverageFactory<TestCoverageType>(*jobInfo) };
    //                    } 
    //                    catch (const Exception& e)
    //                    {
    //                        printException(e);
    //                        // No coverage, however, is a failure
    //                        runs[jobId] = AZStd::nullopt;
    //                    }
    //                }
    //            }
    //
    //            return runs;
    //        };
    //
    //        return JobRunner::ExecuteJobs(
    //            jobInfos, payloadGenerator, StdOutputRouting::None, StdErrorRouting::None, runTimeout, runnerTimeout, clientCallback,
    //            AZStd::nullopt);
    //    }
    //};

    //template<>
    //inline TestCoverage TestCoverageFactory(const InstrumentedTestRunJobData& jobData)
    //{
    //     AZStd::vector<ModuleCoverage> moduleCoverages = Cobertura::ModuleCoveragesFactory(
    //        ReadFileContents<TestEngineException>(jobData.GetCoverageArtifactPath()));
    //     return TestCoverage(AZStd::move(moduleCoverages));
    //};

    class InstrumentedTestRunner
        : public TestRunner_<InstrumentedTestRunJobData, AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>
    {
    public:
        using TestRunner_<InstrumentedTestRunJobData, AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>::TestRunner_;
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
