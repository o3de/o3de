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

#include <Process/JobRunner/TestImpactProcessJobInfo.h>
#include <Process/TestImpactProcessInfo.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/optional.h>

namespace TestImpact
{
    //! Result of a job that was run.
    enum class JobResult
    {
        NotExecuted, //!< The job was not executed (e.g. the job runner terminated before the job could be executed).
        FailedToExecute, //!< The job failed to execute (e.g. due to the arguments used to execute the job being invalid).
        Terminated, //!< The job was terminated by the job runner (e.g. job or runner timeout exceeded while job was in-flight).
        ExecutedWithFailure, //!< The job was executed but exited in an erroneous state (the underlying process returned non-zero).
        ExecutedWithSuccess //!< The job was executed and exited in a successful state (the underlying processes returned zero).
    };

    //! The meta-data for a given job.
    struct JobMeta
    {
        JobResult m_result = JobResult::NotExecuted;
        AZStd::optional<AZStd::chrono::high_resolution_clock::time_point>
            m_startTime; //!< The time, relative to the job runner start, that this job started.
        AZStd::optional<AZStd::chrono::milliseconds> m_duration; //!< The duration that this job took to complete.
        AZStd::optional<ReturnCode> m_returnCode; //!< The return code of the underlying processes of this job.
    };

    //! Representation of a unit of work to be performed by a process.
    //! @tparam JobInfoT The JobInfo structure containing the information required to run this job.
    //! @tparam JobPayloadT The resulting output of the processed artifact produced by this job.
    template<typename JobInfoT, typename JobPayloadT>
    class Job
    {
    public:
        using Info = JobInfoT;
        using Payload = JobPayloadT;

        //! Constructor with r-values for the specific use case of the job runner.
        Job(Info jobInfo, JobMeta&& jobMeta, AZStd::optional<Payload>&& payload);

        //! Returns the job info associated with this job.
        const Info& GetJobInfo() const;

        //! Returns the result of this job.
        JobResult GetResult() const;

        //! Returns the start time, relative to the job runner start, that this job started.
        AZStd::chrono::high_resolution_clock::time_point GetStartTime() const;

        //! Returns the end time, relative to the job runner start, that this job ended.
        AZStd::chrono::high_resolution_clock::time_point GetEndTime() const;

        //! Returns the duration that this job took to complete.
        AZStd::chrono::milliseconds GetDuration() const;

        //! Returns the return code of the underlying processes of this job.
        AZStd::optional<ReturnCode> GetReturnCode() const;

        //! Returns the payload produced by this job.
        const AZStd::optional<Payload>& GetPayload() const;

    private:
        Info m_jobInfo;
        JobMeta m_meta;
        AZStd::optional<Payload> m_payload;
    };

    template<typename JobInfoT, typename JobPayloadT>
    Job<JobInfoT, JobPayloadT>::Job(Info jobInfo, JobMeta&& jobMeta, AZStd::optional<Payload>&& payload)
        : m_jobInfo(jobInfo)
        , m_meta(AZStd::move(jobMeta))
        , m_payload(AZStd::move(payload))
    {
    }

    template<typename JobInfoT, typename JobPayloadT>
    const JobInfoT& Job<JobInfoT, JobPayloadT>::GetJobInfo() const
    {
        return m_jobInfo;
    }

    template<typename JobInfoT, typename JobPayloadT>
    JobResult Job<JobInfoT, JobPayloadT>::GetResult() const
    {
        return m_meta.m_result;
    }

    template<typename JobInfoT, typename JobPayloadT>
    AZStd::optional<ReturnCode> Job<JobInfoT, JobPayloadT>::GetReturnCode() const
    {
        return m_meta.m_returnCode;
    }

    template<typename JobInfoT, typename JobPayloadT>
    AZStd::chrono::high_resolution_clock::time_point Job<JobInfoT, JobPayloadT>::GetStartTime() const
    {
        return m_meta.m_startTime.value_or(AZStd::chrono::high_resolution_clock::time_point());
    }

    template<typename JobInfoT, typename JobPayloadT>
    AZStd::chrono::high_resolution_clock::time_point Job<JobInfoT, JobPayloadT>::GetEndTime() const
    {
        if (m_meta.m_startTime.has_value() && m_meta.m_duration.has_value())
        {
            return m_meta.m_startTime.value() + m_meta.m_duration.value();
        }
        else
        {
            return AZStd::chrono::high_resolution_clock::time_point();
        }
    }

    template<typename JobInfoT, typename JobPayloadT>
    AZStd::chrono::milliseconds Job<JobInfoT, JobPayloadT>::GetDuration() const
    {
        return m_meta.m_duration.value_or(AZStd::chrono::milliseconds{0});
    }

    template<typename JobInfoT, typename JobPayloadT>
    const AZStd::optional<JobPayloadT>& Job<JobInfoT, JobPayloadT>::GetPayload() const
    {
        return m_payload;
    }
} // namespace TestImpact
