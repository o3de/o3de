/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Job/TestImpactTestJobRunner.h>
#include <TestRunner/Common/Job/TestImpactTestRunJobData.h>

namespace TestImpact
{
    //! Runs a batch of tests to determine the test passes/failures.
    template<typename AdditionalInfo, typename Payload>
    class TestRunnerBase
        : public TestJobRunner<AdditionalInfo, Payload>
    {
    public:
        using TestJobRunner = TestJobRunner<AdditionalInfo, Payload>;
        using TestJobRunner::TestJobRunner;

        //! Executes the specified test run jobs according to the specified job exception policies.
        //! @param jobInfos The test run jobs to execute.
        //! @param stdOutRouting The standard output routing to be specified for all jobs.
        //! @param stdErrRouting The standard error routing to be specified for all jobs.
        //! @param jobExceptionPolicy The test run job exception policy to be used for this run (use
        //! TestJobExceptionPolicy::OnFailedToExecute to throw on test failures).
        //! @param runTimeout The maximum duration a run may be in-flight for before being forcefully terminated.
        //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight runs.
        //! @param clientCallback The optional client callback to be called whenever a run job changes state.
        //! @param stdContentCallback 
        //! @return The result of the run sequence and the run jobs with their associated test run payloads.
        AZStd::pair<ProcessSchedulerResult, AZStd::vector<TestJobRunner::Job>> RunTests(
            const AZStd::vector<TestJobRunner::JobInfo>& jobInfos,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
            AZStd::optional<TestJobRunner::JobCallback> clientCallback,
            AZStd::optional<TestJobRunner::StdContentCallback> stdContentCallback)
        {
            const auto payloadGenerator = [](const typename TestJobRunner::JobDataMap& jobDataMap)
            {
                typename TestJobRunner::PayloadMap runs;
                for (const auto& [jobId, jobData] : jobDataMap)
                {
                    const auto& [meta, jobInfo] = jobData;
                    if (meta.m_result == JobResult::ExecutedWithSuccess || meta.m_result == JobResult::ExecutedWithFailure)
                    {
                        if (auto outcome = PayloadFactory<AdditionalInfo, Payload>(*jobInfo, meta);
                            outcome.IsSuccess())
                        {
                            runs[jobId] = AZStd::move(outcome.TakeValue());
                        }
                        else
                        {
                            runs[jobId] = AZStd::nullopt;
                            AZ_Printf("RunTests", outcome.GetError().c_str());
                        }
                    }
                }

                return runs;
            };

            return this->m_jobRunner.Execute(
                jobInfos,
                payloadGenerator,
                stdOutRouting,
                stdErrRouting,
                runTimeout,
                runnerTimeout,
                clientCallback, stdContentCallback);
        }
    };
} // namespace TestImpact
