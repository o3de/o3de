/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include "ScriptCanvasAssetTracker.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/Assets/ScriptCanvasMemoryAsset.h>

#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Asset/Functions/ScriptCanvasFunctionAsset.h>

#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include <AzCore/std/smart_ptr/make_shared.h>

namespace ScriptCanvasEditor
{
    /////////////////
    // AssetTracker
    /////////////////

    AZ::Data::AssetId AssetTracker::Create(AZStd::string_view assetAbsolutePath, AZ::Data::AssetType assetType, Callbacks::OnAssetCreatedCallback onAssetCreatedCallback)
    {
        AZ::Data::AssetId newAssetId = AZ::Uuid::CreateRandom();

        MemoryAssetMapIterator iterator = m_assetsInUse.emplace(newAssetId, AZStd::make_shared<ScriptCanvasMemoryAsset>());

        ScriptCanvasMemoryAsset::pointer memoryAsset = iterator.first->second;

        memoryAsset->Create(newAssetId, assetAbsolutePath, assetType, onAssetCreatedCallback);

        return newAssetId;
    }

    bool AssetTracker::IsSaving(AZ::Data::AssetId assetId) const
    {
        assetId = CheckAssetId(assetId);
        return m_savingAssets.find(assetId) != m_savingAssets.end();
    }

    void AssetTracker::Save(AZ::Data::AssetId assetId, Callbacks::OnSave onSaveCallback)
    {
        SaveAs(assetId, {}, onSaveCallback);
    }

    void AssetTracker::SaveAs(AZ::Data::AssetId assetId, const AZStd::string& path, Callbacks::OnSave onSaveCallback)
    {
        auto assetIter = m_assetsInUse.find(assetId);

        if (assetIter != m_assetsInUse.end())
        {
            auto onSave = [this, assetId, onSaveCallback](bool saveSuccess, AZ::Data::AssetPtr asset, AZ::Data::AssetId previousFileAssetId)
            {
                AZ::Data::AssetId signalId = assetId;
                AZ::Data::AssetId fileAssetId = asset->GetId();

                // If there is a previous file Id is valid, it means this is a save-as operation and we need to remap the tracking.
                if (previousFileAssetId.IsValid())
                {
                    if (saveSuccess)
                    {
                        fileAssetId = m_assetsInUse[assetId]->GetFileAssetId();
                        m_remappedAsset[asset->GetId()] = fileAssetId;

                        // Erase the asset first so the smart pointer can deal with it's things.
                        m_assetsInUse.erase(fileAssetId);

                        // Then perform the insert once we know nothing will attempt to delete this while we are operating on it.
                        m_assetsInUse[fileAssetId] = m_assetsInUse[assetId];
                        m_assetsInUse.erase(assetId);
                    }

                    m_savingAssets.erase(assetId);
                    m_savingAssets.insert(fileAssetId);

                    signalId = fileAssetId;

                    if (m_queuedCloses.erase(assetId))
                    {
                        m_queuedCloses.insert(fileAssetId);
                    }
                    
                    auto assetIter = m_assetsInUse.find(fileAssetId);

                    if (assetIter != m_assetsInUse.end())
                    {
                        AZStd::invoke(onSaveCallback, saveSuccess, m_assetsInUse[fileAssetId]->GetAsset().Get(), previousFileAssetId);
                    }
                    else
                    {
                        AZ_Error("ScriptCanvas", !saveSuccess, "Unable to find Memory Asset for Asset(%s)", fileAssetId.ToString<AZStd::string>().c_str());
                        AZStd::invoke(onSaveCallback, saveSuccess, asset, previousFileAssetId);
                    }
                }
                else
                {
                    if (saveSuccess)
                    {
                        // This should be the case when we get a save as from a newly created file.
                        //
                        // If we find the 'memory' asset id in the assets in use. This means this was a new file that was saved.
                        // To maintain all of the look-up stuff, we need to treat this like a remapping stage.
                        auto assetInUseIter = m_assetsInUse.find(assetId);
                        if (assetInUseIter != m_assetsInUse.end())
                        {
                            fileAssetId = assetInUseIter->second->GetFileAssetId();

                            if (assetId != fileAssetId)
                            {
                                m_remappedAsset[assetId] = fileAssetId;

                                m_assetsInUse.erase(fileAssetId);
                                m_assetsInUse[fileAssetId] = AZStd::move(assetInUseIter->second);
                                m_assetsInUse.erase(assetId);

                                m_savingAssets.erase(assetId);
                                m_savingAssets.insert(fileAssetId);

                                if (m_queuedCloses.erase(assetId))
                                {
                                    m_queuedCloses.insert(fileAssetId);
                                }
                            }
                        }
                        else
                        {
                            fileAssetId = CheckAssetId(fileAssetId);
                        }

                        signalId = fileAssetId;
                    }

                    if (onSaveCallback)
                    {
                        AZStd::invoke(onSaveCallback, saveSuccess, m_assetsInUse[signalId]->GetAsset().Get(), previousFileAssetId);
                    }

                    AssetTrackerNotificationBus::Broadcast(&AssetTrackerNotifications::OnAssetSaved, m_assetsInUse[signalId], saveSuccess);
                }

                SignalSaveComplete(signalId);
            };

            m_savingAssets.insert(assetId);
            assetIter->second->SaveAs(path, onSave);
        }
        else
        {
            AZ_Assert(false, "Cannot SaveAs into an existing AssetId");
        }
    }

    bool AssetTracker::Load(AZ::Data::AssetId fileAssetId, AZ::Data::AssetType assetType, Callbacks::OnAssetReadyCallback onAssetReadyCallback)
    {
        if (!fileAssetId.IsValid())
        {
            return false;
        }

        auto assetIter = m_assetsInUse.find(fileAssetId);

        if (assetIter != m_assetsInUse.end())
        {
            if (!assetIter->second->IsSourceInError())
            {
                if (onAssetReadyCallback)
                {
                    // The asset is already loaded and tracked
                    AZStd::invoke(onAssetReadyCallback, *m_assetsInUse[fileAssetId]);
                    AssetTrackerNotificationBus::Event(fileAssetId, &AssetTrackerNotifications::OnAssetReady, m_assetsInUse[fileAssetId]);
                }

                return true;
            }
            else
            {
                m_assetsInUse.erase(assetIter);
            }
        }

        m_assetsInUse[fileAssetId] = AZStd::make_shared<ScriptCanvasMemoryAsset>();

        m_onAssetReadyCallback = onAssetReadyCallback;

        auto onReady = [this, fileAssetId](ScriptCanvasMemoryAsset& asset)
        {
            m_remappedAsset[asset.GetId()] = fileAssetId;

            if (m_onAssetReadyCallback)
            {
                AZStd::invoke(m_onAssetReadyCallback, *m_assetsInUse[fileAssetId]);
            }
        };

        // If we failed to load the asset, signal back as much
        if (!m_assetsInUse[fileAssetId]->Load(fileAssetId, assetType, onReady))
        {
            m_assetsInUse.erase(fileAssetId);
            return false;
        }

        return true;
    }

    void AssetTracker::Close(AZ::Data::AssetId assetId)
    {
        assetId = CheckAssetId(assetId);

        if (m_assetsInUse.find(assetId) == m_assetsInUse.end())
        {
            return;
        }

        if (m_savingAssets.find(assetId) == m_savingAssets.end())
        {
            m_assetsInUse.erase(assetId);
        }
        else
        {
            m_queuedCloses.insert(assetId);
        }
    }

    void AssetTracker::ClearView(AZ::Data::AssetId assetId)
    {
        assetId = CheckAssetId(assetId);

        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            m_assetsInUse[assetId]->ClearView();
        }
    }

    void AssetTracker::UntrackAsset(AZ::Data::AssetId assetId)
    {
        assetId = CheckAssetId(assetId);
        m_assetsInUse.erase(assetId);
    }

    void AssetTracker::CreateView(AZ::Data::AssetId assetId, QWidget* parent)
    {
        assetId = CheckAssetId(assetId);

        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            m_assetsInUse[assetId]->CreateView(parent);
        }
    }

    ScriptCanvasMemoryAsset::pointer AssetTracker::GetAsset(AZ::Data::AssetId assetId)
    {
        if (!assetId.IsValid())
        {
            return nullptr;
        }

        assetId = CheckAssetId(assetId);

        if (m_assetsInUse.find(assetId) == m_assetsInUse.end())
        {
            auto asset = AZ::Data::AssetManager::Instance().FindAsset(assetId, AZ::Data::AssetLoadBehavior::Default);
            if (asset)
            {
                auto insertResult = m_assetsInUse.emplace(AZStd::make_pair(assetId, AZStd::make_shared<ScriptCanvasMemoryAsset>()));

                auto onReady = [this, assetId](ScriptCanvasMemoryAsset& asset)
                {
                    m_remappedAsset[asset.GetId()] = assetId;
                };
                
                insertResult.first->second->Load(assetId, AZ::Data::AssetType::CreateNull(), onReady);
                insertResult.first->second->ActivateAsset();
                
                return insertResult.first->second;
            }

            // Handle the weird case of saving out a file you can't load because of pathing issues.
            for (auto assetPair : m_assetsInUse)
            {
                if (assetPair.second->GetFileAssetId() == assetId)
                {
                    return assetPair.second;
                }
            }

            return nullptr;
        }

        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            return m_assetsInUse[assetId];
        }

        return nullptr;
    }

    AZ::Data::AssetId AssetTracker::GetAssetId(ScriptCanvas::ScriptCanvasId scriptCanvasSceneId)
    {
        for (const auto& asset : m_assetsInUse)
        {
            if (asset.second && asset.second->GetScriptCanvasId() == scriptCanvasSceneId)
            {
                AZ::Data::AssetId assetId = asset.second->GetAsset().GetId();
                return assetId;
            }
        }
        return {};
    }

    AZ::Data::AssetType AssetTracker::GetAssetType(ScriptCanvas::ScriptCanvasId scriptCanvasSceneId)
    {
        for (const auto& asset : m_assetsInUse)
        {
            if (asset.second && asset.second->GetScriptCanvasId() == scriptCanvasSceneId)
            {
                return asset.second->GetAssetType();
            }
        }
        return AZ::Data::AssetType::CreateNull();
    }


    ScriptCanvas::ScriptCanvasId AssetTracker::GetScriptCanvasId(AZ::Data::AssetId assetId)
    {
        assetId = CheckAssetId(assetId);

        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            return m_assetsInUse[assetId]->GetScriptCanvasId();
        }
        return ScriptCanvas::ScriptCanvasId();
    }

    AZ::EntityId AssetTracker::GetGraphCanvasId(AZ::EntityId scriptCanvasEntityId)
    {
        if (scriptCanvasEntityId.IsValid())
        {
            for (auto& asset : m_assetsInUse)
            {
                if (asset.second && asset.second->GetScriptCanvasId() == scriptCanvasEntityId)
                {
                    return asset.second->GetGraphId();
                }
            }
        }

        return AZ::EntityId();
    }

    ScriptCanvas::ScriptCanvasId AssetTracker::GetScriptCanvasIdFromGraphId(AZ::EntityId graphId)
    {
        if (graphId.IsValid())
        {
            for (auto& asset : m_assetsInUse)
            {
                if (asset.second && asset.second->GetGraphId() == graphId)
                {
                    return asset.second->GetScriptCanvasId();
                }
            }
        }
        return AZ::EntityId();
    }

    ScriptCanvas::ScriptCanvasId AssetTracker::GetGraphId(AZ::Data::AssetId assetId)
    {
        if (assetId.IsValid())
        {
            assetId = CheckAssetId(assetId);

            auto assetIter = m_assetsInUse.find(assetId);
            if (assetIter != m_assetsInUse.end())
            {
                return assetIter->second->GetGraphId();
            }
        }

        return ScriptCanvas::ScriptCanvasId();
    }

    AZStd::string AssetTracker::GetTabName(AZ::Data::AssetId assetId)
    {
        assetId = CheckAssetId(assetId);

        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            return m_assetsInUse[assetId]->GetTabName();
        }

        return {};
    }

    ScriptCanvasEditor::Tracker::ScriptCanvasFileState AssetTracker::GetFileState(AZ::Data::AssetId assetId)
    {
        assetId = CheckAssetId(assetId);

        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            return m_assetsInUse[assetId]->GetFileState();
        }

        return Tracker::ScriptCanvasFileState::INVALID;
    }

    void AssetTracker::SignalSaveComplete(const AZ::Data::AssetId& fileAssetId)
    {
        size_t eraseCount = m_savingAssets.erase(fileAssetId);

        if (eraseCount > 0)
        {
            if (m_queuedCloses.erase(fileAssetId) > 0)
            {
                Close(fileAssetId);
            }
        }
    }

    AZ::Data::AssetId AssetTracker::CheckAssetId(AZ::Data::AssetId assetId) const
    {
        auto remappedIter = m_remappedAsset.find(assetId);
        if (remappedIter != m_remappedAsset.end())
        {
            assetId = remappedIter->second;
        }
        return assetId;
    }

    ScriptCanvasEditor::ScriptCanvasAssetHandler* AssetTracker::GetAssetHandlerForType(AZ::Data::AssetType assetType)
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

    void AssetTracker::UpdateFileState(AZ::Data::AssetId assetId, Tracker::ScriptCanvasFileState state)
    {
        assetId = CheckAssetId(assetId);

        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            m_assetsInUse[assetId]->SetFileState(state);
        }
    }

    AssetTrackerRequests::AssetList AssetTracker::GetUnsavedAssets()
    {
        auto pred = [](ScriptCanvasMemoryAsset::pointer asset)
        {
            auto fileState = asset->GetFileState();
            if (fileState == Tracker::ScriptCanvasFileState::NEW || fileState == Tracker::ScriptCanvasFileState::MODIFIED)
            {
                return true;
            }
            return false;
        };

        return GetAssetsIf(pred);
    }

    AssetTrackerRequests::AssetList AssetTracker::GetAssets()
    {
        return GetAssetsIf([](ScriptCanvasMemoryAsset::pointer) { return true; });
    }

    AssetTrackerRequests::AssetList AssetTracker::GetAssetsIf(AZStd::function<bool(ScriptCanvasMemoryAsset::pointer asset)> pred)
    {
        AZStd::vector<ScriptCanvasMemoryAsset::pointer> unsavedAssets;
        for (auto& assetIterator : m_assetsInUse)
        {
            auto testAsset = assetIterator.second;
            if (AZStd::invoke(pred, testAsset) == true)
            {
                unsavedAssets.push_back(testAsset);
            }
        }
        return unsavedAssets;
    }

    AZ::EntityId AssetTracker::GetSceneEntityIdFromEditorEntityId(AZ::Data::AssetId assetId, AZ::EntityId editorEntityId)
    {
        assetId = CheckAssetId(assetId);
        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            return m_assetsInUse[assetId]->GetSceneEntityIdFromEditorEntityId(editorEntityId);
        }

        return AZ::EntityId();
    }

    AZ::EntityId AssetTracker::GetEditorEntityIdFromSceneEntityId(AZ::Data::AssetId assetId, AZ::EntityId sceneEntityId)
    {
        assetId = CheckAssetId(assetId);
        if (m_assetsInUse.find(assetId) != m_assetsInUse.end())
        {
            return m_assetsInUse[assetId]->GetEditorEntityIdFromSceneEntityId(sceneEntityId);
        }

        return AZ::EntityId();
    }

    void AssetTracker::OnAssetReady(const ScriptCanvasMemoryAsset* asset)
    {
        AZ::Data::AssetId assetId = CheckAssetId(asset->GetId());

        auto assetInUseIter = m_assetsInUse.find(assetId);
        if (assetInUseIter != m_assetsInUse.end())
        {
            AssetTrackerNotificationBus::Broadcast(&AssetTrackerNotifications::OnAssetReady, assetInUseIter->second);
        }
    }

    void AssetTracker::OnAssetReloaded(const ScriptCanvasMemoryAsset* asset)
    {
        AZ::Data::AssetId assetId = CheckAssetId(asset->GetId());

        auto assetInUseIter = m_assetsInUse.find(assetId);
        if (assetInUseIter != m_assetsInUse.end())
        {
            AssetTrackerNotificationBus::Broadcast(&AssetTrackerNotifications::OnAssetReloaded, assetInUseIter->second);
        }
    }

    void AssetTracker::OnAssetSaved(const ScriptCanvasMemoryAsset* asset, bool isSuccessful)
    {
        AZ::Data::AssetId assetId = CheckAssetId(asset->GetId());

        auto assetInUseIter = m_assetsInUse.find(assetId);
        if (assetInUseIter != m_assetsInUse.end())
        {
            AssetTrackerNotificationBus::Broadcast(&AssetTrackerNotifications::OnAssetSaved, assetInUseIter->second, isSuccessful);
        }
    }

    void AssetTracker::OnAssetError(const ScriptCanvasMemoryAsset* asset)
    {
        AZ::Data::AssetId assetId = CheckAssetId(asset->GetId());

        auto assetInUseIter = m_assetsInUse.find(assetId);
        if (assetInUseIter != m_assetsInUse.end())
        {
            AssetTrackerNotificationBus::Broadcast(&AssetTrackerNotifications::OnAssetError, assetInUseIter->second);
        }
    }

}
