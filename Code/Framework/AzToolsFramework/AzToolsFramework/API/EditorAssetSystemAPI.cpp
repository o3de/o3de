/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZStd
{
    template class vector<AzToolsFramework::AssetSystem::JobInfo>;
}

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        const char* JobStatusString(JobStatus status)
        {
            switch(status)
            {
                case JobStatus::Any: return "Any";
                case JobStatus::Queued: return "Queued";
                case JobStatus::InProgress: return "InProgress";
                case JobStatus::Failed: return "Failed";
                case JobStatus::Failed_InvalidSourceNameExceedsMaxLimit: return "Failed_InvalidSourceNameExceedsMaxLimit";
                case JobStatus::Completed: return "Completed";
                case JobStatus::Missing: return "Missing";
            }
            return "";
        }

        JobInfo::JobInfo()
        {
            m_jobRunKey = rand();
        }

        AZ::u32 JobInfo::GetHash() const
        {
            AZ::Crc32 crc(m_watchFolder.c_str());
            crc.Add(m_sourceFile.c_str());
            crc.Add(m_platform.c_str());
            crc.Add(m_jobKey.c_str());
            crc.Add(m_builderGuid.ToString<AZStd::string>().c_str());
            return crc;
        }

        void JobInfo::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<JobInfo>()
                    ->Version(4)
                    ->Field("sourceFile", &JobInfo::m_sourceFile)
                    ->Field("platform", &JobInfo::m_platform)
                    ->Field("builderUuid", &JobInfo::m_builderGuid)
                    ->Field("jobKey", &JobInfo::m_jobKey)
                    ->Field("jobRunKey", &JobInfo::m_jobRunKey)
                    ->Field("status", &JobInfo::m_status)
                    ->Field("firstFailLogTime", &JobInfo::m_firstFailLogTime)
                    ->Field("firstFailLogFile", &JobInfo::m_firstFailLogFile)
                    ->Field("lastFailLogTime", &JobInfo::m_lastFailLogTime)
                    ->Field("lastFailLogFile", &JobInfo::m_lastFailLogFile)
                    ->Field("lastLogTime", &JobInfo::m_lastLogTime)
                    ->Field("lastLogFile", &JobInfo::m_lastLogFile)
                    ->Field("jobID", &JobInfo::m_jobID)
                    ->Field("watchFolder", &JobInfo::m_watchFolder)
                    ->Field("errorCount", &JobInfo::m_errorCount)
                    ->Field("warningCount", &JobInfo::m_warningCount)
                    ;
            }
        }
    }
}
