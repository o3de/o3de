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

#include <Test/Job/TestImpactTestJobRunner.h>
#include <Test/Run/TestImpactTestRun.h>
#include <Test/Run/TestImpactTestRunJobData.h>

namespace TestImpact
{
    //! Runs a batch of test targets to determine the test passes/failures.
    class TestRunner
        : public TestJobRunner<TestRunJobData, TestRun>
    {
        using JobRunner = TestJobRunner<TestRunJobData, TestRun>;

    public:
        //! Constructs a test runner with the specified parameters common to all job runs of this runner.
        //! @param clientCallback The optional client callback to be called whenever a run job changes state.
        //! @param maxConcurrentRuns The maximum number of runs to be in flight at any given time.
        //! @param runTimeout The maximum duration a run may be in-flight for before being forcefully terminated.
        //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight runs.
        TestRunner(
            AZStd::optional<ClientJobCallback> clientCallback,
            size_t maxConcurrentRuns,
            AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout);

        //! Executes the specified test run jobs according to the specified job exception policies.
        //! @param jobInfos The test run jobs to execute.
        //! @param jobExceptionPolicy The test run job exception policy to be used for this run (use
        //! TestJobExceptionPolicy::OnFailedToExecute to throw on test failures).
        //! @return the test run jobs with their associated test run payloads.
        AZStd::vector<Job> RunTests(const AZStd::vector<JobInfo>& jobInfos, JobExceptionPolicy jobExceptionPolicy);
    };
} // namespace TestImpact
