/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Crc.h>

namespace AzToolsFramework
{
    using namespace AZ;
    using namespace AzFramework::AssetSystem;

    namespace AssetSystem
    {
        //---------------------------------------------------------------------
        AssetFingerprintClearRequest::AssetFingerprintClearRequest(bool requireFencing /*= true*/)
            : BaseAssetProcessorMessage(requireFencing)
        {
        }

        AssetFingerprintClearRequest::AssetFingerprintClearRequest(const AZ::OSString& searchTerm, bool requireFencing /*= true*/)
            : BaseAssetProcessorMessage(requireFencing)
            , m_searchTerm(searchTerm)
        {
            AZ_Assert(!searchTerm.empty(), "AssetJobsInfoRequest: Search Term is empty");
        }

        unsigned int AssetFingerprintClearRequest::GetMessageType() const
        {
            return MessageType;
        }

        void AssetFingerprintClearRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetFingerprintClearRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("SearchTerm", &AssetFingerprintClearRequest::m_searchTerm);
            }
        }

        //---------------------------------------------------------------------
        AssetFingerprintClearResponse::AssetFingerprintClearResponse(bool isSuccess)
            : m_isSuccess(isSuccess)
        {
        }

        unsigned int AssetFingerprintClearResponse::GetMessageType() const
        {
            return AssetFingerprintClearRequest::MessageType;
        }

        void AssetFingerprintClearResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetFingerprintClearResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Success", &AssetFingerprintClearResponse::m_isSuccess);
            }
        }

        //---------------------------------------------------------------------
        AssetJobsInfoRequest::AssetJobsInfoRequest(const AZ::OSString& searchTerm, bool requireFencing /*= true*/)
            : BaseAssetProcessorMessage(requireFencing)
            , m_searchTerm(searchTerm) 
        {
            AZ_Assert(!searchTerm.empty(), "AssetJobsInfoRequest: Search Term is empty");
        }


        AssetJobsInfoRequest::AssetJobsInfoRequest(const AZ::Data::AssetId& assetId, bool escalateJobs /*= true*/, bool requireFencing /*= true*/)
            : BaseAssetProcessorMessage(requireFencing)
            , m_assetId(assetId)
            , m_escalateJobs(escalateJobs)
            
        {
        }

        AssetJobsInfoRequest::AssetJobsInfoRequest(bool requireFencing /*= true*/)
            : BaseAssetProcessorMessage(requireFencing)
        {
        }

        unsigned int AssetJobsInfoRequest::GetMessageType() const
        {
            return MessageType;
        }

        void AssetJobsInfoRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetJobsInfoRequest, BaseAssetProcessorMessage>()
                    ->Version(4)
                    ->Field("SearchTerm", &AssetJobsInfoRequest::m_searchTerm)
                    ->Field("EscalateJobs", &AssetJobsInfoRequest::m_escalateJobs)
                    ->Field("IsSearchTermJobKey", &AssetJobsInfoRequest::m_isSearchTermJobKey)
                    ->Field("AssetId", &AssetJobsInfoRequest::m_assetId);
            }
        }

        //---------------------------------------------------------------------
        AssetJobsInfoResponse::AssetJobsInfoResponse(AssetSystem::JobInfoContainer& jobList, bool isSuccess)
            : m_isSuccess(isSuccess)
        {
            m_jobList.swap(jobList);
        }

        AssetJobsInfoResponse::AssetJobsInfoResponse(AssetSystem::JobInfoContainer&& jobList, bool isSuccess)
            : m_isSuccess(isSuccess)
            , m_jobList(AZStd::move(jobList))
        {
        }

        unsigned int AssetJobsInfoResponse::GetMessageType() const
        {
            return AssetJobsInfoRequest::MessageType;
        }

        void AssetJobsInfoResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetJobsInfoResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("JobList", &AssetJobsInfoResponse::m_jobList)
                    ->Field("Success", &AssetJobsInfoResponse::m_isSuccess);
            }
        }

        //---------------------------------------------------------------------
        AssetJobLogRequest::AssetJobLogRequest(AZ::u64 jobRunKey, bool requireFencing /*= true*/)
            : BaseAssetProcessorMessage(requireFencing)
            , m_jobRunKey(jobRunKey)
        {
            AZ_Assert(m_jobRunKey > 0 , "AssetJobLogRequest: asset run key is invalid");
        }


        AssetJobLogRequest::AssetJobLogRequest(bool requireFencing /*= true*/)
            : BaseAssetProcessorMessage(requireFencing)
        {
        }

        unsigned int AssetJobLogRequest::GetMessageType() const
        {
            return MessageType;
        }

        void AssetJobLogRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetJobLogRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("JobRunKey", &AssetJobLogRequest::m_jobRunKey);
            }
        }

        //---------------------------------------------------------------------
        AssetJobLogResponse::AssetJobLogResponse(const AZStd::string& jobLog,  bool isSuccess)
            : m_jobLog(jobLog)
            , m_isSuccess(isSuccess)
        {
        }

        unsigned int AssetJobLogResponse::GetMessageType() const
        {
            return AssetJobLogRequest::MessageType;
        }

        void AssetJobLogResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetJobLogResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("JobLog", &AssetJobLogResponse::m_jobLog)
                    ->Field("Success", &AssetJobLogResponse::m_isSuccess);
            }
        }

        //---------------------------------------------------------------------
        SourceFileNotificationMessage::SourceFileNotificationMessage(const AZ::OSString& relativeSourcePath, const AZ::OSString& scanFolder, NotificationType type, AZ::Uuid sourceUUID)
            : m_relativeSourcePath(relativeSourcePath)
            , m_scanFolder(scanFolder)
            , m_type(type)
            , m_sourceUUID(sourceUUID)
        {
            AZ_Assert(!m_relativeSourcePath.empty(), "SourceFileNotificationMessage: empty relative path");
            AZ_Assert(!scanFolder.empty(), "SourceFileNotificationMessage: empty scanFolder");
        }

        unsigned int SourceFileNotificationMessage::GetMessageType() const
        {
            return MessageType;
        }

        void SourceFileNotificationMessage::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SourceFileNotificationMessage, BaseAssetProcessorMessage>()
                    ->Version(2)
                    ->Field("RelativeSourcePath", &SourceFileNotificationMessage::m_relativeSourcePath)
                    ->Field("ScanFolder", &SourceFileNotificationMessage::m_scanFolder)
                    ->Field("NotificationType", &SourceFileNotificationMessage::m_type)
                    ->Field("SourceUUID", &SourceFileNotificationMessage::m_sourceUUID);
            }
        }

        unsigned int GetAbsoluteAssetDatabaseLocationRequest::GetMessageType() const
        {
            return MessageType;
        }

        void GetAbsoluteAssetDatabaseLocationRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetAbsoluteAssetDatabaseLocationRequest, BaseAssetProcessorMessage>()
                    ->Version(1);
           }
        }

        unsigned int GetAbsoluteAssetDatabaseLocationResponse::GetMessageType() const
        {
            return GetAbsoluteAssetDatabaseLocationRequest::MessageType;
        }

        void GetAbsoluteAssetDatabaseLocationResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetAbsoluteAssetDatabaseLocationResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("AssetDatabasePath", &GetAbsoluteAssetDatabaseLocationResponse::m_absoluteAssetDatabaseLocation)
                    ->Field("Success", &GetAbsoluteAssetDatabaseLocationResponse::m_isSuccess);
            }
        }

        //---------------------------------------------------------------------

        unsigned int GetScanFoldersRequest::GetMessageType() const
        {
            return MessageType;
        }

        void GetScanFoldersRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetScanFoldersRequest>()
                    ->Version(1)
                    ->SerializeWithNoData();
            }
        }

        //---------------------------------------------------------------------
        GetScanFoldersResponse::GetScanFoldersResponse(const AZStd::vector<AZStd::string>& scanFolders)
            : m_scanFolders(scanFolders)
        {
        }

        GetScanFoldersResponse::GetScanFoldersResponse(AZStd::vector<AZStd::string>&& scanFolders)
            : m_scanFolders(AZStd::move(scanFolders))
        {
        }

        unsigned int GetScanFoldersResponse::GetMessageType() const
        {
            return GetScanFoldersRequest::MessageType;
        }

        void GetScanFoldersResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetScanFoldersResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ScanFolders", &GetScanFoldersResponse::m_scanFolders);
            }
        }

        //---------------------------------------------------------------------
        
        unsigned int GetAssetSafeFoldersRequest::GetMessageType() const
        {
            return MessageType;
        }

        void GetAssetSafeFoldersRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetAssetSafeFoldersRequest>()
                    ->Version(1)
                    ->SerializeWithNoData();
            }
        }

        //---------------------------------------------------------------------
        GetAssetSafeFoldersResponse::GetAssetSafeFoldersResponse(const AZStd::vector<AZStd::string>& assetSafeFolders)
            : m_assetSafeFolders(assetSafeFolders)
        {
        }

        GetAssetSafeFoldersResponse::GetAssetSafeFoldersResponse(AZStd::vector<AZStd::string>&& assetSafeFolders)
            : m_assetSafeFolders(AZStd::move(assetSafeFolders))
        {
        }

        unsigned int GetAssetSafeFoldersResponse::GetMessageType() const
        {
            return GetAssetSafeFoldersRequest::MessageType;
        }

        void GetAssetSafeFoldersResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetAssetSafeFoldersResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("AssetSafeFolders", &GetAssetSafeFoldersResponse::m_assetSafeFolders);
            }
        }

        //---------------------------------------------------------------------
        void FileInfosNotificationMessage::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileInfosNotificationMessage, BaseAssetProcessorMessage>()
                    ->Field("NotificationType", &FileInfosNotificationMessage::m_type)
                    ->Field("FileID", &FileInfosNotificationMessage::m_fileID)
                    ->Version(1);
            }
        }

        unsigned FileInfosNotificationMessage::GetMessageType() const
        {
            static unsigned int messageType = AZ_CRC_CE("FileProcessor::FileInfosNotification");
            return messageType;
        }

        //---------------------------------------------------------------------
        void AssetProcessorPlatformStatusRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetProcessorPlatformStatusRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Platform", &AssetProcessorPlatformStatusRequest::m_platform);
            }
        }

        unsigned int AssetProcessorPlatformStatusRequest::GetMessageType() const
        {
            return AssetProcessorPlatformStatusRequest::MessageType;
        }

        //------------------------------------------------------------------------
        void AssetProcessorPlatformStatusResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetProcessorPlatformStatusResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("IsPlatformEnabled", &AssetProcessorPlatformStatusResponse::m_isPlatformEnabled);
            }
        }

        unsigned int AssetProcessorPlatformStatusResponse::GetMessageType() const
        {
            return AssetProcessorPlatformStatusRequest::MessageType;
        }

        //---------------------------------------------------------------------
        void AssetProcessorPendingPlatformAssetsRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetProcessorPendingPlatformAssetsRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Platform", &AssetProcessorPendingPlatformAssetsRequest::m_platform);
            }
        }

        unsigned int AssetProcessorPendingPlatformAssetsRequest::GetMessageType() const
        {
            return AssetProcessorPendingPlatformAssetsRequest::MessageType;
        }

        //------------------------------------------------------------------------
        void AssetProcessorPendingPlatformAssetsResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetProcessorPendingPlatformAssetsResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("NumberOfPendingJobs", &AssetProcessorPendingPlatformAssetsResponse::m_numberOfPendingJobs);
            }
        }

        unsigned int AssetProcessorPendingPlatformAssetsResponse::GetMessageType() const
        {
            return AssetProcessorPendingPlatformAssetsRequest::MessageType;
        }

        unsigned int WantAssetBrowserShowRequest::GetMessageType() const
        {
            return WantAssetBrowserShowRequest::MessageType;
        }

        void WantAssetBrowserShowRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<WantAssetBrowserShowRequest, BaseAssetProcessorMessage>()
                    ->Version(1);
            }
        }


        unsigned int WantAssetBrowserShowResponse::GetMessageType() const
        {
            return WantAssetBrowserShowResponse::MessageType;
        }

        void WantAssetBrowserShowResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<WantAssetBrowserShowResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ProcessId", &WantAssetBrowserShowResponse::m_processId);
            }
        }

        unsigned int AssetBrowserShowRequest::GetMessageType() const
        {
            return AssetBrowserShowRequest::MessageType;
        }

        void AssetBrowserShowRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetBrowserShowRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FilePath", &AssetBrowserShowRequest::m_filePath);
            }
        }

        //---------------------------------------------------------------------
        SourceAssetProductsInfoRequest::SourceAssetProductsInfoRequest(const AZ::Data::AssetId& assetId)
            : m_assetId(assetId)
        {
        }

        unsigned int SourceAssetProductsInfoRequest::GetMessageType() const
        {
            return MessageType;
        }

        void SourceAssetProductsInfoRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SourceAssetProductsInfoRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("AssetId", &SourceAssetProductsInfoRequest::m_assetId);
            }
        }

        //---------------------------------------------------------------------
        unsigned int SourceAssetProductsInfoResponse::GetMessageType() const
        {
            return SourceAssetProductsInfoRequest::MessageType;
        }

        void SourceAssetProductsInfoResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SourceAssetProductsInfoResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Found", &SourceAssetProductsInfoResponse::m_found)
                    ->Field("ProductsAssetInfo", &SourceAssetProductsInfoResponse::m_productsAssetInfo);
            }
        }
    } // namespace AssetSystem
} // namespace AzToolsFramework
