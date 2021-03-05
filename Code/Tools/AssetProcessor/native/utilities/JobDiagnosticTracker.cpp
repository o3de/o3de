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

#include "JobDiagnosticTracker.h"

namespace AssetProcessor
{
    bool JobDiagnosticInfo::operator==(const JobDiagnosticInfo& rhs) const
    {
        return m_errorCount == rhs.m_errorCount
            && m_warningCount == rhs.m_warningCount;
    }

    bool JobDiagnosticInfo::operator!=(const JobDiagnosticInfo& rhs) const
    {
        return !operator==(rhs);
    }

    //////////////////////////////////////////////////////////////////////////

    JobDiagnosticTracker::JobDiagnosticTracker()
    {
        BusConnect();
    }

    JobDiagnosticTracker::~JobDiagnosticTracker()
    {
        BusDisconnect();
    }

    JobDiagnosticInfo AssetProcessor::JobDiagnosticTracker::GetDiagnosticInfo(AZ::u64 jobRunKey) const
    {
        auto jobIter = m_jobInfo.find(jobRunKey);

        if(jobIter != m_jobInfo.end())
        {
            return jobIter->second;
        }

        return {};
    }

    void JobDiagnosticTracker::RecordDiagnosticInfo(AZ::u64 jobRunKey, JobDiagnosticInfo info)
    {
        if (info != JobDiagnosticInfo{})
        {
            // Only store non-empty entries
            m_jobInfo[jobRunKey] = info;
        }
    }

    WarningLevel JobDiagnosticTracker::GetWarningLevel() const
    {
        return m_warningLevel;
    }

    void JobDiagnosticTracker::SetWarningLevel(WarningLevel level)
    {
        m_warningLevel = level;
    }
}
