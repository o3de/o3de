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

#include <TestImpactBitwise.h>

#include <Process/JobRunner/TestImpactProcessJob.h>
#include <Process/JobRunner/TestImpactProcessJobRunner.h>
#include <TestEngine/JobRunner/TestImpactTestJobException.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    namespace Bitwise
    {
        //! Exception policy for test jobs (and derived jobs).
        enum class TestJobExceptionPolicy
        {
            Never = 0, //!< Never throw.
            OnFailedToExecute = 1, //!< Throw when a job fails to execute.
            OnExecutedWithFailure = 1 << 1 //!< Throw when a job returns with an error code.
        };
    } // namespace Bitwise

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
        using ClientJobCallback = AZStd::function<void(const JobInfo& jobInfo, const JobMeta& meta)>;
        using DerivedJobCallback = JobCallback<Job>;
        using JobExceptionPolicy = Bitwise::TestJobExceptionPolicy;
        using JobDataMap = JobDataMap<Job>;

        //! Constructs the job runner with the specified parameters common to all job runs of this runner.
        //! @param clientCallback The optional callback function provided by the client to be called upon job state change.
        //! @param clientCallback The optional callback function provided by the derived job runner to be called upon job state change.
        //! @param stdOutRouting The standard output routing from the underlying job processes to the derived runner.
        //! @param stdErrorRouting The standard error routing from the underlying job processes to the derived runner.
        //! @param maxConcurrentJobs The maximum number of jobs to be in flight at any given time.
        //! @param jobTimeout The maximum duration a job may be in-flight for before being forcefully terminated (nullopt if no timeout).
        //! @param runnerTimeout The maximum duration the runner may run before forcefully terminating all in-flight jobs (nullopt if no
        //! timeout).
        TestJobRunner(
            AZStd::optional<ClientJobCallback> clientCallback,
            AZStd::optional<DerivedJobCallback> derivedJobCallback,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            size_t maxConcurrentJobs,
            AZStd::optional<AZStd::chrono::milliseconds> jobTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout);

    protected:
        //! Runs the specified jobs and returns the completed payloads produced by each job.
        //! @param jobInfos The batch of jobs to execute.
        //! @param payloadMapProducer The client callback for producing the payload map based on the completed job data.
        //! @param jobExceptionPolicy The job execution policy for this job run.
        AZStd::vector<Job> ExecuteJobs(
            const AZStd::vector<JobInfo>& jobInfos,
            PayloadMapProducer<Job> payloadMapProducer,
            JobExceptionPolicy jobExceptionPolicy);

        const AZStd::optional<ClientJobCallback> m_clientJobCallback;

    private:
        JobRunner<Job> m_jobRunner;
        const AZStd::optional<DerivedJobCallback> m_derivedJobCallback;
    };

    template<typename Data, typename Payload>
    TestJobRunner<Data, Payload>::TestJobRunner(
        AZStd::optional<ClientJobCallback> clientCallback,
        AZStd::optional<DerivedJobCallback> derivedJobCallback,
        StdOutputRouting stdOutRouting,
        StdErrorRouting stdErrRouting,
        size_t maxConcurrentJobs,
        AZStd::optional<AZStd::chrono::milliseconds> jobTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout)
        : m_jobRunner(stdOutRouting, stdErrRouting, maxConcurrentJobs, jobTimeout, runnerTimeout)
        , m_clientJobCallback(clientCallback)
        , m_derivedJobCallback(derivedJobCallback)
    {
    }

    template<typename Data, typename Payload>
    AZStd::vector<typename TestJobRunner<Data, Payload>::Job> TestJobRunner<Data, Payload>::ExecuteJobs(
        const AZStd::vector<JobInfo>& jobInfos,
        PayloadMapProducer<Job> payloadMapProducer,
        JobExceptionPolicy jobExceptionPolicy)
    {
        // Callback to handle job exception policies and client/derived callbacks
        const auto jobCallback = [this, &jobExceptionPolicy](const JobInfo& jobInfo, const JobMeta& meta, StdContent&& std)
        {
            if (meta.m_result == JobResult::FailedToExecute && IsFlagSet(jobExceptionPolicy, JobExceptionPolicy::OnFailedToExecute))
            {
                throw TestJobException("Job failed to execute");
            }
            else if (meta.m_result == JobResult::ExecutedWithFailure && IsFlagSet(jobExceptionPolicy, JobExceptionPolicy::OnExecutedWithFailure))
            {
                throw TestJobException("Job executed with failure");
            }

            if (m_derivedJobCallback.has_value())
            {
                if (const auto result = (*m_derivedJobCallback)(jobInfo, meta, AZStd::move(std)); result != ProcessCallbackResult::Continue)
                {
                    return result;
                }
            }

            if (m_clientJobCallback.has_value())
            {
                (*m_clientJobCallback)(jobInfo, meta);
            }

            return ProcessCallbackResult::Continue;
        };

        return m_jobRunner.Execute(jobInfos, jobCallback, payloadMapProducer);
    }
} // namespace TestImpact
