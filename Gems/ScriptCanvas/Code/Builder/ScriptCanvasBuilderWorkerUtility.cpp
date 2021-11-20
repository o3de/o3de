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
#include <ScriptCanvas/Asset/SubgraphInterfaceAssetHandler.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Results/ErrorText.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>
#include <Source/Components/SceneComponent.h>

namespace ScriptCanvasBuilder
{
    AssetHandlers::AssetHandlers(SharedHandlers& source)
        : m_editorAssetHandler(source.m_editorAssetHandler.first)
        , m_editorFunctionAssetHandler(source.m_editorFunctionAssetHandler.first)
        , m_runtimeAssetHandler(source.m_runtimeAssetHandler.first)
        , m_subgraphInterfaceHandler(source.m_subgraphInterfaceHandler.first)
    {}

    void SharedHandlers::DeleteOwnedHandlers()
    {
        DeleteIfOwned(m_editorAssetHandler);
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

    AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> CreateLuaAsset(AZ::Entity* buildEntity, AZ::Data::AssetId scriptAssetId, AZStd::string_view rawLuaFilePath)
    {
        AZStd::string fullPath(rawLuaFilePath);
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(rawLuaFilePath.data(), fileNameOnly);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        auto sourceGraph = PrepareSourceGraph(buildEntity);

        ScriptCanvas::Grammar::Request request;
        request.scriptAssetId = scriptAssetId;
        request.graph = sourceGraph;
        request.name = fileNameOnly;
        request.rawSaveDebugOutput = ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile;
        request.printModelToConsole = ScriptCanvas::Grammar::g_printAbstractCodeModel;
        request.path = fullPath;

        bool pathFound = false;
        AZStd::string relativePath;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult
            ( pathFound
            , &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath
            , fullPath.c_str(), relativePath);

        if (!pathFound)
        {
            AZ::Failure(AZStd::string::format("Failed to get engine relative path from %s", fullPath.c_str()));
        }

        request.namespacePath = relativePath;
        const ScriptCanvas::Translation::Result translationResult = TranslateToLua(request);

        auto isSuccessOutcome = translationResult.IsSuccess(ScriptCanvas::Translation::TargetFlags::Lua);
        if (!isSuccessOutcome.IsSuccess())
        {
            return AZ::Failure(isSuccessOutcome.TakeError());
        }

        auto& translation = translationResult.m_translations.find(ScriptCanvas::Translation::TargetFlags::Lua)->second;
        AZ::Data::Asset<AZ::ScriptAsset> asset;
        scriptAssetId.m_subId = AZ::ScriptAsset::CompiledAssetSubId;
        asset.Create(scriptAssetId);
        auto writeStream = asset.Get()->CreateWriteStream();

        AZ::IO::MemoryStream inputStream(translation.m_text.data(), translation.m_text.size());
        AzFramework::ScriptCompileRequest compileRequest;
        compileRequest.m_errorWindow = s_scriptCanvasBuilder;
        compileRequest.m_input = &inputStream;
        compileRequest.m_output = &writeStream;

        AzFramework::ConstructScriptAssetPaths(compileRequest);
        auto compileOutcome = AzFramework::CompileScript(compileRequest);
        if (!compileOutcome.IsSuccess())
        {
            return AZ::Failure(AZStd::string(compileOutcome.TakeError()));
        }

        ScriptCanvas::Translation::LuaAssetResult result;
        result.m_scriptAsset = asset;
        result.m_runtimeInputs = AZStd::move(translation.m_runtimeInputs);
        result.m_debugMap = AZStd::move(translation.m_debugMap);
        result.m_dependencies = translationResult.m_model->GetOrderedDependencies();
        result.m_parseDuration = translationResult.m_parseDuration;
        result.m_translationDuration = translation.m_duration;
        return AZ::Success(result);
    }

    AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(const AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset>& editAsset)
    {
        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        auto runtimeAssetId = editAsset.GetId();
        runtimeAssetId.m_subId = AZ_CRC("RuntimeData", 0x163310ae);
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
        runtimeAsset.Create(runtimeAssetId);

        return AZ::Success(runtimeAsset);
    }

    AZ::Outcome<ScriptCanvas::GraphData, AZStd::string> CompileGraphData(AZ::Entity* scriptCanvasEntity)
    {
        typedef AZStd::pair< ScriptCanvas::Node*, AZ::Entity* > NodeEntityPair;

        if (!scriptCanvasEntity)
        {
            return AZ::Failure(AZStd::string("Cannot compile graph data from a nullptr Script Canvas Entity"));
        }

        auto sourceGraph = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::Graph>(scriptCanvasEntity);
        if (!sourceGraph)
        {
            return AZ::Failure(AZStd::string("Failed to find Script Canvas Graph Component"));
        }

        const ScriptCanvas::GraphData& sourceGraphData = *sourceGraph->GetGraphDataConst();
        ScriptCanvas::GraphData compiledGraphData;
        auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        serializeContext->CloneObjectInplace(compiledGraphData, &sourceGraphData);

        AZStd::unordered_set<ScriptCanvas::Endpoint> disabledEndpoints;
        AZStd::unordered_set<AZ::Entity*> disabledNodeEntities;

        AZStd::unordered_map<AZ::EntityId, NodeEntityPair > nodeLookUpMap;

        AZStd::unordered_set<AZ::EntityId> deletedNodeEntities;

        {
            auto nodeIter = compiledGraphData.m_nodes.begin();

            while (nodeIter != compiledGraphData.m_nodes.end())
            {
                AZ::Entity* nodeEntity = (*nodeIter);
                auto nodeComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity);

                if (nodeComponent == nullptr)
                {
                    deletedNodeEntities.insert(nodeEntity->GetId());

                    delete nodeEntity;
                    nodeIter = compiledGraphData.m_nodes.erase(nodeIter);

                    continue;
                }

                bool disabledNode = false;

                if (!nodeComponent->IsNodeEnabled())
                {
                    disabledNode = true;
                    auto nodeSlots = nodeComponent->GetAllSlots();

                    for (const ScriptCanvas::Slot* slot : nodeSlots)
                    {
                        // While slot has a GetEndpoint method. It uses the Node pointer which has not been set yet.
                        // Bypass it for the more manual way in the builder.
                        disabledEndpoints.insert(ScriptCanvas::Endpoint(nodeEntity->GetId(), slot->GetId()));
                    }
                }

                if (disabledNode)
                {
                    delete nodeEntity;
                    nodeIter = compiledGraphData.m_nodes.erase(nodeIter);
                }
                else
                {
                    // Keep them in the map to make future look-ups easier.
                    nodeLookUpMap[nodeEntity->GetId()] = AZStd::make_pair(nodeComponent, nodeEntity);
                    ++nodeIter;
                }
            }
        }

        // Keep track of all the endpoints we've fully removed so we can cleanse out all of the invalid connections
        AZStd::unordered_set<ScriptCanvas::Endpoint> fullyRemovedEndpoints;

        while (!disabledEndpoints.empty())
        {
            // Keep track of the list of all the potentially disabled nodes that are a result of any of the current batch of disabled endpoints
            AZStd::unordered_set< NodeEntityPair > potentiallyDisabledNodes;

            for (const ScriptCanvas::Endpoint& disabledEndpoint : disabledEndpoints)
            {
                fullyRemovedEndpoints.insert(disabledEndpoint);

                AZStd::unordered_set<ScriptCanvas::Endpoint> reversedEndpoints;

                auto mapRange = compiledGraphData.m_endpointMap.equal_range(disabledEndpoint);

                // Start by looking up all of our connected endpoints so we can clean-up this map
                for (auto endpointIter = mapRange.first; endpointIter != mapRange.second; ++endpointIter)
                {
                    reversedEndpoints.insert(endpointIter->second);
                }

                // Remove all of the endpoint entries relating to the currently disabled map.
                compiledGraphData.m_endpointMap.erase(disabledEndpoint);

                // Go through all of the reversed connections and find all of the corresponding entries that
                // match our removed connection and remove them as well.
                for (auto disconnectedEndpoint : reversedEndpoints)
                {
                    auto mapRange2 = compiledGraphData.m_endpointMap.equal_range(disconnectedEndpoint);

                    for (auto endpointIter = mapRange2.first; endpointIter != mapRange2.second; ++endpointIter)
                    {
                        if (endpointIter->second == disabledEndpoint)
                        {
                            compiledGraphData.m_endpointMap.erase(endpointIter);
                            break;
                        }
                    }

                    // Look up the node so we can do some introspection on the slot.
                    auto nodeIter = nodeLookUpMap.find(disconnectedEndpoint.GetNodeId());

                    if (nodeIter != nodeLookUpMap.end())
                    {
                        ScriptCanvas::Node* node = nodeIter->second.first;

                        ScriptCanvas::Slot* slot = node->GetSlot(disconnectedEndpoint.GetSlotId());

                        // If the slot is an input, we want to recurse on that as a potentially disabled node. So keep track of it and we'll parse it in the next step.
                        if (slot && slot->IsExecution() && slot->IsInput())
                        {
                            // If we no longer have any active connections. We can also strip out this node to simplify down the graph further.
                            if (compiledGraphData.m_endpointMap.count(disconnectedEndpoint) == 0)
                            {
                                potentiallyDisabledNodes.insert(nodeIter->second);
                            }
                        }
                    }
                }
            }

            disabledEndpoints.clear();

            for (const NodeEntityPair& nodePair : potentiallyDisabledNodes)
            {
                ScriptCanvas::Node* node = nodePair.first;
                auto inputSlots = node->GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::ExecutionIn());

                bool hasExecutionIn = false;

                for (const ScriptCanvas::Slot* slot : inputSlots)
                {
                    // Can't use the slot method since it requires the node to be registered which has not happened yet.
                    ScriptCanvas::Endpoint endpoint = { nodePair.second->GetId(), slot->GetId() };
                    if (compiledGraphData.m_endpointMap.count(endpoint) > 0)
                    {
                        hasExecutionIn = true;
                        break;
                    }
                }

                // Once here we know the node has no input but was apart of the previous chain so we can just disable the entire node.
                // Since it will no longer have any incoming data via the disabled checks from above.
                if (!hasExecutionIn)
                {
                    auto disabledSlots = node->GetAllSlots();

                    for (auto disabledSlot : disabledSlots)
                    {
                        ScriptCanvas::Endpoint endpoint = { nodePair.second->GetId(), disabledSlot->GetId() };
                        disabledEndpoints.insert(endpoint);
                    }

                    nodeLookUpMap.erase(nodePair.second->GetId());
                    [[maybe_unused]] size_t eraseCount = compiledGraphData.m_nodes.erase(nodePair.second);
                    AZ_Assert(eraseCount == 1, "Failed to erase node from compiled graph data");

                    delete node;
                }
            }

            potentiallyDisabledNodes.clear();
        }

        {
            auto connectionIter = compiledGraphData.m_connections.begin();

            while (connectionIter != compiledGraphData.m_connections.end())
            {
                AZ::Entity* connectionEntity = (*connectionIter);
                auto connection = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Connection>(connectionEntity);
                AZ_Assert(connection, "Connection is missing connection component.");

                if (connection == nullptr)
                {
                    delete connectionEntity;
                    connectionIter = compiledGraphData.m_connections.erase(connectionIter);
                    continue;
                }

                ScriptCanvas::Endpoint targetEndpoint = connection->GetTargetEndpoint();
                ScriptCanvas::Endpoint sourceEndpoint = connection->GetSourceEndpoint();

                if (fullyRemovedEndpoints.count(targetEndpoint) > 0
                    || fullyRemovedEndpoints.count(sourceEndpoint) > 0
                    || deletedNodeEntities.count(targetEndpoint.GetNodeId()) > 0
                    || deletedNodeEntities.count(sourceEndpoint.GetNodeId()) > 0)
                {
                    delete connectionEntity;
                    connectionIter = compiledGraphData.m_connections.erase(connectionIter);
                }
                else
                {
                    ++connectionIter;
                }
            }
        }

        return AZ::Success(compiledGraphData);
    }

    AZ::Outcome<ScriptCanvas::VariableData, AZStd::string> CompileVariableData(AZ::Entity* scriptCanvasEntity)
    {
        if (!scriptCanvasEntity)
        {
            return AZ::Failure(AZStd::string("Cannot compile variable data from a nullptr Script Canvas Entity"));
        }

        auto sourceGraphVariableManager = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::EditorGraphVariableManagerComponent>(scriptCanvasEntity);
        if (!sourceGraphVariableManager)
        {
            return AZ::Failure(AZStd::string("Failed to find Editor Script Canvas Graph Variable Manager Component"));
        }

        return AZ::Success(*sourceGraphVariableManager->GetVariableData());
    }

    int GetBuilderVersion()
    {
        // #functions2 remove-execution-out-hash include version from all library nodes, split fingerprint generation to relax Is Out of Data restriction when graphs only need a recompile
        return static_cast<int>(BuilderVersion::Current)
            + static_cast<int>(ScriptCanvas::GrammarVersion::Current)
            + static_cast<int>(ScriptCanvas::RuntimeVersion::Current)
            ;
    }

    AZ::Outcome < AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset>, AZStd::string> LoadEditorAsset(AZStd::string_view filePath, AZ::Data::AssetId assetId, AZ::Data::AssetFilterCB assetFilterCB)
    {
        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream stream(filePath.data(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", filePath.data());
                AZ::Failure(AZStd::string::format("Failed to load ScriptCavas asset: %s", filePath.data()));
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be read.", filePath.data());
                AZ::Failure(AZStd::string::format("Failed to load ScriptCavas asset: %s", filePath.data()));
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        ScriptCanvasEditor::ScriptCanvasAssetHandler editorAssetHandler;

        AZ::SerializeContext* context{};
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> asset;
        asset.Create(assetId);

        if (editorAssetHandler.LoadAssetData(asset, assetDataStream, assetFilterCB) != AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            return AZ::Failure(AZStd::string::format("Failed to load ScriptCavas asset: %s", filePath.data()));
        }

        return AZ::Success(asset);
    }

    ScriptCanvasEditor::Graph* PrepareSourceGraph(AZ::Entity* const buildEntity)
    {
        auto sourceGraph = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::Graph>(buildEntity);
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
        constexpr bool writeAssetInfo{ true };
        auto compileOutcome = AzFramework::CompileScriptAndSaveAsset(compileRequest, writeAssetInfo);
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
        const AZ::Data::Asset<AZ::ScriptAsset> scriptAsset(scriptAssetId, jobProduct.m_productAssetType, {});
        input.runtimeDataOut.m_script = scriptAsset;

        const ScriptCanvas::OrderedDependencies& orderedDependencies = translationResult.m_model->GetOrderedDependencies();
        const ScriptCanvas::DependencyReport& dependencyReport = orderedDependencies.source;

        for (const auto& subgraphAssetID : orderedDependencies.orderedAssetIds)
        {
            const AZ::Data::AssetId dependentSubgraphAssetID(subgraphAssetID.m_guid, AZ_CRC("RuntimeData", 0x163310ae));
            const AZ::Data::Asset<ScriptCanvas::RuntimeAsset> subgraphAsset(dependentSubgraphAssetID, azrtti_typeid<ScriptCanvas::RuntimeAsset>(), {});
            input.runtimeDataOut.m_requiredAssets.push_back(subgraphAsset);
        }

        for (const auto& scriptEventAssetID : dependencyReport.scriptEventsAssetIds)
        {
            const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> subgraphAsset(scriptEventAssetID, azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), {});
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
        runtimeAsset.Get()->SetData(subgraphInterface);

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
        runtimeAsset.Create(AZ::Data::AssetId(input.assetID.m_guid, AZ_CRC("RuntimeData", 0x163310ae)));
        runtimeAsset.Get()->SetData(runtimeData);

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
            AZ_CRC("RuntimeData", 0x163310ae), jobProduct);

        // Output Object marks dependencies as handled.
        // We still have more to evaluate
        jobProduct.m_dependenciesHandled = false;

        jobProduct.m_dependencies.push_back({ runtimeData.m_script.GetId(), {} });

        for (const auto& assetDependency : runtimeData.m_requiredAssets)
        {
            if (AZ::Data::AssetManager::Instance().GetAsset(assetDependency.GetId(), assetDependency.GetType(), AZ::Data::AssetLoadBehavior::PreLoad))
            {
                jobProduct.m_dependencies.push_back({ assetDependency.GetId(), {} });
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
                jobProduct.m_dependencies.push_back({ scriptEventDependency.GetId(), {} });
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
