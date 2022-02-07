/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/Scheduler/TestImpactProcessScheduler.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Callback for job completion/failure.
    //! @param jobInfo The job information associated with this job.
    //! @param meta The meta-data about the job run.
    //! @param std The standard output and standard error of the process running the job.
    template<typename Job>
    using JobCallback = AZStd::function<ProcessCallbackResult(const typename Job::Info& jobInfo, const JobMeta& meta, StdContent&& std)>;

    //! The payloads produced by the job-specific payload producer in the form of a map associating each job id with the job's payload.
    template<typename Job>
    using PayloadMap = AZStd::unordered_map<typename Job::Info::IdType, AZStd::optional<typename Job::Payload>>;

    //! The map used by the client to associate the job information and meta-data with the job ids.
    template<typename Job>
    using JobDataMap = AZStd::unordered_map<typename Job::Info::IdType, AZStd::pair<JobMeta, const typename Job::Info*>>;

    //! The callback for producing the payloads for the jobs after all jobs have finished executing.
    //! @param jobInfos The information for each job run.
    //! @param jobDataMap The job data (in the form of job info and meta-data) for each job run.
    template<typename Job>
    using PayloadMapProducer = AZStd::function<PayloadMap<Job>(const JobDataMap<Job>& jobDataMap)>;

    //! Generic job runner that launches a process for each job, records metrics about each job run and hands the payload artifacts
    //! produced by each job to the client before compositing the metrics and payload artifacts for each job into a single interface
    //! to be consumed by the client.
    template<typename JobT>
    class JobRunner
    {
    public:
        //! Constructs the job runner with the specified parameters to constrain job runs.
        //! @param maxConcurrentProcesses he maximum number of concurrent jobs in-flight.
        explicit JobRunner(size_t maxConcurrentProcesses);
           
        //! Executes the specified jobs and returns the products of their labor.
        //! @param jobs The arguments (and other pertinent information) required for each job to be run.
        //! @param stdOutRouting The standard output routing to be specified for all jobs.
        //! @param stdErrRouting The standard error routing to be specified for all jobs.
        //! @param jobTimeout The maximum duration a job may be in-flight before being forcefully terminated (nullopt if no timeout).
        //! @param runnerTimeout The maximum duration the scheduler may run before forcefully terminating all in-flight jobs (nullopt if no timeout).
        //! @param payloadMapProducer The client callback to be called when all jobs have finished to transform the work produced by each job into the desired output.
        //! @param jobCallback The client callback to be called when each job changes state.
        //! @return The result of the run sequence and the jobs with their associated payloads.
        AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename JobT>> Execute(
            const AZStd::vector<typename JobT::Info>& jobs,
            PayloadMapProducer<JobT> payloadMapProducer,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            AZStd::optional<AZStd::chrono::milliseconds> jobTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
            JobCallback<typename JobT> jobCallback);

    private:
        ProcessScheduler m_processScheduler;
        StdOutputRouting m_stdOutRouting; //!< Standard output routing from each job process to job runner.
        StdErrorRouting m_stdErrRouting; //!< Standard error routing from each job process to job runner
        AZStd::optional<AZStd::chrono::milliseconds> m_jobTimeout; //!< Maximum time a job can run for before being forcefully terminated.
        AZStd::optional<AZStd::chrono::milliseconds> m_runnerTimeout; //!< Maximum time the job runner can run before forcefully terminating all in-flight jobs and shutting down.
    };

    template<typename JobT>
    JobRunner<JobT>::JobRunner(size_t maxConcurrentProcesses)
        : m_processScheduler(maxConcurrentProcesses)
    {
    }

    template<typename JobT>
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename JobT>> JobRunner<JobT>::Execute(
        const AZStd::vector<typename JobT::Info>& jobInfos,
        PayloadMapProducer<JobT> payloadMapProducer,
        StdOutputRouting stdOutRouting,
        StdErrorRouting stdErrRouting,
        AZStd::optional<AZStd::chrono::milliseconds> jobTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
        JobCallback<typename JobT> jobCallback)
    {
        AZStd::vector<ProcessInfo> processes;
        AZStd::unordered_map<JobT::Info::IdType, AZStd::pair<JobMeta, const typename JobT::Info*>> metas;
        AZStd::vector<JobT> jobs;
        jobs.reserve(jobInfos.size());
        processes.reserve(jobInfos.size());

        // Transform the job infos into the underlying process infos required for each job
        for (size_t jobIndex = 0; jobIndex < jobInfos.size(); jobIndex++)
        {
            const auto* jobInfo = &jobInfos[jobIndex];
            const auto jobId = jobInfo->GetId().m_value;
            metas.emplace(jobId, AZStd::pair<JobMeta, const typename JobT::Info*>{JobMeta{}, jobInfo});
            processes.emplace_back(jobId, stdOutRouting, stdErrRouting, jobInfo->GetCommand().m_args);
        }

        // Wrapper around low-level process launch callback to gather job meta-data and present a simplified callback interface to the client
        const ProcessLaunchCallback processLaunchCallback = [&jobCallback, &metas](
            TestImpact::ProcessId pid,
            TestImpact::LaunchResult launchResult,
            AZStd::chrono::high_resolution_clock::time_point createTime)
        {
            auto& [meta, jobInfo] = metas.at(pid);
            if (launchResult == LaunchResult::Failure)
            {
                meta.m_result = JobResult::FailedToExecute;
                return jobCallback(*jobInfo, meta, {});
            }
            else
            {
                meta.m_startTime = createTime;
                return ProcessCallbackResult::Continue;
            }
        };

        // Wrapper around low-level process exit callback to gather job meta-data and present a simplified callback interface to the client
        const ProcessExitCallback processExitCallback = [&jobCallback, &metas](
            TestImpact::ProcessId pid,
            TestImpact::ExitCondition exitCondition,
            TestImpact::ReturnCode returnCode,
            TestImpact::StdContent&& std,
            AZStd::chrono::high_resolution_clock::time_point exitTime)
        {
            auto& [meta, jobInfo] = metas.at(pid);
            meta.m_returnCode = returnCode;
            meta.m_duration = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(exitTime - *meta.m_startTime);
            if (exitCondition == ExitCondition::Gracefull && returnCode == 0)
            {
                meta.m_result = JobResult::ExecutedWithSuccess;
            }
            else if (exitCondition == ExitCondition::Terminated)
            {
                meta.m_result = JobResult::Terminated;
            }
            else if (exitCondition == ExitCondition::Timeout)
            {
                meta.m_result = JobResult::Timeout;
            }
            else
            {
                meta.m_result = JobResult::ExecutedWithFailure;
            }

            return jobCallback(*jobInfo, meta, AZStd::move(std));
        };

        // Schedule all jobs for execution
        const auto result = m_processScheduler.Execute(
            processes,
            jobTimeout,
            runnerTimeout,
            processLaunchCallback,
            processExitCallback);

        // Hand off the jobs to the client for payload generation
        auto payloadMap = payloadMapProducer(metas);

        // Unpack the payload map produced by the client into a vector of jobs containing the job data and payload for each job
        for (const auto& jobInfo : jobInfos)
        {
            const auto jobId = jobInfo.GetId().m_value;
            jobs.emplace_back(JobT(jobInfo, AZStd::move(metas.at(jobId).first), AZStd::move(payloadMap[jobId])));
        }

        return { result, jobs };
    }
} // namespace TestImpact
