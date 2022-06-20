/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/PlatformDef.h>

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        class AssetJobsInfoResponse;
        class AssetJobsInfoRequest;

        //! A bus to talk to the asset system as a tool or editor
        //! This contains things that only tools or editors should be given access to
        //! If you want to talk to it as if a game engine component or runtime component
        //! \ref AssetSystemBus.h
        //! in the common header location.
        class AssetSystemRequest
            : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // single bus

            using MutexType = AZStd::recursive_mutex;

            // don't lock this bus during dispatch - its mainly just a forwarder of socket-based network requests
            // so when one thread is asking for status of an asset, its okay for another thread to do the same.
            static const bool LocklessDispatch = true; 

            virtual ~AssetSystemRequest() = default;

            //! Retrieve the absolute path for the Asset Database Location
            virtual bool GetAbsoluteAssetDatabaseLocation(AZStd::string& /*result*/) { return false; }
        
            /// Convert a full source path like "c:\\dev\\gamename\\blah\\test.tga" into a relative product path.
            /// asset paths never mention their alias and are relative to the asset cache root
            virtual bool GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullPath, AZStd::string& relativeProductPath) = 0;

            /** Convert a source path like "c:\\dev\\gamename\\blah\\test.tga" into a relative source path, like "blah/test.tga".
            * If no valid relative path could be created, the input source path will be returned in relativePath.
            * @param sourcePath partial or full path to a source file.  (The file doesn't need to exist)
            * @param relativePath the output relative path for the source file, if a valid one could be created
            * @param rootFilePath the root path that relativePath is relative to
            * @return true if a valid relative path was created, false if it wasn't
            */
            virtual bool GenerateRelativeSourcePath(
                const AZStd::string& sourcePath, AZStd::string& relativePath, AZStd::string& rootFilePath) = 0;

            /// Convert a relative asset path like "blah/test.tga" to a full source path path.
            /// Once the asset processor has finished building, this function is capable of handling even when the extension changes
            /// or when the source is in a different folder or in a different location (such as inside gems)
            virtual bool GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullSourcePath) = 0;

            //! retrieve an Az::Data::AssetInfo class for the given assetId.  this may map to source too in which case rootFilePath will be non-empty.
            virtual bool GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const AZStd::string& platformName, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath) = 0;

            /**
            * Given a path to a source file, retrieve its actual watch folder path and details.
            * @param sourcePath is either a relative or absolute path to a source file.
            * @param assetInfo is a /ref AZ::Data::AssetInfo filled out with details about the asset including its relative path to its watch folder
            *   note that inside assetInfo is a AssetId, but only the UUID-part will ever have a value since we are dealing with a source file (no subid)
            * @param watchFolder is the scan folder that it was found inside (the path in the assetInfo is relative to this folder).
            *   If you Path::Join the watchFolder and the assetInfo relative path, you get the full path.
            * returns false if it cannot find the source, true otherwise.
            */
            virtual bool GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) = 0;
            
            /**
            * Given a UUID of a source file, retrieve its actual watch folder path and other details.
            * @param sourceUUID is the UUID of a source file - If you have an AssetID, its the m_guid member of that assetId
            * @param assetInfo is a /ref AZ::Data::AssetInfo filled out with details about the asset including its relative path to its watch folder
            *           note that inside assetInfo is a AssetId, but only the UUID-part will ever have a value since we are dealing with a source file (no subid)
            * @param watchFolder is the scan folder that it was found inside (the path in the assetInfo is relative to this folder).
            *           If you Path::Join the watchFolder and the assetInfo relative path, you get the full path to the source file on physical media
            * returns false if it cannot find the source, true otherwise.
            */
            virtual bool GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) = 0;

            /**
            * Returns a list of scan folders recorded in the database.
            * @param scanFolder gets appended with the found folders.
            */
            virtual bool GetScanFolders(AZStd::vector<AZStd::string>& scanFolders) = 0;

            /**
            * Populates a list with folders that are safe to store assets in.
            * This is a subset of the scan folders.
            * @param scanFolder gets appended with the found folders.
            * @return false if this process fails.
            */
            virtual bool GetAssetSafeFolders(AZStd::vector<AZStd::string>& assetSafeFolders) = 0;

            /**
            * Query to see if a specific asset platform is enabled
            * @param platform the asset platform to check e.g. android, ios, etc.
            * @return true if enabled, false otherwise
            */
            virtual bool IsAssetPlatformEnabled(const char* platform) = 0;

            /**
            * Get the total number of pending assets left to process for a specific asset platform
            * @param platform the asset platform to check e.g. android, ios, etc.
            * @return -1 if the process fails, a positive number otherwise
            */
            virtual int GetPendingAssetsForPlatform(const char* platform) = 0;

            /**
            * Given a UUID of a source file, retrieve the products info.
            * @param sourceUUID is the UUID of a source file - If you have an AssetID, its the m_guid member of that assetId
            * @param productsAssetInfo is a /ref AZStd::vector<AZ::Data::AssetInfo> filled out with details about the products
            * returns false if it cannot find the source, true otherwise.
            */
            virtual bool GetAssetsProducedBySourceUUID(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo) = 0;
        };
        

        //! AssetSystemBusTraits
        //! This bus is for events that concern individual assets and is addressed by file extension
        class AssetSystemNotifications
            : public AZ::EBusTraits
        {
        public:
            typedef AZStd::recursive_mutex MutexType;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multiple listeners
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const bool EnableEventQueue = true; // enabled queued events, asset messages come from any thread

            virtual ~AssetSystemNotifications() = default;

            //! Called by the AssetProcessor when a source of an asset has been modified.
            virtual void SourceFileChanged(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/) {}
            //! Called by the AssetProcessor when a source of an asset has been removed.
            virtual void SourceFileRemoved(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/) {}
            //! This will be used by the asset processor to notify whenever a source file fails to process.
            virtual void SourceFileFailed(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/) {}
        };

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

        inline const char* JobStatusString(JobStatus status)
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
            return nullptr;
        }


        //! This struct is used for responses and requests about Asset Processor Jobs
        struct JobInfo
        {
            AZ_TYPE_INFO(JobInfo, "{276C9DE3-0C81-4721-91FE-F7C961D28DA8}")
            JobInfo()
            {
                m_jobRunKey = rand();
            }

            AZ::u32 GetHash() const
            {
                AZ::Crc32 crc(m_sourceFile.c_str());
                crc.Add(m_platform.c_str());
                crc.Add(m_jobKey.c_str());
                crc.Add(m_builderGuid.ToString<AZStd::string>().c_str());
                return crc;
            }

            static void Reflect(AZ::ReflectContext* context)
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

        typedef AZStd::vector<JobInfo> JobInfoContainer; 

        //! This Ebus will be used to retrieve all the job related information from AP
        class AssetSystemJobRequest
            : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

            virtual ~AssetSystemJobRequest() = default;

            /// Retrieve Jobs information for the given source file, setting escalteJobs to true will escalate all queued jobs 
            virtual AZ::Outcome<JobInfoContainer> GetAssetJobsInfo(const AZStd::string& sourcePath, const bool escalateJobs) = 0;

            /// Retrieve Jobs information for the given assetId, setting escalteJobs to true will escalate all queued jobs 
            /// you can also specify whether fencing is required  
            virtual AZ::Outcome<JobInfoContainer> GetAssetJobsInfoByAssetID(const AZ::Data::AssetId& assetId, const bool escalateJobs, bool requireFencing) = 0;

            /// Retrieve Jobs information for the given jobKey 
            virtual AZ::Outcome<JobInfoContainer> GetAssetJobsInfoByJobKey(const AZStd::string& jobKey, const bool escalateJobs) = 0;

            /// Retrieve Job Status for the given jobKey.
            /// If no jobs are present, return missing,
            /// else, if any matching jobs have failed, it will return failed
            /// else, if any of the matching jobs are queued, it will return queued
            /// else, if any matching jobs are in progress, will return inprogress
            /// else it will return the completed job status.
            virtual AZ::Outcome<JobStatus> GetAssetJobsStatusByJobKey(const AZStd::string& jobKey, const bool escalateJobs) = 0;

            /// Retrieve the actual log content for a particular job.   you can retrieve the run key from the above info function.
            virtual AZ::Outcome<AZStd::string> GetJobLog(AZ::u64 jobrunkey) = 0;
        };

        inline const char* GetHostAssetPlatform()
        {
#if defined(AZ_PLATFORM_MAC)
            return "mac";
#elif defined(AZ_PLATFORM_WINDOWS)
            return "pc";
#elif defined(AZ_PLATFORM_LINUX)
            return "linux";
#else
            #error Unimplemented Host Asset Platform
#endif
        }

    } // namespace AssetSystem
    using AssetSystemBus = AZ::EBus<AssetSystem::AssetSystemNotifications>;
    using AssetSystemRequestBus = AZ::EBus<AssetSystem::AssetSystemRequest>;
    using AssetSystemJobRequestBus = AZ::EBus<AssetSystem::AssetSystemJobRequest>;
} // namespace AzToolsFramework
