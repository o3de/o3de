/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Process/JobRunner/TestImpactProcessJobMeta.h>

namespace TestImpact
{
    JobMetaWrapper::JobMetaWrapper(const JobMeta& jobMeta)
        : m_meta(jobMeta)
    {
    }

    JobMetaWrapper::JobMetaWrapper(JobMeta&& jobMeta)
        : m_meta(AZStd::move(jobMeta))
    {
    }

    JobResult JobMetaWrapper::GetJobResult() const
    {
        return m_meta.m_result;
    }

    AZStd::optional<ReturnCode> JobMetaWrapper::GetReturnCode() const
    {
        return m_meta.m_returnCode;
    }

    AZStd::chrono::steady_clock::time_point JobMetaWrapper::GetStartTime() const
    {
        return m_meta.m_startTime.value_or(AZStd::chrono::steady_clock::time_point());
    }

    AZStd::chrono::steady_clock::time_point JobMetaWrapper::GetEndTime() const
    {
        if (m_meta.m_startTime.has_value() && m_meta.m_duration.has_value())
        {
            return m_meta.m_startTime.value() + m_meta.m_duration.value();
        }
        else
        {
            return AZStd::chrono::steady_clock::time_point();
        }
    }

    AZStd::chrono::milliseconds JobMetaWrapper::GetDuration() const
    {
        return m_meta.m_duration.value_or(AZStd::chrono::milliseconds{ 0 });
    }

} // namespace TestImpact
