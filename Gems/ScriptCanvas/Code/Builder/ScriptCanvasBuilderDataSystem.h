/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderDataSystemBus.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    class EditorAssetTree;
}

namespace ScriptCanvasBuilder
{
// this MACRO enables highly verbose status updates from the builder data system which will later be routed through an imgui window.
// #define DATA_SYSTEM_STATUS(window, msg, ...) AZ_TracePrintf(window, msg, __VA_ARGS__);
#define DATA_SYSTEM_STATUS(window, msg, ...)

    /// <summary>
    /// Provides simplified access to status and compiled data for ScriptCanvas source files.
    /// </summary>
    /// 
    /// This class handles both DataSystemAssetRequestsBus and DataSystemSourceRequestsBus. It listens to AP notifications and
    /// the tools framework notifications for ScriptCanvas source file changes. It stores the results of processing a source file for both
    /// editor-configurable properties and for runtime ready assets for faster retrieval when many are being simultaneously processed. For
    /// example, this occurs during prefab compilation time, when multiple ScriptCanvasEditorComponents require builder data for their
    /// configuration loaded from latest source file on disk. This system reduces file I/O and compilation work by maintaining and providing
    /// access to the very latest results.
    class DataSystem final
        : public DataSystemAssetRequestsBus::Handler
        , public DataSystemSourceRequestsBus::Handler
        , public AzToolsFramework::AssetSystemBus::Handler
        , public AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_TYPE_INFO(DataSystem, "{27B74209-319D-4A8C-B37D-F85EFA6D2FFA}");
        AZ_CLASS_ALLOCATOR(DataSystem, AZ::SystemAllocator, 0);

        DataSystem();
        virtual ~DataSystem();

        /// <summary>
        /// Returns the latest built editor properties for the source file
        /// </summary>
        /// <param name="sourceHandle"></param>
        /// <returns>BuilderSourceResult editor properties status and data </returns>
        BuilderSourceResult CompileBuilderData(ScriptCanvasEditor::SourceHandle sourceHandle) override;

        /// <summary>
        /// Returns the latest built runtime data for the source file
        /// </summary>
        /// <param name="sourceHandle"></param>
        /// <returns>BuilderAssetResult runtime status and data </returns>
        BuilderAssetResult LoadAsset(ScriptCanvasEditor::SourceHandle sourceHandle) override;

    private:
        struct BuilderSourceStorage
        {
            BuilderSourceStatus status = BuilderSourceStatus::Failed;
            BuildVariableOverrides data;
        };

        using MutexLock = AZStd::lock_guard<AZStd::recursive_mutex>;
        AZStd::recursive_mutex m_mutex;
        AZStd::unordered_map<AZ::Uuid, BuilderSourceStorage> m_buildResultsByHandle;
        AZStd::unordered_map<AZ::Uuid, BuilderAssetResult> m_assets;
        
        void AddResult(const ScriptCanvasEditor::SourceHandle& handle, BuilderSourceStorage&& result);
        void AddResult(AZ::Uuid&& id, BuilderSourceStorage&& result);
        void CompileBuilderDataInternal(ScriptCanvasEditor::SourceHandle sourceHandle);
        BuilderAssetResult& MonitorAsset(AZ::Uuid fileAssetId);
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> /*asset*/, bool /*isSuccessful*/) {}
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType);
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void ReportReady(AZ::Data::Asset<AZ::Data::AssetData> asset);
        // temporary work-around for the discrepancies between loaded Lua modules and loaded AZ::ScriptAssets.
        void ReportReadyFilter(AZ::Data::Asset<AZ::Data::AssetData> asset);
    };
}
