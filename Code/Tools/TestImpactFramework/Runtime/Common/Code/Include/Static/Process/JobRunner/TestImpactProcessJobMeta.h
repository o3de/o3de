/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/TestImpactProcessInfo.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/optional.h>

namespace TestImpact
{
    //! Result of a job that was run.
    enum class JobResult : AZ::u8
    {
        NotExecuted, //!< The job was not executed (e.g. the job runner terminated before the job could be executed).
        FailedToExecute, //!< The job failed to execute (e.g. due to the arguments used to execute the job being invalid).
        Timeout, //!< The job was terminated by the job runner (e.g. job timeout exceeded while job was in-flight).
        Terminated, //!< The job was terminated by the job runner (e.g. global timeout exceeded while job was in-flight).
        ExecutedWithFailure, //!< The job was executed but exited in an erroneous state (the underlying process returned non-zero).
        ExecutedWithSuccess //!< The job was executed and exited in a successful state (the underlying processes returned zero).
    };

    //! The meta-data for a given job.
    struct JobMeta
    {
        JobResult m_result = JobResult::NotExecuted;
        AZStd::optional<AZStd::chrono::steady_clock::time_point> m_startTime; //!< The time, relative to the job runner start, that this job started.
        AZStd::optional<AZStd::chrono::milliseconds> m_duration; //!< The duration that this job took to complete.
        AZStd::optional<ReturnCode> m_returnCode; //!< The return code of the underlying processes of this job.
    };

    //! Wrapper for job meta structure to inheritance/aggregation without being coupled to the JobInfo or Job classes.
    class JobMetaWrapper
    {
    public:
        JobMetaWrapper(const JobMeta& jobMeta);
        JobMetaWrapper(JobMeta&& jobMeta);

        //! Returns the result of this job.
        JobResult GetJobResult() const;

        //! Returns the start time, relative to the job runner start, that this job started.
        AZStd::chrono::steady_clock::time_point GetStartTime() const;

        //! Returns the end time, relative to the job runner start, that this job ended.
        AZStd::chrono::steady_clock::time_point GetEndTime() const;

        //! Returns the duration that this job took to complete.
        AZStd::chrono::milliseconds GetDuration() const;

        //! Returns the return code of the underlying processes of this job.
        AZStd::optional<ReturnCode> GetReturnCode() const;

    private:
        JobMeta m_meta;
    };
} // namespace TestImpact

