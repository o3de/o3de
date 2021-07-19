/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/JobRunner/TestImpactProcessJobInfo.h>
#include <Process/JobRunner/TestImpactProcessJobMeta.h>

namespace TestImpact
{
    //! Representation of a unit of work to be performed by a process.
    //! @tparam JobInfoT The JobInfo structure containing the information required to run this job.
    //! @tparam JobPayloadT The resulting output of the processed artifact produced by this job.
    template<typename JobInfoT, typename JobPayloadT>
    class Job
        : public JobMetaWrapper
    {
    public:
        using Info = JobInfoT;
        using Payload = JobPayloadT;

        //! Constructor with r-values for the specific use case of the job runner.
        Job(const Info& jobInfo, JobMeta&& jobMeta, AZStd::optional<Payload>&& payload);

        //! Returns the job info associated with this job.
        const Info& GetJobInfo() const;

        //! Returns the payload produced by this job.
        const AZStd::optional<Payload>& GetPayload() const;

        //! Facilitates the client consuming the payload.
        //! @note It is valid for a job life cycle to continue after having released its payload.
        AZStd::optional<Payload> ReleasePayload();

    private:
        Info m_jobInfo;
        AZStd::optional<Payload> m_payload;
    };

    template<typename JobInfoT, typename JobPayloadT>
    Job<JobInfoT, JobPayloadT>::Job(const Info& jobInfo, JobMeta&& jobMeta, AZStd::optional<Payload>&& payload)
        : JobMetaWrapper(AZStd::move(jobMeta))
        , m_jobInfo(jobInfo)
        , m_payload(AZStd::move(payload))
    {
    }

    template<typename JobInfoT, typename JobPayloadT>
    const JobInfoT& Job<JobInfoT, JobPayloadT>::GetJobInfo() const
    {
        return m_jobInfo;
    }

    template<typename JobInfoT, typename JobPayloadT>
    const AZStd::optional<JobPayloadT>& Job<JobInfoT, JobPayloadT>::GetPayload() const
    {
        return m_payload;
    }

    template<typename JobInfoT, typename JobPayloadT>
    AZStd::optional<JobPayloadT> Job<JobInfoT, JobPayloadT>::ReleasePayload()
    {
        return AZStd::exchange(m_payload, AZStd::nullopt);
    }
} // namespace TestImpact
