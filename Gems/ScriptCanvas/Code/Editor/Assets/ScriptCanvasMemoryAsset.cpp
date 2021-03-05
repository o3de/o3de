/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "precompiled.h"

#include "ScriptCanvasMemoryAsset.h"
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <ScriptCanvas/Asset/ScriptCanvasAssetBase.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>

namespace ScriptCanvasEditor
{
    ScriptCanvasMemoryAsset::ScriptCanvasMemoryAsset()
        : m_sourceInError(false)
        , m_triggerSaveCallback(false)
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

    void ScriptCanvasMemoryAsset::CloneTo(ScriptCanvasMemoryAsset& memoryAsset)
    {
        if (m_assetType == azrtti_typeid<ScriptCanvasAsset>())
        {
            AZ::Data::Asset<ScriptCanvasAsset> newAsset = Clone<ScriptCanvasAsset>();
            memoryAsset.m_inMemoryAsset = newAsset;            
        }
        else if (m_assetType == azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>())
        {
            AZ::Data::Asset<ScriptCanvas::ScriptCanvasFunctionAsset> newAsset = Clone<ScriptCanvas::ScriptCanvasFunctionAsset>();
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

        AZ::Data::Asset<AZ::Data::AssetData> asset;
        if (assetType == azrtti_typeid<ScriptCanvasAsset>())
        {
            asset = { assetHandler->CreateAsset(assetId, azrtti_typeid<ScriptCanvasAsset>()), AZ::Data::AssetLoadBehavior::Default };
        }
        else if (assetType == azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>())
        {
            asset = { assetHandler->CreateAsset(assetId, azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>()), AZ::Data::AssetLoadBehavior::Default };
        }

        m_inMemoryAsset = asset;

        ActivateAsset();

        // For new assets, we directly set its status as "Ready" in order to make it usable.
        ScriptCanvas::ScriptCanvasAssetBusRequestBus::Event(assetId, &ScriptCanvas::ScriptCanvasAssetBusRequests::SetAsNewAsset);

        Internal::MemoryAssetNotificationBus::Broadcast(&Internal::MemoryAssetNotifications::OnAssetReady, this);

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

        ScriptCanvas::ScriptCanvasDataRequestBus::Event(GetScriptCanvasId(), &ScriptCanvas::ScriptCanvasDataRequests::SetPrettyName, GetTabName().c_str());

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

        m_fileAssetId = assetId;

        auto asset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvasAsset>(assetId, m_sourceAsset.GetAutoLoadBehavior());
        if (!asset || !asset.IsReady())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        }

        if (assetInfo.m_assetType == azrtti_typeid<ScriptCanvasAsset>())
        {
            m_inMemoryAsset = AZ::Data::AssetManager::Instance().GetAsset<ScriptCanvasAsset>(assetId, m_sourceAsset.GetAutoLoadBehavior());            
        }
        else if (assetInfo.m_assetType == azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>())
        {
            m_inMemoryAsset = AZ::Data::AssetManager::Instance().GetAsset<ScriptCanvas::ScriptCanvasFunctionAsset>(assetId, m_sourceAsset.GetAutoLoadBehavior());            
        }

        m_sourceAsset = m_inMemoryAsset;

        m_assetType = m_inMemoryAsset.GetType();

        AZ_Assert(m_inMemoryAsset.GetId() == assetId, "The asset IDs must match");

        m_onAssetReadyCallback = onAssetReadyCallback;

        if (m_inMemoryAsset.IsReady())
        {
            OnAssetReady(m_inMemoryAsset);
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

        ScriptCanvas::ScriptCanvasDataRequestBus::Event(m_scriptCanvasId, &ScriptCanvas::ScriptCanvasDataRequests::SetPrettyName, GetTabName().c_str());

        EditorGraphNotificationBus::Handler::BusDisconnect();
        EditorGraphNotificationBus::Handler::BusConnect(m_scriptCanvasId);

        m_undoHelper = AZStd::make_unique<UndoHelper>(*this);
    }

    ScriptCanvasEditor::Widget::CanvasWidget* ScriptCanvasMemoryAsset::CreateView(QWidget* parent)
    {
        m_canvasWidget = new Widget::CanvasWidget(m_fileAssetId, parent);
        return m_canvasWidget;
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

    void ScriptCanvasMemoryAsset::OnSystemTick()
    {
        if (m_triggerSaveCallback && m_onSaveCallback)
        {
            AZ::SystemTickBus::Handler::BusDisconnect();

            m_onSaveCallback(false, m_inMemoryAsset, AZ::Data::AssetId());
            m_triggerSaveCallback = false;
        }
    }

    void ScriptCanvasMemoryAsset::OnGraphCanvasSceneDisplayed()
    {
        // We need to wait until this event in order the get the m_graphId which represents the GraphCanvas scene Id
        EditorGraphRequestBus::EventResult(m_graphId, m_scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);
        EditorGraphNotificationBus::Handler::BusDisconnect();
    }

    void ScriptCanvasMemoryAsset::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_fileAssetId);

        AZStd::string rootPath;
        AZ::Data::AssetInfo assetInfo = AssetHelpers::GetAssetInfo(m_fileAssetId, rootPath);

        AZStd::string absolutePath;
        AzFramework::StringFunc::Path::Join(rootPath.c_str(), assetInfo.m_relativePath.c_str(), absolutePath);

        m_absolutePath = absolutePath;
        m_fileState = Tracker::ScriptCanvasFileState::UNMODIFIED;
        m_assetType = asset.GetType();

        // Keep the canonical asset's Id, we will need it when we want to save the asset back to file
        m_fileAssetId = asset.GetId();

        // The source file is ready, we need to make the an in-memory version of it.
        AZ::Data::AssetId inMemoryAssetId = AZ::Uuid::CreateRandom();

        m_inMemoryAsset = AZStd::move(CloneAssetData(inMemoryAssetId));

        AZ_Assert(m_inMemoryAsset, "Asset should have been successfully cloned.");
        AZ_Assert(m_inMemoryAsset.GetId() == inMemoryAssetId, "Asset Id should match to the newly created one");

        m_inMemoryAssetId = m_inMemoryAsset.GetId();

        ActivateAsset();

        if (m_onAssetReadyCallback)
        {
            AZStd::invoke(m_onAssetReadyCallback, *this);
        }

        Internal::MemoryAssetNotificationBus::Broadcast(&Internal::MemoryAssetNotifications::OnAssetReady, this);
    }

    void ScriptCanvasMemoryAsset::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_fileAssetId == asset.GetId())
        {
            // The source file was reloaded, but we have an in-memory version of it.
            // We need to handle this.
        }
        else
        {
            AZ::Data::AssetId assetId = asset.GetId();
            Internal::MemoryAssetNotificationBus::Broadcast(&Internal::MemoryAssetNotifications::OnAssetReloaded, this);
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
            AZ::Data::AssetId assetId = asset.GetId();
            Internal::MemoryAssetNotificationBus::Broadcast(&Internal::MemoryAssetNotifications::OnAssetError, this);
        }
    }

    void ScriptCanvasMemoryAsset::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType)
    {
        AssetTrackerNotificationBus::Event(assetId, &AssetTrackerNotifications::OnAssetUnloaded, assetId, assetType);
    }

    void ScriptCanvasMemoryAsset::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceAssetId)
    {
        // This updates the asset Id with the canonical assetId on SourceFileChanged

        // This occurs for new ScriptCanvas assets because before the SC asset is saved to disk, the asset database
        // has no asset Id associated with it, so this uses the supplied source path to find the asset Id registered 
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(scanFolder.data(), relativePath.data(), fullPath);
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, fullPath);

        auto assetPathIdIt = AZStd::find(m_pendingSave.begin(), m_pendingSave.end(), fullPath);

        if (assetPathIdIt != m_pendingSave.end())
        {
            AZ::SystemTickBus::Handler::BusDisconnect();

            AZ::Data::AssetId previousFileAssetId;
            if (sourceAssetId != m_fileAssetId.m_guid)
            {
                previousFileAssetId = m_fileAssetId;

                // The source file has changed, store the AssetId to the canonical asset on file
                m_fileAssetId = sourceAssetId;
            }
            else if (!m_fileAssetId.IsValid())
            {
                m_fileAssetId = sourceAssetId;
            }

            m_formerGraphIdPair = AZStd::make_pair(m_scriptCanvasId, m_graphId);

            m_fileState = Tracker::ScriptCanvasFileState::UNMODIFIED;

            // Connect to the source asset's bus to monitor for situations we may need to handle
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_inMemoryAsset.GetId());

            m_pendingSave.erase(assetPathIdIt);

            if (m_onSaveCallback)
            {
                m_onSaveCallback(true, m_inMemoryAsset, previousFileAssetId);
            }
        }
    }

    void ScriptCanvasMemoryAsset::SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId)
    {
        AZ_UNUSED(relativePath);
        AZ_UNUSED(scanFolder);
        AZ_UNUSED(fileAssetId);
    }

    void ScriptCanvasMemoryAsset::SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(scanFolder.data(), relativePath.data(), fullPath);
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, fullPath);

        auto assetPathIdIt = AZStd::find(m_pendingSave.begin(), m_pendingSave.end(), fullPath);

        if (assetPathIdIt != m_pendingSave.end())
        {
            m_pendingSave.erase(assetPathIdIt);

            if (m_onSaveCallback)
            {
                m_onSaveCallback(false, m_inMemoryAsset, AZ::Data::AssetId());
            }
        }
    }

    void ScriptCanvasMemoryAsset::FinalizeAssetSave(bool, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZ::Data::AssetStreamInfo& saveInfo, Callbacks::OnSave onSaveCallback)
    {
        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileInfo.IsLockedByOther())
        {
            AZ_Error("Script Canvas", !fileInfo.IsLockedByOther(), "The file is already exclusively opened by another user: %s", fileInfo.m_filePath.data());
            AZStd::invoke(onSaveCallback, false, m_inMemoryAsset, m_fileAssetId);
            return;
        }
        else if (fileInfo.IsReadOnly() && fileIO->Exists(fileInfo.m_filePath.c_str()))
        {
            AZ_Error("Script Canvas", !fileInfo.IsReadOnly(), "File %s is read-only. It cannot be saved."
                " If this file is in perforce it may not have been checked out by the Source Control API.", fileInfo.m_filePath.data());
            AZStd::invoke(onSaveCallback, false, m_inMemoryAsset, m_fileAssetId);
            return;
        }

        AZStd::string normPath = saveInfo.m_streamName;
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, normPath);
        m_pendingSave.emplace_back(normPath);

        ScriptCanvasEditor::SystemRequestBus::Broadcast(&ScriptCanvasEditor::SystemRequests::AddAsyncJob, [this, saveInfo, onSaveCallback]()
            {
                // ScriptCanvas Asset must be saved to a temporary location as the FileWatcher will pick up the file immediately if it detects any changes
                // and attempt to reload it

                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

                const AZStd::string tempPath = MakeTemporaryFilePathForSave(saveInfo.m_streamName);
                AZ::IO::FileIOStream stream(tempPath.data(), saveInfo.m_streamFlags);
                if (stream.IsOpen())
                {
                    ScriptCanvasAssetHandler* assetHandler = nullptr;
                    AssetTrackerRequestBus::BroadcastResult(assetHandler, &AssetTrackerRequests::GetAssetHandlerForType, m_inMemoryAsset.GetType());
                    AZ_Assert(assetHandler, "An asset handler must be found for an asset of type %s", AssetHelpers::AssetIdToString(m_inMemoryAsset.GetId()).c_str());

                    bool savedSuccess;
                    {
                        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvasAssetHandler::SaveAssetData");
                        savedSuccess = assetHandler->SaveAssetData(m_inMemoryAsset, &stream);
                    }
                    stream.Close();
                    if (savedSuccess)
                    {
                        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "AssetTracker::SaveAssetPostSourceControl : TempToTargetFileReplacement");

                        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                        const bool targetFileExists = fileIO->Exists(saveInfo.m_streamName.data());

                        bool removedTargetFile;
                        {
                            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "AssetTracker::SaveAssetPostSourceControl : TempToTargetFileReplacement : RemoveTarget");
                            removedTargetFile = fileIO->Remove(saveInfo.m_streamName.data());
                        }

                        if (targetFileExists && !removedTargetFile)
                        {
                            savedSuccess = false;
                        }
                        else
                        {
                            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "AssetTracker::SaveAssetPostSourceControl : TempToTargetFileReplacement : RenameTempFile");
                            AZ::IO::Result renameResult = fileIO->Rename(tempPath.data(), saveInfo.m_streamName.data());
                            if (!renameResult)
                            {
                                savedSuccess = false;
                            }
                        }
                    }

                    // Store the onSave callback so that we can call it from the proper place
                    m_onSaveCallback = onSaveCallback;

                    if (savedSuccess)
                    {
                        AZStd::string watchFolder;
                        AZ::Data::AssetInfo assetInfo;
                        bool sourceInfoFound{};
                        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, saveInfo.m_streamName.data(), assetInfo, watchFolder);

                        if (sourceInfoFound)
                        {
                            AZ_TracePrintf("Script Canvas", "Script Canvas successfully saved as Asset \"%s\"", saveInfo.m_streamName.data());
                            m_absolutePath = m_saveAsPath;
                        }
                        else
                        {
                            AZ_TracePrintf("Script Canvas", "Script Canvas successfully saved as Asset \"%s\" but is outside of project scope and cannot be loaded.", saveInfo.m_streamName.data());
                            m_triggerSaveCallback = true;
                        }

                        m_saveAsPath.clear();
                    }
                }
            });

        // Because this is connecting from within the lambda, the this is necessary. Otherwise it 'connects' but won't actually trigger.
        AZ::SystemTickBus::Handler::BusConnect();
    }

    AZ::Data::Asset<AZ::Data::AssetData> ScriptCanvasMemoryAsset::CloneAssetData(AZ::Data::AssetId newAssetId)
    {
        if (m_assetType == azrtti_typeid<ScriptCanvasAsset>())
        {
            return CloneAssetData<ScriptCanvasAsset>(newAssetId);
        }
        else if (m_assetType == azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>())
        {
            return CloneAssetData<ScriptCanvas::ScriptCanvasFunctionAsset>(newAssetId);
        }

        AZ_Assert(false, "The provides asset type is not supported as a valid Script Canvas memory asset");
        return AZ::Data::Asset<AZ::Data::AssetData>();
    }

    void ScriptCanvasMemoryAsset::OnUndoStackChanged()
    {
        UndoNotificationBus::Broadcast(&UndoNotifications::OnCanUndoChanged, m_undoState->m_undoStack->CanUndo());
        UndoNotificationBus::Broadcast(&UndoNotifications::OnCanRedoChanged, m_undoState->m_undoStack->CanRedo());
    }

    AZStd::string ScriptCanvasMemoryAsset::MakeTemporaryFilePathForSave(AZStd::string_view targetFilename)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "File IO is not initialized.");

        AZStd::string tempFilename;
        AzFramework::StringFunc::Path::GetFullFileName(targetFilename.data(), tempFilename);
        AZStd::string tempPath = AZStd::string::format("@cache@/scriptcanvas/%s.temp", tempFilename.data());

        AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath{};
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(tempPath.data(), resolvedPath.data(), resolvedPath.size());
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

    ///////////////
    // UndoHelper
    ///////////////

    UndoHelper::UndoHelper(ScriptCanvasMemoryAsset& memoryAsset)
        : m_memoryAsset(memoryAsset)
    {
        UndoRequestBus::Handler::BusConnect(memoryAsset.GetScriptCanvasId());
    }

    UndoHelper::~UndoHelper()
    {
        UndoRequestBus::Handler::BusDisconnect();
    }

    ScriptCanvasEditor::UndoCache* UndoHelper::GetSceneUndoCache()
    {
        return m_memoryAsset.GetUndoState()->m_undoCache.get();
    }

    ScriptCanvasEditor::UndoData UndoHelper::CreateUndoData()
    {
        AZ::EntityId graphCanvasGraphId = m_memoryAsset.GetGraphId();
        ScriptCanvas::ScriptCanvasId scriptCanvasId = m_memoryAsset.GetScriptCanvasId();

        GraphCanvas::GraphModelRequestBus::Event(graphCanvasGraphId, &GraphCanvas::GraphModelRequests::OnSaveDataDirtied, graphCanvasGraphId);

        UndoData undoData;

        ScriptCanvas::GraphData* graphData{};
        ScriptCanvas::GraphRequestBus::EventResult(graphData, scriptCanvasId, &ScriptCanvas::GraphRequests::GetGraphData);

        const ScriptCanvas::VariableData* varData{};
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varData, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableDataConst);

        if (graphData && varData)
        {
            undoData.m_graphData = *graphData;
            undoData.m_variableData = *varData;

            EditorGraphRequestBus::EventResult(undoData.m_visualSaveData, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasSaveData);
        }

        return undoData;
    }

    void UndoHelper::BeginUndoBatch(AZStd::string_view label)
    {
        m_memoryAsset.GetUndoState()->BeginUndoBatch(label);
    }

    void UndoHelper::EndUndoBatch()
    {
        m_memoryAsset.GetUndoState()->EndUndoBatch();
    }

    void UndoHelper::AddUndo(AzToolsFramework::UndoSystem::URSequencePoint* sequencePoint)
    {
        if (SceneUndoState * sceneUndoState = m_memoryAsset.GetUndoState())
        {
            if (!sceneUndoState->m_currentUndoBatch)
            {
                sceneUndoState->m_undoStack->Post(sequencePoint);
            }
            else
            {
                sequencePoint->SetParent(sceneUndoState->m_currentUndoBatch);
            }
        }
    }

    void UndoHelper::AddGraphItemChangeUndo(AZStd::string_view undoLabel)
    {
        GraphItemChangeCommand* command = aznew GraphItemChangeCommand(undoLabel);
        command->Capture(m_memoryAsset, true);
        command->Capture(m_memoryAsset, false);
        AddUndo(command);
    }

    void UndoHelper::AddGraphItemAdditionUndo(AZStd::string_view undoLabel)
    {
        GraphItemAddCommand* command = aznew GraphItemAddCommand(undoLabel);
        command->Capture(m_memoryAsset, false);
        AddUndo(command);
    }

    void UndoHelper::AddGraphItemRemovalUndo(AZStd::string_view undoLabel)
    {
        GraphItemRemovalCommand* command = aznew GraphItemRemovalCommand(undoLabel);
        command->Capture(m_memoryAsset, true);
        AddUndo(command);
    }

    void UndoHelper::Undo()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

        SceneUndoState* sceneUndoState = m_memoryAsset.GetUndoState();
        if (sceneUndoState)
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when performing a redo operation");

            if (sceneUndoState->m_undoStack->CanUndo())
            {
                m_status = Status::InUndo;
                sceneUndoState->m_undoStack->Undo();
                m_status = Status::Idle;

                UpdateCache();
            }
        }
    }

    void UndoHelper::Redo()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

        SceneUndoState* sceneUndoState = m_memoryAsset.GetUndoState();
        if (sceneUndoState)
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when performing a redo operation");

            if (sceneUndoState->m_undoStack->CanRedo())
            {
                m_status = Status::InUndo;
                sceneUndoState->m_undoStack->Redo();
                m_status = Status::Idle;

                UpdateCache();
            }
        }
    }

    void UndoHelper::Reset()
    {
        if (SceneUndoState* sceneUndoState = m_memoryAsset.GetUndoState())
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when reseting the undo stack");
            sceneUndoState->m_undoStack->Reset();
        }
    }

    bool UndoHelper::IsIdle()
    {
        return m_status == Status::Idle;
    }

    bool UndoHelper::IsActive()
    {
        return m_status != Status::Idle;
    }

    bool UndoHelper::CanUndo() const
    {
        return m_memoryAsset.GetUndoState()->m_undoStack->CanUndo();
    }

    bool UndoHelper::CanRedo() const
    {
        return m_memoryAsset.GetUndoState()->m_undoStack->CanRedo();
    }

    void UndoHelper::UpdateCache()
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId = m_memoryAsset.GetScriptCanvasId();

        UndoCache* undoCache = nullptr;
        UndoRequestBus::EventResult(undoCache, scriptCanvasId, &UndoRequests::GetSceneUndoCache);

        if (undoCache)
        {
            undoCache->UpdateCache(scriptCanvasId);
        }
    }

}
