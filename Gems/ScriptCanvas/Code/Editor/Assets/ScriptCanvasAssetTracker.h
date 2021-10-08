/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>

#include <GraphCanvas/Editor/EditorTypes.h>


namespace ScriptCanvasEditor
{
    class ScriptCanvasMemoryAsset;

    // This class tracks all things related to the assets that the Script Canvas editor
    // has in play. It also provides helper functionality to quickly getting asset information
    // from GraphCanvas
    //
    // The AssetTracker will be the ONLY allowed place to connect Script Canvas to the Asset System and any of its buses.
    //
    // The goal is to centralize all of the asset operations to a single place in order to simplify Script Canvas' 
    // interactions with the asset system as well as keeping any transient cache of information that is
    // only important while Script Canvas graphs are open.

    class AssetTracker
        : AssetTrackerRequestBus::Handler
        , Internal::MemoryAssetSystemNotificationBus::Handler
    {
    public:
        AssetTracker() = default;
        ~AssetTracker() = default;
        void Activate()
        {
            AssetTrackerRequestBus::Handler::BusConnect();
            Internal::MemoryAssetSystemNotificationBus::Handler::BusConnect();
        }

        void Deactivate()
        {
            Internal::MemoryAssetSystemNotificationBus::Handler::BusDisconnect();
            AssetTrackerRequestBus::Handler::BusDisconnect();
        }

        // AssetTrackerRequestBus
        AZ::Data::AssetId Create(AZStd::string_view assetAbsolutePath, AZ::Data::AssetType assetType, Callbacks::OnAssetCreatedCallback onAssetCreatedCallback) override;
        bool IsSaving(AZ::Data::AssetId assetId) const override;
        void Save(AZ::Data::AssetId assetId, Callbacks::OnSave onSaveCallback) override;
        void SaveAs(AZ::Data::AssetId assetId, const AZStd::string& path, Callbacks::OnSave onSaveCallback) override;
        bool Load(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType, Callbacks::OnAssetReadyCallback onAssetReadyCallback) override;
        void Close(AZ::Data::AssetId assetId) override;
        void CreateView(AZ::Data::AssetId assetId, QWidget* parent) override;
        void ClearView(AZ::Data::AssetId assetId) override;
        void UntrackAsset(AZ::Data::AssetId assetId) override;

        // Getters

        // Given the assetId it returns a reference to the in-memory asset, returns false if the asset is not tracked
        ScriptCanvasMemoryAsset::pointer GetAsset(AZ::Data::AssetId assetId) override;

        // Given a ScriptCanvas EntityId it will lookup the AssetId
        AZ::Data::AssetId GetAssetId(ScriptCanvas::ScriptCanvasId scriptCanvasSceneId) override;

        // Given a ScriptCanvas EntityId it will lookup the asset's type
        AZ::Data::AssetType GetAssetType(ScriptCanvas::ScriptCanvasId scriptCanvasSceneId) override;

        // Retrieves the ScriptCanvasEntity for a given asset
        ScriptCanvas::ScriptCanvasId GetScriptCanvasId(AZ::Data::AssetId assetId) override;

        // Retrieves the Graph Canvas scene Id for a given asset
        AZ::EntityId GetGraphCanvasId(AZ::EntityId scriptCanvasEntityId) override;
        ScriptCanvas::ScriptCanvasId GetScriptCanvasIdFromGraphId(AZ::EntityId graphId) override;
        ScriptCanvas::ScriptCanvasId GetGraphId(AZ::Data::AssetId assetId) override;
        Tracker::ScriptCanvasFileState GetFileState(AZ::Data::AssetId assetId) override;
        AZStd::string GetTabName(AZ::Data::AssetId assetId) override;
        ScriptCanvasEditor::ScriptCanvasAssetHandler* GetAssetHandlerForType(AZ::Data::AssetType assetType) override;
        void UpdateFileState(AZ::Data::AssetId assetId, Tracker::ScriptCanvasFileState state) override;

        AssetTrackerRequests::AssetList GetUnsavedAssets() override;
        AssetTrackerRequests::AssetList GetAssets() override;
        AssetTrackerRequests::AssetList GetAssetsIf(AZStd::function<bool(ScriptCanvasMemoryAsset::pointer asset)> pred = []() { return true; }) override;

        AZ::EntityId GetSceneEntityIdFromEditorEntityId(AZ::Data::AssetId assetId, AZ::EntityId editorEntityId) override;
        AZ::EntityId GetEditorEntityIdFromSceneEntityId(AZ::Data::AssetId assetId, AZ::EntityId sceneEntityId) override;
        //

    private:

        void SignalSaveComplete(const AZ::Data::AssetId& fileAssetId);

        // Verifies if an asset Id has been remapped, this happens when we save a new graph because the AssetId will
        // change on save, but there may be some UX elements still referring to the initial asset Id, this ensures
        // we are getting the right key into m_assetsInUse
        AZ::Data::AssetId CheckAssetId(AZ::Data::AssetId assetId) const;

        // Map of all the currently tracked in-memory assets
        using MemoryAssetMap = AZStd::unordered_map<AZ::Data::AssetId, ScriptCanvasMemoryAsset::pointer>;
        using MemoryAssetMapIterator = AZStd::pair<AZStd::list_iterator<AZStd::pair<AZ::Data::AssetId, ScriptCanvasMemoryAsset::pointer>>, bool>;

        AZStd::unordered_set<AZ::Data::AssetId> m_savingAssets;
        AZStd::unordered_set<AZ::Data::AssetId> m_queuedCloses;

        // Map of all assets being tracked
        MemoryAssetMap  m_assetsInUse;

        // When a graph has been saved to file, its Id will change but it may still have external references with the old Id, this maps the file Id to the in-memory Id
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::AssetId> m_remappedAsset;

        // Invoked when an asset is loaded from file and becomes ready
        Callbacks::OnAssetReadyCallback m_onAssetReadyCallback;

        // Internal::MemoryAssetNotificationBus
        void OnAssetReady(const ScriptCanvasMemoryAsset* asset) override;
        void OnAssetReloaded(const ScriptCanvasMemoryAsset* asset) override;
        void OnAssetSaved(const ScriptCanvasMemoryAsset* asset, bool isSuccessful) override;
        void OnAssetError(const ScriptCanvasMemoryAsset* asset) override;
    };
}
