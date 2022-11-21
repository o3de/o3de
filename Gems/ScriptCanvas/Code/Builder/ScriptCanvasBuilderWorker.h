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

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Translation/Translation.h>

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
    struct SubgraphInterfaceData;
}

namespace ScriptCanvasEditor
{
    class EditorGraph;
}

namespace ScriptCanvasBuilder
{
    using SourceHandle = ScriptCanvas::SourceHandle;

    constexpr const char* s_scriptCanvasBuilder = "ScriptCanvasBuilder";
    constexpr const char* s_scriptCanvasProcessJobKey = "Script Canvas Process Job";
    constexpr const char* s_unitTestParseErrorPrefix = "LY_SC_UnitTest";

    enum class BuilderVersion : int
    {
        SplitCopyFromCompileJobs = 9,
        ChangeScriptRequirementToAsset,
        RemoveDebugVariablesFromRelease,
        FailJobsOnMissingLKG,
        QuantumLeap,
        DependencyArguments,
        DependencyRequirementsData,
        AddAssetDependencySearch,
        PrefabIntegration,
        CorrectGraphVariableVersion,
        ReflectEntityIdNodes,
        FixExecutionStateNodeableConstruction,
        SwitchAssetsToBinary,
        ReinforcePreloadBehavior,
        SeparateFromEntityComponentSystem,
        DistinguishEntityScriptFromScript,
        ExecutionStateAsLightUserdata,
        UpdateDependencyHandling,
        AddExplicitDestructCallForMemberVariables,
        DoNotLoadScriptEventsDuringCreateJobs,
        FixEntityIdReturnValuesInEvents,
        // add new entries above
        Current,
    };

    using HandlerOwnership = AZStd::pair<AZ::Data::AssetHandler*, bool>;

    struct SharedHandlers;

    struct AssetHandlers
    {
        AZ::Data::AssetHandler* m_editorFunctionAssetHandler = nullptr;
        AZ::Data::AssetHandler* m_runtimeAssetHandler = nullptr;
        AZ::Data::AssetHandler* m_subgraphInterfaceHandler = nullptr;

        AssetHandlers() = default;
        explicit AssetHandlers(SharedHandlers& source);
    };

    struct SharedHandlers
    {
        HandlerOwnership m_editorFunctionAssetHandler{};
        HandlerOwnership m_runtimeAssetHandler{};
        HandlerOwnership m_subgraphInterfaceHandler{};

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
    };

    class JobDependencyVerificationHandler : public ScriptCanvas::RuntimeAssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(JobDependencyVerificationHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(JobDependencyVerificationHandler, "{3997EF50-350A-46F0-9D84-7FA403855CC5}", ScriptCanvas::RuntimeAssetHandler);

        void InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override
        {
            AZ_UNUSED(asset);
            AZ_UNUSED(loadStageSucceeded);
            AZ_UNUSED(isReload);
            // do nothing, this just verifies the asset existed
        }
    };

    AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(const SourceHandle& asset);

    AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> CreateLuaAsset(const SourceHandle& editAsset, AZStd::string_view rawLuaFilePath);

    int GetBuilderVersion();

    AZ::Outcome<ScriptCanvas::Grammar::AbstractCodeModelConstPtr, AZStd::string> ParseGraph(AZ::Entity& buildEntity, AZStd::string_view graphPath);

    AZ::Outcome<void, AZStd::string> ProcessTranslationJob(ProcessTranslationJobInput& input);

    ScriptCanvasEditor::EditorGraph* PrepareSourceGraph(AZ::Entity* const buildEntity);

    AZ::Outcome<void, AZStd::string> SaveSubgraphInterface(ProcessTranslationJobInput& input, ScriptCanvas::SubgraphInterfaceData& subgraphInterface);

    AZ::Outcome<void, AZStd::string> SaveRuntimeAsset(ProcessTranslationJobInput& input, ScriptCanvas::RuntimeData& runtimeData);

    ScriptCanvas::Translation::Result TranslateToLua(ScriptCanvas::Grammar::Request& request);

    class Worker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        static AZ::Uuid GetUUID();

        Worker() = default;
        Worker(const Worker&) = delete;

        void Activate(const AssetHandlers& handlers);

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;

        const char* GetFingerprintString() const;

        int GetVersionNumber() const;

        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        void ShutDown() override {};

    private:
        AZ::Data::AssetHandler* m_runtimeAssetHandler = nullptr;
        AZ::Data::AssetHandler* m_subgraphInterfaceHandler = nullptr;
        AZ::Uuid m_sourceUuid;

        mutable AZStd::vector<AZ::Data::AssetFilterInfo> m_processEditorAssetDependencies;
        // cached on first time query
        mutable AZStd::string m_fingerprintString;
    };
}
