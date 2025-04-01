/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>

#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        void OnAssetSystemMessage(unsigned int /*typeId*/, const void* buffer, unsigned int bufferSize, AZ::SerializeContext* context)
        {
            SourceFileNotificationMessage message;
            if (!AZ::Utils::LoadObjectFromBufferInPlace(buffer, bufferSize, message, context))
            {
                AZ_TracePrintf("AssetSystem", "Problem deserializing SourceFileNotificationMessage");
                return;
            }

            switch (message.m_type)
            {
            case SourceFileNotificationMessage::FileChanged:
                AssetSystemBus::QueueBroadcast(&AssetSystemBus::Events::SourceFileChanged,
                    message.m_relativeSourcePath, message.m_scanFolder, message.m_sourceUUID);
                break;
            case SourceFileNotificationMessage::FileRemoved:
                AssetSystemBus::QueueBroadcast(&AssetSystemBus::Events::SourceFileRemoved,
                    message.m_relativeSourcePath, message.m_scanFolder, message.m_sourceUUID);
                break;
            case SourceFileNotificationMessage::FileFailed :
                AssetSystemBus::QueueBroadcast(&AssetSystemBus::Events::SourceFileFailed,
                    message.m_relativeSourcePath, message.m_scanFolder, message.m_sourceUUID);
                break;
            default:
                AZ_TracePrintf("AssetSystem", "Unknown SourceFileNotificationMessage type");
                break;
            }
        }

        void OnAssetBrowserShowRequest(const void* buffer, unsigned int bufferSize)
        {
            AssetBrowserShowRequest message;
            if (!AZ::Utils::LoadObjectFromBufferInPlace(buffer, bufferSize, message))
            {
                AZ_TracePrintf("AssetSystem", "Problem deserializing AssetBrowserShowRequest");
                return;
            }

            QString absolutePath = QString::fromUtf8(message.m_filePath.data());
            AZStd::function<void()> finalizeOnMainThread = [absolutePath]()
            {
                AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::SelectAsset, absolutePath);
            };

            AZ::SystemTickBus::QueueFunction(finalizeOnMainThread);
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> SendAssetJobsRequest(AssetJobsInfoRequest request, AssetJobsInfoResponse &response)
        {
            if (!SendRequest(request, response))
            {
                bool hasAssetId = request.m_assetId.IsValid();
                bool hasSearchTerm = !request.m_searchTerm.empty();

                if(hasAssetId)
                {
                    AZ_Error("Editor", false, "GetAssetJobsInfo request failed for AssetId: %s", request.m_assetId.ToString<AZStd::string>().c_str());
                }
                else if(hasSearchTerm)
                {
                    AZ_Error("Editor", false, "GetAssetJobsInfo request failed for search term: %s", request.m_searchTerm.c_str());
                }
                else
                {
                    AZ_Error("Editor", false, "GetAssetJobsInfo request failed, no AssetId or search term was provided");
                }
                
                return AZ::Failure();
            }

            if (response.m_isSuccess)
            {
                return AZ::Success(response.m_jobList);
            }

            return AZ::Failure();
        }

        void AssetSystemComponent::Activate()
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);

            AzFramework::SocketConnection* socketConn = AzFramework::SocketConnection::GetInstance();
            AZ_Assert(socketConn, "AzToolsFramework::AssetSystem::AssetSystemComponent requires a valid socket conection!");
            if (socketConn)
            {
                m_cbHandle = socketConn->AddMessageHandler(AZ_CRC_CE("AssetProcessorManager::SourceFileNotification"),
                    [context](unsigned int typeId, unsigned int /*serial*/, const void* data, unsigned int dataLength)
                {
                    OnAssetSystemMessage(typeId, data, dataLength, context);
                });

                m_showAssetBrowserCBHandle = socketConn->AddMessageHandler(AssetSystem::AssetBrowserShowRequest::MessageType,
                    [](unsigned int /*typeId*/, unsigned int /*serial*/, const void* data, unsigned int dataLength)
                {
                    OnAssetBrowserShowRequest(data, dataLength);
                });

                m_wantShowAssetBrowserCBHandle = socketConn->AddMessageHandler(AssetSystem::WantAssetBrowserShowRequest::MessageType,
                    [](unsigned int /*typeId*/, unsigned int serial, const void* data, unsigned int dataLength)
                {
                    Q_UNUSED(data);
                    Q_UNUSED(dataLength);

                    AssetSystem::WantAssetBrowserShowResponse message;
#ifdef AZ_PLATFORM_WINDOWS
                    message.m_processId = GetCurrentProcessId();
#endif // #ifdef AZ_PLATFORM_WINDOWS
                    SendResponse(message, serial);
                });
            }

            AssetSystemBus::AllowFunctionQueuing(true);
            AssetSystemRequestBus::Handler::BusConnect();
            AssetSystemJobRequestBus::Handler::BusConnect();
            AzToolsFramework::ToolsAssetSystemBus::Handler::BusConnect();
            AZ::SystemTickBus::Handler::BusConnect();
        }

        void AssetSystemComponent::Deactivate()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            AzToolsFramework::ToolsAssetSystemBus::Handler::BusDisconnect();
            AssetSystemJobRequestBus::Handler::BusDisconnect();
            AssetSystemRequestBus::Handler::BusDisconnect();

            AzFramework::SocketConnection* socketConn = AzFramework::SocketConnection::GetInstance();
            AZ_Assert(socketConn, "AzToolsFramework::AssetSystem::AssetSystemComponent requires a valid socket conection!");
            if (socketConn)
            {
                socketConn->RemoveMessageHandler(AssetSystem::WantAssetBrowserShowRequest::MessageType, m_wantShowAssetBrowserCBHandle);
                socketConn->RemoveMessageHandler(AssetSystem::AssetBrowserShowRequest::MessageType, m_showAssetBrowserCBHandle);
                socketConn->RemoveMessageHandler(AZ_CRC_CE("AssetProcessorManager::SourceFileNotification"), m_cbHandle);
            }

            AssetSystemBus::AllowFunctionQueuing(false);
            AssetSystemBus::ClearQueuedEvents();
        }

        void AssetSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            //source file
            SourceFileNotificationMessage::Reflect(context);

            // Requests
            AssetFingerprintClearRequest::Reflect(context);
            AssetJobsInfoRequest::Reflect(context);
            AssetJobLogRequest::Reflect(context);
            GetAbsoluteAssetDatabaseLocationRequest::Reflect(context);
            GetScanFoldersRequest::Reflect(context);
            GetAssetSafeFoldersRequest::Reflect(context);
            AssetProcessorPlatformStatusRequest::Reflect(context);
            AssetProcessorPendingPlatformAssetsRequest::Reflect(context);
            WantAssetBrowserShowRequest::Reflect(context);
            AssetBrowserShowRequest::Reflect(context);
            SourceAssetProductsInfoRequest::Reflect(context);

            // Responses
            AssetFingerprintClearResponse::Reflect(context);
            AssetJobsInfoResponse::Reflect(context);
            AssetJobLogResponse::Reflect(context);
            GetAbsoluteAssetDatabaseLocationResponse::Reflect(context);
            GetScanFoldersResponse::Reflect(context);
            GetAssetSafeFoldersResponse::Reflect(context);
            AssetProcessorPlatformStatusResponse::Reflect(context);
            AssetProcessorPendingPlatformAssetsResponse::Reflect(context);
            WantAssetBrowserShowResponse::Reflect(context);
            SourceAssetProductsInfoResponse::Reflect(context);

            //JobInfo
            AzToolsFramework::AssetSystem::JobInfo::Reflect(context);

            //AssetSystemComponent
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AssetSystemComponent, AZ::Component>()
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<AssetEditor::AssetEditorRequestsBus>("AssetEditorRequestsBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Event("CreateNewAsset", &AssetEditor::AssetEditorRequests::CreateNewAsset)
                    ->Event("OpenAssetEditorById", &AssetEditor::AssetEditorRequests::OpenAssetEditorById)
                    ;

                behaviorContext->EBus<AssetEditor::AssetEditorWidgetRequestsBus>("AssetEditorWidgetRequestsBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Event("CreateAsset", &AssetEditor::AssetEditorWidgetRequests::CreateAsset)
                    ->Event("SaveAssetAs", &AssetEditor::AssetEditorWidgetRequests::SaveAssetAs)
                    ->Event("OpenAssetById", &AssetEditor::AssetEditorWidgetRequests::OpenAssetById)
                    ;
            }
        }

        void AssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AssetProcessorToolsConnection"));
        }

        void AssetSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AssetProcessorToolsConnection"));
        }

        void AssetSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("AssetProcessorConnection"));
        }

        bool AssetSystemComponent::GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullPath, AZStd::string& outputPath)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                outputPath = fullPath;
                return false;
            }

            AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathRequest request(fullPath);
            AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAssetId request for %s", fullPath.c_str());
                outputPath = fullPath;
                return false;
            }

            outputPath = response.m_relativeProductPath;
            return response.m_resolved;
        }

        bool AssetSystemComponent::GenerateRelativeSourcePath(
            const AZStd::string& sourcePath, AZStd::string& relativePath, AZStd::string& rootFilePath)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                relativePath = sourcePath;
                return false;
            }

            AzFramework::AssetSystem::GenerateRelativeSourcePathRequest request(sourcePath);
            AzFramework::AssetSystem::GenerateRelativeSourcePathResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GenerateRelativeSourcePath request for %s", sourcePath.c_str());
                relativePath = sourcePath;
                return false;
            }

            relativePath = response.m_relativeSourcePath;
            rootFilePath = response.m_rootFolder;
            return response.m_resolved;
        }

        bool AssetSystemComponent::GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullPath)
        {
            auto foundIt = m_assetSourceRelativePathToFullPathCache.find(relPath);
            if (foundIt != m_assetSourceRelativePathToFullPathCache.end())
            {
                fullPath = foundIt->second;
                return true;
            }

            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                fullPath = "";
                return false;
            }

            AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathRequest request(relPath);
            AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAssetPath request for %s", relPath.c_str());
                fullPath = "";
                return false;
            }


            if (response.m_resolved)
            {
                fullPath = response.m_fullSourcePath;
                m_assetSourceRelativePathToFullPathCache[relPath] = fullPath;
                return true;
            }
            else
            {
                fullPath = "";
                return false;
            }
        }

        bool AssetSystemComponent::GetAbsoluteAssetDatabaseLocation(AZStd::string& result)
        {
            result = "";

            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            AzToolsFramework::AssetSystem::GetAbsoluteAssetDatabaseLocationRequest request;
            AzToolsFramework::AssetSystem::GetAbsoluteAssetDatabaseLocationResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAbsoluteAssetDatabaseLocation request");
                return false;
            }


            if (response.m_isSuccess)
            {
                result = response.m_absoluteAssetDatabaseLocation;
                return true;
            }
            else
            {
                return false;
            }
        }

        void AssetSystemComponent::OnSystemTick()
        {
            AssetSystemBus::ExecuteQueuedEvents();
        }

        bool AssetSystemComponent::GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const AZStd::string& /*platformName*/, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();

            assetInfo.m_assetId.SetInvalid();
            assetInfo.m_assetType = AZ::Data::s_invalidAssetType;

            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            AzFramework::AssetSystem::SourceAssetInfoRequest request(assetId, assetType);
            AzFramework::AssetSystem::SourceAssetInfoResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAssetInfoById request for %s", assetId.ToString<AZStd::string>().c_str());
                return false;
            }

            if (response.m_found)
            {
                assetInfo = response.m_assetInfo;
                rootFilePath = response.m_rootFolder;
                return true;
            }

            return false;
        }

        bool AssetSystemComponent::GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();

            assetInfo.m_assetId.SetInvalid();
            assetInfo.m_assetType = AZ::Data::s_invalidAssetType;

            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            AzFramework::AssetSystem::SourceAssetInfoRequest request(sourcePath);
            AzFramework::AssetSystem::SourceAssetInfoResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetSourceInfoBySourcePath request for %s", sourcePath);
                return false;
            }

            if (response.m_found)
            {
                assetInfo = response.m_assetInfo;
                watchFolder = response.m_rootFolder;
            }
            return response.m_found;
        }

        bool AssetSystemComponent::GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();

            assetInfo.m_assetId.SetInvalid();
            assetInfo.m_assetType = AZ::Data::s_invalidAssetType;

            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            AzFramework::AssetSystem::SourceAssetInfoRequest request(sourceUuid, AZ::Uuid::CreateNull());
            AzFramework::AssetSystem::SourceAssetInfoResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetSourceInfoBySourceUUID request for uuid: %s", sourceUuid.ToString<AZ::OSString>().c_str());
                return false;
            }

            if (response.m_found)
            {
                assetInfo = response.m_assetInfo;
                watchFolder = response.m_rootFolder;
            }
            return response.m_found;
        }

        bool AssetSystemComponent::GetAssetsProducedBySourceUUID(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            SourceAssetProductsInfoRequest request(sourceUuid);
            SourceAssetProductsInfoResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAssetsProducedBySourceUUID request for uuid: %s", sourceUuid.ToString<AZ::OSString>().c_str());
                return false;
            }

            if (response.m_found)
            {
                productsAssetInfo = response.m_productsAssetInfo;
            }

            return response.m_found;
        }

        bool AssetSystemComponent::GetScanFolders(AZStd::vector<AZStd::string>& scanFolders)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            GetScanFoldersRequest request;
            GetScanFoldersResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetScanFolders request");
                return false;
            }

            scanFolders.insert(scanFolders.end(), response.m_scanFolders.begin(), response.m_scanFolders.end());
            return !response.m_scanFolders.empty();
        }

        bool AssetSystemComponent::GetAssetSafeFolders(AZStd::vector<AZStd::string>& assetSafeFolders)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            GetAssetSafeFoldersRequest request;
            GetAssetSafeFoldersResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetScanFolders request");
                return false;
            }

            assetSafeFolders.insert(assetSafeFolders.end(), response.m_assetSafeFolders.begin(), response.m_assetSafeFolders.end());
            return !response.m_assetSafeFolders.empty();
        }

        bool AssetSystemComponent::IsAssetPlatformEnabled(const char* platform)
        {
            AssetProcessorPlatformStatusRequest request;
            request.m_platform = platform;

            AssetProcessorPlatformStatusResponse response;
            if (!SendRequest(request, response))
            {
                return false;
            }

            return response.m_isPlatformEnabled;
        }

        int AssetSystemComponent::GetPendingAssetsForPlatform(const char* platform)
        {
            AssetProcessorPendingPlatformAssetsRequest request;
            request.m_platform = platform;

            AssetProcessorPendingPlatformAssetsResponse response;
            if (!SendRequest(request, response))
            {
                return -1;
            }

            return response.m_numberOfPendingJobs;
        }

        bool AssetSystemComponent::ClearFingerprintForAsset(const AZStd::string& sourcePath)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            AssetFingerprintClearRequest request(sourcePath);

            AssetFingerprintClearResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error(
                    "Editor",
                    false, "ClearFingerprintForAsset request failed for source asset: %.*s", AZ_STRING_ARG(sourcePath));
                return false;
            }
            return response.m_isSuccess;
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> AssetSystemComponent::GetAssetJobsInfo(const AZStd::string& path, const bool escalateJobs)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return AZ::Failure();
            }

            AssetJobsInfoRequest request(path);
            request.m_escalateJobs = escalateJobs;
            AssetJobsInfoResponse response;

            return SendAssetJobsRequest(request, response);
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> AssetSystemComponent::GetAssetJobsInfoByAssetID(const AZ::Data::AssetId& assetId, const bool escalateJobs, bool requireFencing = true)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return AZ::Failure();
            }

            AssetJobsInfoRequest request(assetId, escalateJobs, requireFencing);
            AssetJobsInfoResponse response;

            return SendAssetJobsRequest(request, response);
        }

        AZ::Outcome<JobStatus> AssetSystemComponent::GetAssetJobsStatusByJobKey(const AZStd::string& jobKey, const bool escalateJobs)
        {
            // Strategy
            // First find all jobInfos for the inputted jobkey.
            // if there is no jobKey than return missing.
            // Otherwise if even one job failed than return failed.
            // If none of the job failed than check to see whether any job status are queued or inprogress, if yes than return the appropriate status.
            // Otherwise return completed status

            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> result = AZ::Failure();
            result = GetAssetJobsInfoByJobKey(jobKey, escalateJobs);
            if (!result.IsSuccess())
            {
                return AZ::Failure();
            }

            AzToolsFramework::AssetSystem::JobInfoContainer& jobInfos = result.GetValue();

            if (!jobInfos.size())
            {
                return AZ::Success(JobStatus::Missing);
            }

            bool isAnyJobInProgress = false;
            for (const AzToolsFramework::AssetSystem::JobInfo& jobInfo : jobInfos)
            {
                if (jobInfo.m_status == JobStatus::Failed)
                {
                    return AZ::Success(JobStatus::Failed);
                }
                else if (jobInfo.m_status == JobStatus::Queued)
                {
                    return AZ::Success(JobStatus::Queued);
                }
                else if (jobInfo.m_status == JobStatus::InProgress)
                {
                    isAnyJobInProgress = true;
                }
            }

            if (isAnyJobInProgress)
            {
                return AZ::Success(JobStatus::InProgress);
            }

            return AZ::Success(JobStatus::Completed);
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> AssetSystemComponent::GetAssetJobsInfoByJobKey(const AZStd::string& jobKey, const bool escalateJobs)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return AZ::Failure();
            }

            AssetJobsInfoRequest request(jobKey);
            request.m_isSearchTermJobKey = true;
            request.m_escalateJobs = escalateJobs;
            AssetJobsInfoResponse response;

            return SendAssetJobsRequest(request, response);
        }

        AZ::Outcome<AZStd::string> AssetSystemComponent::GetJobLog(AZ::u64 jobrunkey)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return AZ::Failure();
            }

            AssetJobLogRequest request(jobrunkey);
            AssetJobLogResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetJobLog Info for jobrunkey: %llu", jobrunkey);
                return AZ::Failure();
            }

            if (response.m_isSuccess)
            {
                return AZ::Success(response.m_jobLog);
            }

            return AZ::Failure();
        }

        void AssetSystemComponent::SourceFileChanged(AZStd::string relativePath, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/)
        {
            m_assetSourceRelativePathToFullPathCache.erase(relativePath);
        }

        void AssetSystemComponent::SourceFileRemoved(AZStd::string relativePath, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/)
        {
            m_assetSourceRelativePathToFullPathCache.erase(relativePath);
        }

        void AssetSystemComponent::SourceFileFailed(AZStd::string relativePath, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/)
        {
            m_assetSourceRelativePathToFullPathCache.erase(relativePath);
        }

        void AssetSystemComponent::RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return;
            }

            AzFramework::AssetSystem::RegisterSourceAssetRequest request(assetType, assetFileFilter);

            if (!SendRequest(request))
            {
                AZ_Error("Editor", false, "Failed to send RegisterSourceAssetType request for asset type %s", assetType.ToString<AZStd::string>().c_str());
            }
        }

        void AssetSystemComponent::UnregisterSourceAssetType(const AZ::Data::AssetType& assetType)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return;
            }

            AzFramework::AssetSystem::UnregisterSourceAssetRequest request(assetType);

            if (!SendRequest(request))
            {
                AZ_Error("Editor", false, "Failed to send UnregisterSourceAssetType request for asset type %s", assetType.ToString<AZStd::string>().c_str());
            }
        }

    } // namespace AssetSystem
} // namespace AzToolsFramework
