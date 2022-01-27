/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "ScriptCanvasMemoryAsset.h"
#include "ScriptCanvasUndoHelper.h"

#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <ScriptCanvas/Asset/ScriptCanvasAssetBase.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>

namespace ScriptCanvasEditor
{
    ScriptCanvasMemoryAsset::ScriptCanvasMemoryAsset()
        : m_sourceInError(false)
    {
        m_undoState = AZStd::make_unique<SceneUndoState>(this);
    }

    ScriptCanvasMemoryAsset::~ScriptCanvasMemoryAsset()
    {
        AssetTrackerRequestBus::Broadcast(&AssetTrackerRequests::UntrackAsset, m_inMemoryAssetId);
        AssetHelpers::PrintInfo(AZStd::string::format("ScriptCanvasMemoryAsset went out of scope and has been released and untracked: %s", m_absolutePath.c_str()).c_str());

        if (m_inMemoryAsset.IsReady() && !m_inMemoryAsset.Release())
        {
            // Something else is holding on to it
            AZ_Assert(false, "Unable to release in memory asset");
        }
    }

    const AZStd::string ScriptCanvasMemoryAsset::GetTabName() const
    {
        AZStd::string tabName;
        AzFramework::StringFunc::Path::GetFileName(m_absolutePath.c_str(), tabName);
        return tabName;
    }

    AZ::EntityId ScriptCanvasMemoryAsset::GetGraphId()
    {
        if (!m_graphId.IsValid())
        {
            EditorGraphRequestBus::EventResult(m_graphId, m_scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);
        }

        return m_graphId;
    }

    Tracker::ScriptCanvasFileState ScriptCanvasMemoryAsset::GetFileState() const
    {
        if (m_sourceRemoved)
        {
            return Tracker::ScriptCanvasFileState::SOURCE_REMOVED;
        }
        else
        {
            return m_fileState;
        }
    }

    void ScriptCanvasMemoryAsset::SetFileState(Tracker::ScriptCanvasFileState fileState)
    {
        m_fileState = fileState;

        SignalFileStateChanged();
    }

    void ScriptCanvasMemoryAsset::CloneTo(ScriptCanvasMemoryAsset& memoryAsset)
    {
        if (m_assetType == azrtti_typeid<ScriptCanvasAsset>())
        {
            AZ::Data::Asset<ScriptCanvasAsset> newAsset = Clone<ScriptCanvasAsset>();
            memoryAsset.m_inMemoryAsset = newAsset;
        }
        else
        {
            AZ_Assert(false, "Unsupported Script Canvas Asset Type");
        }

        memoryAsset.m_sourceAsset = m_sourceAsset;
    }

    void ScriptCanvasMemoryAsset::Create(AZ::Data::AssetId assetId, AZStd::string_view assetAbsolutePath, AZ::Data::AssetType assetType, Callbacks::OnAssetCreatedCallback onAssetCreatedCallback)
    {
        m_inMemoryAssetId = assetId;
        m_absolutePath = assetAbsolutePath;
        m_assetType = assetType;
        m_fileState = Tracker::ScriptCanvasFileState::NEW;

        ScriptCanvasAssetHandler* assetHandler = GetAssetHandlerForType(assetType);

        AZ::Data::AssetPtr asset = nullptr;
        if (assetType == azrtti_typeid<ScriptCanvasAsset>())
        {
            asset = assetHandler->CreateAsset(assetId, azrtti_typeid<ScriptCanvasAsset>());
        }

        m_inMemoryAsset = AZ::Data::Asset<AssetType>(asset, AZ::Data::AssetLoadBehavior::PreLoad);

        ActivateAsset();

        // For new assets, we directly set its status as "Ready" in order to make it usable.
        ScriptCanvas::ScriptCanvasAssetBusRequestBus::Event(assetId, &ScriptCanvas::ScriptCanvasAssetBusRequests::SetAsNewAsset);

        Internal::MemoryAssetSystemNotificationBus::Broadcast(&Internal::MemoryAssetSystemNotifications::OnAssetReady, this);

        AssetHelpers::PrintInfo("Newly created Script Canvas asset is now tracked: %s", AssetHelpers::AssetIdToString(assetId).c_str());

        if (onAssetCreatedCallback)
        {
            AZStd::invoke(onAssetCreatedCallback, *this);
        }
    }

    void ScriptCanvasMemoryAsset::Save(Callbacks::OnSave onSaveCallback)
    {
        if (m_fileState == Tracker::ScriptCanvasFileState::UNMODIFIED)
        {
            // The file hasn't changed, don't save it
            return;
        }

        SaveAs({}, onSaveCallback);
    }

    void ScriptCanvasMemoryAsset::SaveAs(const AZStd::string& path, Callbacks::OnSave onSaveCallback)
    {
        if (!path.empty())
        {
            m_saveAsPath = path;
        }
        else
        {
            m_saveAsPath = m_absolutePath;
        }

        AZ::Data::AssetStreamInfo streamInfo;
        streamInfo.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
        streamInfo.m_streamName = m_saveAsPath;

        if (!streamInfo.IsValid())
        {
            return;
        }

        bool sourceControlActive = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlActive, &AzToolsFramework::SourceControlConnectionRequests::IsActive);
        // If Source Control is active then use it to check out the file before saving otherwise query the file info and save only if the file is not read-only
        if (sourceControlActive)
        {
            AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                streamInfo.m_streamName.c_str(),
                true,
                [this, streamInfo, onSaveCallback](bool success, AzToolsFramework::SourceControlFileInfo info) { FinalizeAssetSave(success, info, streamInfo, onSaveCallback); }
            );
        }
        else
        {
            AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetFileInfo,
                streamInfo.m_streamName.c_str(),
                [this, streamInfo, onSaveCallback](bool success, AzToolsFramework::SourceControlFileInfo info) { FinalizeAssetSave(success, info, streamInfo, onSaveCallback); }
            );
        }
    }

    void ScriptCanvasMemoryAsset::Set(AZ::Data::AssetId fileAssetId)
    {
        Callbacks::OnAssetReadyCallback onAssetReady = [](ScriptCanvasMemoryAsset&) {};
        Load(fileAssetId, AZ::Data::AssetType::CreateNull(), onAssetReady);

        ActivateAsset();
    }

    bool ScriptCanvasMemoryAsset::Load(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType, Callbacks::OnAssetReadyCallback onAssetReadyCallback)
    {
        AZStd::string rootPath;
        AZ::Data::AssetInfo assetInfo = AssetHelpers::GetAssetInfo(assetId, rootPath);
        AzFramework::StringFunc::Path::Join(rootPath.c_str(), assetInfo.m_relativePath.c_str(), m_absolutePath);

        if (assetInfo.m_assetType.IsNull())
        {
            // Try to find the asset type from the source file asset
            assetInfo.m_assetType = AssetHelpers::GetAssetType(AZStd::string::format("%s/%s", rootPath.c_str(), assetInfo.m_relativePath.c_str()).c_str());
        }

        if (!assetType.IsNull() && assetInfo.m_assetType.IsNull())
        {
            assetInfo.m_assetType = assetType;
        }
        else
        {
            AZ_Assert(assetInfo.m_assetId.IsValid(), "Failed to get the asset info properly from the asset system");
        }

        SetFileAssetId(assetId);

        auto asset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvas::ScriptCanvasAssetBase>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        if (!asset || !asset.IsReady())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        }

        if (assetInfo.m_assetType == azrtti_typeid<ScriptCanvasAsset>())
        {
            m_inMemoryAsset = AZ::Data::AssetManager::Instance().GetAsset<ScriptCanvasAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);
        }

        if (m_inMemoryAsset)
        {
            m_inMemoryAsset.BlockUntilLoadComplete();

            m_sourceAsset = m_inMemoryAsset;

            m_assetType = m_inMemoryAsset->GetType();

            AZ_Assert(m_inMemoryAsset.GetId() == assetId, "The asset IDs must match");

            m_onAssetReadyCallback = onAssetReadyCallback;

            if (m_inMemoryAsset.IsReady())
            {
                OnAssetReady(m_inMemoryAsset);
            }
        }

        return !m_inMemoryAsset.IsError();
    }

    void ScriptCanvasMemoryAsset::ActivateAsset()
    {
        ScriptCanvas::ScriptCanvasAssetBase* assetData = m_inMemoryAsset.Get();
        AZ_Assert(assetData, "ActivateAsset should have a valid asset of type %s", AssetHelpers::AssetIdToString(azrtti_typeid<ScriptCanvas::ScriptCanvasAssetBase>()).c_str());

        if (assetData == nullptr)
        {
            return;
        }

        AZ::Entity* scriptCanvasEntity = assetData->GetScriptCanvasEntity();
        AZ_Assert(scriptCanvasEntity, "ActivateAsset should have a valid ScriptCanvas Entity");

        // Only activate the entity for assets that have been saved
        if (scriptCanvasEntity->GetState() == AZ::Entity::State::Constructed)
        {
            scriptCanvasEntity->Init();
        }

        if (scriptCanvasEntity->GetState() == AZ::Entity::State::Init)
        {
            scriptCanvasEntity->Activate();
        }

        const AZStd::string& assetPath = m_absolutePath;
        AZStd::string graphName;
        AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), graphName);

        if (!graphName.empty())
        {
            scriptCanvasEntity->SetName(graphName);
        }

        Graph* editorGraph = AZ::EntityUtils::FindFirstDerivedComponent<Graph>(scriptCanvasEntity);
        AZ_Assert(editorGraph, "Script Canvas entity must have a Graph component");

        if (editorGraph == nullptr)
        {
            return;
        }

        m_scriptCanvasId = editorGraph->GetScriptCanvasId();

        EditorGraphNotificationBus::Handler::BusDisconnect();
        EditorGraphNotificationBus::Handler::BusConnect(m_scriptCanvasId);
    }

    ScriptCanvasEditor::Widget::CanvasWidget* ScriptCanvasMemoryAsset::CreateView(QWidget* /*parent*/)
    {
        return nullptr;
    }

    void ScriptCanvasMemoryAsset::ClearView()
    {
        delete m_canvasWidget;
        m_canvasWidget = nullptr;
    }

    void ScriptCanvasMemoryAsset::UndoStackChange()
    {
        OnUndoStackChanged();
    }

    bool ScriptCanvasMemoryAsset::IsSourceInError() const
    {
        return m_sourceInError;
    }

    void ScriptCanvasMemoryAsset::OnGraphCanvasSceneDisplayed()
    {
        // We need to wait until this event in order the get the m_graphId which represents the GraphCanvas scene Id
        EditorGraphRequestBus::EventResult(m_graphId, m_scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);
        EditorGraphNotificationBus::Handler::BusDisconnect();
    }

    void ScriptCanvasMemoryAsset::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // If we've already cloned the memory asset, we don't want to do the start-up things again.
        if (m_inMemoryAsset->GetId() == m_sourceAsset.GetId())
        {
            AZStd::string rootPath;
            AZ::Data::AssetInfo assetInfo = AssetHelpers::GetAssetInfo(m_fileAssetId, rootPath);

            AZStd::string absolutePath;
            AzFramework::StringFunc::Path::Join(rootPath.c_str(), assetInfo.m_relativePath.c_str(), absolutePath);

            m_absolutePath = absolutePath;
            m_fileState = Tracker::ScriptCanvasFileState::UNMODIFIED;
            m_assetType = asset.GetType();

            // Keep the canonical asset's Id, we will need it when we want to save the asset back to file
            SetFileAssetId(asset.GetId());

            // The source file is ready, we need to make the an in-memory version of it.
            AZ::Data::AssetId inMemoryAssetId = AZ::Uuid::CreateRandom();

            m_sourceAsset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvas::ScriptCanvasAssetBase>(m_fileAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            m_inMemoryAsset = AZStd::move(CloneAssetData(inMemoryAssetId));

            AZ_Assert(m_inMemoryAsset, "Asset should have been successfully cloned.");
            AZ_Assert(m_inMemoryAsset.GetId() == inMemoryAssetId, "Asset Id should match to the newly created one");

            m_inMemoryAssetId = m_inMemoryAsset.GetId();

            ActivateAsset();

            if (m_onAssetReadyCallback)
            {
                AZStd::invoke(m_onAssetReadyCallback, *this);
            }
        }
        // Instead just update the source asset to the get the new asset to keep it in memory.
        else
        {
            m_sourceAsset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvas::ScriptCanvasAssetBase>(m_fileAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        }

        if (m_fileAssetId == asset.GetId())
        {
            Internal::MemoryAssetSystemNotificationBus::Broadcast(&Internal::MemoryAssetSystemNotifications::OnAssetReady, this);
        }
    }

    void ScriptCanvasMemoryAsset::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_fileAssetId == asset.GetId())
        {
            m_sourceInError = false;

            // Update our source asset information so we keep references alive.
            m_sourceAsset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvas::ScriptCanvasAssetBase>(m_fileAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

            // The source file was reloaded, but we have an in-memory version of it.
            // We need to handle this.
        }
        else
        {
            Internal::MemoryAssetSystemNotificationBus::Broadcast(&Internal::MemoryAssetSystemNotifications::OnAssetReloaded, this);
        }
    }

    void ScriptCanvasMemoryAsset::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_fileAssetId == asset.GetId())
        {
            m_sourceInError = true;

            if (m_onAssetReadyCallback)
            {
                AZStd::invoke(m_onAssetReadyCallback, *this);
            }
        }
        else
        {
            Internal::MemoryAssetSystemNotificationBus::Broadcast(&Internal::MemoryAssetSystemNotifications::OnAssetError, this);
        }
    }

    void ScriptCanvasMemoryAsset::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType)
    {
        if (m_fileAssetId == assetId)
        {
            AssetTrackerNotificationBus::Event(assetId, &AssetTrackerNotifications::OnAssetUnloaded, assetId, assetType);
        }
    }

    void ScriptCanvasMemoryAsset::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceAssetId)
    {
        // This updates the asset Id with the canonical assetId on SourceFileChanged

        // This occurs for new ScriptCanvas assets because before the SC asset is saved to disk, the asset database
        // has no asset Id associated with it, so this uses the supplied source path to find the asset Id registered 
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(scanFolder.data(), relativePath.data(), fullPath);
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, fullPath);

        SavingComplete(fullPath, sourceAssetId);
    }



    void ScriptCanvasMemoryAsset::SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId)
    {
        AZ_UNUSED(relativePath);
        AZ_UNUSED(scanFolder);

        if (m_fileAssetId == fileAssetId)
        {
            m_sourceRemoved = true;
            SignalFileStateChanged();
        }
    }

    void ScriptCanvasMemoryAsset::SourceFileFailed(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid)
    {

    }

    void ScriptCanvasMemoryAsset::SavingComplete(const AZStd::string& /*streamName*/, AZ::Uuid /*sourceAssetId*/)
    {
    }

    void ScriptCanvasMemoryAsset::FinalizeAssetSave(bool, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZ::Data::AssetStreamInfo& saveInfo, Callbacks::OnSave onSaveCallback)
    {
        m_onSaveCallback = onSaveCallback;

        AZStd::string normPath = saveInfo.m_streamName;
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, normPath);
        m_pendingSave.emplace_back(normPath);

        m_assetSaveFinalizer.Reset();
        m_assetSaveFinalizer.Start(this, fileInfo, saveInfo, onSaveCallback, AssetSaveFinalizer::OnCompleteHandler([](AZ::Data::AssetId /*assetId*/)
            {
            }));
    }

    AZ::Data::Asset<AZ::Data::AssetData> ScriptCanvasMemoryAsset::CloneAssetData(AZ::Data::AssetId newAssetId)
    {
        if (m_assetType == azrtti_typeid<ScriptCanvasAsset>())
        {
            return CloneAssetData<ScriptCanvasAsset>(newAssetId);
        }

        AZ_Assert(false, "The provides asset type is not supported as a valid Script Canvas memory asset");
        return AZ::Data::Asset<AZ::Data::AssetData>();
    }

    void ScriptCanvasMemoryAsset::OnUndoStackChanged()
    {
        UndoNotificationBus::Broadcast(&UndoNotifications::OnCanUndoChanged, m_undoState->m_undoStack->CanUndo());
        UndoNotificationBus::Broadcast(&UndoNotifications::OnCanRedoChanged, m_undoState->m_undoStack->CanRedo());
    }

    void ScriptCanvasMemoryAsset::SetFileAssetId(const AZ::Data::AssetId& /*fileAssetId*/)
    {

    }

    void ScriptCanvasMemoryAsset::SignalFileStateChanged()
    {
    }

    AZStd::string ScriptCanvasMemoryAsset::MakeTemporaryFilePathForSave(AZStd::string_view targetFilename)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "File IO is not initialized.");

        AZStd::string tempFilename;
        AzFramework::StringFunc::Path::GetFullFileName(targetFilename.data(), tempFilename);
        AZStd::string tempPath = AZStd::string::format("@usercache@/scriptcanvas/%s.temp", tempFilename.data());

        AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath{};
        fileIO->ResolvePath(tempPath.data(), resolvedPath.data(), resolvedPath.size());
        return resolvedPath.data();
    }

    ScriptCanvasAssetHandler* ScriptCanvasMemoryAsset::GetAssetHandlerForType(AZ::Data::AssetType assetType)
    {
        ScriptCanvasAssetHandler* assetHandler = nullptr;

        AZ::EBusAggregateResults<AZ::Data::AssetHandler*> foundAssetHandlers;
        ScriptCanvas::AssetRegistryRequestBus::BroadcastResult(foundAssetHandlers, &ScriptCanvas::AssetRegistryRequests::GetAssetHandler);

        for (auto handler : foundAssetHandlers.values)
        {
            if (handler != nullptr)
            {
                ScriptCanvasAssetHandler* theHandler = azrtti_cast<ScriptCanvasAssetHandler*>(handler);
                if (theHandler != nullptr && theHandler->GetAssetType() == assetType)
                {
                    assetHandler = theHandler;
                    break;
                }
            }
        }

        AZ_Assert(assetHandler, "The specified asset type does not have a registered asset handler.");
        return assetHandler;
    }

    AZ::EntityId ScriptCanvasMemoryAsset::GetEditorEntityIdFromSceneEntityId(AZ::EntityId sceneEntityId)
    {
        if (m_editorEntityIdMap.find(sceneEntityId) != m_editorEntityIdMap.end())
        {
            return m_editorEntityIdMap[sceneEntityId];
        }

        return AZ::EntityId();
    }

    AZ::EntityId ScriptCanvasMemoryAsset::GetSceneEntityIdFromEditorEntityId(AZ::EntityId editorEntityId)
    {
        for (auto mapEntry : m_editorEntityIdMap)
        {
            if (mapEntry.second == editorEntityId)
            {
                return mapEntry.first;
            }
        }

        return AZ::EntityId();
    }

    // AssetSaveFinalizer
    //////////////////////////////////////

    bool AssetSaveFinalizer::ValidateStatus(const AzToolsFramework::SourceControlFileInfo& /*fileInfo*/)
    {
        return true;
    }

    AssetSaveFinalizer::AssetSaveFinalizer()
        : m_inMemoryAsset(nullptr)
        , m_sourceAsset(nullptr)
        , m_saving(false)
        , m_fileAvailableForSave(false)
    {

    }

    AssetSaveFinalizer::~AssetSaveFinalizer()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void AssetSaveFinalizer::Start(ScriptCanvasMemoryAsset* sourceAsset, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZ::Data::AssetStreamInfo& saveInfo, Callbacks::OnSave onSaveCallback, OnCompleteHandler onComplete)
    {
        m_onCompleteHandler = onComplete;
        m_onCompleteHandler.Connect(m_onComplete);

        m_saveInfo = saveInfo;
        m_onSave = onSaveCallback;
        m_sourceAsset = sourceAsset;
        m_inMemoryAsset = sourceAsset->GetAsset().Get();
        m_assetType = sourceAsset->GetAssetType();

        if (!ValidateStatus(fileInfo))
        {
            return;
        }

        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
        AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(saveInfo.m_streamName.data());
        streamer->SetRequestCompleteCallback(flushRequest, [this]([[maybe_unused]] AZ::IO::FileRequestHandle request)
            {
                m_fileAvailableForSave = true;
            });
        streamer->QueueRequest(flushRequest);

        AZ::SystemTickBus::Handler::BusConnect();

        m_saving = true;
    }

    AZStd::string AssetSaveFinalizer::MakeTemporaryFilePathForSave(AZStd::string_view targetFilename)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "File IO is not initialized.");

        AZStd::string tempFilename;
        AzFramework::StringFunc::Path::GetFullFileName(targetFilename.data(), tempFilename);
        AZStd::string tempPath = AZStd::string::format("@usercache@/scriptcanvas/%s.temp", tempFilename.data());

        AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath{};
        fileIO->ResolvePath(tempPath.data(), resolvedPath.data(), resolvedPath.size());
        return resolvedPath.data();
    }

    void AssetSaveFinalizer::OnSystemTick()
    {
        if (m_fileAvailableForSave)
        {

            AZ::SystemTickBus::Handler::BusDisconnect();

            m_fileAvailableForSave = false;

            const AZStd::string tempPath = MakeTemporaryFilePathForSave(m_saveInfo.m_streamName);
            AZ::IO::FileIOStream stream(tempPath.data(), m_saveInfo.m_streamFlags);
            if (stream.IsOpen())
            {
                ScriptCanvasAssetHandler* assetHandler = nullptr;
                AssetTrackerRequestBus::BroadcastResult(assetHandler, &AssetTrackerRequests::GetAssetHandlerForType, m_assetType);
                AZ_Assert(assetHandler, "An asset handler must be found");

                bool savedSuccess;
                {
                    AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvasAssetHandler::SaveAssetData");

                    ScriptCanvasMemoryAsset cloneAsset;
                    m_sourceAsset->CloneTo(cloneAsset);

                    savedSuccess = assetHandler->SaveAssetData(cloneAsset.GetAsset(), &stream);
                }
                stream.Close();
                if (savedSuccess)
                {
                    AZ_PROFILE_SCOPE(ScriptCanvas, "AssetTracker::SaveAssetPostSourceControl : TempToTargetFileReplacement");

                    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                    const bool targetFileExists = fileIO->Exists(m_saveInfo.m_streamName.data());

                    bool removedTargetFile;
                    {
                        AZ_PROFILE_SCOPE(ScriptCanvas, "AssetTracker::SaveAssetPostSourceControl : TempToTargetFileReplacement : RemoveTarget");
                        removedTargetFile = fileIO->Remove(m_saveInfo.m_streamName.data());
                    }

                    if (targetFileExists && !removedTargetFile)
                    {
                        savedSuccess = false;
                    }
                    else
                    {
                        AZ_PROFILE_SCOPE(ScriptCanvas, "AssetTracker::SaveAssetPostSourceControl : TempToTargetFileReplacement : RenameTempFile");
                        AZ::IO::Result renameResult = fileIO->Rename(tempPath.data(), m_saveInfo.m_streamName.data());
                        if (!renameResult)
                        {
                            savedSuccess = false;
                        }
                    }
                }

                if (savedSuccess)
                {
                    AZ_TracePrintf("Script Canvas", "Script Canvas successfully saved as Asset \"%s\"", m_saveInfo.m_streamName.data());
                }

                m_onComplete.Signal(m_sourceAsset->GetId());

            }

            Reset();

        }

    }

    void AssetSaveFinalizer::Reset()
    {
        m_sourceAsset = nullptr;
        m_fileAssetId = {};
        m_onSave = nullptr;
        m_saving = false;
        m_inMemoryAsset = nullptr;
        m_saveInfo = {};
    }

}
