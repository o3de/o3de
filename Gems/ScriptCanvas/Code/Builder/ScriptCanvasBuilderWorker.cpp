/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"

#include <AssetBuilderSDK/SerializationDependencies.h>

#include <AzCore/Math/Uuid.h>

#include <Builder/ScriptCanvasBuilderWorker.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Asset/Functions/RuntimeFunctionAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Assets/Functions/ScriptCanvasFunctionAssetHandler.h>

#include <Asset/AssetDescription.h>
#include <Asset/Functions/ScriptCanvasFunctionAsset.h>

namespace ScriptCanvasBuilder
{
    static const char* s_scriptCanvasBuilder = "ScriptCanvasBuilder";
    static const char* s_scriptCanvasFunctionBuilder = "ScriptCanvasFunctionBuilder";

    static AZ::Outcome<ScriptCanvas::GraphData, AZStd::string> CompileGraphData(AZ::Entity* scriptCanvasEntity);
    static AZ::Outcome<ScriptCanvas::VariableData, AZStd::string> CompileVariableData(AZ::Entity* scriptCanvasEntity);

    Worker::Worker()
    {
    }

    Worker::~Worker()
    {
        Deactivate();
    }

    int Worker::GetVersionNumber() const
    {
        return 4;
    }

    const char* Worker::GetFingerprintString() const
    {
        if (m_fingerprintString.empty())
        {
            // compute it the first time
            const AZStd::string runtimeAssetTypeId = azrtti_typeid<ScriptCanvas::RuntimeAsset>().ToString<AZStd::string>();
            m_fingerprintString = AZStd::string::format("%i%s", GetVersionNumber(), runtimeAssetTypeId.c_str());
        }
        return m_fingerprintString.c_str();
    }

    void Worker::Activate()
    {
        // Use AssetCatalog service to register ScriptCanvas asset type and extension
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ScriptCanvas::AssetDescription::GetExtension<ScriptCanvasEditor::ScriptCanvasAsset>());

        m_editorAssetHandler = AZStd::make_unique<ScriptCanvasEditor::ScriptCanvasAssetHandler>();
        if (!AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(m_editorAssetHandler.get(), assetType);
        }

        // Runtime Asset
        m_runtimeAssetHandler = AZStd::make_unique<ScriptCanvas::RuntimeAssetHandler>();
        if (!AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<ScriptCanvas::RuntimeAsset>()))
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(m_runtimeAssetHandler.get(), azrtti_typeid<ScriptCanvas::RuntimeAsset>());
        }
    }

    AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> Worker::CreateRuntimeAsset(AZStd::string_view filePath)
    {
        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream ioStream;
            if (!ioStream.Open(filePath.data(), AZ::IO::OpenMode::ModeRead))
            {
                return AZ::Failure(AZStd::string::format("File failed to open: %s", filePath.data()));
            }
            AZStd::vector<AZ::u8> fileBuffer(ioStream.GetLength());
            size_t bytesRead = ioStream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != ioStream.GetLength())
            {
                return AZ::Failure(AZStd::string::format("File failed to read completely: %s", filePath.data()));
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> asset;

        if (AzFramework::StringFunc::Path::IsExtension(filePath.data(), ScriptCanvas::AssetDescription::GetExtension<ScriptCanvas::ScriptCanvasFunctionAsset>(), true))
        {
            ScriptCanvasEditor::ScriptCanvasFunctionAssetHandler functionAssetHandler;
            asset = ProcessEditorAsset(functionAssetHandler, assetDataStream);
        }
        else
        {
            ScriptCanvasEditor::ScriptCanvasAssetHandler editorAssetHandler;
            asset = ProcessEditorAsset(editorAssetHandler, assetDataStream);
        }

   
        if (!asset.Get())
        {
            return AZ::Failure(AZStd::string::format("Editor Asset failed to process: %s", filePath.data()));
        }

        return AZ::Success(asset);
    }
    
    void Worker::Deactivate()
    {
        if (m_editorAssetHandler)
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_editorAssetHandler.get());
            m_editorAssetHandler.reset();
        }        
                
        if (m_runtimeAssetHandler)
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_runtimeAssetHandler.get());
            m_runtimeAssetHandler.reset();
        }
    }

    void Worker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void Worker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_scriptCanvasBuilder, "CreateJobs for script canvas \"%s\"\n", fullPath.data());

        if (!m_editorAssetHandler)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(CreateJobs for %s failed because the ScriptCanvas Editor Asset handler is missing.)", fullPath.data());
        }

        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.data());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be read.", fullPath.data());
                return;
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        // Asset filter always returns false to prevent parsing dependencies, but makes note of the script canvas dependencies
        auto assetFilter = [&response](const AZ::Data::AssetFilterInfo& filterInfo)
        {
            if (filterInfo.m_assetType == azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptCanvas::RuntimeAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
            {
                AssetBuilderSDK::SourceFileDependency dependency;
                dependency.m_sourceFileDependencyUUID = filterInfo.m_assetId.m_guid;

                response.m_sourceFileDependencyList.push_back(dependency);
            }

            return false;
        };

        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        if (m_editorAssetHandler->LoadAssetDataFromStream(asset, assetDataStream, assetFilter) !=
            AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the asset data could not be loaded from the file", fullPath.data());
            return;
        }

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 2;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "Script Canvas";
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());
            jobDescriptor.m_additionalFingerprintInfo = GetFingerprintString();

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void Worker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // A runtime script canvas component is generated, which creates a .scriptcanvas_compiled file
        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_scriptCanvasBuilder, "Processing script canvas \"%s\".\n", fullPath.c_str());

        if (!m_editorAssetHandler)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Exporting of .scriptcanvas for "%s" file failed as no editor asset handler was registered for script canvas. The ScriptCanvas Gem might not be enabled.)", fullPath.data());
            return;
        }

        if (!m_runtimeAssetHandler)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Exporting of .scriptcanvas for "%s" file failed as no runtime asset handler was registered for script canvas.)", fullPath.data());
            return;
        }

        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!stream.IsOpen())
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "Exporting of .scriptcanvas for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "Exporting of .scriptcanvas for \"%s\" failed because the source file could not be read.", fullPath.c_str());
                return;
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        // Asset filter is used to record dependencies.  Only returns true for editor script canvas assets
        auto assetFilter = [](const AZ::Data::AssetFilterInfo& filterInfo)
        {
            return filterInfo.m_assetType == azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>();
        };

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset preload\n");
        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        if (m_editorAssetHandler->LoadAssetDataFromStream(asset, assetDataStream, assetFilter) !=
            AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Loading of ScriptCanvas asset for source file "%s" has failed)", fullPath.data());
            return;
        }

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset loaded successfully\n");

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZStd::string runtimeScriptCanvasOutputPath;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), runtimeScriptCanvasOutputPath,  true, true);
        AzFramework::StringFunc::Path::ReplaceExtension(runtimeScriptCanvasOutputPath, ScriptCanvas::RuntimeAsset::GetFileExtension());

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset graph editor precompile\n");
        AZ::Entity* buildEntity = asset.Get()->GetScriptCanvasEntity();
        auto compileGraphOutcome = CompileGraphData(buildEntity);
        if (!compileGraphOutcome)
        {
            AZ_Error(s_scriptCanvasBuilder, false, "%s in script canvas file %s", compileGraphOutcome.GetError().data(), fullPath.data());
            return;
        }
        
        auto compileVariablesOutcome = CompileVariableData(buildEntity);
        if (!compileVariablesOutcome)
        {
            AZ_Error(s_scriptCanvasBuilder, false, "%s in script canvas file %s", compileVariablesOutcome.GetError().data(), fullPath.data());
            return;
        }
        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset graph compile finished\n");

        ScriptCanvas::RuntimeData runtimeData;
        runtimeData.m_graphData = compileGraphOutcome.TakeValue();
        runtimeData.m_variableData = compileVariablesOutcome.TakeValue();

        // Populate the runtime Asset 
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
        runtimeAsset.Create(AZ::Uuid::CreateRandom());
        runtimeAsset.Get()->SetData(runtimeData);

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset presave to object stream for %s\n", fullPath.c_str());
        bool runtimeCanvasSaved = m_runtimeAssetHandler->SaveAssetData(runtimeAsset, &byteStream);

        if (!runtimeCanvasSaved)
        {
            AZ_Error(s_scriptCanvasBuilder, runtimeCanvasSaved, "Failed to save runtime script canvas to object stream");
            return;
        }
        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset has been saved to the object stream successfully\n");

        AZ::IO::FileIOStream outFileStream(runtimeScriptCanvasOutputPath.data(), AZ::IO::OpenMode::ModeWrite);
        if (!outFileStream.IsOpen())
        {
            AZ_Error(s_scriptCanvasBuilder, false, "Failed to open output file %s", runtimeScriptCanvasOutputPath.data());
            return;
        }

        runtimeCanvasSaved = outFileStream.Write(byteBuffer.size(), byteBuffer.data()) == byteBuffer.size() && runtimeCanvasSaved;
        if (!runtimeCanvasSaved)
        {
            AZ_Error(s_scriptCanvasBuilder, runtimeCanvasSaved, "Unable to save runtime script canvas file %s", runtimeScriptCanvasOutputPath.data());
            return;
        }

        // ScriptCanvas Editor Asset Copy job
        // The SubID is zero as this represents the main asset
        AssetBuilderSDK::JobProduct jobProduct;
        // Even though this is an Editor only asset, it's still important to gather product dependencies so that they get loaded correctly
        // when this is loaded in the Editor.
        if (!AssetBuilderSDK::OutputObject(&asset->GetScriptCanvasData(), fullPath, azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>(), 0, jobProduct))
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Failed to output product dependencies.");
            return;
        }

        response.m_outputProducts.push_back(AZStd::move(jobProduct));

        // Runtime ScriptCanvas
        jobProduct = {};
        if(!AssetBuilderSDK::OutputObject(&runtimeAsset.Get()->GetData(), runtimeScriptCanvasOutputPath, azrtti_typeid<ScriptCanvas::RuntimeAsset>(), AZ_CRC("RuntimeData", 0x163310ae), jobProduct))
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Failed to output product dependencies.");
            return;
        }
        response.m_outputProducts.push_back(AZStd::move(jobProduct));

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        AZ_TracePrintf(s_scriptCanvasBuilder, "Finished processing script canvas %s\n", fullPath.c_str());
    }

    bool Worker::GatherProductDependencies(
        AZ::SerializeContext& serializeContext,
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset>& runtimeAsset,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencySet)
    {
        return AssetBuilderSDK::GatherProductDependencies(serializeContext, &runtimeAsset.Get()->GetData(), productDependencies, productPathDependencySet);
    }

    AZ::Data::Asset<ScriptCanvas::RuntimeAsset> Worker::ProcessEditorAsset(AZ::Data::AssetHandler& editorAssetHandler,
                                                                           AZStd::shared_ptr<AZ::Data::AssetDataStream> stream)
    {
        AZ::SerializeContext* context{};
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        
        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset preload\n");
        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        
        if (editorAssetHandler.LoadAssetDataFromStream(asset, stream, AZ::Data::AssetFilterCB{}) !=
            AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Loading of ScriptCanvas asset has failed)");
            return AZ::Data::Asset<ScriptCanvas::RuntimeAsset>{};
        }

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset loaded successfully\n");

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset graph editor precompile\n");
        AZ::Entity* buildEntity = asset.Get()->GetScriptCanvasEntity();
        auto compileGraphOutcome = CompileGraphData(buildEntity);
        if (!compileGraphOutcome)
        {
            AZ_Error(s_scriptCanvasBuilder, false, "%s in editor script canvas stream", compileGraphOutcome.GetError().data());
            return AZ::Data::Asset<ScriptCanvas::RuntimeAsset>{};
        }

        auto compileVariablesOutcome = CompileVariableData(buildEntity);
        if (!compileVariablesOutcome)
        {
            AZ_Error(s_scriptCanvasBuilder, false, "%s in script canvas stream", compileVariablesOutcome.GetError().data());
            return AZ::Data::Asset<ScriptCanvas::RuntimeAsset>{};
        }
        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset graph compile finished\n");

        ScriptCanvas::RuntimeData runtimeData;
        runtimeData.m_graphData = compileGraphOutcome.TakeValue();
        runtimeData.m_variableData = compileVariablesOutcome.TakeValue();

        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
        runtimeAsset.Create(AZ::Uuid::CreateRandom());
        runtimeAsset.Get()->SetData(runtimeData);
        return runtimeAsset;
    }

    AZ::Uuid Worker::GetUUID()
    {
        return AZ::Uuid::CreateString("{6E86272B-7C06-4A65-9C25-9FA4AE21F993}");
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

                nodeComponent->SetExecutionType(ScriptCanvas::ExecutionType::Runtime);

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
                // Since it will no longer have any incomming data via the disabled checks from above.
                if (!hasExecutionIn)
                {
                    auto disabledSlots = node->GetAllSlots();

                    for (auto disabledSlot : disabledSlots)
                    {
                        ScriptCanvas::Endpoint endpoint = { nodePair.second->GetId(), disabledSlot->GetId() };
                        disabledEndpoints.insert(endpoint);
                    }

                    nodeLookUpMap.erase(nodePair.second->GetId());
                    size_t eraseCount = compiledGraphData.m_nodes.erase(nodePair.second);
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

    const char* FunctionWorker::GetFingerprintString() const
    {
        if (m_fingerprintString.empty())
        {
            // compute it the first time
            const AZStd::string runtimeAssetTypeId = azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>().ToString<AZStd::string>();
            m_fingerprintString = AZStd::string::format("%i%s", GetVersionNumber(), runtimeAssetTypeId.c_str());
        }
        return m_fingerprintString.c_str();
    }
    
    void FunctionWorker::Activate()
    {
        // Use AssetCatalog service to register ScriptCanvas asset type and extension
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>());

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ScriptCanvas::AssetDescription::GetExtension<ScriptCanvas::ScriptCanvasFunctionAsset>());

        m_functionAssetHandler = AZStd::make_unique<ScriptCanvasEditor::ScriptCanvasFunctionAssetHandler>();
        if (!AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(m_functionAssetHandler.get(), assetType);
        }

        // Runtime Asset
        m_runtimeAssetHandler = AZStd::make_unique<ScriptCanvas::RuntimeFunctionAssetHandler>();
        if (!AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>()))
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(m_runtimeAssetHandler.get(), azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>());
        }
    }

    AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> FunctionWorker::CreateRuntimeAsset(AZStd::string_view filePath)
    {
        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream ioStream;
            if (!ioStream.Open(filePath.data(), AZ::IO::OpenMode::ModeRead))
            {
                return AZ::Failure(AZStd::string::format("File failed to open: %s", filePath.data()));
            }
            AZStd::vector<AZ::u8> fileBuffer(ioStream.GetLength());
            size_t bytesRead = ioStream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != ioStream.GetLength())
            {
                return AZ::Failure(AZStd::string::format("File failed to read successfully: %s", filePath.data()));
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> asset;

        if (AzFramework::StringFunc::Path::IsExtension(filePath.data(), ScriptCanvas::AssetDescription::GetExtension<ScriptCanvas::ScriptCanvasFunctionAsset>(), true))
        {
            ScriptCanvasEditor::ScriptCanvasFunctionAssetHandler functionAssetHandler;
            asset = ProcessFunctionAsset(functionAssetHandler, assetDataStream);
        }
        else
        {
            ScriptCanvasEditor::ScriptCanvasAssetHandler editorAssetHandler;
            asset = ProcessFunctionAsset(editorAssetHandler, assetDataStream);
        }


        if (!asset.Get())
        {
            return AZ::Failure(AZStd::string::format("Editor Asset failed to process: %s", filePath.data()));
        }

        return AZ::Success(asset);
    }

    AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset> FunctionWorker::ProcessFunctionAsset(
        AZ::Data::AssetHandler& functionAssetHandler,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream)
    {
        AZ::SerializeContext* context{};
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZ_TracePrintf(s_scriptCanvasFunctionBuilder, "Script Canvas Asset preload\n");
        AZ::Data::Asset<ScriptCanvas::ScriptCanvasFunctionAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        if (functionAssetHandler.LoadAssetDataFromStream(asset, stream, AZ::Data::AssetFilterCB{}) !=
            AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Error(s_scriptCanvasFunctionBuilder, false, R"(Loading of ScriptCanvas asset has failed)");
            return AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset>{};
        }

        AZ_TracePrintf(s_scriptCanvasFunctionBuilder, "Script Canvas Asset loaded successfully\n");

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZ_TracePrintf(s_scriptCanvasFunctionBuilder, "Script Canvas Asset graph editor precompile\n");
        AZ::Entity* buildEntity = asset.Get()->GetScriptCanvasEntity();
        auto compileGraphOutcome = CompileGraphData(buildEntity);
        if (!compileGraphOutcome)
        {
            AZ_Error(s_scriptCanvasFunctionBuilder, false, "%s in editor script canvas stream", compileGraphOutcome.GetError().data());
            return AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset>{};
        }

        auto compileVariablesOutcome = CompileVariableData(buildEntity);
        if (!compileVariablesOutcome)
        {
            AZ_Error(s_scriptCanvasFunctionBuilder, false, "%s in script canvas stream", compileVariablesOutcome.GetError().data());
            return AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset>{};
        }
        AZ_TracePrintf(s_scriptCanvasFunctionBuilder, "Script Canvas Asset graph compile finished\n");

        ScriptCanvas::ScriptCanvasFunctionDataComponent* sourceDataComponent = asset.Get()->GetFunctionData();

        ScriptCanvas::FunctionRuntimeData runtimeData;
        runtimeData.m_name = asset.Get()->GetScriptCanvasEntity()->GetName();
        runtimeData.m_version = sourceDataComponent->m_version;
        runtimeData.m_executionNodeOrder = sourceDataComponent->m_executionNodeOrder;
        runtimeData.m_variableOrder = sourceDataComponent->m_variableOrder;
        runtimeData.m_graphData = compileGraphOutcome.TakeValue();
        runtimeData.m_variableData = compileVariablesOutcome.TakeValue();

        AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset> runtimeAsset;
        runtimeAsset.Create(AZ::Uuid::CreateRandom());
        runtimeAsset.Get()->SetData(runtimeData);
        return runtimeAsset;
    }

    void FunctionWorker::Deactivate()
    {
        if (m_functionAssetHandler)
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_functionAssetHandler.get());
            m_functionAssetHandler.reset();
        }

        if (m_runtimeAssetHandler)
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_runtimeAssetHandler.get());
            m_runtimeAssetHandler.reset();
        }
    }

    AZ::Uuid FunctionWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{7227E0E1-4113-456A-877B-B2276ACB292B}");
    }

    void FunctionWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void FunctionWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_scriptCanvasBuilder, "CreateJobs for script canvas \"%s\"\n", fullPath.data());

        if (!m_functionAssetHandler)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(CreateJobs for %s failed because the ScriptCanvas Editor Asset handler is missing.)", fullPath.data());
        }

        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.data());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be read.", fullPath.data());
                return;
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        // Asset filter always returns false to prevent parsing dependencies, but makes note of the script canvas dependencies
        auto assetFilter = [&response](const AZ::Data::AssetFilterInfo& filterInfo)
        {
            if (filterInfo.m_assetType == azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptCanvas::RuntimeAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>())
            {
                AssetBuilderSDK::SourceFileDependency dependency;
                dependency.m_sourceFileDependencyUUID = filterInfo.m_assetId.m_guid;

                response.m_sourceFileDependencyList.push_back(dependency);
            }

            return false;
        };

        AZ::Data::Asset<ScriptCanvas::ScriptCanvasFunctionAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        if (m_functionAssetHandler->LoadAssetDataFromStream(asset, assetDataStream, assetFilter) !=
            AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the asset data could not be loaded from the file", fullPath.data());
            return;
        }

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 2;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "Script Canvas";
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());
            jobDescriptor.m_additionalFingerprintInfo = GetFingerprintString();

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void FunctionWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // A runtime script canvas component is generated, which creates a .scriptcanvas_compiled file
        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_scriptCanvasBuilder, "Processing script canvas \"%s\".\n", fullPath.c_str());

        if (!m_functionAssetHandler)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Exporting of .scriptcanvas for "%s" file failed as no editor asset handler was registered for script canvas. The ScriptCanvas Gem might not be enabled.)", fullPath.data());
            return;
        }

        if (!m_runtimeAssetHandler)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Exporting of .scriptcanvas for "%s" file failed as no runtime asset handler was registered for script canvas.)", fullPath.data());
            return;
        }

        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!stream.IsOpen())
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "Exporting of .scriptcanvas for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "Exporting of .scriptcanvas for \"%s\" failed because the source file could not be read.", fullPath.c_str());
                return;
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        AZ::SerializeContext* context{};
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;

        // Asset filter is used to record dependencies.  Only returns true for editor script canvas assets
        auto assetFilter = [&productDependencies](const AZ::Data::AssetFilterInfo& filterInfo)
        {
            productDependencies.emplace_back(filterInfo.m_assetId, 0);

            return filterInfo.m_assetType == azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>();
        };

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset preload\n");
        AZ::Data::Asset<ScriptCanvas::ScriptCanvasFunctionAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        if (m_functionAssetHandler->LoadAssetDataFromStream(asset, assetDataStream, assetFilter) !=
            AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Loading of ScriptCanvas asset for source file "%s" has failed)", fullPath.data());
            return;
        }

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset loaded successfully\n");

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZStd::string runtimeScriptCanvasOutputPath;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), runtimeScriptCanvasOutputPath,  true, true);
        AzFramework::StringFunc::Path::ReplaceExtension(runtimeScriptCanvasOutputPath, ScriptCanvas::RuntimeFunctionAsset::GetFileExtension());

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset graph editor precompile\n");
        AZ::Entity* buildEntity = asset.Get()->GetScriptCanvasEntity();
        auto compileGraphOutcome = CompileGraphData(buildEntity);
        if (!compileGraphOutcome)
        {
            AZ_Error(s_scriptCanvasBuilder, false, "%s in script canvas file %s", compileGraphOutcome.GetError().data(), fullPath.data());
            return;
        }

        auto compileVariablesOutcome = CompileVariableData(buildEntity);
        if (!compileVariablesOutcome)
        {
            AZ_Error(s_scriptCanvasBuilder, false, "%s in script canvas file %s", compileVariablesOutcome.GetError().data(), fullPath.data());
            return;
        }
        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset graph compile finished\n");

        AZStd::string functionName = fileNameOnly;
        AzFramework::StringFunc::Path::StripExtension(functionName);

        ScriptCanvas::ScriptCanvasFunctionDataComponent* sourceDataComponent = asset.Get()->GetFunctionData();

        ScriptCanvas::FunctionRuntimeData runtimeData;
        runtimeData.m_name = functionName;

        runtimeData.m_version = sourceDataComponent->m_version;
        runtimeData.m_executionNodeOrder = sourceDataComponent->m_executionNodeOrder;
        runtimeData.m_variableOrder = sourceDataComponent->m_variableOrder;
        runtimeData.m_graphData = compileGraphOutcome.TakeValue();
        runtimeData.m_variableData = compileVariablesOutcome.TakeValue();

        // Populate the runtime Asset 
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset> runtimeAsset;
        if (!runtimeAsset.Create(AZ::Uuid::CreateRandom()))
        {
            AZ_Error(s_scriptCanvasBuilder, false, "Error creating runtime asset for: %s", fullPath.data());
            return;

        }

        if (runtimeAsset.Get())
        {
            runtimeAsset.Get()->SetData(runtimeData);
        }

        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset presave to object stream for %s\n", fullPath.c_str());
        bool runtimeCanvasSaved = m_runtimeAssetHandler->SaveAssetData(runtimeAsset, &byteStream);

        if (!runtimeCanvasSaved)
        {
            AZ_Error(s_scriptCanvasBuilder, runtimeCanvasSaved, "Failed to save runtime script canvas to object stream");
            return;
        }
        AZ_TracePrintf(s_scriptCanvasBuilder, "Script Canvas Asset has been saved to the object stream successfully\n");

        AZ::IO::FileIOStream outFileStream(runtimeScriptCanvasOutputPath.data(), AZ::IO::OpenMode::ModeWrite);
        if (!outFileStream.IsOpen())
        {
            AZ_Error(s_scriptCanvasBuilder, false, "Failed to open output file %s", runtimeScriptCanvasOutputPath.data());
            return;
        }

        runtimeCanvasSaved = outFileStream.Write(byteBuffer.size(), byteBuffer.data()) == byteBuffer.size() && runtimeCanvasSaved;
        if (!runtimeCanvasSaved)
        {
            AZ_Error(s_scriptCanvasBuilder, runtimeCanvasSaved, "Unable to save runtime script canvas file %s", runtimeScriptCanvasOutputPath.data());
            return;
        }

        // ScriptCanvas Editor Asset Copy job
        // The SubID is zero as this represents the main asset
        AssetBuilderSDK::JobProduct jobProduct;
        jobProduct.m_productFileName = fullPath;
        jobProduct.m_productAssetType = azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>();
        jobProduct.m_productSubID = 0;
        response.m_outputProducts.push_back(AZStd::move(jobProduct));

        // Runtime ScriptCanvas
        jobProduct = {};
        jobProduct.m_productFileName = runtimeScriptCanvasOutputPath;
        jobProduct.m_productAssetType = azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>();
        jobProduct.m_productSubID = AZ_CRC("RuntimeData", 0x163310ae);
        jobProduct.m_dependencies = AZStd::move(productDependencies);
        response.m_outputProducts.push_back(AZStd::move(jobProduct));

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        AZ_TracePrintf(s_scriptCanvasBuilder, "Finished processing script canvas %s\n", fullPath.c_str());
    }
}
