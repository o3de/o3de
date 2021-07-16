/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/JobRunner/TestImpactTestJobRunner.h>
#include <TestEngine/Run/TestImpactTestRun.h>
#include <TestEngine/Run/TestImpactTestRunJobData.h>

namespace TestImpact
{
    //! Runs a batch of test targets to determine the test passes/failures.
    class TestRunner
        : public TestJobRunner<TestRunJobData, TestRun>
    {
        using JobRunner = TestJobRunner<TestRunJobData, TestRun>;

    public:
        //! Constructs a test runner with the specified parameters common to all job runs of this runner.
        //! @param maxConcurrentRuns The maximum number of runs to be in flight at any given time.
        explicit TestRunner(size_t maxConcurrentRuns);

        //! Executes the specified test run jobs according to the specified job exception policies.
        //! @param jobInfos The test run jobs to execute.
        //! @param jobExceptionPolicy The test run job exception policy to be used for this run (use
        //! TestJobExceptionPolicy::OnFailedToExecute to throw on test failures).
        //! @param runTimeout The maximum duration a run may be in-flight for before being forcefully terminated.
        //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight runs.
        //! @param clientCallback The optional client callback to be called whenever a run job changes state.
        //! @return The result of the run sequence and the run jobs with their associated test run payloads.
        AZStd::pair<ProcessSchedulerResult, AZStd::vector<Job>> RunTests(
            const AZStd::vector<JobInfo>& jobInfos,
            AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
            AZStd::optional<ClientJobCallback> clientCallback);
    };
} // namespace TestImpact
