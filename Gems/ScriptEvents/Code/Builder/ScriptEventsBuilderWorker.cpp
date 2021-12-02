/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <Builder/ScriptEventsBuilderWorker.h>

#include <Editor/ScriptEventsSystemEditorComponent.h>

#include <ScriptEvents/ScriptEventsAsset.h>

namespace ScriptEventsBuilder
{
    [[maybe_unused]] static const char* s_scriptEventsBuilder = "ScriptEventsBuilder";

    Worker::Worker()
    {
    }

    Worker::~Worker()
    {
        Deactivate();
    }

    int Worker::GetVersionNumber() const
    {
        return 1;
    }

    const char* Worker::GetFingerprintString() const
    {
        if (m_fingerprintString.empty())
        {
            // compute it the first time
            const AZStd::string runtimeAssetTypeId = azrtti_typeid<ScriptEvents::ScriptEventsAsset>().ToString<AZStd::string>();
            m_fingerprintString = AZStd::string::format("%i%s", GetVersionNumber(), runtimeAssetTypeId.c_str());
        }
        return m_fingerprintString.c_str();
    }

    void Worker::Activate()
    {
    }

    void Worker::Deactivate()
    {
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

        AZ_TracePrintf(s_scriptEventsBuilder, "CreateJobs for script events\"%s\"\n", fullPath.data());

        AZ::Data::AssetHandler* editorAssetHandler = AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
        if (!editorAssetHandler)
        {
            AZ_Error(s_scriptEventsBuilder, false, R"(CreateJobs for %s failed because the ScriptEvents Editor Asset handler is missing.)", fullPath.data());
        }

        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Warning(s_scriptEventsBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.data());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_scriptEventsBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be read.", fullPath.data());
                return;
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        if (editorAssetHandler->LoadAssetDataFromStream(asset, assetDataStream, nullptr) != AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Warning(s_scriptEventsBuilder, false, "CreateJobs for \"%s\" failed because the asset data could not be loaded from the file", fullPath.data());
            return;
        }

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 2;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = ScriptEvents::k_builderJobKey;
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());
            jobDescriptor.m_additionalFingerprintInfo = GetFingerprintString();

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void Worker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // A runtime script events component is generated, which creates a .scriptevents_compiled file
        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_scriptEventsBuilder, "Processing script events \"%s\".\n", fullPath.c_str());

        auto editorAssetHandler = azrtti_cast<ScriptEventsEditor::ScriptEventAssetHandler*>(
            AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<ScriptEvents::ScriptEventsAsset>()));
        if (!editorAssetHandler)
        {
            AZ_Error(s_scriptEventsBuilder, false, R"(Exporting of .ScriptEvents for "%s" file failed as no editor asset handler was registered for scrit events. The ScriptEvents Gem might not be enabled.)", fullPath.data());
            return;
        }

        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read the asset into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!stream.IsOpen())
            {
                AZ_Warning(s_scriptEventsBuilder, false, "Exporting of .ScriptEvents for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_scriptEventsBuilder, false, "Exporting of .ScriptEvents for \"%s\" failed because the source file could not be read.", fullPath.c_str());
                return;
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        AZ::SerializeContext* context{};
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;

        AZ_TracePrintf(s_scriptEventsBuilder, "Script Events Asset preload\n");
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset;
        asset.Create(request.m_sourceFileUUID);
        if (editorAssetHandler->LoadAssetData(asset, assetDataStream, nullptr) != AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Error(s_scriptEventsBuilder, false, R"(Loading of ScriptEvents asset for source file "%s" has failed)", fullPath.data());
            return;
        }

        AZ_TracePrintf(s_scriptEventsBuilder, "Script Events Asset loaded successfully\n");

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZStd::string runtimeScriptEventsOutputPath;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), runtimeScriptEventsOutputPath,  true, true);

        ScriptEvents::ScriptEvent definition = asset.Get()->m_definition;
        definition.Flatten();

        // Populate the runtime Asset 
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> productionAsset;
        productionAsset.Create(request.m_sourceFileUUID);
        productionAsset.Get()->m_definition = AZStd::move(definition);

        editorAssetHandler->SetSaveAsBinary(true);

        AZ_TracePrintf(s_scriptEventsBuilder, "Script Events Asset presave to object stream for %s\n", fullPath.c_str());
        bool productionAssetSaved = editorAssetHandler->SaveAssetData(productionAsset, &byteStream);

        if (!productionAssetSaved)
        {
            AZ_Error(s_scriptEventsBuilder, productionAssetSaved, "Failed to save runtime Script Events to object stream");
            return;
        }
        AZ_TracePrintf(s_scriptEventsBuilder, "Script Events Asset has been saved to the object stream successfully\n");

        // TODO: make this binary
        AZ::IO::FileIOStream outFileStream(runtimeScriptEventsOutputPath.data(), AZ::IO::OpenMode::ModeWrite);
        if (!outFileStream.IsOpen())
        {
            AZ_Error(s_scriptEventsBuilder, false, "Failed to open output file %s", runtimeScriptEventsOutputPath.data());
            return;
        }

        productionAssetSaved = outFileStream.Write(byteBuffer.size(), byteBuffer.data()) == byteBuffer.size() && productionAssetSaved;
        if (!productionAssetSaved)
        {
            AZ_Error(s_scriptEventsBuilder, productionAssetSaved, "Unable to save runtime Script Events file %s", runtimeScriptEventsOutputPath.data());
            return;
        }

        // ScriptEvents Editor Asset Copy job
        // The SubID is zero as this represents the main asset
        AssetBuilderSDK::JobProduct jobProduct;
        jobProduct.m_productFileName = runtimeScriptEventsOutputPath;
        jobProduct.m_productAssetType = azrtti_typeid<ScriptEvents::ScriptEventsAsset>();
        jobProduct.m_productSubID = 0;
        jobProduct.m_dependenciesHandled = true; // This builder has no product dependencies.
        response.m_outputProducts.push_back(AZStd::move(jobProduct));

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        AZ_TracePrintf(s_scriptEventsBuilder, "Finished processing Script Events %s\n", fullPath.c_str());
    }

    AZ::Uuid Worker::GetUUID()
    {
        return AZ::Uuid::CreateString("{CD64F85A-0147-45EF-B02A-9828E25D99EB}");
    }

}
