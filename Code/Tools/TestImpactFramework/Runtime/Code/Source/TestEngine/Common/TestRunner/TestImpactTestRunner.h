/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/JobRunner/TestImpactTestJobRunner.h>
#include <TestEngine/Native/Job/TestImpactnativeRegularTestRunJobData.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestImpactFramework/TestImpactUtils.h>

namespace TestImpact
{
    //! Runs a batch of tests to determine the test passes/failures.
    template<typename AdditionalInfo, typename Payload>
    class TestRunner
        : public TestJobRunner<AdditionalInfo, Payload>
    {
    public:
        using JobRunner = TestJobRunner<AdditionalInfo, Payload>;
        using JobRunner::JobRunner;

        //! Executes the specified test run jobs according to the specified job exception policies.
        //! @param jobInfos The test run jobs to execute.
        //! @param jobExceptionPolicy The test run job exception policy to be used for this run (use
        //! TestJobExceptionPolicy::OnFailedToExecute to throw on test failures).
        //! @param runTimeout The maximum duration a run may be in-flight for before being forcefully terminated.
        //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight runs.
        //! @param clientCallback The optional client callback to be called whenever a run job changes state.
        //! @return The result of the run sequence and the run jobs with their associated test run payloads.
        AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename JobRunner::Job>> RunTests(
            const AZStd::vector<typename JobRunner::JobInfo>& jobInfos,
            AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
            AZStd::optional<typename JobRunner::ClientJobCallback> clientCallback)
        {
            const auto payloadGenerator = [](const typename JobRunner::JobDataMap& jobDataMap)
            {
                PayloadMap<JobRunner::Job> runs;
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

            return JobRunner::ExecuteJobs(
                jobInfos, payloadGenerator, StdOutputRouting::None, StdErrorRouting::None, runTimeout, runnerTimeout, clientCallback,
                AZStd::nullopt);
        }
    };
} // namespace TestImpact
