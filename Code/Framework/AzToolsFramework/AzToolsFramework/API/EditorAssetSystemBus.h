/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/PlatformDef.h>
#include <AzToolsFramework/API/AssetSystemJobInfo.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

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

            /** This will cause the file to reprocess the next time it changes, even if it's identical to what it was before.
            * This is useful for in-editor tools that save source files. A content creator expects that, every time they save a file,
            * the file will be processed, even if nothing has actually changed, so they will sometimes save a file specifically to
            * force the asset to reprocess.
            * @return true if the fingerprint was cleared, false if not.
            */
            virtual bool ClearFingerprintForAsset(const AZStd::string& sourcePath) = 0;
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

        //! This Ebus will be used to retrieve all the job related information from AP
        class AssetSystemJobRequest
            : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

            using MutexType = AZStd::recursive_mutex;
            static const bool LocklessDispatch = true;

            virtual ~AssetSystemJobRequest() = default;

            /// Retrieve Jobs information for the given source file, setting escalateJobs to true will escalate all queued jobs
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

    } // namespace AssetSystem
    using AssetSystemBus = AZ::EBus<AssetSystem::AssetSystemNotifications>;
    using AssetSystemRequestBus = AZ::EBus<AssetSystem::AssetSystemRequest>;
    using AssetSystemJobRequestBus = AZ::EBus<AssetSystem::AssetSystemJobRequest>;
} // namespace AzToolsFramework

namespace AZStd
{
    extern template class vector<AzToolsFramework::AssetSystem::JobInfo>;
}

AZTF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AzToolsFramework::AssetSystem::AssetSystemNotifications);
AZTF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AzToolsFramework::AssetSystem::AssetSystemRequest);
AZTF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AzToolsFramework::AssetSystem::AssetSystemJobRequest);
