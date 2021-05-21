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

#include <Process/JobRunner/TestImpactProcessJobMeta.h>

namespace TestImpact
{
    JobMetaContainer::JobMetaContainer(const JobMeta& jobMeta)
        : m_meta(jobMeta)
    {
    }

    JobMetaContainer::JobMetaContainer(JobMeta&& jobMeta)
        : m_meta(AZStd::move(jobMeta))
    {
    }

    JobResult JobMetaContainer::GetJobResult() const
    {
        return m_meta.m_result;
    }

    AZStd::optional<ReturnCode> JobMetaContainer::GetReturnCode() const
    {
        return m_meta.m_returnCode;
    }

    AZStd::chrono::high_resolution_clock::time_point JobMetaContainer::GetStartTime() const
    {
        return m_meta.m_startTime.value_or(AZStd::chrono::high_resolution_clock::time_point());
    }

    AZStd::chrono::high_resolution_clock::time_point JobMetaContainer::GetEndTime() const
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

    AZStd::chrono::milliseconds JobMetaContainer::GetDuration() const
    {
        return m_meta.m_duration.value_or(AZStd::chrono::milliseconds{ 0 });
    }

} // namespace TestImpact
