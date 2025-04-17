/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// The Tools Framework AssetProcessorMessages header is for all of the asset processor messages that should only
// be available to tools, not the runtime.

#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        //!  Request that the asset processor clears the fingerprint for the given asset,
        //!  so that it will re-process the asset if the timestamp updates but has no changes.
        //!  This is useful for Editor tools: Content creators sometimes purposely save files
        //!  with no changes to force an asset to reprocess.
        class AssetFingerprintClearRequest : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetFingerprintClearRequest, AZ::OSAllocator);
            AZ_RTTI(AssetFingerprintClearRequest, "{2B7B5477-D3F8-43FF-8595-89D023690FCB}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetProcessor::AssetFingerprintClearRequest"); // CRC = 54071616 0x3391140

            explicit AssetFingerprintClearRequest(bool requireFencing = true);
            AssetFingerprintClearRequest(const AZ::OSString& searchTerm, bool requireFencing = true);
            unsigned int GetMessageType() const override;

            AZ::OSString m_searchTerm;
        };

        //! This will be send in response to the AssetFingerprintClearRequest request,
        //! and will contain if a fingerprint was actually cleared.
        class AssetFingerprintClearResponse : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetFingerprintClearResponse, AZ::OSAllocator);
            AZ_RTTI(AssetFingerprintClearResponse, "{FA7960F5-3F02-46C8-B85B-CB23A1D529B1}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            AssetFingerprintClearResponse() = default;
            AssetFingerprintClearResponse(bool isSuccess);
            unsigned int GetMessageType() const override;
            bool m_isSuccess = false;
            AssetSystem::JobInfoContainer m_jobList;
        };

        //!  Request the jobs information for a given asset from the AssetProcessor
        class AssetJobsInfoRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetJobsInfoRequest, AZ::OSAllocator);
            AZ_RTTI(AssetJobsInfoRequest, "{E5DEF45C-C4CF-47ED-843F-97B3C4A3D5B3}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetProcessor::AssetJobsInfoRequest");

            explicit AssetJobsInfoRequest(bool requireFencing = true);
            explicit AssetJobsInfoRequest(const AZ::Data::AssetId& assetId, bool m_escalateJobs = true, bool requireFencing = true);
            AssetJobsInfoRequest(const AZ::OSString& searchTerm, bool requireFencing = true);
            unsigned int GetMessageType() const override;

            AZ::OSString m_searchTerm;
            AZ::Data::AssetId m_assetId;
            bool m_isSearchTermJobKey = false;
            bool m_escalateJobs = true;
        };

        //! This will be send in response to the AssetJobsInfoRequest request,
        //! and will contain jobs information for the requested asset along with the jobid
        class AssetJobsInfoResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetJobsInfoResponse, AZ::OSAllocator);
            AZ_RTTI(AssetJobsInfoResponse, "{743AFB3B-F24C-4546-BEEC-2769442B52DB}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            AssetJobsInfoResponse() = default;
            AssetJobsInfoResponse(AssetSystem::JobInfoContainer& jobList, bool isSuccess);
            AssetJobsInfoResponse(AssetSystem::JobInfoContainer&& jobList, bool isSuccess);
            unsigned int GetMessageType() const override;
            bool m_isSuccess = false;
            AssetSystem::JobInfoContainer m_jobList;
        };

        //!  Request the log data for a given jobId from the AssetProcessor
        class AssetJobLogRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetJobLogRequest, AZ::OSAllocator);
            AZ_RTTI(AssetJobLogRequest, "{8E69F76E-F25D-486E-BC3F-26BB3FF5A3A3}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetProcessor::AssetJobLogRequest");
            explicit AssetJobLogRequest(bool requireFencing = true);
            explicit AssetJobLogRequest(AZ::u64 jobRunKey, bool requireFencing = true);
            unsigned int GetMessageType() const override;

            AZ::u64 m_jobRunKey;
        };

        //! This will be sent in response to the AssetJobLogRequest request, and will contain the complete job log as a string
        class AssetJobLogResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetJobLogResponse, AZ::OSAllocator);
            AZ_RTTI(AssetJobLogResponse, "{4CBB55AB-24E3-4A7A-ACB7-54069289AF2C}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            AssetJobLogResponse() = default;
            AssetJobLogResponse(const AZStd::string& jobLog, bool isSuccess);
            unsigned int GetMessageType() const override;

            bool m_isSuccess = false;
            AZStd::string m_jobLog;
        };
        
        //! Tools side message that a source file has changed or been removed
        class SourceFileNotificationMessage
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            enum NotificationType : unsigned int
            {
                FileChanged,
                FileRemoved,
                FileFailed,
            };

            AZ_CLASS_ALLOCATOR(SourceFileNotificationMessage, AZ::OSAllocator);
            AZ_RTTI(SourceFileNotificationMessage, "{61126952-242A-4299-B1D6-4D0E24DB1B06}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetProcessorManager::SourceFileNotification");

            SourceFileNotificationMessage() = default;
            SourceFileNotificationMessage(const AZ::OSString& relPath, const AZ::OSString& scanFolder, NotificationType type, AZ::Uuid sourceUUID);
            unsigned int GetMessageType() const override;

            AZ::OSString m_relativeSourcePath;
            AZ::OSString m_scanFolder;
            AZ::Uuid m_sourceUUID;
            NotificationType m_type;
        };

        class GetAbsoluteAssetDatabaseLocationRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetAbsoluteAssetDatabaseLocationRequest, AZ::OSAllocator);
            AZ_RTTI(GetAbsoluteAssetDatabaseLocationRequest, "{8696976E-F19D-48E3-BDDF-2GB63FA1AF23}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetProcessor::GetAbsoluteAssetDatabaseLocationRequest");
            GetAbsoluteAssetDatabaseLocationRequest() = default;
            unsigned int GetMessageType() const override;
        };

        class GetAbsoluteAssetDatabaseLocationResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetAbsoluteAssetDatabaseLocationResponse, AZ::OSAllocator);
            AZ_RTTI(GetAbsoluteAssetDatabaseLocationResponse, "{BDF155AB-EE74-FACA-3654-54069289AF2C}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            GetAbsoluteAssetDatabaseLocationResponse() = default;
            unsigned int GetMessageType() const override;

            bool m_isSuccess = false;
            AZStd::string m_absoluteAssetDatabaseLocation;
        };

        class GetScanFoldersRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetScanFoldersRequest, AZ::OSAllocator);
            AZ_RTTI(GetScanFoldersRequest, "{A3D7FD31-C260-4D6C-B970-D565B43F1316}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetProcessor::GetScanFoldersRequest");

            ~GetScanFoldersRequest() override = default;

            unsigned int GetMessageType() const override;
        };

        class GetScanFoldersResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetScanFoldersResponse, AZ::OSAllocator);
            AZ_RTTI(GetScanFoldersResponse, "{13100365-009E-4C82-A682-A8E3646EB0E0}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            GetScanFoldersResponse() = default;
            explicit GetScanFoldersResponse(const AZStd::vector<AZStd::string>& scanFolders);
            explicit GetScanFoldersResponse(AZStd::vector<AZStd::string>&& scanFolders);
            ~GetScanFoldersResponse() override = default;

            unsigned int GetMessageType() const override;

            AZStd::vector<AZStd::string> m_scanFolders;
        };


        class GetAssetSafeFoldersRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetAssetSafeFoldersRequest, AZ::OSAllocator);
            AZ_RTTI(GetAssetSafeFoldersRequest, "{9A7951B1-257C-45F0-B334-A52A42A5A871}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetProcessor::GetAssetSafeFoldersRequest");

            ~GetAssetSafeFoldersRequest() override = default;

            unsigned int GetMessageType() const override;
        };

        class GetAssetSafeFoldersResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetAssetSafeFoldersResponse, AZ::OSAllocator);
            AZ_RTTI(GetAssetSafeFoldersResponse, "{36C1AA51-8940-4909-A01B-19454B6312E5}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            GetAssetSafeFoldersResponse() = default;
            explicit GetAssetSafeFoldersResponse(const AZStd::vector<AZStd::string>& assetSafeFolders);
            explicit GetAssetSafeFoldersResponse(AZStd::vector<AZStd::string>&& assetSafeFolders);
            ~GetAssetSafeFoldersResponse() override = default;

            unsigned int GetMessageType() const override;

            AZStd::vector<AZStd::string> m_assetSafeFolders;
        };

        class FileInfosNotificationMessage
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            enum NotificationType : unsigned int
            {
                Synced,
                FileAdded,
                FileRemoved
            };

            AZ_CLASS_ALLOCATOR(FileInfosNotificationMessage, AZ::OSAllocator);
            AZ_RTTI(FileInfosNotificationMessage, "{F5AF3ED1-1644-4972-AE21-B6A1B28D898A}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileInfosNotificationMessage() = default;
            unsigned int GetMessageType() const override;

            NotificationType m_type = NotificationType::Synced;
            AZ::s64 m_fileID = 0;
        };

        //////////////////////////////////////////////////////////////////////////
        //! Request the enabled status of an asset platform
        class AssetProcessorPlatformStatusRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetProcessorPlatformStatusRequest, AZ::OSAllocator);
            AZ_RTTI(AssetProcessorPlatformStatusRequest, "{529A8549-DD78-4E66-9BEA-D633846115C6}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetSystem::AssetProcessorPlatformStatusRequest");

            AssetProcessorPlatformStatusRequest() = default;
            unsigned int GetMessageType() const override;
            AZ::OSString m_platform;
        };

        //! This will be sent in response to the AssetProcessorPlatformStatusRequest request,
        //! indicating if the asset platform is currently enabled or not
        class AssetProcessorPlatformStatusResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetProcessorPlatformStatusResponse, AZ::OSAllocator);
            AZ_RTTI(AssetProcessorPlatformStatusResponse, "{3F804A16-3C5A-41A5-9051-7714E3CFAC9A}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            AssetProcessorPlatformStatusResponse() = default;
            unsigned int GetMessageType() const override;
            bool m_isPlatformEnabled = false;
        };

        //////////////////////////////////////////////////////////////////////////
        //! Request the total number of pending jobs for an asset platform
        class AssetProcessorPendingPlatformAssetsRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetProcessorPendingPlatformAssetsRequest, AZ::OSAllocator);
            AZ_RTTI(AssetProcessorPendingPlatformAssetsRequest, "{5B16F6F2-0F94-4BAE-8238-1E8F3E66D507}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetSystem::AssetProcessorPendingPlatformAssetsRequest");

            AssetProcessorPendingPlatformAssetsRequest() = default;
            unsigned int GetMessageType() const override;
            AZ::OSString m_platform;
        };


        //! This will be sent in response to the AssetProcessorPendingPlatformAssetsRequest request,
        //! indicating the number of pending assets for the specified platform
        class AssetProcessorPendingPlatformAssetsResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetProcessorPendingPlatformAssetsResponse, AZ::OSAllocator);
            AZ_RTTI(AssetProcessorPendingPlatformAssetsResponse, "{E63825D6-4704-471D-8594-B96656FA4477}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            AssetProcessorPendingPlatformAssetsResponse() = default;
            unsigned int GetMessageType() const override;
            int m_numberOfPendingJobs = -1;
        };

        // WantAssetBrowserShowRequest
        class WantAssetBrowserShowRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(WantAssetBrowserShowRequest, AZ::OSAllocator);
            AZ_RTTI(WantAssetBrowserShowRequest, "{C66852BF-1A8C-47AC-9CFF-183CC4241075}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetSystem::WantAssetBrowserShowRequest");

            WantAssetBrowserShowRequest() = default;
            unsigned int GetMessageType() const override;
        };

        // WantAssetBrowserShowResponse
        class WantAssetBrowserShowResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(WantAssetBrowserShowResponse, AZ::OSAllocator);
            AZ_RTTI(WantAssetBrowserShowResponse, "{B3015EF2-A91F-4E7D-932A-DA6043EDB678}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetSystem::WantAssetBrowserShowResponse");

            WantAssetBrowserShowResponse() = default;
            unsigned int GetMessageType() const override;

            unsigned int m_processId = 0;
        };

        // AssetBrowserShowRequest
        class AssetBrowserShowRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserShowRequest, AZ::OSAllocator);
            AZ_RTTI(AssetBrowserShowRequest, "{D44903DD-45D8-4CBA-8A9D-C9D5E7FFB0A6}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetSystem::AssetBrowserShowRequest");

            AssetBrowserShowRequest() = default;
            unsigned int GetMessageType() const override;

            AZ::OSString m_filePath;
        };

        // SourceAssetProductsInfoRequest
        class SourceAssetProductsInfoRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(SourceAssetProductsInfoRequest, AZ::OSAllocator);
            AZ_RTTI(SourceAssetProductsInfoRequest, "{14D0994C-7096-44D9-A239-2A7B51DDC95A}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC_CE("AssetProcessor::SourceAssetProductsInfoRequest");

            SourceAssetProductsInfoRequest() = default;
            explicit SourceAssetProductsInfoRequest(const AZ::Data::AssetId& assetId);
            unsigned int GetMessageType() const override;

            AZ::Data::AssetId m_assetId;
        };

        // SourceAssetProductsInfoResponse
        class SourceAssetProductsInfoResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(SourceAssetProductsInfoResponse, AZ::OSAllocator);
            AZ_RTTI(SourceAssetProductsInfoResponse, "{DF0B7C57-534E-480E-8889-C4872C87C1C3}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            SourceAssetProductsInfoResponse() = default;

            unsigned int GetMessageType() const override;

            bool m_found = false;
            AZStd::vector<AZ::Data::AssetInfo> m_productsAssetInfo;
        };
    } // namespace AssetSystem
} // namespace AzToolsFramework
