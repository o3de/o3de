/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/JobRunner/TestImpactTestJobRunner.h>
#include <TestEngine/Run/TestImpactTestRunJobData.h>
#include <AzCore/Outcome/Outcome.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestImpactFramework/TestImpactUtils.h>




#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <TestEngine/Run/TestImpactTestRun.h>

namespace TestImpact
{
    template<typename Payload>
    using PayloadOutcome = AZ::Outcome<Payload, AZStd::string>;

    template<typename AdditionalInfo, typename Payload>
    PayloadOutcome<Payload> PayloadFactory(const JobInfo<AdditionalInfo>& jobData, const JobMeta& jobMeta)
    {
        static_assert(false, "Please specify a factory function for the payload and job info type.");
    };

    //! Runs a batch of test targets to determine the test passes/failures.
    template<typename AdditionalInfo, typename Payload>
    class TestRunner
        : public TestJobRunner<AdditionalInfo, Payload>
    {
    protected:
        // using JobRunner = TestJobRunner<TestRunJobData, TestRun>;
        using JobRunner = TestJobRunner<AdditionalInfo, Payload>;
        using JobRunner::JobRunner;

    public:
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

    class RegularTestRunner
        : public TestRunner<TestRunJobData, TestRun>
    {
    public:
        using TestRunner<TestRunJobData, TestRun>::TestRunner;
    };

    template<>
    inline PayloadOutcome<TestRun> PayloadFactory(const JobInfo<TestRunJobData>& jobData, const JobMeta& jobMeta)
    {
        try
        {
            return AZ::Success(TestRun(
                GTest::TestRunSuitesFactory(ReadFileContents<TestEngineException>(jobData.GetRunArtifactPath())),
                jobMeta.m_duration.value()));
        }
        catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string::format("%s\n", e.what()));
        }
    };

} // namespace TestImpact
