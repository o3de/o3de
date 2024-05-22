/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetRequestHandler.h"


#include <AzCore/Asset/AssetSerializer.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include "native/AssetManager/assetProcessorManager.h"
#include <QDir>
#include <QTimer>

using namespace AssetProcessor;

namespace
{
    static const uint32_t s_assetPath = AssetUtilities::ComputeCRC32Lowercase("assetPath");
}

AssetRequestHandler::AssetRequestLine::AssetRequestLine(
    QString platform, QString searchTerm, const AZ::Data::AssetId& assetId, bool isStatusRequest, int searchType)
    : m_platform(platform)
    , m_searchTerm(searchTerm)
    , m_isStatusRequest(isStatusRequest)
    , m_assetId(assetId)
    , m_searchType(searchType)
{
}

bool AssetRequestHandler::AssetRequestLine::IsStatusRequest() const
{
    return m_isStatusRequest;
}

QString AssetRequestHandler::AssetRequestLine::GetPlatform() const
{
    return m_platform;
}
QString AssetRequestHandler::AssetRequestLine::GetSearchTerm() const
{
    return m_searchTerm;
}

int AssetRequestHandler::AssetRequestLine::GetSearchType() const
{
    return m_searchType;
}

const AZ::Data::AssetId& AssetRequestHandler::AssetRequestLine::GetAssetId() const
{
    return m_assetId;
}

QString AssetRequestHandler::AssetRequestLine::GetDisplayString() const
{
    if (m_assetId.IsValid())
    {
        return QString::fromUtf8(m_assetId.ToString<AZStd::string>().c_str());
    }
    return m_searchTerm;
}

int AssetRequestHandler::GetNumOutstandingAssetRequests() const
{
    return m_pendingAssetRequests.size();
}

namespace
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzFramework::AssetSystem;

    //! utility function - splits a string into lines and outputs them to the console at the same time as a trace.
    void ParseToLines(AZStd::vector<AZStd::string>& lines, const AZStd::string& text)
    {
        AzFramework::StringFunc::TokenizeVisitor(
            text,
            [&lines](AZStd::string line)
            {
                lines.push_back(line);
                AZ::Debug::Trace::Instance().Output(AssetProcessor::ConsoleChannel, (line + "\n").c_str());
            },
            "\n");
    }

    // generic version of BuildFailure, generally assumes that the failure type is a string.
    template<typename T> 
    void BuildFailure(const T& failure,  AZStd::vector<AZStd::string>& lines)
    {
       ParseToLines(lines, failure);
    }

    // specialized version of BuildFailure, for when the failure type is a MoveFailure, the string will be in m_reason
    template<> 
    void BuildFailure(const MoveFailure& failure,  AZStd::vector<AZStd::string>& lines)
    {
        ParseToLines(lines, failure.m_reason);
    }

    // Build a report based on the result of an Asset Change Request and echo to the console.
    // The expected output is a list of strings in the 'lines' variable.
    // The expected input is a result of an Asset Change Report Request function below
    template<typename T>
    void BuildReport(AssetProcessor::ISourceFileRelocation* relocationInterface, T& result, AZStd::vector<AZStd::string>& lines)
    {
        if (result.IsSuccess())
        {
            AssetProcessor::RelocationSuccess success = result.TakeValue();

            // The report can be too long for the AZ_Printf buffer, so split it into individual lines
            AZStd::string report = relocationInterface->BuildChangeReport(success.m_relocationContainer, success.m_updateTasks);
            ParseToLines(lines, report);
        }
        else
        {
            BuildFailure(result.GetError(), lines);
        }
    }

    AssetChangeReportResponse HandleAssetChangeReportRequest(MessageData<AssetChangeReportRequest> messageData)
    {
        AZStd::vector<AZStd::string> lines;
        bool success = false;

        auto* relocationInterface = AZ::Interface<AssetProcessor::ISourceFileRelocation>::Get();
        if (relocationInterface)
        {
            switch (messageData.m_message->m_type)
            {
            case AssetChangeReportRequest::ChangeType::CheckMove:
                {
                    auto resultCheck = relocationInterface->Move(
                        messageData.m_message->m_fromPath,
                        messageData.m_message->m_toPath,
                        RelocationParameters_PreviewOnlyFlag | RelocationParameters_AllowDependencyBreakingFlag |
                            RelocationParameters_UpdateReferencesFlag | RelocationParameters_AllowNonDatabaseFilesFlag);

                    BuildReport(relocationInterface, resultCheck, lines);
                    success = resultCheck.IsSuccess();
                    break;
                }
            case AssetChangeReportRequest::ChangeType::Move:
                {
                    auto* metadataUpdates = AZ::Interface<AssetProcessor::IMetadataUpdates>::Get();
                    AZ_Assert(metadataUpdates, "Programmer Error - IMetadataUpdates interface is not available.");

                    metadataUpdates->PrepareForFileMove(messageData.m_message->m_fromPath.c_str(), messageData.m_message->m_toPath.c_str());

                    auto resultMove = relocationInterface->Move(
                        messageData.m_message->m_fromPath,
                        messageData.m_message->m_toPath,
                        RelocationParameters_AllowDependencyBreakingFlag | RelocationParameters_UpdateReferencesFlag |
                            RelocationParameters_AllowNonDatabaseFilesFlag);

                    BuildReport(relocationInterface, resultMove, lines);
                    success = resultMove.IsSuccess();
                    break;
                }
            case AssetChangeReportRequest::ChangeType::CheckDelete:
                {
                    auto flags = RelocationParameters_PreviewOnlyFlag | RelocationParameters_AllowDependencyBreakingFlag |
                        RelocationParameters_AllowNonDatabaseFilesFlag;
                    if (messageData.m_message->m_isFolder)
                    {
                        flags |= RelocationParameters_RemoveEmptyFoldersFlag;
                    }
                    auto resultCheck = relocationInterface->Delete(messageData.m_message->m_fromPath, flags);

                    BuildReport(relocationInterface, resultCheck, lines);
                    success = resultCheck.IsSuccess();
                    break;
                }
            case AssetChangeReportRequest::ChangeType::Delete:
                {
                    int flags = RelocationParameters_AllowDependencyBreakingFlag | RelocationParameters_AllowNonDatabaseFilesFlag;
                    if (messageData.m_message->m_isFolder)
                    {
                        flags |= RelocationParameters_RemoveEmptyFoldersFlag;
                    }
                    auto resultDelete = relocationInterface->Delete(messageData.m_message->m_fromPath, flags);

                    BuildReport(relocationInterface, resultDelete, lines);
                    success = resultDelete.IsSuccess();
                    break;
                }
            }
        }
        return AssetChangeReportResponse(lines, success);
    }

    GetFullSourcePathFromRelativeProductPathResponse HandleGetFullSourcePathFromRelativeProductPathRequest(MessageData<GetFullSourcePathFromRelativeProductPathRequest> messageData)
    {
        bool fullPathFound = false;
        AZStd::string fullSourcePath;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, messageData.m_message->m_relativeProductPath, fullSourcePath);

        if (!fullPathFound)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Could not find full source path from the relative product path (%s).\n", messageData.m_message->m_relativeProductPath.c_str());
        }

        return GetFullSourcePathFromRelativeProductPathResponse(fullPathFound, fullSourcePath);
    }

    GetRelativeProductPathFromFullSourceOrProductPathResponse HandleGetRelativeProductPathFromFullSourceOrProductPathRequest(MessageData<GetRelativeProductPathFromFullSourceOrProductPathRequest> messageData)
    {
        bool relPathFound = false;
        AZStd::string relProductPath;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(relPathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, messageData.m_message->m_sourceOrProductPath, relProductPath);

        if (!relPathFound)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Could not find relative product path for the source file (%s).", messageData.m_message->m_sourceOrProductPath.c_str());
        }

        return GetRelativeProductPathFromFullSourceOrProductPathResponse(relPathFound, relProductPath);
    }

    GenerateRelativeSourcePathResponse HandleGenerateRelativeSourcePathRequest(
        MessageData<GenerateRelativeSourcePathRequest> messageData)
    {
        bool relPathFound = false;
        AZStd::string relPath;
        AZStd::string watchFolder;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            relPathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
            messageData.m_message->m_sourcePath, relPath, watchFolder);

        if (!relPathFound)
        {
            AZ_TracePrintf(
                AssetProcessor::ConsoleChannel, "Could not find relative source path for the source file (%s).",
                messageData.m_message->m_sourcePath.c_str());
        }

        return GenerateRelativeSourcePathResponse(relPathFound, relPath, watchFolder);
    }

    SourceAssetInfoResponse HandleSourceAssetInfoRequest(MessageData<SourceAssetInfoRequest> messageData)
    {
        SourceAssetInfoResponse response;

        if (messageData.m_message->m_assetId.IsValid())
        {
            AZStd::string rootFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(response.m_found, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID, messageData.m_message->m_assetId.m_guid, response.m_assetInfo, rootFolder);

            if (response.m_found)
            {
                response.m_assetInfo.m_assetId.m_subId = messageData.m_message->m_assetId.m_subId;
                response.m_assetInfo.m_assetType = messageData.m_message->m_assetType;
                response.m_rootFolder = rootFolder.c_str();
            }
            else
            {
                response.m_assetInfo.m_assetId.SetInvalid();
            }
        }
        else if (!messageData.m_message->m_assetPath.empty())
        {
            AZStd::string rootFolder;
            // its being asked for via path instead of ID.  slightly different call.
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(response.m_found, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, messageData.m_message->m_assetPath.c_str(), response.m_assetInfo, rootFolder);
            response.m_rootFolder = rootFolder.c_str();
        }
        // note that in the case of an invalid request, response is defaulted to false for m_found, so there is no need to
        // populate the response in that case.

        return response;
    }

    SourceAssetProductsInfoResponse HandleSourceAssetProductsInfoRequest(MessageData<SourceAssetProductsInfoRequest> messageData)
    {
        SourceAssetProductsInfoResponse response;
        if (messageData.m_message->m_assetId.IsValid())
        {
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(response.m_found, &AssetSystemRequest::GetAssetsProducedBySourceUUID,
                messageData.m_message->m_assetId.m_guid, response.m_productsAssetInfo);
        }

        // note that in the case of an invalid request, response is defaulted to false for m_found, so there is no need to
        // populate the response in that case.

        return response;
    }

    GetScanFoldersResponse HandleGetScanFoldersRequest(MessageData<GetScanFoldersRequest> messageData)
    {
        bool success = true;
        AZStd::vector<AZStd::string> scanFolders;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetScanFolders, scanFolders);

        if (!success)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Could not acquire a list of scan folders from the database.");
        }

        return GetScanFoldersResponse(move(scanFolders));
    }

    GetAssetSafeFoldersResponse HandleGetAssetSafeFoldersRequest(MessageData<GetAssetSafeFoldersRequest> messageData)
    {
        bool success = true;
        AZStd::vector<AZStd::string> assetSafeFolders;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetSafeFolders, assetSafeFolders);

        if (!success)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Could not acquire a list of asset safe folders from the database.");
        }

        return GetAssetSafeFoldersResponse(move(assetSafeFolders));
    }

    void HandleRegisterSourceAssetRequest(MessageData<RegisterSourceAssetRequest> messageData)
    {
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, messageData.m_message->m_assetType, messageData.m_message->m_assetFileFilter.c_str());
    }

    void HandleUnregisterSourceAssetRequest(MessageData<UnregisterSourceAssetRequest> messageData)
    {
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::UnregisterSourceAssetType, messageData.m_message->m_assetType);
    }

    AssetInfoResponse HandleAssetInfoRequest(MessageData<AssetInfoRequest> messageData)
    {
        AssetInfoResponse response;

        if (messageData.m_message->m_assetId.IsValid())
        {
            AZStd::string rootFilePath;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(response.m_found, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetInfoById,
                messageData.m_message->m_assetId, messageData.m_message->m_assetType, messageData.m_message->m_platformName, response.m_assetInfo, rootFilePath);
            response.m_rootFolder = rootFilePath;
        }
        else if (!messageData.m_message->m_assetPath.empty())
        {
            bool autoRegisterIfNotFound = false;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(response.m_assetInfo.m_assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, messageData.m_message->m_assetPath.c_str(), AZ::Data::s_invalidAssetType, autoRegisterIfNotFound);
            response.m_found = response.m_assetInfo.m_assetId.IsValid();
        }

        return response;
    }

    AssetDependencyInfoResponse HandleAssetDependencyInfoRequest(MessageData<AssetDependencyInfoRequest> messageData)
    {
        using namespace AzFramework::AssetSystem;

        AssetDependencyInfoResponse response;

        if (messageData.m_message->m_assetId.IsValid())
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure(AZStd::string());

            // Call the appropriate AssetCatalog API based on the type of dependencies requested.
            switch (messageData.m_message->m_dependencyType)
            {
            case AssetDependencyInfoRequest::DependencyType::DirectDependencies:
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(result,
                        &AZ::Data::AssetCatalogRequestBus::Events::GetDirectProductDependencies, messageData.m_message->m_assetId);
                break;
            case AssetDependencyInfoRequest::DependencyType::AllDependencies:
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(result,
                        &AZ::Data::AssetCatalogRequestBus::Events::GetAllProductDependencies, messageData.m_message->m_assetId);
                break;
            case AssetDependencyInfoRequest::DependencyType::LoadBehaviorDependencies:
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(result,
                    &AZ::Data::AssetCatalogRequestBus::Events::GetLoadBehaviorProductDependencies,
                    messageData.m_message->m_assetId, response.m_noloadSet, response.m_preloadAssetList);
                break;
            }

            // Decompose the AZ::Outcome into separate variables, since AZ::Outcome is not a serializable type.
            response.m_found = result.IsSuccess();
            if (response.m_found)
            {
                response.m_dependencies = result.GetValue();
            }
            else
            {
                response.m_errorString = result.GetError();
            }
        }
        else
        {
            response.m_found = false;
            response.m_errorString.assign("Invalid Asset Id");
        }

        return response;
    }
}

void AssetRequestHandler::HandleRequestEscalateAsset(MessageData<RequestEscalateAsset> messageData)
{
    if (!messageData.m_message->m_assetUuid.IsNull())
    {
        // search by UUID is preferred.
        Q_EMIT RequestEscalateAssetByUuid(messageData.m_platform, messageData.m_message->m_assetUuid);
    }
    else if (!messageData.m_message->m_searchTerm.empty())
    {
        // fall back to search term.
        Q_EMIT RequestEscalateAssetBySearchTerm(messageData.m_platform, QString::fromUtf8(messageData.m_message->m_searchTerm.c_str()));
    }
    else
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Invalid RequestEscalateAsset.  Both the search term and uuid are empty/null\n");
    }
}

bool AssetRequestHandler::InvokeHandler(MessageData<AzFramework::AssetSystem::BaseAssetProcessorMessage> messageData)
{
    // This function checks to see whether the incoming message is either one of those request, which require decoding the type of message
    // and then invoking the appropriate EBUS handler. If the message is not one of those type than it checks to see whether some one has
    // registered a request handler for that message type and then invokes it.

    using namespace AzFramework::AssetSystem;

    {
        auto located = m_requestRouter.m_messageHandlers.find(messageData.m_message->GetMessageType());

        if(located != m_requestRouter.m_messageHandlers.end())
        {
            located->second(messageData);
            return false;
        }

        AZ_Warning(AssetProcessor::DebugChannel, false, "OnNewIncomingRequest: Message Handler not found for message type %d, ignoring."
            "  Make sure to register new messages with IRequestRouter::RegisterMessageHandler", messageData.m_message->GetMessageType());
        return true;
    }
}

void AssetRequestHandler::ProcessAssetRequest(MessageData<RequestAssetStatus> messageData)
{
    if ((messageData.m_message->m_searchTerm.empty())&&(!messageData.m_message->m_assetId.IsValid()))
    {
        AZ_Info(AssetProcessor::DebugChannel, "Failed to decode incoming RequestAssetStatus - both path and uuid is empty\n");
        SendAssetStatus(messageData.m_key, RequestAssetStatus::MessageType, AssetStatus_Unknown);
        return;
    }
    AssetRequestLine newLine(messageData.m_platform, QString::fromUtf8(messageData.m_message->m_searchTerm.c_str()), messageData.m_message->m_assetId, messageData.m_message->m_isStatusRequest, messageData.m_message->m_searchType);
    AZ_Info(AssetProcessor::DebugChannel, "GetAssetStatus/CompileAssetSync: %s.\n", newLine.GetDisplayString().toUtf8().constData());

    QString assetPath = QString::fromUtf8(messageData.m_message->m_searchTerm.c_str());  // utf8-decode just once here, reuse below
    m_pendingAssetRequests.insert(messageData.m_key, newLine);
    Q_EMIT RequestCompileGroup(messageData.m_key, messageData.m_platform, assetPath, messageData.m_message->m_assetId, messageData.m_message->m_isStatusRequest, messageData.m_message->m_searchType);
}

void AssetRequestHandler::OnCompileGroupCreated(NetworkRequestID groupID, AssetStatus status)
{
    using namespace AzFramework::AssetSystem;
    auto located = m_pendingAssetRequests.find(groupID);

    if (located == m_pendingAssetRequests.end())
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "OnCompileGroupCreated: No such asset group found, ignoring.\n");
        return;
    }

    if (status == AssetStatus_Unknown)
    {
        // if this happens it means we made an async request and got a response from the build queue that no such thing
        // exists in the queue.  It might still be a valid asset - for example, it may have already finished compiling and thus
        // won't be in the queue.  To cover this we also make a request to the asset manager here (its also async)
        Q_EMIT RequestAssetExists(groupID, located.value().GetPlatform(), located.value().GetSearchTerm(), located.value().GetAssetId(), located.value().GetSearchType());
    }
    else
    {
        // if its a status request, return it immediately and then remove it.
        if (located.value().IsStatusRequest())
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "GetAssetStatus: Responding with status of: %s\n", located.value().GetDisplayString().toUtf8().constData());
            SendAssetStatus(groupID, RequestAssetStatus::MessageType, status);
            m_pendingAssetRequests.erase(located);
        }
        // if its not a status request then we'll wait for OnCompileGroupFinished before responding.
    }
}

void AssetRequestHandler::OnCompileGroupFinished(NetworkRequestID groupID, AssetStatus status)
{
    auto located = m_pendingAssetRequests.find(groupID);

    if (located == m_pendingAssetRequests.end())
    {
        // this is okay to happen if its a status request.
        return;
    }

    // if the compile group finished, but the request was for a SPECIFIC asset, we have to take an extra step since
    // the compile group being finished just means the source file has compiled, doesn't necessarly mean that specific asset is emitted.
    if (located.value().GetAssetId().IsValid())
    {
        Q_EMIT RequestAssetExists(groupID, located.value().GetPlatform(), located.value().GetSearchTerm(), located.value().GetAssetId(), located.value().GetSearchType());
    }
    else
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Compile Group finished: %s.\n", located.value().GetDisplayString().toUtf8().constData());
        SendAssetStatus(groupID, RequestAssetStatus::MessageType, status);
        m_pendingAssetRequests.erase(located);
    }
}

//! Called from the outside in response to a RequestAssetExists.
void AssetRequestHandler::OnRequestAssetExistsResponse(NetworkRequestID groupID, bool exists)
{
    using namespace AzFramework::AssetSystem;
    auto located = m_pendingAssetRequests.find(groupID);

    if (located == m_pendingAssetRequests.end())
    {
        AZ_Info(AssetProcessor::DebugChannel, "OnRequestAssetExistsResponse: No such compile group found, ignoring.\n");
        return;
    }

    AZ_Info(AssetProcessor::DebugChannel, "GetAssetStatus / CompileAssetSync: Asset %s is %s.\n",
        located.value().GetDisplayString().toUtf8().constData(),
        exists ? "compiled already" : "missing" );

    SendAssetStatus(groupID, RequestAssetStatus::MessageType, exists ? AssetStatus_Compiled : AssetStatus_Missing);

    m_pendingAssetRequests.erase(located);
}

void AssetRequestHandler::SendAssetStatus(NetworkRequestID groupID, unsigned int /*type*/, AssetStatus status)
{
    ResponseAssetStatus resp;
    resp.m_assetStatus = status;
    AssetProcessor::ConnectionBus::Event(groupID.first, &AssetProcessor::ConnectionBus::Events::SendResponse, groupID.second, resp);
}

AssetRequestHandler::AssetRequestHandler()
{
    m_requestRouter.RegisterQueuedCallbackHandler(this, &AssetRequestHandler::ProcessAssetRequest);

    m_requestRouter.RegisterMessageHandler(&HandleGetFullSourcePathFromRelativeProductPathRequest);
    m_requestRouter.RegisterMessageHandler(&HandleGetRelativeProductPathFromFullSourceOrProductPathRequest);
    m_requestRouter.RegisterMessageHandler(&HandleGenerateRelativeSourcePathRequest);
    m_requestRouter.RegisterMessageHandler(&HandleSourceAssetInfoRequest);
    m_requestRouter.RegisterMessageHandler(&HandleSourceAssetProductsInfoRequest);
    m_requestRouter.RegisterMessageHandler(&HandleGetScanFoldersRequest);
    m_requestRouter.RegisterMessageHandler(&HandleGetAssetSafeFoldersRequest);
    m_requestRouter.RegisterMessageHandler(&HandleRegisterSourceAssetRequest);
    m_requestRouter.RegisterMessageHandler(&HandleUnregisterSourceAssetRequest);
    m_requestRouter.RegisterMessageHandler(&HandleAssetInfoRequest);
    m_requestRouter.RegisterMessageHandler(&HandleAssetDependencyInfoRequest);
    m_requestRouter.RegisterMessageHandler(&HandleAssetChangeReportRequest);

    m_requestRouter.RegisterMessageHandler(ToFunction(&AssetRequestHandler::HandleRequestEscalateAsset));
}

QString AssetRequestHandler::CreateFenceFile(unsigned int fenceId)
{
    QDir fenceDir;
    if (!AssetUtilities::ComputeFenceDirectory(fenceDir))
    {
        return QString();
    }

    QString fileName = QString("fenceFile~%1.%2").arg(fenceId).arg(FENCE_FILE_EXTENSION);
    QString fenceFileName = fenceDir.filePath(fileName);
    QFileInfo fileInfo(fenceFileName);

    if (!fileInfo.absoluteDir().exists())
    {
        // if fence dir does not exists ,than try to create it
        if (!fileInfo.absoluteDir().mkpath("."))
        {
            return QString();
        }
    }

    QFile fenceFile(fenceFileName);

    if (fenceFile.exists())
    {
        return QString();
    }

    bool result = fenceFile.open(QFile::WriteOnly);

    if (!result)
    {
        return QString();
    }

    fenceFile.close();
    return fileInfo.absoluteFilePath();
}

bool AssetRequestHandler::DeleteFenceFile(QString fenceFileName)
{
    return QFile::remove(fenceFileName);
}

void AssetRequestHandler::DeleteFenceFile_Retry(unsigned int fenceId, QString fenceFileName, NetworkRequestID key, AZStd::shared_ptr<BaseAssetProcessorMessage> message, QString platform, int retriesRemaining)
{
    if (DeleteFenceFile(fenceFileName))
    {
        // add an entry in map
        // We have successfully created and deleted the fence file, insert an entry for it in the pendingFenceRequest map
        // and return, we will only process this request once the APM indicates that it has detected the fence file
        m_pendingFenceRequestMap[fenceId] = AZStd::move(RequestInfo(key, AZStd::move(message), platform));
        return;
    }

    retriesRemaining--;

    if (retriesRemaining == 0)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "AssetProcessor was unable to delete the fence file");

        // send request to the appropriate handler with fencingfailed set to true and return
        InvokeHandler(MessageData(AZStd::move(message), key, platform, true));
    }
    else
    {
        auto deleteFenceFilefunctor = [this, fenceId, fenceFileName, key, message = AZStd::move(message), platform, retriesRemaining]() mutable
        {
            DeleteFenceFile_Retry(fenceId, fenceFileName, key, AZStd::move(message), platform, retriesRemaining);
        };

        QTimer::singleShot(100, this, AZStd::move(deleteFenceFilefunctor));
    }
}

void AssetRequestHandler::OnNewIncomingRequest(unsigned int connId, unsigned int serial, QByteArray payload, QString platform)
{
    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(serializeContext, "Unable to retrieve serialize context.");
    AZStd::shared_ptr<BaseAssetProcessorMessage> message{ AZ::Utils::LoadObjectFromBuffer<BaseAssetProcessorMessage>(payload.constData(), payload.size(), serializeContext) };

    if (!message)
    {
        AZ_Warning("Asset Request Handler", false, "OnNewIncomingRequest: Invalid object sent as network message to AssetRequestHandler.");
        return;
    }

    NetworkRequestID key(connId, serial);
    QString fenceFileName;
    if (message->RequireFencing())
    {
        bool successfullyCreatedFenceFile = false;
        int fenceID = 0;
        for (int idx = 0; idx < g_RetriesForFenceFile; ++idx)
        {
            fenceID = ++m_fenceId;
            fenceFileName = CreateFenceFile(fenceID);
            if (!fenceFileName.isEmpty())
            {
                successfullyCreatedFenceFile = true;
                break;
            }
        }

        if (!successfullyCreatedFenceFile)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "AssetProcessor was unable to create the fence file");
            // send request to the appropriate handler with fencingFailed set to true and return
            InvokeHandler(MessageData(AZStd::move(message), key, platform, true));
        }
        else
        {
            // if we are here it means that we were able to create the fence file, we will try to delete it now with a fixed number of retries
            DeleteFenceFile_Retry(fenceID, fenceFileName, key, AZStd::move(message), platform, g_RetriesForFenceFile);
        }
    }
    else
    {
        // If we are here it indicates that the request does not require fencing, we either call the required bus or invoke the handler directly
        InvokeHandler(MessageData(AZStd::move(message), key, platform));
    }
}

void AssetRequestHandler::OnFenceFileDetected(unsigned int fenceId)
{
    auto fenceRequestFound = m_pendingFenceRequestMap.find(fenceId);
    if (fenceRequestFound == m_pendingFenceRequestMap.end())
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "OnFenceFileDetected: Fence File Request not found, ignoring.\n");
        return;
    }

    InvokeHandler(MessageData(fenceRequestFound->second.m_message, fenceRequestFound->second.m_requestId, fenceRequestFound->second.m_platform));

    m_pendingFenceRequestMap.erase(fenceRequestFound);
}


