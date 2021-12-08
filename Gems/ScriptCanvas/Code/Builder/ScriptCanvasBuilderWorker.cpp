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
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Results/ErrorText.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasBuilder
{
    void Worker::Activate(const AssetHandlers& handlers)
    {
        m_runtimeAssetHandler = handlers.m_runtimeAssetHandler;
        m_subgraphInterfaceHandler = handlers.m_subgraphInterfaceHandler;
    }

    void Worker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        AZ_TracePrintf(s_scriptCanvasBuilder, "Start Creating Job");
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> asset;
        const ScriptCanvasEditor::Graph* sourceGraph = nullptr;
        const ScriptCanvas::GraphData* graphData = nullptr;

        ScriptCanvasEditor::SourceHandle sourceHandle;

        auto sourceOutcome = ScriptCanvasEditor::LoadFromFile(fullPath);
        if (sourceOutcome.IsSuccess())
        {
            sourceHandle = sourceOutcome.TakeValue();
            sourceGraph = sourceHandle.Get();
            graphData = sourceGraph->GetGraphDataConst();
        }
        
        AZ_Assert(sourceGraph, "Graph component is missing from entity.");
        AZ_Assert(graphData, "GraphData is missing from entity");

        struct EntityIdComparer
        {
            bool operator()(AZ::Entity* lhs, AZ::Entity* rhs)
            {
                AZ::EntityId lhsEntityId = lhs != nullptr ? lhs->GetId() : AZ::EntityId();
                AZ::EntityId rhsEntityId = rhs != nullptr ? rhs->GetId() : AZ::EntityId();
                return lhsEntityId < rhsEntityId;
            }
        };
        const AZStd::set<AZ::Entity*, EntityIdComparer> sortedEntities(graphData->m_nodes.begin(), graphData->m_nodes.end());

        size_t fingerprint = 0;
        for (const auto& nodeEntity : sortedEntities)
        {
            if (auto nodeComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity))
            {
                AZStd::hash_combine(fingerprint, nodeComponent->GenerateFingerprint());
            }
        }

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "SerializeContext is required to enumerate dependent assets in the ScriptCanvas file");

        AZStd::unordered_multimap<AZStd::string, AssetBuilderSDK::SourceFileDependency> jobDependenciesByKey;
        auto assetFilter = [this, &jobDependenciesByKey]
            ( void* instancePointer
            , const AZ::SerializeContext::ClassData* classData
            , [[maybe_unused]] const AZ::SerializeContext::ClassElement* classElement)
        {
            auto azTypeId = classData->m_azRtti->GetTypeId();
            
            if (azTypeId == azrtti_typeid<AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>>())
            {
                const auto* subgraphAsset = reinterpret_cast<AZ::Data::Asset<const ScriptCanvas::SubgraphInterfaceAsset>*>(instancePointer);
                if (subgraphAsset->GetId().IsValid())
                {
                    AssetBuilderSDK::SourceFileDependency dependency;
                    dependency.m_sourceFileDependencyUUID = subgraphAsset->GetId().m_guid;
                    jobDependenciesByKey.insert({ s_scriptCanvasProcessJobKey, dependency });
                    this->m_processEditorAssetDependencies.push_back
                        ( { subgraphAsset->GetId(), azTypeId, AZ::Data::AssetLoadBehavior::PreLoad });
                }
            }
            else if (azTypeId == azrtti_typeid<AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>>())
            {
                const auto* eventAsset = reinterpret_cast<AZ::Data::Asset<const ScriptEvents::ScriptEventsAsset>*>(instancePointer);
                if (eventAsset->GetId().IsValid())
                {
                    AssetBuilderSDK::SourceFileDependency dependency;
                    dependency.m_sourceFileDependencyUUID = eventAsset->GetId().m_guid;
                    jobDependenciesByKey.insert({ ScriptEvents::k_builderJobKey, dependency });
                    this->m_processEditorAssetDependencies.push_back
                        ( { eventAsset->GetId(), azTypeId, AZ::Data::AssetLoadBehavior::PreLoad });
                }
            }

            // always continue, make note of the script canvas dependencies
            return true;
        };

        AZ_Verify(serializeContext->EnumerateInstanceConst
            ( graphData
            , azrtti_typeid<ScriptCanvas::GraphData>()
            , assetFilter
            , {}
            , AZ::SerializeContext::ENUM_ACCESS_FOR_READ
            , nullptr
            , nullptr), "Failed to gather dependencies from graph data");

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 2;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = s_scriptCanvasProcessJobKey;
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = AZStd::string(GetFingerprintString()).append("|").append(AZStd::to_string(static_cast<AZ::u64>(fingerprint)));

            // Graph process job needs to wait until its dependency asset job finished
            for (const auto& processingDependency : jobDependenciesByKey)
            {
                response.m_sourceFileDependencyList.push_back(processingDependency.second);
                jobDescriptor.m_jobDependencyList.emplace_back(processingDependency.first, info.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Order, processingDependency.second);
            }

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        AZ_TracePrintf(s_scriptCanvasBuilder, "Finish Creating Job");
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

    int Worker::GetVersionNumber() const
    {
        return GetBuilderVersion();
    }

    AZ::Uuid Worker::GetUUID()
    {
        return AZ::Uuid::CreateString("{6E86272B-7C06-4A65-9C25-9FA4AE21F993}");
    }

    void Worker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        AZ_TracePrintf(s_scriptCanvasBuilder, "Start Processing Job");
        // A runtime script canvas component is generated, which creates a .scriptcanvas_compiled file
        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        bool pathFound = false;
        AZStd::string relativePath;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult
            ( pathFound
            , &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath
            , request.m_fullPath.c_str(), relativePath);

        if (!pathFound)
        {
            AZ_Error(s_scriptCanvasBuilder, false, "Failed to get engine relative path from %s", request.m_fullPath.c_str());
            return;
        }

        if (!m_runtimeAssetHandler)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Exporting of .scriptcanvas for "%s" file failed as no runtime asset handler was registered for script canvas.)", fullPath.data());
            return;
        }

        auto loadOutcome = ScriptCanvasEditor::LoadFromFile(request.m_fullPath);
        if (!loadOutcome.IsSuccess())
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Loading of ScriptCanvas asset for source file "%s" has failed)", fullPath.data());
            return;
        }

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZStd::string runtimeScriptCanvasOutputPath;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), runtimeScriptCanvasOutputPath, true, true);
        AzFramework::StringFunc::Path::ReplaceExtension(runtimeScriptCanvasOutputPath, ScriptCanvas::RuntimeAsset::GetFileExtension());

        auto sourceHandle = loadOutcome.TakeValue();

        if (request.m_jobDescription.m_jobKey == s_scriptCanvasProcessJobKey)
        {
            AZ::Entity* buildEntity = sourceHandle.Get()->GetEntity();
            ProcessTranslationJobInput input;
            input.assetID = AZ::Data::AssetId(request.m_sourceFileUUID, AZ_CRC("RuntimeData", 0x163310ae));
            input.request = &request;
            input.response = &response;
            input.runtimeScriptCanvasOutputPath = runtimeScriptCanvasOutputPath;
            input.assetHandler = m_runtimeAssetHandler;
            input.buildEntity = buildEntity;
            input.fullPath = fullPath;
            input.fileNameOnly = fileNameOnly;
            input.namespacePath = relativePath;
            input.saveRawLua = true;

            auto translationOutcome = ProcessTranslationJob(input);
            if (translationOutcome.IsSuccess())
            {
                auto saveOutcome = SaveRuntimeAsset(input, input.runtimeDataOut);
                if (saveOutcome.IsSuccess())
                {
                    // save function interface
                    AzFramework::StringFunc::Path::StripExtension(fileNameOnly);
                    ScriptCanvas::SubgraphInterfaceData functionInterface;
                    functionInterface.m_name = fileNameOnly;
                    functionInterface.m_interface = AZStd::move(input.interfaceOut);
                    input.assetHandler = m_subgraphInterfaceHandler;

                    AzFramework::StringFunc::Path::ReplaceExtension(input.runtimeScriptCanvasOutputPath, ScriptCanvas::SubgraphInterfaceAsset::GetFileExtension());
                    auto saveInterfaceOutcome = SaveSubgraphInterface(input, functionInterface);
                    if (saveInterfaceOutcome.IsSuccess())
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                    }
                    else
                    {
                        AZ_Error(s_scriptCanvasBuilder, false, saveInterfaceOutcome.GetError().data());
                    }
                }
                else
                {
                    AZ_Error(s_scriptCanvasBuilder, false, saveOutcome.GetError().c_str());
                }
            }
            else
            {
                if (AzFramework::StringFunc::Find(translationOutcome.GetError().c_str(), ScriptCanvas::ParseErrors::SourceUpdateRequired) != AZStd::string::npos)
                {
                    AZ_Warning(s_scriptCanvasBuilder, false, ScriptCanvas::ParseErrors::SourceUpdateRequired);
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                }
                else if (AzFramework::StringFunc::Find(translationOutcome.GetError().c_str(), ScriptCanvas::ParseErrors::EmptyGraph) != AZStd::string::npos)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                }
                else
                {
                    if (AzFramework::StringFunc::Find(fileNameOnly, s_unitTestParseErrorPrefix) != AZStd::string::npos)
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                    }
                    AZ_Error(s_scriptCanvasBuilder, false, translationOutcome.GetError().c_str());
                }
            }

            m_processEditorAssetDependencies.clear();
        }

        AZ_TracePrintf(s_scriptCanvasBuilder, "Finish Processing Job");
    }
}
