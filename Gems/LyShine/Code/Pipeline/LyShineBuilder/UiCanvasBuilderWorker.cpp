/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pipeline/LyShineBuilder/UiCanvasBuilderWorker.h>

#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzCore/Component/ComponentApplication.h>
#include <LyShine/UiAssetTypes.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzQtComponents/Utilities/ScopedCleanup.h>

namespace LyShine
{
    [[maybe_unused]] static const char* const s_uiSliceBuilder = "UiSliceBuilder";

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

        AZ_TracePrintf(s_uiSliceBuilder, "CreateJobs for UI canvas \"%s\"\n", fullPath.c_str());

        // Open the source canvas file
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!AZ::IO::RetryOpenStream(stream))
        {
            AZ_Warning(s_uiSliceBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
            return;
        }

        // Asset filter always returns false to prevent parsing dependencies, but makes note of the slice dependencies
        auto assetFilter = [&response](const AZ::Data::AssetFilterInfo& filterInfo)
        {
            if (filterInfo.m_assetType == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
            {
                bool isSliceDependency = (filterInfo.m_loadBehavior != AZ::Data::AssetLoadBehavior::NoLoad);

                if (isSliceDependency)
                {
                    AssetBuilderSDK::SourceFileDependency dependency;
                    dependency.m_sourceFileDependencyUUID = filterInfo.m_assetId.m_guid;

                    response.m_sourceFileDependencyList.push_back(dependency);
                }
            }

            return false;
        };

        // Serialize in the canvas from the stream. This uses the LyShineSystemComponent to do it because
        // it does some complex support for old canvas formats
        UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
        UiSystemToolsBus::BroadcastResult(canvasAsset, &UiSystemToolsInterface::LoadCanvasFromStream, stream, AZ::ObjectStream::FilterDescriptor(assetFilter, AZ::ObjectStream::FilterFlags::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        if (!canvasAsset)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed to load canvas from stream.", fullPath.c_str());
            return;
        }

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
        // i.e. missing assets or serialization errors.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", fullPath.c_str());
            UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
            return;
        }

        const char* compilerVersion = "4";
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 0;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "UI Canvas";
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = AZStd::string(compilerVersion).append(azrtti_typeid<AZ::DynamicSliceAsset>().ToString<AZStd::string>());

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
    }

    void UiCanvasBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // .uicanvas files are converted as they are copied to the cache
        // a) to flatten all prefab instances
        // b) to replace any editor components with runtime components

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
        AZ_TracePrintf(s_uiSliceBuilder, "Processing UI canvas\n");

        // Open the source canvas file
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
        if (!AZ::IO::RetryOpenStream(stream))
        {
            AZ_Warning(s_uiSliceBuilder, false, "Compiling UI canvas failed because source file could not be opened.");
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

                AZ_TracePrintf(s_uiSliceBuilder, "Output file %s\n", outputPath.c_str());
            }
            else
            {
                AZ_Error(s_uiSliceBuilder, false, "Failed to open output file %s", outputPath.c_str());
                return;
            }

            AssetBuilderSDK::JobProduct jobProduct(outputPath);
            jobProduct.m_productAssetType = azrtti_typeid<LyShine::CanvasAsset>();
            jobProduct.m_productSubID = 0;
            jobProduct.m_dependencies = AZStd::move(productDependencies);
            jobProduct.m_pathDependencies = AZStd::move(productPathDependencySet);
            jobProduct.m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies

            response.m_outputProducts.push_back(AZStd::move(jobProduct));
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        AZ_TracePrintf(s_uiSliceBuilder, "Finished processing uicanvas\n");
    }

    bool UiCanvasBuilderWorker::ProcessUiCanvasAndGetDependencies(AZ::IO::GenericStream& stream, AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies, AssetBuilderSDK::ProductPathDependencySet& productPathDependencySet,
        UiSystemToolsInterface::CanvasAssetHandle*& canvasAsset, AZ::Entity*& sourceCanvasEntity, AZ::Entity& exportCanvasEntity) const
    {
        AssetBuilderSDK::AssertAndErrorAbsorber assertAndErrorAbsorber(true);

        // Serialize in the canvas from the stream. This uses the LyShineSystemComponent to do it because
        // it does some complex support for old canvas formats
        canvasAsset = nullptr;
        UiSystemToolsBus::BroadcastResult(canvasAsset, &UiSystemToolsInterface::LoadCanvasFromStream, stream, AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterSourceSlicesOnly, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        if (!canvasAsset)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas failed to load canvas from stream.");
            return false;
        }

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
        // i.e. missing assets or serialization errors.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas failed due to errors loading editor UI canvas.");
            return false;
        }

        // Get the prefab component from the canvas
        AZ::Entity* canvasSliceEntity = nullptr;
        UiSystemToolsBus::BroadcastResult(canvasSliceEntity, &UiSystemToolsInterface::GetRootSliceEntity, canvasAsset);

        if (!canvasSliceEntity)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas failed to find the root slice entity.");
            return false;
        }

        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error(s_uiSliceBuilder, context, "Unable to obtain serialize context");
            return false;
        }

        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Save the canvas slice entity into a memory buffer, then hand ownership of the buffer to assetDataStream
        {
            AZStd::vector<AZ::u8> prefabBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> prefabStream(&prefabBuffer);
            if (!AZ::Utils::SaveObjectToStream<AZ::Entity>(prefabStream, AZ::ObjectStream::ST_XML, canvasSliceEntity))
            {
                AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas failed due to errors serializing editor UI canvas.");
                return false;
            }

            assetDataStream->Open(AZStd::move(prefabBuffer));
        }

        AZ::Data::Asset<AZ::SliceAsset> sourceSliceAsset;
        sourceSliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        AZ::SliceAssetHandler assetHandler(context);

        if (assetHandler.LoadAssetData(sourceSliceAsset, assetDataStream, &AZ::Data::AssetFilterSourceSlicesOnly) !=
            AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            AZ_Error(s_uiSliceBuilder, false, "Failed to load the serialized Slice Asset.");
            return false;
        }

        // Flush sourceSliceAsset manager events to ensure no sourceSliceAsset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
        // i.e. missing assets or serialization errors.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas failed due to errors deserializing editor UI canvas.");
            return false;
        }

        // Emulate client flags.
        AZ::PlatformTagSet platformTags = { AZ_CRC("renderer", 0xf199a19c) };

        // Compile the source slice into the runtime slice (with runtime components).
        AzToolsFramework::UiEditorOnlyEntityHandler uiEditorOnlyEntityHandler;
        AzToolsFramework::EditorOnlyEntityHandlers handlers =
        {
            &uiEditorOnlyEntityHandler,
        };

        // Get the prefab component from the prefab sourceSliceAsset
        AzToolsFramework::SliceCompilationResult sliceCompilationResult = AzToolsFramework::CompileEditorSlice(sourceSliceAsset, platformTags, *context, handlers);

        if (!sliceCompilationResult)
        {
            AZ_Error(s_uiSliceBuilder, false, "Failed to export entities for runtime:\n%s", sliceCompilationResult.GetError().c_str());
            return false;
        }

        // Get the canvas entity from the canvas
        sourceCanvasEntity = nullptr;
        UiSystemToolsBus::BroadcastResult(sourceCanvasEntity, &UiSystemToolsInterface::GetCanvasEntity, canvasAsset);

        if (!sourceCanvasEntity)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas failed to find the canvas entity.");
            return false;
        }

        // create a new canvas entity that will contain the game components rather than editor components
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
                // The component is already runtime-ready. I.e. it is not an editor component.
                // Clone the component and add it to the export entity
                AZ::Component* clonedComponent = context->CloneObject(canvasEntityComponent);
                exportCanvasEntity.AddComponent(clonedComponent);
            }
        }

        AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset = sliceCompilationResult.GetValue();
        AZ::Entity* exportSliceAssetEntity = exportSliceAsset.Get()->GetEntity();
        AZ::SliceComponent* exportSliceComponent = exportSliceAssetEntity->FindComponent<AZ::SliceComponent>();
        exportSliceAssetEntity->RemoveComponent(exportSliceComponent);

        UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::ReplaceRootSliceSliceComponent, canvasAsset, exportSliceComponent);
        UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::ReplaceCanvasEntity, canvasAsset, &exportCanvasEntity);

        // Now that the runtime canvas is built, go through and find any asset references.
        // Both the canvas entity and the slice component can have asset references.
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
        AssetBuilderSDK::GatherProductDependencies(*context, exportSliceComponent, productDependencies, productPathDependencySet, callback);

        return true;
    }
}
