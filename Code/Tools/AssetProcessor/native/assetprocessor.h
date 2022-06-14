/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QPair>
#include <QMetaType>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Crc.h>
#include <QString>
#include <QList>
#include <QSet>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzCore/Math/Crc.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "AssetProcessor_Traits_Platform.h"

namespace AssetProcessor
{
    const char* const DebugChannel = "Debug"; //Use this channel name if you want to write the message to the log file only.
    const char* const ConsoleChannel = "AssetProcessor";// Use this channel name if you want to write the message to both the console and the log file.
    const char* const FENCE_FILE_EXTENSION = "fence"; //fence file extension
    const char* const AutoFailReasonKey = "failreason"; // the key to look in for auto-fail reason.
    const char* const AutoFailLogFile = "faillogfile"; // if this is provided, this is a complete log of the failure and will be added after the failreason.
    const char* const AutoFailOmitFromDatabaseKey = "failreason_omitFromDatabase"; // if set in your job info hash, your job will not be tracked by the database.
    const char* const PlaceHolderFileName = "$missing_dependency$"; // Used as a placeholder in the dependency system, such as when a source file is deleted and a previously met dependency is broken.
    const unsigned int g_RetriesForFenceFile = 5; // number of retries for fencing
    constexpr int RetriesForJobLostConnection = ASSETPROCESSOR_TRAIT_ASSET_BUILDER_LOST_CONNECTION_RETRIES; // number of times to retry a job when a network error due to network issues or a crashed AssetBuilder process is determined to have caused a job failure
    // Even though AP can handle files with path length greater than window's legacy path length limit, we have some 3rdparty sdk's
    // which do not handle this case ,therefore we will make AP fail any jobs whose either source file or output file name exceeds the windows legacy path length limit
#define AP_MAX_PATH_LEN 260

    //! a shared convenience typedef for requests that have come over the network
    //! The first element is the connection id it came from and the second element is the serial number
    //! which can be used to send a response.
    typedef QPair<quint32, quint32> NetworkRequestID;

    //! a shared convenience typedef for Escalating Jobs
    //! The first element is the jobRunKey of the job and the second element is the escalation
    typedef QList<QPair<AZ::s64, int> > JobIdEscalationList;

    //! A map which is used to keep absolute paths --> Database Paths of source files.
    //! This is intentionally a map (not unordered_map) in order to ensure order is stable, and to eliminate duplicates.
    typedef AZStd::map<AZStd::string, AZStd::string> SourceFilesForFingerprintingContainer;

    enum AssetScanningStatus
    {
        Unknown,
        Started,
        InProgress,
        Completed,
        Stopped
    };

    //! This enum stores all the different job escalation values
    enum JobEscalation
    {
        ProcessAssetRequestSyncEscalation = 200,
        ProcessAssetRequestStatusEscalation = 150,
        AssetJobRequestEscalation = 100,
        Default = 0
    };
    //! This enum stores all the different asset processor status values
    enum AssetProcessorStatus
    {
        Initializing_Gems,
        Initializing_Builders,
        Scanning_Started,
        Analyzing_Jobs,
        Processing_Jobs,
    };

    enum AssetCatalogStatus
    {
        RequiresSaving,
        UpToDate
    };

    //! AssetProcessorStatusEntry stores all the necessary information related to AssetProcessorStatus
    struct AssetProcessorStatusEntry
    {
        AssetProcessorStatus m_status;
        unsigned int m_count = 0;
        QString m_extraInfo; //this can be used to send any other info like name etc

        explicit AssetProcessorStatusEntry(AssetProcessorStatus status, unsigned int count = 0, QString extraInfo = QString())
            : m_status(status)
            , m_count(count)
            , m_extraInfo(extraInfo)
        {
        }

        AssetProcessorStatusEntry() = default;
    };

    struct AssetRecognizer;

    //! JobEntry is an internal structure that is used to uniquely identify a specific job and keeps track of it as it flows through the AP system
    //! It prevents us from having to copy the entire of JobDetails, which is a very heavy structure.
    //! In general, communication ABOUT jobs will have the JobEntry as the key
    class JobEntry
    {
    public:
        // note that QStrings are ref-counted copy-on-write, so a move operation will not be beneficial unless this struct gains considerable heap allocated fields.

        QString m_databaseSourceName;                           //! DATABASE "SourceName" Column, which includes the 'output prefix' if present, used for keying
        QString m_watchFolderPath;                              //! contains the absolute path to the watch folder that the file was found in.
        QString m_pathRelativeToWatchFolder;                    //! contains the relative path (from the above watch folder) that the file was found in.
        AZ::Uuid m_builderGuid = AZ::Uuid::CreateNull();        //! the builder that will perform the job
        AssetBuilderSDK::PlatformInfo m_platformInfo;
        AZ::Uuid m_sourceFileUUID = AZ::Uuid::CreateNull(); ///< The actual UUID of the source being processed
        QString m_jobKey;     // JobKey is used when a single input file, for a single platform, for a single builder outputs many separate jobs
        AZ::u32 m_computedFingerprint = 0;     // what the fingerprint was at the time of job creation.
        qint64 m_computedFingerprintTimeStamp = 0; // stores the number of milliseconds since the universal coordinated time when the fingerprint was computed.
        AZ::u64 m_jobRunKey = 0;
        bool m_checkExclusiveLock = false;      ///< indicates whether we need to check the input file for exclusive lock before we process this job
        bool m_addToDatabase = true; ///< If false, this is just a UI job, and should not affect the database.

        QString GetAbsoluteSourcePath() const
        {
            if (!m_watchFolderPath.isEmpty())
            {
                return m_watchFolderPath + "/" + m_pathRelativeToWatchFolder;
            }
            return m_pathRelativeToWatchFolder;
        }

        AZ::u32 GetHash() const
        {
            AZ::Crc32 crc(m_databaseSourceName.toUtf8().constData());
            crc.Add(m_platformInfo.m_identifier.c_str());
            crc.Add(m_jobKey.toUtf8().constData());
            crc.Add(m_builderGuid.ToString<AZStd::string>().c_str());
            return crc;
        }

        JobEntry() = default;
        JobEntry(QString watchFolderPath, QString relativePathToFile, QString databaseSourceName, const AZ::Uuid& builderGuid, const AssetBuilderSDK::PlatformInfo& platformInfo, QString jobKey, AZ::u32 computedFingerprint, AZ::u64 jobRunKey, const AZ::Uuid &sourceUuid, bool addToDatabase = true)
            : m_watchFolderPath(watchFolderPath)
            , m_pathRelativeToWatchFolder(relativePathToFile)
            , m_databaseSourceName(databaseSourceName)
            , m_builderGuid(builderGuid)
            , m_platformInfo(platformInfo)
            , m_jobKey(jobKey)
            , m_computedFingerprint(computedFingerprint)
            , m_jobRunKey(jobRunKey)
            , m_addToDatabase(addToDatabase)
            , m_sourceFileUUID(sourceUuid)
        {
        }
    };

    //! This is an internal structure that hold all the information related to source file Dependency
    struct SourceFileDependencyInternal
    {
        AZStd::string m_sourceWatchFolder;   // this is the absolute path to the watch folder.
        AZStd::string m_relativeSourcePath;  // this is a pure relative path, not a database path
        AZ::Uuid m_sourceUUID;
        AZ::Uuid m_builderId;
        AssetBuilderSDK::SourceFileDependency m_sourceFileDependency; // this is the raw data captured from the builder.

        AZStd::string ToString() const
        {
            return AZStd::string::format(" %s %s %s", m_sourceUUID.ToString<AZStd::string>().c_str(), m_builderId.ToString<AZStd::string>().c_str(), m_relativeSourcePath.c_str());
        }
    };
    //! JobDependencyInternal is an internal structure that is used to store job dependency related info
    //! for later processing once we have resolved all the job dependency.
    struct JobDependencyInternal
    {
        JobDependencyInternal(const AssetBuilderSDK::JobDependency& jobDependency)
            :m_jobDependency(jobDependency)
        {
        }

        AZStd::set<AZ::Uuid> m_builderUuidList;// ordered set because we have to use dependent jobs fingerprint in some sorted order.
        AssetBuilderSDK::JobDependency m_jobDependency;

        AZStd::string ToString() const
        {
            return AZStd::string::format("%s %s %s", m_jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str(), m_jobDependency.m_jobKey.c_str(), m_jobDependency.m_platformIdentifier.c_str());
        }
    };
    //! JobDetails is an internal structure that is used to store job related information by the Asset Processor
    //! Its heavy, since it contains the parameter map and the builder desc so is expensive to copy and in general only used to create jobs
    //! After which, the Job Entry is used to track and identify jobs.
    class JobDetails
    {
    public:
        JobEntry m_jobEntry;
        AZStd::string m_extraInformationForFingerprinting;
        const ScanFolderInfo* m_scanFolder; // the scan folder info the file was found in
        QString m_destinationPath; // the final folder that will be where your products are placed if you give relative path names
        // destinationPath will be a cache folder.  If you tell it to emit something like "blah.dds"
        // it will put it in (destinationPath)/blah.dds for example
        AZStd::vector<JobDependencyInternal> m_jobDependencyList;

        // which files to include in the fingerprinting. (Not including job dependencies)
        SourceFilesForFingerprintingContainer m_fingerprintFiles;

        bool m_critical = false;
        int m_priority = -1;
        // indicates whether we need to check the server first for the outputs of this job
        // before we start processing locally
        bool m_checkServer = false;

        // Indicates whether this job needs to be processed irrespective of whether its fingerprint got modified or not.
        bool m_autoProcessJob = false;

        AssetBuilderSDK::AssetBuilderDesc   m_assetBuilderDesc;
        AssetBuilderSDK::JobParameterMap    m_jobParam;

        AZStd::vector<AZStd::string> m_warnings;

        // autoFail makes jobs which are added to the list and will automatically fail, and are used
        // to make sure that a "failure" shows up on the list so that the user can click to inspect the job and see why
        // it has failed instead of having a job fail mysteriously or be hard to find out why.
        // it is currently the only way for the job to be marked as a failure because of data integrity reasons after the builder
        // has already succeeded in actually making the asset data.
        // if you set a job to "auto fail" it will check the m_jobParam map for a AZ_CRC(AutoFailReasonKey) and use that, if present, for fail information
        bool m_autoFail = false;

        AZStd::string ToString() const
        {
            return QString("%1 %2 %3").arg(m_jobEntry.m_databaseSourceName, m_jobEntry.m_platformInfo.m_identifier.c_str(), m_jobEntry.m_jobKey).toUtf8().data();
        }

        bool operator==(const JobDetails& rhs) const
        {
            return ((m_jobEntry.m_databaseSourceName == rhs.m_jobEntry.m_databaseSourceName) &&
                (m_jobEntry.m_platformInfo.m_identifier == rhs.m_jobEntry.m_platformInfo.m_identifier) &&
                (m_jobEntry.m_jobKey == rhs.m_jobEntry.m_jobKey) &&
                m_jobEntry.m_builderGuid == rhs.m_jobEntry.m_builderGuid);
        }

        static bool DatabaseSourceLexCompare(const JobDetails& left, const JobDetails& right)
        {
            return left.m_jobEntry.m_databaseSourceName <= right.m_jobEntry.m_databaseSourceName;
        }

        JobDetails() = default;
    };

    //! JobDesc struct is used for identifying jobs that need to be processed again
    //! because of job dependency declared on them by other jobs
    struct JobDesc
    {
        AZStd::string m_databaseSourceName;
        AZStd::string m_jobKey;
        AZStd::string m_platformIdentifier;

        bool operator==(const JobDesc& rhs) const
        {
            return AzFramework::StringFunc::Equal(m_databaseSourceName.c_str(), rhs.m_databaseSourceName.c_str())
                && m_platformIdentifier == rhs.m_platformIdentifier
                && m_jobKey == rhs.m_jobKey;
        }

        JobDesc(const AZStd::string& databaseSourceName, const AZStd::string& jobKey, const AZStd::string& platformIdentifier)
            : m_databaseSourceName(databaseSourceName)
            , m_jobKey(jobKey)
            , m_platformIdentifier(platformIdentifier)
        {
        }

        AZStd::string ToString() const
        {
            AZStd::string lowerSourceName = m_databaseSourceName;
            AZStd::to_lower(lowerSourceName.begin(), lowerSourceName.end());

            return AZStd::string::format("%s %s %s", lowerSourceName.c_str(), m_platformIdentifier.c_str(), m_jobKey.c_str());
        }
    };

    //! JobIndentifier is an internal structure that store all the data that can uniquely identify a job
    struct JobIndentifier
    {
        JobDesc m_jobDesc;

        AZ::Uuid m_builderUuid = AZ::Uuid::CreateNull();

        bool operator==(const JobIndentifier& rhs) const
        {
            return (m_jobDesc == rhs.m_jobDesc) && (m_builderUuid == rhs.m_builderUuid);
        }

        JobIndentifier(const JobDesc& jobDesc, const AZ::Uuid builderUuid)
            : m_jobDesc(jobDesc)
            , m_builderUuid(builderUuid)
        {
        }
    };
} // namespace AssetProcessor

namespace AZStd
{
    template<>
    struct hash<AssetProcessor::JobDetails>
    {
        using argument_type = AssetProcessor::JobDetails;
        using result_type = size_t;

        result_type operator() (const argument_type& jobDetails) const
        {
            size_t h = 0;
            hash_combine(h, jobDetails.ToString());
            hash_combine(h, jobDetails.m_jobEntry.m_builderGuid);
            return h;
        }
    };
    template<>
    struct hash<AssetProcessor::JobDesc>
    {
        using argument_type = AssetProcessor::JobDesc;
        using result_type = size_t;

        result_type operator() (const argument_type& jobDesc) const
        {
            size_t h = 0;
            hash_combine(h, jobDesc.ToString());
            return h;
        }
    };
    template<>
    struct hash<AssetProcessor::JobIndentifier>
    {
        using argument_type = AssetProcessor::JobIndentifier;
        using result_type = size_t;

        result_type operator() (const argument_type& jobIndentifier) const
        {
            size_t h = 0;
            hash_combine(h, jobIndentifier.m_jobDesc);
            hash_combine(h, jobIndentifier.m_builderUuid);
            return h;
        }
    };
}

Q_DECLARE_METATYPE(AssetBuilderSDK::ProcessJobResponse)
Q_DECLARE_METATYPE(AssetProcessor::JobEntry)
Q_DECLARE_METATYPE(AssetProcessor::AssetProcessorStatusEntry)
Q_DECLARE_METATYPE(AssetProcessor::JobDetails)
Q_DECLARE_METATYPE(AssetProcessor::NetworkRequestID)
Q_DECLARE_METATYPE(AssetProcessor::AssetScanningStatus)
Q_DECLARE_METATYPE(AssetProcessor::AssetCatalogStatus)
