/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/JobRunner/TestImpactProcessJob.h>
#include <Process/JobRunner/TestImpactProcessJobRunner.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Base class for test related job runners.
    //! @tparam AdditionalInfo The data structure containing the information additional to the command arguments necessary to execute and
    //! complete a job.
    //! @tparam Payload The output produced by a job.
    template<typename AdditionalInfo, typename Payload>
    class TestJobRunner
    {
    public:
        using JobData = AdditionalInfo;
        using JobInfo = JobInfo<AdditionalInfo>;
        using Command = typename JobInfo::Command;
        using JobPayload = Payload;
        using Job = Job<JobInfo, Payload>;
        using ClientJobCallback = AZStd::function<ProcessCallbackResult(const JobInfo& jobInfo, const JobMeta& meta)>;
        using DerivedJobCallback = JobCallback<Job>;
        using JobDataMap = JobDataMap<Job>;

        //! Constructs the job runner with the specified parameters common to all job runs of this runner.
        //! @param maxConcurrentJobs The maximum number of jobs to be in flight at any given time.
        explicit TestJobRunner(size_t maxConcurrentJobs);

    protected:
        //! Runs the specified jobs and returns the completed payloads produced by each job.
        //! @param jobInfos The batch of jobs to execute.
        //! @param jobExceptionPolicy The job execution policy for this job run.
        //! @param payloadMapProducer The client callback for producing the payload map based on the completed job data.
        //! @param stdOutRouting The standard output routing from the underlying job processes to the derived runner.
        //! @param stdErrorRouting The standard error routing from the underlying job processes to the derived runner.
        //! @param jobTimeout The maximum duration a job may be in-flight for before being forcefully terminated (nullopt if no timeout).
        //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight jobs (nullopt if no timeout).
        //! @param clientCallback The optional callback function provided by the client to be called upon job state change.
        //! @param clientCallback The optional callback function provided by the derived job runner to be called upon job state change.
        //! @returns The result of the run sequence and the jobs that the sequence produced.
        AZStd::pair<ProcessSchedulerResult, AZStd::vector<Job>> ExecuteJobs(
            const AZStd::vector<JobInfo>& jobInfos,
            PayloadMapProducer<Job> payloadMapProducer,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            AZStd::optional<AZStd::chrono::milliseconds> jobTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
            AZStd::optional<ClientJobCallback> clientCallback,
            AZStd::optional<DerivedJobCallback> derivedJobCallback);

        const AZStd::optional<ClientJobCallback> m_clientJobCallback;

    private:
        JobRunner<Job> m_jobRunner;
        const AZStd::optional<DerivedJobCallback> m_derivedJobCallback;
    };

    template<typename Data, typename Payload>
    TestJobRunner<Data, Payload>::TestJobRunner(size_t maxConcurrentJobs)
        : m_jobRunner(maxConcurrentJobs)
    {
    }

    template<typename Data, typename Payload>
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename TestJobRunner<Data, Payload>::Job>> TestJobRunner<Data, Payload>::ExecuteJobs(
        const AZStd::vector<JobInfo>& jobInfos,
        PayloadMapProducer<Job> payloadMapProducer,
        StdOutputRouting stdOutRouting,
        StdErrorRouting stdErrRouting,
        AZStd::optional<AZStd::chrono::milliseconds> jobTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
        AZStd::optional<ClientJobCallback> clientCallback,
        AZStd::optional<DerivedJobCallback> derivedJobCallback)
    {
        // Callback to handle job exception policies and client/derived callbacks
        const auto jobCallback = [&clientCallback, &derivedJobCallback](const JobInfo& jobInfo, const JobMeta& meta, StdContent&& std)
        {
            auto callbackResult = ProcessCallbackResult::Continue;
            if (derivedJobCallback.has_value())
            {
                callbackResult = (*derivedJobCallback)(jobInfo, meta, AZStd::move(std));
            }

            if (clientCallback.has_value())
            {
                if (const auto result = (*clientCallback)(jobInfo, meta);
                    result == ProcessCallbackResult::Abort)
                {
                    callbackResult = ProcessCallbackResult::Abort;
                }
            }

            return callbackResult;
        };

        return m_jobRunner.Execute(jobInfos, payloadMapProducer, stdOutRouting, stdErrRouting, jobTimeout, runnerTimeout, jobCallback);
    }
} // namespace TestImpact
