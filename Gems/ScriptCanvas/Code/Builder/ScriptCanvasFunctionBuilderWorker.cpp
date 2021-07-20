/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

#include "platform.h"
#include <Asset/AssetDescription.h>
#include <Asset/Functions/ScriptCanvasFunctionAsset.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Script/ScriptComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <ScriptCanvas/Asset/Functions/RuntimeFunctionAssetHandler.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Assets/Functions/ScriptCanvasFunctionAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Results/ErrorText.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>

namespace ScriptCanvasBuilder
{
    void FunctionWorker::Activate(const AssetHandlers& handlers)
    {
        m_editorAssetHandler = handlers.m_editorFunctionAssetHandler;
        m_runtimeAssetHandler = handlers.m_runtimeAssetHandler;
        m_subgraphInterfaceHandler = handlers.m_subgraphInterfaceHandler;
    }

    void FunctionWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        AZ_TracePrintf(s_scriptCanvasBuilder, "Start Creating Job");
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

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

        m_sourceDependencies.clear();

        auto assetFilter = [&response, this](const AZ::Data::AssetFilterInfo& filterInfo)
        {
            if (filterInfo.m_assetType == azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptCanvasEditor::ScriptCanvasFunctionAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptEvents::ScriptEventsAsset>() ||
                filterInfo.m_assetType == azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>()) // this required, since nodes reference this, rather than the editor asset
            {
                AssetBuilderSDK::SourceFileDependency dependency;
                dependency.m_sourceFileDependencyUUID = filterInfo.m_assetId.m_guid;

                response.m_sourceFileDependencyList.push_back(dependency);
            }

            // Asset filter always returns false to prevent parsing dependencies, but makes note of the script canvas dependencies
            return false;
        };

        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasFunctionAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        ScriptCanvasEditor::ScriptCanvasFunctionAssetHandler* functionAssetHandler = azrtti_cast<ScriptCanvasEditor::ScriptCanvasFunctionAssetHandler*>(m_editorAssetHandler);
        if (functionAssetHandler->LoadAssetData(asset, assetDataStream, assetFilter) != AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Warning(s_scriptCanvasBuilder, false, "CreateJobs for \"%s\" failed because the asset data could not be loaded from the file", fullPath.data());
            return;
        }

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        auto* scriptCanvasEntity = asset.Get()->GetScriptCanvasEntity();
        auto* sourceGraph = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::Graph>(scriptCanvasEntity);
        AZ_Assert(sourceGraph, "Graph component is missing from entity.");
        size_t fingerprint = 0;
        const AZStd::set<AZ::Entity*> sortedEntities(sourceGraph->GetGraphData()->m_nodes.begin(), sourceGraph->GetGraphData()->m_nodes.end());
        for (auto& nodeEntity : sortedEntities)
        {
            if (auto nodeComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity))
            {
                AZStd::hash_combine(fingerprint, nodeComponent->GenerateFingerprint());
            }
        }

#if defined(FUNCTION_LEGACY_SUPPORT_ENABLED)
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            if (info.HasTag("tools"))
            {
                AssetBuilderSDK::JobDescriptor copyDescriptor;
                copyDescriptor.m_priority = 2;
                copyDescriptor.m_critical = true;
                copyDescriptor.m_jobKey = s_scriptCanvasCopyJobKey;
                copyDescriptor.SetPlatformIdentifier(info.m_identifier.c_str());
                copyDescriptor.m_additionalFingerprintInfo = AZStd::string(GetFingerprintString()).append("|").append(AZStd::to_string(static_cast<AZ::u64>(fingerprint)));
                response.m_createJobOutputs.push_back(copyDescriptor);
        }

            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 2;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = s_scriptCanvasProcessJobKey;
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = AZStd::string(GetFingerprintString()).append("|").append(AZStd::to_string(static_cast<AZ::u64>(fingerprint)));
            // Function process job needs to wait until its dependency asset job finished
            for (const auto& sourceDependency : response.m_sourceFileDependencyList)
            {
                AssetBuilderSDK::JobDependency jobDep(s_scriptCanvasBuilder, info.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::OrderOnce, sourceDependency);
                jobDescriptor.m_jobDependencyList.emplace_back(jobDep);
            }
            response.m_createJobOutputs.push_back(jobDescriptor);
        }
#endif

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        AZ_TracePrintf(s_scriptCanvasBuilder, "Finish Creating Job");
    }

    

    const char* FunctionWorker::GetFingerprintString() const
    {
        if (m_fingerprintString.empty())
        {
            // compute it the first time
            const AZStd::string runtimeAssetTypeId = azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>().ToString<AZStd::string>();
            m_fingerprintString = AZStd::string::format("%i%s", GetVersionNumber(), runtimeAssetTypeId.c_str());
        }
        return m_fingerprintString.c_str();
    }

    int FunctionWorker::GetVersionNumber() const
    {
        return GetBuilderVersion();
    }

    AZ::Uuid FunctionWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{7227E0E1-4113-456A-877B-B2276ACB292B}");
    }


    void FunctionWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
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
            AZ::IO::FileIOStream stream(fullPath.data(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "Exporting of .scriptcanvas for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_scriptCanvasBuilder, false, "Exporting of .scriptcanvas for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
                return;
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        AZ::SerializeContext* context{};
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasFunctionAsset> asset;
        asset.Create(request.m_sourceFileUUID);
        ScriptCanvasEditor::ScriptCanvasFunctionAssetHandler* functionAssetHandler = azrtti_cast<ScriptCanvasEditor::ScriptCanvasFunctionAssetHandler*>(m_editorAssetHandler);
        if (functionAssetHandler->LoadAssetData(asset, assetDataStream, nullptr) != AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Error(s_scriptCanvasBuilder, false, R"(Loading of ScriptCanvas asset for source file "%s" has failed)", fullPath.data());
            return;
        }

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZStd::string runtimeScriptCanvasOutputPath;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), runtimeScriptCanvasOutputPath, true, true);
        AzFramework::StringFunc::Path::ReplaceExtension(runtimeScriptCanvasOutputPath, ScriptCanvas::SubgraphInterfaceAsset::GetFileExtension());

        if (request.m_jobDescription.m_jobKey == s_scriptCanvasCopyJobKey)
        {
            // ScriptCanvas Editor Asset Copy job
            // The SubID is zero as this represents the main asset
            AssetBuilderSDK::JobProduct jobProduct;
            jobProduct.m_productFileName = fullPath;
            jobProduct.m_productAssetType = azrtti_typeid<ScriptCanvasEditor::ScriptCanvasFunctionAsset>();
            jobProduct.m_productSubID = 0;
            jobProduct.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(AZStd::move(jobProduct));
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }
        else
        {
            // force load all dependencies into memory
            for (auto& dependency : m_sourceDependencies)
            {
                AZ::Data::AssetManager::Instance().GetAsset(dependency.GetId(), dependency.GetType(), AZ::Data::AssetLoadBehavior::PreLoad);
            }

            AZ::Entity* buildEntity = asset.Get()->GetScriptCanvasEntity();

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
                AzFramework::StringFunc::Path::ReplaceExtension(input.runtimeScriptCanvasOutputPath, ScriptCanvas::RuntimeAsset::GetFileExtension());
                auto saveRuntimAssetOutcome = SaveRuntimeAsset(input, input.runtimeDataOut);
                if (saveRuntimAssetOutcome.IsSuccess())
                {
                    // \todo this file is only required on PC editor builds, cull it in packaging, here or wherever appropriate
                    AzFramework::StringFunc::Path::StripExtension(fileNameOnly);

                    ScriptCanvas::SubgraphInterfaceData functionInterface;
                    functionInterface.m_name = fileNameOnly;
                    functionInterface.m_interface = AZStd::move(input.interfaceOut);

                    // save function interface
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
                    AZ_Error(s_scriptCanvasBuilder, false, saveRuntimAssetOutcome.GetError().data());
                }
            }
            else if (AzFramework::StringFunc::Find(translationOutcome.GetError().data(), ScriptCanvas::ParseErrors::SourceUpdateRequired) != AZStd::string::npos)
            {
                AZ_Warning(s_scriptCanvasBuilder, false, ScriptCanvas::ParseErrors::SourceUpdateRequired);
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            }
            else if (AzFramework::StringFunc::Find(fileNameOnly, s_unitTestParseErrorPrefix) != AZStd::string::npos)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            }
            else
            {
                AZ_Warning(s_scriptCanvasBuilder, false, translationOutcome.GetError().data());
            }
        }

        AZ_TracePrintf(s_scriptCanvasBuilder, "Finish Processing Job");
    }
}
