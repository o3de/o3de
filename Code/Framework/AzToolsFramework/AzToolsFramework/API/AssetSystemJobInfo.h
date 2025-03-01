/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

//AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/PlatformDef.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        class AssetJobsInfoResponse;
        class AssetJobsInfoRequest;

        //! This enum have all the different job states
        //! Please note that these job status are written to the database, so it is very important that any new status should be added at the end else the database might get corrupted
        enum class JobStatus
        {
            Any = -1, //used exclusively by the database to indicate any state query, no job should ever actually be in this state, we start in Queued and progress from there
            Queued,  // its in the queue and will be built shortly
            InProgress,  // its being compiled right now.
            Failed,
            Failed_InvalidSourceNameExceedsMaxLimit, // use this enum to indicate that the job failed because the source file name length exceeds the maximum length allowed
            Completed, // built successfully (no failure occurred)
            Missing //indicate that the job is not present for example if the source file is not there, or if job key is not there
        };

        AZTF_API const char* JobStatusString(JobStatus status);

        //! This struct is used for responses and requests about Asset Processor Jobs
        struct AZTF_API JobInfo
        {
            AZ_TYPE_INFO(JobInfo, "{276C9DE3-0C81-4721-91FE-F7C961D28DA8}")
            JobInfo();

            AZ::u32 GetHash() const;

            static void Reflect(AZ::ReflectContext* context);

            //! the file from which this job was originally spawned.  Is just the relative source file name ("whatever/something.tif", not an absolute path)
            AZStd::string m_sourceFile;

            //! the watchfolder for the file from which this job was originally spawned.
            AZStd::string m_watchFolder;

            //! which platform this is for.  Will be something like "pc" or "android"
            AZStd::string m_platform;

            //! The uuid of the builder
            AZ::Uuid m_builderGuid = AZ::Uuid::CreateNull();

            //! Job Key is arbitrarily defined by the builder.  Used to differentiate between different jobs emitted for the same input file, for the same platform, for the same builder.
            //! for example, you might want to split a particularly complicated and time consuming job into multiple sub-jobs.  In which case they'd all have the same input file,
            //! the same platform, the same builder UUID (since its the UUID of the builder itself)
            //! but would have different job keys.
            AZStd::string m_jobKey;

            //random int made to identify this attempt to process this job
            AZ::u64 m_jobRunKey = 0;

            //current status
            JobStatus m_status = JobStatus::Queued;

            //logging
            AZ::s64 m_firstFailLogTime = 0;
            AZStd::string m_firstFailLogFile;
            AZ::s64 m_lastFailLogTime = 0;
            AZStd::string m_lastFailLogFile;
            AZ::s64 m_lastLogTime = 0;
            AZStd::string m_lastLogFile;
            AZ::s64 m_errorCount = 0;
            AZ::s64 m_warningCount = 0;

            AZ::s64 m_jobID = 0; // this is the actual database row.   Client is unlikely to need this.
        };

        using JobInfoContainer = ::AZStd::vector<JobInfo>;

        //! Returns "mac", "pc", or "linux" statically.
        AZTF_API const char* GetHostAssetPlatform();

    } // namespace AssetSystem
} // namespace AzToolsFramework

namespace AZStd
{
    extern template class vector<AzToolsFramework::AssetSystem::JobInfo>;
}
