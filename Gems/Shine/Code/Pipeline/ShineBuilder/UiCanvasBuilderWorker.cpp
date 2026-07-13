/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pipeline/ShineBuilder/UiCanvasBuilderWorker.h>
#include <Source/UiCanvasFileObject.h>

#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/UiEditorOnlyEntityHandler.h>

#include <Editor/UiEditorEntityCompilation.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Shine/UiAssetTypes.h>
#include <LmbrCentral/Rendering/TextureAsset.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzQtComponents/Utilities/ScopedCleanup.h>

namespace Shine
{
    [[maybe_unused]] static const char* const s_uiCanvasBuilder = "UiCanvasBuilder";

    void UiCanvasBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    AZ::Uuid UiCanvasBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{2708874f-52e8-48db-bbc4-4c33fa8ceb2e}");
    }

    void UiCanvasBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        // Check for shutdown
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AssetBuilderSDK::AssertAndErrorAbsorber assertAndErrorAbsorber(true);

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_uiCanvasBuilder, "CreateJobs for UI canvas \"%s\"\n", fullPath.c_str());

        // Open the source canvas file
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!AZ::IO::RetryOpenStream(stream))
        {
            AZ_Warning(s_uiCanvasBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
            return;
        }

        // Asset filter returns false to prevent loading assets during deserialization
        auto assetFilter = [](const AZ::Data::AssetFilterInfo&)
        {
            return false;
        };

        // Serialize in the canvas from the stream
        UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
        UiSystemToolsBus::BroadcastResult(canvasAsset, &UiSystemToolsInterface::LoadCanvasFromStream, stream, AZ::ObjectStream::FilterDescriptor(assetFilter, AZ::ObjectStream::FilterFlags::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        if (!canvasAsset)
        {
            AZ_Error(s_uiCanvasBuilder, false, "Compiling UI canvas \"%s\" failed to load canvas from stream.", fullPath.c_str());
            return;
        }

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
        // i.e. missing assets or serialization errors.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_uiCanvasBuilder, false, "Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", fullPath.c_str());
            UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
            return;
        }

        const char* compilerVersion = "6";
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 0;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "UI Canvas";
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = AZStd::string(compilerVersion);

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
    }

    void UiCanvasBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // .uicanvas files are converted as they are copied to the cache
        // to replace any editor components with runtime components

        // Check for shutdown
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Cancelled job %s because shutdown was requested.\n", request.m_sourceFile.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AZStd::string outputPath;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), outputPath, true, true);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TraceContext("Source", fullPath.c_str());
        AZ_TracePrintf(s_uiCanvasBuilder, "Processing UI canvas\n");

        // Open the source canvas file
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
        if (!AZ::IO::RetryOpenStream(stream))
        {
            AZ_Warning(s_uiCanvasBuilder, false, "Compiling UI canvas failed because source file could not be opened.");
            return;
        }

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_processingMutex);

            AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
            AssetBuilderSDK::ProductPathDependencySet productPathDependencySet;
            UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
            AZ::Entity* sourceCanvasEntity = nullptr;
            AZ::Entity exportCanvasEntity;

            // We need to ensure the canvas asset is destroyed no matter how we exit this scope
            auto scopedCanvasCleanup = scopedCleanup([&canvasAsset]()
            {
                if (canvasAsset)
                {
                    UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
                }
            });
            
            if(!ProcessUiCanvasAndGetDependencies(stream, productDependencies, productPathDependencySet, canvasAsset, sourceCanvasEntity, exportCanvasEntity))
            {
                return;
            }

            // Save runtime UI canvas to disk.
            AZ::IO::FileIOStream outputStream(outputPath.c_str(), AZ::IO::OpenMode::ModeWrite);
            if (outputStream.IsOpen())
            {
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::SaveCanvasToStream, canvasAsset, outputStream);
                outputStream.Close();

                // switch them back after we write the file so that the source canvas entity gets freed.
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::ReplaceCanvasEntity, canvasAsset, sourceCanvasEntity);

                AZ_TracePrintf(s_uiCanvasBuilder, "Output file %s\n", outputPath.c_str());
            }
            else
            {
                AZ_Error(s_uiCanvasBuilder, false, "Failed to open output file %s", outputPath.c_str());
                return;
            }

            AssetBuilderSDK::JobProduct jobProduct(outputPath);
            jobProduct.m_productAssetType = azrtti_typeid<Shine::CanvasAsset>();
            jobProduct.m_productSubID = 0;
            jobProduct.m_dependencies = AZStd::move(productDependencies);
            jobProduct.m_pathDependencies = AZStd::move(productPathDependencySet);
            jobProduct.m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies

            response.m_outputProducts.push_back(AZStd::move(jobProduct));
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        AZ_TracePrintf(s_uiCanvasBuilder, "Finished processing uicanvas\n");
    }

    bool UiCanvasBuilderWorker::ProcessUiCanvasAndGetDependencies(AZ::IO::GenericStream& stream, AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies, AssetBuilderSDK::ProductPathDependencySet& productPathDependencySet,
        UiSystemToolsInterface::CanvasAssetHandle*& canvasAsset, AZ::Entity*& sourceCanvasEntity, AZ::Entity& exportCanvasEntity) const
    {
        AssetBuilderSDK::AssertAndErrorAbsorber assertAndErrorAbsorber(true);

        // Load the canvas from the stream
        canvasAsset = nullptr;
        UiSystemToolsBus::BroadcastResult(canvasAsset, &UiSystemToolsInterface::LoadCanvasFromStream, stream, AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        if (!canvasAsset)
        {
            AZ_Error(s_uiCanvasBuilder, false, "Compiling UI canvas failed to load canvas from stream.");
            return false;
        }

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_uiCanvasBuilder, false, "Compiling UI canvas failed due to errors loading editor UI canvas.");
            return false;
        }

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error(s_uiCanvasBuilder, context, "Unable to obtain serialize context");
            return false;
        }

        // Get the child entities from the canvas
        AZStd::vector<AZ::Entity*> childEntities;
        UiSystemToolsBus::BroadcastResult(childEntities, &UiSystemToolsInterface::GetChildEntities, canvasAsset);

        // Emulate client flags.
        AZ::PlatformTagSet platformTags = { AZ_CRC_CE("renderer") };

        // Compile editor entities to runtime entities.
        AzToolsFramework::Prefab::PrefabConversionUtils::UiEditorOnlyEntityHandler uiEditorOnlyEntityHandler;
        Shine::EditorOnlyEntityHandlers handlers = { &uiEditorOnlyEntityHandler };
        auto compilationResult = Shine::CompileEditorEntities(childEntities, platformTags, *context, handlers);

        if (!compilationResult)
        {
            AZ_Error(s_uiCanvasBuilder, false, "Failed to export entities for runtime:\n%s", compilationResult.GetError().c_str());
            return false;
        }

        // Get the canvas entity from the canvas
        sourceCanvasEntity = nullptr;
        UiSystemToolsBus::BroadcastResult(sourceCanvasEntity, &UiSystemToolsInterface::GetCanvasEntity, canvasAsset);

        if (!sourceCanvasEntity)
        {
            AZ_Error(s_uiCanvasBuilder, false, "Compiling UI canvas failed to find the canvas entity.");
            return false;
        }

        // Create a new canvas entity that will contain the game components rather than editor components
        exportCanvasEntity = AZ::Entity{ sourceCanvasEntity->GetName() };
        exportCanvasEntity.SetId(sourceCanvasEntity->GetId());

        const AZ::Entity::ComponentArrayType& editorCanvasComponents = sourceCanvasEntity->GetComponents();
        for (AZ::Component* canvasEntityComponent : editorCanvasComponents)
        {
            auto* asEditorComponent =
                azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(canvasEntityComponent);

            if (asEditorComponent)
            {
                size_t oldComponentCount = exportCanvasEntity.GetComponents().size();
                asEditorComponent->BuildGameEntity(&exportCanvasEntity);
                if (exportCanvasEntity.GetComponents().size() > oldComponentCount)
                {
                    AZ::Component* newComponent = exportCanvasEntity.GetComponents().back();
                    AZ_Error("Export", asEditorComponent->GetId() != AZ::InvalidComponentId, "For entity \"%s\", component \"%s\" doesn't have a valid component id",
                        sourceCanvasEntity->GetName().c_str(), asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str());
                    newComponent->SetId(asEditorComponent->GetId());
                }
            }
            else
            {
                // The component is already runtime-ready. Clone and add to export entity
                AZ::Component* clonedComponent = context->CloneObject(canvasEntityComponent);
                exportCanvasEntity.AddComponent(clonedComponent);
            }
        }

        AZStd::vector<AZ::Entity*> compiledEntities = compilationResult.TakeValue();

        // Delete the original child entities before replacing
        for (auto* entity : childEntities)
        {
            delete entity;
        }

        // Replace the canvas child entities with the compiled runtime entities
        UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::ReplaceChildEntities, canvasAsset, AZStd::move(compiledEntities));
        UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::ReplaceCanvasEntity, canvasAsset, &exportCanvasEntity);

        // Now that the runtime canvas is built, go through and find any asset references.
        auto callback = [](
            const AZ::SerializeContext& serializeContext,
            void* instancePointer,
            const AZ::SerializeContext::ClassData* classData,
            const AZ::SerializeContext::ClassElement* classElement,
            AssetBuilderSDK::UniqueDependencyList& productDependencySet,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencySet,
            bool enumerateChildren)
        {
            // Look for any TextureAsset references and add an additional reference to a sprite file with the same name
            static const auto textureAssetRtti = azrtti_typeid<AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>>();
            if (classData->m_typeId == textureAssetRtti)
            {
                auto* asset = reinterpret_cast<AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>*>(instancePointer);

                AZStd::string path = asset->GetAssetPath();

                if (!path.empty())
                {
                    AzFramework::StringFunc::Path::ReplaceExtension(path, "sprite");
                    productPathDependencySet.emplace(path, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
                }
            }

            return AssetBuilderSDK::UpdateDependenciesFromClassData(
                serializeContext,
                instancePointer,
                classData,
                classElement,
                productDependencySet,
                productPathDependencySet,
                enumerateChildren);
        };

        AssetBuilderSDK::GatherProductDependencies(*context, &exportCanvasEntity, productDependencies, productPathDependencySet, callback);

        // Gather dependencies from each compiled child entity
        AZStd::vector<AZ::Entity*> finalChildEntities;
        UiSystemToolsBus::BroadcastResult(finalChildEntities, &UiSystemToolsInterface::GetChildEntities, canvasAsset);
        for (auto* entity : finalChildEntities)
        {
            AssetBuilderSDK::GatherProductDependencies(*context, entity, productDependencies, productPathDependencySet, callback);
        }

        return true;
    }
}
