/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIO.h>

#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerDefinitions.h>

#include <ScriptCanvas/Asset/ScriptCanvasAssetBase.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>

#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <Editor/View/Widgets/CanvasWidget.h>
#include <Editor/Undo/ScriptCanvasUndoManager.h>
#include <ScriptCanvas/Bus/RequestBus.h>


namespace ScriptCanvasEditor
{

    class ScriptCanvasAssetHandler;

    template <typename BaseAssetType>
    class MemoryAsset
        : protected AZ::Data::AssetBus::MultiHandler
        , protected AzToolsFramework::AssetSystemBus::Handler
        , public AzToolsFramework::UndoSystem::IUndoNotify
    {
    public:

        MemoryAsset()
        {
            AzToolsFramework::AssetSystemBus::Handler::BusConnect();
        }

        ~MemoryAsset()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
        }

        using AssetType = BaseAssetType;

        virtual void Create(AZ::Data::AssetId assetId, AZStd::string_view assetAbsolutePath, AZ::Data::AssetType assetType, Callbacks::OnAssetCreatedCallback onAssetCreatedCallback) = 0;

        virtual void SaveAs(const AZStd::string& path, Callbacks::OnSave onSaveCallback) = 0;
        virtual void Save(Callbacks::OnSave onSaveCallback) = 0;

        virtual bool Load(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType, Callbacks::OnAssetReadyCallback onAssetReadyCallback) = 0;
    };

    class UndoHelper;

    // Saving an asset is an asynchronous process that requires several steps, this helper
    // class ensures asset saving takes place correctly and reduces the complexity of
    // ScriptCanvasMemoryAsset's saving requirements
    class AssetSaveFinalizer
        : AZ::SystemTickBus::Handler
    {
    public:

        using OnCompleteEvent = AZ::Event<AZ::Data::AssetId>;
        using OnCompleteHandler = AZ::Event<AZ::Data::AssetId>::Handler;

        AssetSaveFinalizer();
        ~AssetSaveFinalizer();

        void Start(ScriptCanvasMemoryAsset* sourceAsset, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZ::Data::AssetStreamInfo& saveInfo, Callbacks::OnSave onSaveCallback, OnCompleteHandler onComplete);
        void Reset();

    private:

        // AZ::SystemTickBus::Handler ...
        void OnSystemTick() override;
        ///

        bool ValidateStatus(const AzToolsFramework::SourceControlFileInfo& fileInfo);
        AZStd::string MakeTemporaryFilePathForSave(AZStd::string_view targetFilename);

        OnCompleteHandler m_onCompleteHandler;
        OnCompleteEvent m_onComplete;
        Callbacks::OnSave m_onSave;

        bool m_saving = false;
        bool m_fileAvailableForSave = false;

        AZ::Data::AssetPtr m_inMemoryAsset;
        AZ::Data::AssetId m_fileAssetId;
        AZ::Data::AssetStreamInfo m_saveInfo;

        ScriptCanvasMemoryAsset* m_sourceAsset;

        AZ::Data::AssetType m_assetType;

    };

    // Script Canvas primarily works with an in-memory copy of an asset.
    // There are two situations, the first is, when a new asset is created and not yet saved.
    // Once saved, we will create a new asset on file, however, and this is important
    // once the file is saved to file, its asset ID will be changed, if the file is to remain
    // open, we need to update the source AssetId to correspond to the file asset.
    //
    // The other is when an asset is loaded, we clone the asset from file and use an in-memory 
    // version of the asset until it is time to save, at that moment we need to save to the 
    // source file
    class ScriptCanvasMemoryAsset 
        : public MemoryAsset<ScriptCanvas::ScriptCanvasAssetBase>
        , public AZStd::enable_shared_from_this<ScriptCanvasMemoryAsset>
        , EditorGraphNotificationBus::Handler
    {
    public:

        ScriptCanvasMemoryAsset();
        ~ScriptCanvasMemoryAsset() override;

        ScriptCanvasMemoryAsset(const ScriptCanvasMemoryAsset& rhs) = delete;
        ScriptCanvasMemoryAsset& operator = (const ScriptCanvasMemoryAsset& rhs) = delete;

        using pointer = AZStd::shared_ptr<ScriptCanvasMemoryAsset>;

        void Create(AZ::Data::AssetId assetId, AZStd::string_view assetAbsolutePath, AZ::Data::AssetType assetType, Callbacks::OnAssetCreatedCallback onAssetCreatedCallback) override;
        void SaveAs(const AZStd::string& path, Callbacks::OnSave onSaveCallback) override;
        void Save(Callbacks::OnSave onSaveCallback) override;
        bool Load(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType, Callbacks::OnAssetReadyCallback onAssetReadyCallback) override;
        void Set(AZ::Data::AssetId assetId);

        const AZ::Data::AssetId& GetId() const { return m_inMemoryAssetId; }
        const AZ::Data::AssetId& GetFileAssetId() const { return m_fileAssetId; }

        const AZ::Data::AssetType GetAssetType() const { return m_assetType; }

        AZ::Data::Asset<AssetType> GetAsset() { return m_inMemoryAsset; }
        const AZ::Data::Asset<AssetType>& GetAsset() const { return m_inMemoryAsset; }

        const AZStd::string GetTabName() const;

        const AZStd::string& GetAbsolutePath() const { return m_absolutePath; }
        AZ::EntityId GetScriptCanvasId() const { return m_scriptCanvasId; }

        AZ::EntityId GetGraphId();
        Tracker::ScriptCanvasFileState GetFileState() const;

        void SetFileState(Tracker::ScriptCanvasFileState fileState);

        void CloneTo(ScriptCanvasMemoryAsset& memoryAsset);

        void ActivateAsset();

        Widget::CanvasWidget* GetView() { return m_canvasWidget; }

        Widget::CanvasWidget* CreateView(QWidget* parent);
        void ClearView();

        void UndoStackChange();

        SceneUndoState* GetUndoState() { return m_undoState.get(); }

        bool IsSourceInError() const;

        void SavingComplete(const AZStd::string& fullPath, AZ::Uuid sourceAssetId);

        AZ::Data::AssetId GetSourceUuid() const { return m_sourceUuid; }

    private:

        template <typename T>
        auto Clone()
        {
            AZ::Data::AssetId assetId = AZ::Uuid::CreateRandom();

            AZ::Data::Asset<T> newAsset = m_inMemoryAsset;
            newAsset = { aznew T(assetId, AZ::Data::AssetData::AssetStatus::Ready), AZ::Data::AssetLoadBehavior::Default };

            auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
            serializeContext->CloneObjectInplace(newAsset.Get()->GetScriptCanvasData(), &m_inMemoryAsset.Get()->GetScriptCanvasData());

            m_editorEntityIdMap.clear();

            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&newAsset.Get()->GetScriptCanvasData(), m_editorEntityIdMap, serializeContext);

            return newAsset;
        }

        // EditorGraphNotificationBus
        void OnGraphCanvasSceneDisplayed() override;
        ///

        //! AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;
        ///////////////////////

        // AzToolsFramework::AssetSystemBus::Handler
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        ///


        void FinalizeAssetSave(bool, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZ::Data::AssetStreamInfo& saveInfo, Callbacks::OnSave onSaveCallback);

        template <typename T>
        void StartAssetLoad(AZ::Data::AssetId assetId, AZ::Data::Asset<T>& asset)
        {
            asset = AZ::Data::AssetManager::Instance().GetAsset<T>(assetId, asset.GetAutoLoadBehavior());
        }

        template <typename AssetType>
        AZ::Data::Asset<AZ::Data::AssetData> CloneAssetData(AZ::Data::AssetId newAssetId)
        {
            AssetType* assetData = aznew AssetType(newAssetId, AZ::Data::AssetData::AssetStatus::Ready);

            auto& scriptCanvasData = assetData->GetScriptCanvasData();

            // Clone asset data into SC Editor asset
            auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
            serializeContext->CloneObjectInplace(scriptCanvasData, &m_inMemoryAsset.Get()->GetScriptCanvasData());

            m_editorEntityIdMap.clear();

            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&scriptCanvasData, m_editorEntityIdMap, serializeContext);

            // Upon doing this move, the canonical asset will be unloaded
            m_inMemoryAsset = { AZStd::move(assetData), AZ::Data::AssetLoadBehavior::Default };

            return m_inMemoryAsset;
        }

        // Upon loading a graph, we clone the source data and we replace the loaded asset with 
        // a clone, this is to prevent modifications to the source data and it gives us some
        // flexibility if we need to load the source asset again
        AZ::Data::Asset<AZ::Data::AssetData> CloneAssetData(AZ::Data::AssetId newAssetId);

        // IUndoNotify
        void OnUndoStackChanged() override;
        //

    private:

        void SetFileAssetId(const AZ::Data::AssetId& fileAssetId);

        void SignalFileStateChanged();

        AZStd::string MakeTemporaryFilePathForSave(AZStd::string_view targetFilename);

        //! Finds the appropriate asset handler for the type of Script Canvas asset given
        ScriptCanvasAssetHandler* GetAssetHandlerForType(AZ::Data::AssetType assetType);

        //! The asset type, we need it to make sure we call the correct factory methods
        AZ::Data::AssetType m_assetType;

        //! The in-memory asset
        AZ::Data::Asset<AssetType> m_inMemoryAsset;

        AZ::Data::Asset<AssetType> m_sourceAsset;

        //! Whether we are making a new asset or loading one, we should always have its absolute path
        AZStd::string m_absolutePath;

        AZStd::string m_saveAsPath;

        //! The AssetId of the canonical asset on file, if the asset has never been saved to file, it is Invalid
        AZ::Data::AssetId m_fileAssetId;

        //! The AssetId that represents this asset, it will always be the in-memory asset Id and never the file asset Id
        AZ::Data::AssetId m_inMemoryAssetId;

        //! When a new asset is saved, we need to keep it's previous internal Ids in order for the front end to remap to the new Ids
        using FormerGraphIdPair = AZStd::pair<AZ::EntityId /*scriptCanvasEntityId*/, AZ::EntityId /* graphId */>;
        FormerGraphIdPair m_formerGraphIdPair;

        //! The EntityId of the ScriptCanvasEntity owned by the ScriptCanvasAsset
        ScriptCanvas::ScriptCanvasId m_scriptCanvasId;

        //! The EntityId that represents the ScriptCanvas graph
        AZ::EntityId m_graphId;

        //! Gives the ability to provide a lambda invoked when the asset is ready
        Callbacks::OnAssetReadyCallback m_onAssetReadyCallback;

        //! The Save is officially complete after SourceFileChange is handled.
        Callbacks::OnSave m_onSaveCallback;

        bool m_sourceRemoved = false;
        Tracker::ScriptCanvasFileState m_fileState = Tracker::ScriptCanvasFileState::INVALID;

        //! We need to track the filename of the file being saved because we need to match it when we handle SourceFileChange (see SourceFileChange for details)
        AZStd::vector<AZStd::string> m_pendingSave;

        //! Each memory asset owns its view widget
        Widget::CanvasWidget* m_canvasWidget = nullptr;

        //! Utility cache of remapped entityIds
        using EditorEntityIdMap = AZStd::unordered_map<AZ::EntityId, AZ::EntityId>;

        //! Cached mapping of Scene Entity ID to Asset Id, used by the debugger
        EditorEntityIdMap m_editorEntityIdMap;

        //! Each asset keeps track of its undo state
        AZStd::unique_ptr<SceneUndoState> m_undoState;

        //! The undo helper is an object that implements the Undo behaviors
        AZStd::unique_ptr<UndoHelper> m_undoHelper;

        bool m_sourceInError;

        AZ::Data::AssetId m_sourceUuid;

        AssetSaveFinalizer m_assetSaveFinalizer;

    public:

        //! Given a scene EntityId, find the respective editor EntityId
        AZ::EntityId GetEditorEntityIdFromSceneEntityId(AZ::EntityId sceneEntityId);

        //! Given an editor EntityId, find the respective scene EntityId
        AZ::EntityId GetSceneEntityIdFromEditorEntityId(AZ::EntityId editorEntityId);

        //! When a new asset is saved, we need to keep it's previous internal Ids in order for the front end to remap to the new Ids
        const FormerGraphIdPair& GetFormerGraphIds() const { return m_formerGraphIdPair; }

    };


}
