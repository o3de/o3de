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

#include <TestEngine/JobRunner/TestImpactTestJobRunner.h>
#include <TestEngine/Run/TestImpactTestCoverage.h>
#include <TestEngine/Run/TestImpactTestRun.h>
#include <TestEngine/Run/TestImpactTestRunJobData.h>

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

    namespace Bitwise
    {
        //! Exception policy for test coverage artifacts.
        enum class CoverageExceptionPolicy
        {
            Never = 0, //! Never throw.
            OnEmptyCoverage = 1 //! Throw when no coverage data was produced.
        };
    } // namespace Bitwise

    //! Runs a batch of test targets to determine the test coverage and passes/failures.
    class InstrumentedTestRunner
        : public TestJobRunner<InstrumentedTestRunJobData, AZStd::pair<TestRun, TestCoverage>>
    {
        using JobRunner = TestJobRunner<InstrumentedTestRunJobData, AZStd::pair<TestRun, TestCoverage>>;

    public:
        using CoverageExceptionPolicy = Bitwise::CoverageExceptionPolicy;

        //! Constructs an instrumented test runner with the specified parameters common to all job runs of this runner.
        //! @param maxConcurrentRuns The maximum number of runs to be in flight at any given time.        
        InstrumentedTestRunner(size_t maxConcurrentRuns);
        
        //! Executes the specified instrumented test run jobs according to the specified job exception policies.
        //! @param jobInfos The test run jobs to execute.
        //! @param CoverageExceptionPolicy The coverage exception policy to be used for this run.
        //! @param jobExceptionPolicy The test run job exception policy to be used for this run (use
        //! TestJobExceptionPolicy::OnFailedToExecute to throw on test failures).
        //! @param runTimeout The maximum duration a run may be in-flight for before being forcefully terminated.
        //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight runs.
        //! @param clientCallback The optional client callback to be called whenever a run job changes state.
        //! @return The result of the run sequence and the instrumented run jobs with their associated test run and coverage payloads.
        AZStd::pair<ProcessSchedulerResult, AZStd::vector<Job>> RunInstrumentedTests(
            const AZStd::vector<JobInfo>& jobInfos,
            CoverageExceptionPolicy coverageExceptionPolicy,
            JobExceptionPolicy jobExceptionPolicy,
            AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
            AZStd::optional<ClientJobCallback> clientCallback);
    };
} // namespace TestImpact
