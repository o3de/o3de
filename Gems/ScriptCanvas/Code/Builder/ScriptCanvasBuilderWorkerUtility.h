/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Translation/Translation.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <Builder/ScriptCanvasBuilder.h>

namespace AZ
{
    class ScriptAsset;

    namespace Data
    {
        class AssetHandler;
    }
}

namespace ScriptCanvas
{
    class RuntimeAsset;
    class SubgraphInterfaceAsset;
}

namespace ScriptCanvasEditor
{
    class EditorGraph;
    class SourceHandle;
}

namespace ScriptCanvasBuilder
{
    using HandlerOwnership = AZStd::pair<AZ::Data::AssetHandler*, bool>;

    struct SharedHandlers;

    struct AssetHandlers
    {
        AZ::Data::AssetHandler* m_editorFunctionAssetHandler = nullptr;
        AZ::Data::AssetHandler* m_runtimeAssetHandler = nullptr;
        AZ::Data::AssetHandler* m_subgraphInterfaceHandler = nullptr;
        AZ::Data::AssetHandler* m_builderHandler = nullptr;

        AssetHandlers() = default;
        explicit AssetHandlers(SharedHandlers& source);
    };

    struct SharedHandlers
    {
        HandlerOwnership m_editorFunctionAssetHandler{};
        HandlerOwnership m_runtimeAssetHandler{};
        HandlerOwnership m_subgraphInterfaceHandler{};
        HandlerOwnership m_builderHandler{};

        void DeleteOwnedHandlers();

    private:
        void DeleteIfOwned(HandlerOwnership& handler);
    };

    struct ProcessTranslationJobInput
    {
        AZ::Data::AssetId assetID;
        const AssetBuilderSDK::ProcessJobRequest* request = nullptr;
        AssetBuilderSDK::ProcessJobResponse* response = nullptr;
        AZStd::string runtimeScriptCanvasOutputPath;
        AZ::Data::AssetHandler* assetHandler = nullptr;
        AZ::Entity* buildEntity = nullptr;
        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AZStd::string namespacePath;
        bool saveRawLua = false;
        ScriptCanvas::RuntimeData runtimeDataOut;
        ScriptCanvas::Grammar::SubgraphInterface interfaceOut;
        ScriptCanvasBuilder::BuildVariableOverrides builderDataOut;
    };

    class JobDependencyVerificationHandler : public ScriptCanvas::RuntimeAssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(JobDependencyVerificationHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(JobDependencyVerificationHandler, "{3997EF50-350A-46F0-9D84-7FA403855CC5}", ScriptCanvas::RuntimeAssetHandler);

        // do nothing, this just verifies the asset existed
        void InitAsset
            ( [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset
            , [[maybe_unused]] bool loadStageSucceeded
            , [[maybe_unused]] bool isReload) override
        {}
    };

    AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(const ScriptCanvasEditor::SourceHandle& asset);

    AZ::Outcome<ScriptCanvas::GraphData, AZStd::string> CompileGraphData(AZ::Entity* scriptCanvasEntity);

    AZ::Outcome<ScriptCanvas::VariableData, AZStd::string> CompileVariableData(AZ::Entity* scriptCanvasEntity);

    AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> CreateLuaAsset(const ScriptCanvasEditor::SourceHandle& editAsset, AZStd::string_view rawLuaFilePath);

    int GetBuilderVersion();

    AZ::Outcome<ScriptCanvas::Grammar::AbstractCodeModelConstPtr, AZStd::string> ParseGraph(AZ::Entity& buildEntity, AZStd::string_view graphPath);

    AZ::Outcome<void, AZStd::string> ProcessBuilderData(ProcessTranslationJobInput& input);

    AZ::Outcome<void, AZStd::string> ProcessTranslationJob(ProcessTranslationJobInput& input);

    ScriptCanvasEditor::EditorGraph* PrepareSourceGraph(AZ::Entity* const buildEntity);

    AZ::Outcome<void, AZStd::string> SaveBuilderAsset(ProcessTranslationJobInput& input, ScriptCanvasBuilder::BuildVariableOverrides&& builderData);

    AZ::Outcome<void, AZStd::string> SaveSubgraphInterfaceAsset(ProcessTranslationJobInput& input, ScriptCanvas::SubgraphInterfaceData& subgraphInterface);

    AZ::Outcome<void, AZStd::string> SaveRuntimeAsset(ProcessTranslationJobInput& input, ScriptCanvas::RuntimeData& runtimeData);

    ScriptCanvas::Translation::Result TranslateToLua(ScriptCanvas::Grammar::Request& request);
}
