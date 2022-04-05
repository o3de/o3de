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
    class DataSystem
        : public DataSystemRequestsBus::Handler
        , public AzToolsFramework::AssetSystemBus::Handler
        , public AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_TYPE_INFO(DataSystem, "{27B74209-319D-4A8C-B37D-F85EFA6D2FFA}");
        AZ_CLASS_ALLOCATOR(DataSystem, AZ::SystemAllocator, 0);

        DataSystem();
        virtual ~DataSystem();

        BuildResult CompileBuilderData(ScriptCanvasEditor::SourceHandle sourceHandle) override;

    protected:
        void AddResult(const ScriptCanvasEditor::SourceHandle& handle, BuildResult&& result);
        void AddResult(AZ::Uuid&& id, BuildResult&& result);
        void CompileBuilderDataInternal(ScriptCanvasEditor::SourceHandle sourceHandle);
        void MonitorAsset(AZ::Uuid fileAssetId);
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> /*asset*/, bool /*isSucces/sful*/) {}
        void OnAssetUnloaded(const AZ::Data::AssetId /*assetId*/, const AZ::Data::AssetType /*assetType*/) {}
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> /*asset*/) {}
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;

    private:
        using MutexLock = AZStd::lock_guard<AZStd::recursive_mutex>;
        AZStd::recursive_mutex m_mutex;
        AZStd::unordered_map<AZ::Uuid, BuildResult> m_buildResultsByHandle;
        AZStd::unordered_map<AZ::Uuid, ScriptCanvas::RuntimeAssetPtr> m_assetsReady;
        AZStd::unordered_map<AZ::Uuid, ScriptCanvas::RuntimeAssetPtr> m_assetsPending;
    };
}
