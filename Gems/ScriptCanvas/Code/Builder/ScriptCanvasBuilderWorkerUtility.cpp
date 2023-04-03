/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Asset/AssetDescription.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Script/ScriptComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAssetHandler.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Results/ErrorText.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>
#include <Source/Components/SceneComponent.h>
#include <ScriptCanvas/Core/Core.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace ScriptCanvasBuilder
{
    AssetHandlers::AssetHandlers(SharedHandlers& source)
        : m_editorFunctionAssetHandler(source.m_editorFunctionAssetHandler.first)
        , m_runtimeAssetHandler(source.m_runtimeAssetHandler.first)
        , m_subgraphInterfaceHandler(source.m_subgraphInterfaceHandler.first)
    {}

    void SharedHandlers::DeleteOwnedHandlers()
    {
        DeleteIfOwned(m_editorFunctionAssetHandler);
        DeleteIfOwned(m_runtimeAssetHandler);
        DeleteIfOwned(m_subgraphInterfaceHandler);
    }

    void SharedHandlers::DeleteIfOwned(HandlerOwnership& handler)
    {
        if (handler.second)
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(handler.first);
            delete handler.first;
            handler.first = nullptr;
        }
    }

    AZ::Outcome<ScriptCanvas::Grammar::AbstractCodeModelConstPtr, AZStd::string> ParseGraph(AZ::Entity& buildEntity, AZStd::string_view graphPath)
    {
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(graphPath.data(), fileNameOnly);

        ScriptCanvas::Grammar::Request request;
        request.graph = PrepareSourceGraph(&buildEntity);
        if (!request.graph)
        {
            return AZ::Failure(AZStd::string("build entity did not have source graph components"));
        }

        request.rawSaveDebugOutput = ScriptCanvas::Grammar::g_saveRawTranslationOuputToFileAtPrefabTime;
        request.printModelToConsole = ScriptCanvas::Grammar::g_printAbstractCodeModelAtPrefabTime;
        request.name = fileNameOnly.empty() ? fileNameOnly : "BuilderGraph";
        request.addDebugInformation = false;

        return ScriptCanvas::Translation::ParseGraph(request);
    }

    AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> CreateLuaAsset(const SourceHandle& editAsset, AZStd::string_view rawLuaFilePath)
    {
        AZStd::string fullPath(rawLuaFilePath);
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(rawLuaFilePath.data(), fileNameOnly);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        auto sourceGraph = PrepareSourceGraph(editAsset.Mod()->GetEntity());

        ScriptCanvas::Grammar::Request request;
        request.scriptAssetId = editAsset.Id();
        request.graph = sourceGraph;
        request.name = fileNameOnly;
        request.rawSaveDebugOutput = ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile;
        request.printModelToConsole = ScriptCanvas::Grammar::g_printAbstractCodeModel;
        request.path = fullPath;

        const ScriptCanvas::Translation::Result translationResult = TranslateToLua(request);

        auto isSuccessOutcome = translationResult.IsSuccess(ScriptCanvas::Translation::TargetFlags::Lua);
        if (!isSuccessOutcome.IsSuccess())
        {
            return AZ::Failure(isSuccessOutcome.TakeError());
        }

        auto& translation = translationResult.m_translations.find(ScriptCanvas::Translation::TargetFlags::Lua)->second;
        AZ::Data::Asset<AZ::ScriptAsset> asset;
        AZ::Data::AssetId scriptAssetId(editAsset.Id(), AZ::ScriptAsset::CompiledAssetSubId);
        asset.Create(scriptAssetId);

        AZ::IO::MemoryStream inputStream(translation.m_text.data(), translation.m_text.size());
        AzFramework::ScriptCompileRequest compileRequest;
        compileRequest.m_errorWindow = s_scriptCanvasBuilder;
        compileRequest.m_input = &inputStream;

        AzFramework::ConstructScriptAssetPaths(compileRequest);
        auto compileOutcome = AzFramework::CompileScript(compileRequest);
        if (!compileOutcome.IsSuccess())
        {
            return AZ::Failure(AZStd::string(compileOutcome.TakeError()));
        }

        asset->m_data = compileRequest.m_luaScriptDataOut;

        ScriptCanvas::Translation::LuaAssetResult result;
        result.m_scriptAsset = asset;
        result.m_runtimeInputs = AZStd::move(translation.m_runtimeInputs);
        result.m_debugMap = AZStd::move(translation.m_debugMap);
        result.m_dependencies = translationResult.m_model->GetOrderedDependencies();
        result.m_parseDuration = translationResult.m_parseDuration;
        result.m_translationDuration = translation.m_duration;
        return AZ::Success(result);
    }

    AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(const SourceHandle& editAsset)
    {
        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZ::Data::AssetId runtimeAssetId = editAsset.Id();
        runtimeAssetId.m_subId = ScriptCanvas::RuntimeDataSubId;
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
        runtimeAsset.Create(runtimeAssetId);

        return AZ::Success(runtimeAsset);
    }

    int GetBuilderVersion()
    {
        // #functions2 remove-execution-out-hash include version from all library nodes, split fingerprint generation to relax Is Out of Data restriction when graphs only need a recompile
        return static_cast<int>(BuilderVersion::Current)
            + static_cast<int>(ScriptCanvas::GrammarVersion::Current)
            + static_cast<int>(ScriptCanvas::RuntimeVersion::Current)
            ;
    }

    ScriptCanvasEditor::EditorGraph* PrepareSourceGraph(AZ::Entity* const buildEntity)
    {
        auto sourceGraph = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::EditorGraph>(buildEntity);
        if (!sourceGraph)
        {
            return nullptr;
        }

        // Remove nodes that do not have components, these could be versioning artifacts
        // or nodes that are missing due to a missing gem
        AZStd::vector<AZ::Entity*> nodesToRemove;
        for (auto* node : sourceGraph->GetGraphData()->m_nodes)
        {
            if (node->GetComponents().empty())
            {
                // This is a problem, we can remove this node.
                AZ_TracePrintf("Script Canvas", "Removing node due to missing components: %s\nVerify that all gems that this script relies on are enabled", node->GetName().c_str());
                nodesToRemove.push_back(node);
            }
        }

        for (AZ::Entity* nodeEntity : nodesToRemove)
        {
            sourceGraph->GetGraphData()->m_nodes.erase(nodeEntity);
        }

        // Remove these front-end components during build time to avoid trying to use components
        // the AP is not meant to use.
        for (auto* component : buildEntity->GetComponents())
        {
            if (component->RTTI_GetType() == azrtti_typeid<GraphCanvas::SceneComponent>())
            {
                buildEntity->RemoveComponent(component);
            }
        }

        ScriptCanvas::ScopedAuxiliaryEntityHandler entityHandler(buildEntity);

        if (buildEntity->GetState() == AZ::Entity::State::Init)
        {
            buildEntity->Activate();
        }

        AZ_Assert(buildEntity->GetState() == AZ::Entity::State::Active, "build entity not active");
        return sourceGraph;
    }

    AZ::Outcome<void, AZStd::string> ProcessTranslationJob(ProcessTranslationJobInput& input)
    {
        using namespace ScriptCanvas;

        auto sourceGraph = PrepareSourceGraph(input.buildEntity);

        auto version = sourceGraph->GetVersion();
        if (version.grammarVersion == ScriptCanvas::GrammarVersion::Initial
            || version.runtimeVersion == ScriptCanvas::RuntimeVersion::Initial)
        {
            return AZ::Failure(AZStd::string(ScriptCanvas::ParseErrors::SourceUpdateRequired));
        }

        ScriptCanvas::Grammar::Request request;
        request.path = input.fullPath;
        request.name = input.fileNameOnly;
        request.namespacePath = input.namespacePath;
        request.scriptAssetId = input.assetID;
        request.graph = sourceGraph;
        request.rawSaveDebugOutput = ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile;
        request.printModelToConsole = ScriptCanvas::Grammar::g_printAbstractCodeModel;

        ScriptCanvas::Translation::Result translationResult = TranslateToLua(request);
        auto outcome = translationResult.IsSuccess(ScriptCanvas::Translation::TargetFlags::Lua);
        if (!outcome.IsSuccess())
        {
            return AZ::Failure(outcome.GetError());
        }

        const auto& translation = translationResult.m_translations.find(ScriptCanvas::Translation::TargetFlags::Lua)->second;

        AZ::IO::MemoryStream inputStream(translation.m_text.data(), translation.m_text.size());
        AzFramework::ScriptCompileRequest compileRequest;

        AZStd::string vmFullPath = input.request->m_fullPath;
        AzFramework::StringFunc::Path::StripExtension(vmFullPath);
        vmFullPath += ScriptCanvas::Grammar::k_internalRuntimeSuffix;
        compileRequest.m_fullPath = vmFullPath;
        compileRequest.m_fileName = input.fileNameOnly;
        compileRequest.m_tempDirPath = input.request->m_tempDirPath;
        compileRequest.m_errorWindow = s_scriptCanvasBuilder;
        compileRequest.m_input = &inputStream;
        AzFramework::ConstructScriptAssetPaths(compileRequest);

        // compiles in input stream Lua in memory, writes output to disk
        auto compileOutcome = AzFramework::CompileScriptAndSaveAsset(compileRequest);
        if (!compileOutcome.IsSuccess())
        {
            return AZ::Failure(compileOutcome.GetError());
        };

        // Interpreted Lua
        AssetBuilderSDK::JobProduct jobProduct;
        jobProduct.m_productFileName = compileRequest.m_destPath;;
        jobProduct.m_productAssetType = azrtti_typeid<AZ::ScriptAsset>();
        jobProduct.m_productSubID = AZ::ScriptAsset::CompiledAssetSubId;
        jobProduct.m_dependenciesHandled = true;
        input.response->m_outputProducts.push_back(AZStd::move(jobProduct));

        const AZ::Data::AssetId scriptAssetId(input.assetID.m_guid, jobProduct.m_productSubID);
        AZ::Data::Asset<AZ::ScriptAsset> scriptAsset(scriptAssetId, jobProduct.m_productAssetType, {});
        scriptAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        input.runtimeDataOut.m_script = scriptAsset;

        const ScriptCanvas::OrderedDependencies& orderedDependencies = translationResult.m_model->GetOrderedDependencies();
        const ScriptCanvas::DependencyReport& dependencyReport = orderedDependencies.source;

        for (const auto& subgraphAssetID : orderedDependencies.orderedAssetIds)
        {
            const AZ::Data::AssetId dependentSubgraphAssetID(subgraphAssetID.m_guid, RuntimeDataSubId);
            AZ::Data::Asset<ScriptCanvas::RuntimeAsset> subgraphAsset(dependentSubgraphAssetID, azrtti_typeid<ScriptCanvas::RuntimeAsset>());
            subgraphAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
            input.runtimeDataOut.m_requiredAssets.push_back(subgraphAsset);
        }

        for (const auto& scriptEventAssetID : dependencyReport.scriptEventsAssetIds)
        {
            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> subgraphAsset(scriptEventAssetID, azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
            subgraphAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
            input.runtimeDataOut.m_requiredScriptEvents.push_back(subgraphAsset);
        }

        input.runtimeDataOut.m_input = AZStd::move(translation.m_runtimeInputs);
        input.runtimeDataOut.m_debugMap = AZStd::move(translation.m_debugMap);
        input.interfaceOut = AZStd::move(translation.m_subgraphInterface);

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> SaveSubgraphInterface(ProcessTranslationJobInput& input, ScriptCanvas::SubgraphInterfaceData& subgraphInterface)
    {
        AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset> runtimeAsset;
        runtimeAsset.Create(AZ::Data::AssetId(input.assetID.m_guid, AZ_CRC("SubgraphInterface", 0xdfe6dc72)));
        runtimeAsset.Get()->m_interfaceData = subgraphInterface;

        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        bool runtimeCanvasSaved = input.assetHandler->SaveAssetData(runtimeAsset, &byteStream);
        if (!runtimeCanvasSaved)
        {
            return AZ::Failure(AZStd::string("Failed to save script canvas subgraph interface to object stream"));
        }

        AZ::IO::FileIOStream outFileStream(input.runtimeScriptCanvasOutputPath.data(), AZ::IO::OpenMode::ModeWrite);
        if (!outFileStream.IsOpen())
        {
            return AZ::Failure(AZStd::string::format("Failed to open output file %s", input.runtimeScriptCanvasOutputPath.data()));
        }

        runtimeCanvasSaved = outFileStream.Write(byteBuffer.size(), byteBuffer.data()) == byteBuffer.size() && runtimeCanvasSaved;
        if (!runtimeCanvasSaved)
        {
            return AZ::Failure(AZStd::string::format("Unable to save script canvas subgraph interface file %s", input.runtimeScriptCanvasOutputPath.data()));
        }

        AssetBuilderSDK::JobProduct jobProduct;
        jobProduct.m_dependenciesHandled = true;
        jobProduct.m_productFileName = input.runtimeScriptCanvasOutputPath;
        jobProduct.m_productAssetType = azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>();
        jobProduct.m_productSubID = AZ_CRC("SubgraphInterface", 0xdfe6dc72);
        input.response->m_outputProducts.push_back(AZStd::move(jobProduct));
        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> SaveRuntimeAsset(ProcessTranslationJobInput& input, ScriptCanvas::RuntimeData& runtimeData)
    {
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
        runtimeAsset.Create(AZ::Data::AssetId(input.assetID.m_guid, ScriptCanvas::RuntimeDataSubId));
        runtimeAsset.Get()->m_runtimeData = runtimeData;

        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        bool runtimeCanvasSaved = input.assetHandler->SaveAssetData(runtimeAsset, &byteStream);
        if (!runtimeCanvasSaved)
        {
            return AZ::Failure(AZStd::string("Failed to save runtime script canvas to object stream"));
        }

        AZ::IO::FileIOStream outFileStream(input.runtimeScriptCanvasOutputPath.data(), AZ::IO::OpenMode::ModeWrite);
        if (!outFileStream.IsOpen())
        {
            return AZ::Failure(AZStd::string::format("Failed to open output file %s", input.runtimeScriptCanvasOutputPath.data()));
        }

        runtimeCanvasSaved = outFileStream.Write(byteBuffer.size(), byteBuffer.data()) == byteBuffer.size() && runtimeCanvasSaved;
        if (!runtimeCanvasSaved)
        {
            return AZ::Failure(AZStd::string::format("Unable to save runtime script canvas file %s", input.runtimeScriptCanvasOutputPath.data()));
        }

        AssetBuilderSDK::JobProduct jobProduct;

        // Scan our runtime input for any asset references
        // Store them as product dependencies
        AssetBuilderSDK::OutputObject(&runtimeData.m_input,
            azrtti_typeid<decltype(runtimeData.m_input)>(),
            input.runtimeScriptCanvasOutputPath,
            azrtti_typeid<ScriptCanvas::RuntimeAsset>(),
            ScriptCanvas::RuntimeDataSubId, jobProduct);

        // Output Object marks dependencies as handled.
        // We still have more to evaluate, so mark false, until complete
        jobProduct.m_dependenciesHandled = false;

        jobProduct.m_dependencies.push_back({ runtimeData.m_script.GetId(), AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad) });

        for (const auto& assetDependency : runtimeData.m_requiredAssets)
        {
            if (AZ::Data::AssetManager::Instance().GetAsset(assetDependency.GetId(), assetDependency.GetType(), AZ::Data::AssetLoadBehavior::PreLoad))
            {
                jobProduct.m_dependencies.push_back({ assetDependency.GetId(), AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad) });
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Unable load runtime Script Canvas dependency: %s", assetDependency.GetId().ToString<AZStd::string>().c_str()));
            }
        }

        for (const auto& scriptEventDependency : runtimeData.m_requiredScriptEvents)
        {
            if (AZ::Data::AssetManager::Instance().GetAsset(scriptEventDependency.GetId(), scriptEventDependency.GetType(), AZ::Data::AssetLoadBehavior::PreLoad))
            {
                jobProduct.m_dependencies.push_back({ scriptEventDependency.GetId(), AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad) });
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Unable load runtime script event dependency: %s", scriptEventDependency.GetId().ToString<AZStd::string>().c_str()));
            }
        }

        jobProduct.m_dependenciesHandled = true;
        input.response->m_outputProducts.push_back(AZStd::move(jobProduct));
        return AZ::Success();
    }

    ScriptCanvas::Translation::Result TranslateToLua(ScriptCanvas::Grammar::Request& request)
    {
        request.translationTargetFlags = ScriptCanvas::Translation::TargetFlags::Lua;
        return ScriptCanvas::Translation::ParseAndTranslateGraph(request);
    }
}
