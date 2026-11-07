/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/SkyBox/HDRiSkyboxBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AzCore/Jobs/JobFunction.h>
#include <Document/InMemoryShaderCompiler.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Document/MaterialGraphCompiler.h>
#include <Window/MaterialCanvasViewportContent.h>

namespace MaterialCanvas
{
    MaterialCanvasViewportContent::MaterialCanvasViewportContent(
        const AZ::Crc32& toolId,
        AtomToolsFramework::RenderViewportWidget* widget,
        AZStd::shared_ptr<AzFramework::EntityContext> entityContext)
        : AtomToolsFramework::EntityPreviewViewportContent(toolId, widget, entityContext)
    {
        // Configure tone mapper
        m_postFxEntity = CreateEntity(
            "PostFxEntity",
            { AZ::Render::PostFxLayerComponentTypeId, AZ::Render::DisplayMapperComponentTypeId, AZ::Render::ExposureControlComponentTypeId,
              azrtti_typeid<AzFramework::TransformComponent>() });

        // Create IBL
        m_environmentEntity = CreateEntity(
            "EnvironmentEntity",
            { AZ::Render::HDRiSkyboxComponentTypeId, AZ::Render::ImageBasedLightComponentTypeId,
              azrtti_typeid<AzFramework::TransformComponent>() });

        // Create model
        m_objectEntity = CreateEntity(
            "ObjectEntity",
            { AZ::Render::MeshComponentTypeId, AZ::Render::MaterialComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>() });

        // Create shadow catcher
        m_shadowCatcherEntity = CreateEntity(
            "ShadowCatcherEntity",
            { AZ::Render::MeshComponentTypeId, AZ::Render::MaterialComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>(),
              azrtti_typeid<AzFramework::NonUniformScaleComponent>() });

        AZ::NonUniformScaleRequestBus::Event(
            GetShadowCatcherEntityId(), &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(100, 100, 1.0));

        // Avoid z-fighting with the cube model when double-sided rendering is enabled
        AZ::TransformBus::Event(
            GetShadowCatcherEntityId(), &AZ::TransformInterface::SetWorldZ, -0.01f);

        AZ::Render::MeshComponentRequestBus::Event(
            GetShadowCatcherEntityId(), &AZ::Render::MeshComponentRequestBus::Events::SetModelAssetId,
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/viewportmodels/plane_1x1.fbx.azmodel"));

        AZ::Render::MaterialComponentRequestBus::Event(
            GetShadowCatcherEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialAssetId,
            AZ::Render::DefaultMaterialAssignmentId,
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materials/special/shadowcatcher.azmaterial"));

        // Create grid
        m_gridEntity = CreateEntity("GridEntity", { AZ::Render::GridComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>() });

        AZ::Render::GridComponentRequestBus::Event(
            GetGridEntityId(),
            [&](AZ::Render::GridComponentRequests* gridComponentRequests)
            {
                gridComponentRequests->SetSize(4.0f);
                gridComponentRequests->SetAxisColor(AZ::Color(0.1f, 0.1f, 0.1f, 1.0f));
                gridComponentRequests->SetPrimaryColor(AZ::Color(0.1f, 0.1f, 0.1f, 1.0f));
                gridComponentRequests->SetSecondaryColor(AZ::Color(0.1f, 0.1f, 0.1f, 1.0f));
            });

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
        AtomToolsFramework::GraphDocumentNotificationBus::Handler::BusConnect(m_toolId);

        // Keep watching the catalog after a compile completes; see ApplyMaterial for why assets often aren't registered yet.
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
        MaterialGraphCompilerNotificationBus::Handler::BusConnect(m_toolId);

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    MaterialCanvasViewportContent::~MaterialCanvasViewportContent()
    {
        MaterialGraphCompilerNotificationBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        AtomToolsFramework::GraphDocumentNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    AZ::EntityId MaterialCanvasViewportContent::GetObjectEntityId() const
    {
        return m_objectEntity ? m_objectEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId MaterialCanvasViewportContent::GetEnvironmentEntityId() const
    {
        return m_environmentEntity ? m_environmentEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId MaterialCanvasViewportContent::GetPostFxEntityId() const
    {
        return m_postFxEntity ? m_postFxEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId MaterialCanvasViewportContent::GetShadowCatcherEntityId() const
    {
        return m_shadowCatcherEntity ? m_shadowCatcherEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId MaterialCanvasViewportContent::GetGridEntityId() const
    {
        return m_gridEntity ? m_gridEntity->GetId() : AZ::EntityId();
    }

    void MaterialCanvasViewportContent::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        // Read the path before the document is gone, and drop only this document's values.
        if (const AZStd::string graphPath = GetDocumentPath(documentId); !graphPath.empty())
        {
            AZStd::scoped_lock lock(m_materialPropertyValuesMutex);
            m_materialPropertyValuesByGraphPath.erase(graphPath);
        }

        // Closing a document that isn't on screen must not blank the preview.
        if (documentId != m_appliedDocumentId)
        {
            return;
        }

        // Drop the tracked document so that catalog updates stop trying to rebuild a material for it.
        m_appliedDocumentId = AZ::Uuid::CreateNull();
        m_appliedMaterialAssetId = {};
        m_appliedMaterialTypeAssetId = {};
        m_applyMaterialQueued = false;
        m_applyMaterialPending = false;

        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialAssetIdOnDefaultSlot, AZ::Data::AssetId());
    }

    void MaterialCanvasViewportContent::OnDocumentOpened([[maybe_unused]] const AZ::Uuid& documentId)
    {
        m_lastOpenedDocumentId = documentId;
        ApplyMaterial(documentId);
    }

    void MaterialCanvasViewportContent::OnCompileGraphStarted(const AZ::Uuid& documentId)
    {
        if (m_lastOpenedDocumentId == documentId)
        {
            // Bump the generation so a still-running job discards its result instead of applying it to this edit.
            ++m_compileGeneration;
        }

        if (m_lastOpenedDocumentId == documentId &&
            AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphStarted", true))
        {
            // Blank the object but keep tracking the document, so a compile that never completes (o3de/o3de#19642) can still recover.
            ClearMaterial();
        }
    }

    double MaterialCanvasViewportContent::MillisecondsSinceCompile() const
    {
        return AZStd::chrono::duration<double, AZStd::milli>(AZStd::chrono::steady_clock::now() - m_compileCompletedAt).count();
    }

    void MaterialCanvasViewportContent::OnCompileGraphProcessing(const AZ::Uuid& documentId)
    {
        if (m_lastOpenedDocumentId != documentId)
        {
            return;
        }

        // Start compiling now, under the ~600 ms Asset Processor wait, using the shaders of the material type already on screen.
        if (m_appliedMaterialTypeAsset)
        {
            QueueInMemoryShaderCompile(m_appliedMaterialTypeAsset, GetGeneratedFilePath(documentId, ".materialtype"));
        }
    }

    void MaterialCanvasViewportContent::OnCompileGraphCompleted(const AZ::Uuid& documentId)
    {
        m_compileCompletedAt = AZStd::chrono::steady_clock::now();

        if (m_lastOpenedDocumentId != documentId)
        {
            return;
        }

        // Defer while this edit's shader is still compiling; the job queues an apply when it finishes.
        if (m_shaderCompileInFlight)
        {
            return;
        }

        ApplyMaterial(documentId);
    }

    void MaterialCanvasViewportContent::OnCompileGraphFailed(const AZ::Uuid& documentId)
    {
        if (m_lastOpenedDocumentId == documentId &&
            AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphFailed", true))
        {
            // A failed compile has no assets on the way, so the document is dropped as well as the material.
            ApplyMaterial({});
        }
    }

    void MaterialCanvasViewportContent::OnViewportSettingsChanged()
    {
        AtomToolsFramework::EntityPreviewViewportContent::OnViewportSettingsChanged();

        AtomToolsFramework::EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](AtomToolsFramework::EntityPreviewViewportSettingsRequestBus::Events* viewportRequests)
            {
                const auto& modelPreset = viewportRequests->GetModelPreset();
                const auto& lightingPreset = viewportRequests->GetLightingPreset();

                AZ::Render::MeshComponentRequestBus::Event(
                    GetObjectEntityId(),
                    [&](AZ::Render::MeshComponentRequests* meshComponentRequests)
                    {
                        if (meshComponentRequests->GetModelAsset() != modelPreset.m_modelAsset)
                        {
                            meshComponentRequests->SetModelAsset(modelPreset.m_modelAsset);
                        }
                    });

                AZ::Render::HDRiSkyboxRequestBus::Event(
                    GetEnvironmentEntityId(),
                    [&](AZ::Render::HDRiSkyboxRequests* skyboxComponentRequests)
                    {
                        skyboxComponentRequests->SetExposure(lightingPreset.m_skyboxExposure);
                        skyboxComponentRequests->SetCubemapAsset(
                            viewportRequests->GetAlternateSkyboxEnabled() ? lightingPreset.m_alternateSkyboxImageAsset
                                                                          : lightingPreset.m_skyboxImageAsset);
                    });

                AZ::Render::MeshComponentRequestBus::Event(
                    GetShadowCatcherEntityId(), &AZ::Render::MeshComponentRequestBus::Events::SetVisibility,
                    viewportRequests->GetShadowCatcherEnabled());

                AZ::Render::MaterialComponentRequestBus::Event(
                    GetShadowCatcherEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetPropertyValue,
                    AZ::Render::DefaultMaterialAssignmentId, "settings.opacity", AZStd::any(lightingPreset.m_shadowCatcherOpacity));

                AZ::Render::DisplayMapperComponentRequestBus::Event(
                    GetPostFxEntityId(), &AZ::Render::DisplayMapperComponentRequestBus::Events::SetDisplayMapperOperationType,
                    viewportRequests->GetDisplayMapperOperationType());

                AZ::Render::GridComponentRequestBus::Event(
                    GetGridEntityId(), &AZ::Render::GridComponentRequestBus::Events::SetSize,
                    viewportRequests->GetGridEnabled() ? 4.0f : 0.0f);
            });
    }

    AZ::Data::AssetId MaterialCanvasViewportContent::GetGeneratedAssetId(
        const AZ::Uuid& documentId, AZStd::string_view extension) const
    {
        AZStd::vector<AZStd::string> generatedFiles;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            generatedFiles, documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGeneratedFilePaths);

        // Prefer the preview set when both exist (after a save); the production set remains as the fallback.
        if (MaterialGraphCompiler::IsPreviewOutputEnabled())
        {
            AZStd::vector<AZStd::string> previewFiles;
            AZStd::vector<AZStd::string> productionFiles;
            previewFiles.reserve(generatedFiles.size());
            productionFiles.reserve(generatedFiles.size());

            for (const auto& generatedFile : generatedFiles)
            {
                (MaterialGraphCompiler::IsPreviewOutputPath(generatedFile) ? previewFiles : productionFiles).push_back(generatedFile);
            }

            previewFiles.insert(previewFiles.end(), productionFiles.begin(), productionFiles.end());
            generatedFiles = AZStd::move(previewFiles);
        }

        for (const auto& generatedFile : generatedFiles)
        {
            if (generatedFile.ends_with(extension))
            {
                // TraceLevel::None: an unresolved file is expected while the AP catches up, and QueueApplyMaterialIfAffected retries.
                if (auto assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(generatedFile, 0, AZ::RPI::AssetUtils::TraceLevel::None))
                {
                    // MakeAssetId succeeds before the product exists, so require a catalog entry; the catalog handler retries later.
                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetIdOutcome.GetValue());

                    if (assetInfo.m_assetId.IsValid())
                    {
                        return assetIdOutcome.GetValue();
                    }
                }
            }
        }

        return {};
    }

    void MaterialCanvasViewportContent::ApplyMaterial(const AZ::Uuid& documentId)
    {
        // Record what's shown so catalog updates can rebuild it; the pipeline stage's shader jobs often haven't finished yet.
        m_appliedDocumentId = documentId;
        m_appliedMaterialAssetId = GetGeneratedAssetId(documentId, ".material");
        m_appliedMaterialTypeAssetId = GetGeneratedAssetId(documentId, ".materialtype");

        // Build the material in process if enabled, skipping FinalStage/MaterialBuilder; any failure falls through to the wait below.
        if (ApplyInMemoryMaterial(documentId))
        {
            AZ_TracePrintf(
                "MaterialCanvas", "Preview material applied from memory, %.0f ms after the compile finished.\n",
                MillisecondsSinceCompile());
            return;
        }

        AZ_TracePrintf(
            "MaterialCanvas",
            "Preview material applied through the asset system%s.\n",
            m_appliedMaterialAssetId.IsValid() ? "" : " (nothing resolved yet, so nothing is on screen)");

        // When material canvas generates assets, material input property values are assigned as default values in the material type instead
        // of overridden values in the material. The generated material asset is empty except for a single field referencing the material
        // type. Because the material asset never changes, it won't be reprocessed by the AP or treated as a unique asset in the asset
        // system. We force the viewport to create a unique material instance every time a change needs to be reflected in material canvas.
        AZ::Render::MaterialAssignment materialAssignment;
        materialAssignment.m_materialAsset.Create(m_appliedMaterialAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        materialAssignment.m_materialInstanceMustBeUnique = true;
        AZ::Render::MaterialAssignmentMap materialAssignmentMap;
        materialAssignmentMap.emplace(AZ::Render::DefaultMaterialAssignmentId, materialAssignment);
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materialAssignmentMap);

        // SetMaterialMap drops existing property overrides, so reapply the last compile's values.
        ApplyMaterialPropertyValues();
    }

    void MaterialCanvasViewportContent::QueueInMemoryShaderCompile(
        const AZ::Data::Asset<AZ::RPI::MaterialTypeAsset>& materialTypeAsset, const AZStd::string& materialTypePath)
    {
        if (m_shaderCompileInFlight.exchange(true))
        {
            return; // One already running. It will queue an apply when it finishes.
        }

        // Collected on the main thread (it reads asset handles); only the compiling runs in the job.
        AZStd::vector<InMemoryShaderRequest> requests = CollectInMemoryShaderRequests(materialTypeAsset, materialTypePath);
        if (requests.empty())
        {
            // The pipeline stage has not written this edit's .azsl and .shader yet. Normal early in a compile.
            m_shaderCompileInFlight = false;
            return;
        }

        const unsigned int generation = m_compileGeneration.load();

        auto* compileJob = AZ::CreateJobFunction(
            [this, requests, generation]()
            {
                auto compiled = CompileInMemoryShaders(requests);

                // Clear fingerprints for anything that failed: SkipIncludeFileDependencies means the AP won't rebuild it on its own.
                if (compiled.size() != requests.size())
                {
                    for (const InMemoryShaderRequest& request : requests)
                    {
                        const bool wasCompiled = AZStd::any_of(
                            compiled.begin(),
                            compiled.end(),
                            [&request](const auto& pair)
                            {
                                return pair.first == request.m_sourceShaderAsset.GetId();
                            });

                        if (!wasCompiled)
                        {
                            AZ_TracePrintf(
                                "MaterialCanvas",
                                "In-memory shader unavailable; asking the Asset Processor to rebuild %s\n",
                                request.m_shaderPath.c_str());

                            AzToolsFramework::AssetSystemRequestBus::Broadcast(
                                &AzToolsFramework::AssetSystemRequestBus::Events::ClearFingerprintForAsset, request.m_shaderPath);
                        }
                    }
                }
                {
                    AZStd::scoped_lock lock(m_compiledShadersMutex);
                    // Discard silently if the graph moved on; showing a shader for a stale edit is worse than being late.
                    if (generation == m_compileGeneration.load())
                    {
                        m_compiledShaders = AZStd::move(compiled);
                        m_compiledShadersGeneration = generation;
                    }
                }

                m_shaderCompileInFlight = false;

                // Rebuild next tick without the catalog debounce: this is one finished result, and waiting would undo the gain.
                m_applyMaterialImmediately = true;
                m_applyMaterialQueued = true;
            },
            true);

        compileJob->Start();
    }

    AZStd::string MaterialCanvasViewportContent::GetGeneratedFilePath(
        const AZ::Uuid& documentId, AZStd::string_view extension) const
    {
        AZStd::vector<AZStd::string> generatedFiles;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            generatedFiles, documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGeneratedFilePaths);

        // Preview first, as in GetGeneratedAssetId: after a save both sets exist and the viewport shows the preview one.
        for (const auto& generatedFile : generatedFiles)
        {
            if (generatedFile.ends_with(extension) && MaterialGraphCompiler::IsPreviewOutputPath(generatedFile))
            {
                return generatedFile;
            }
        }

        for (const auto& generatedFile : generatedFiles)
        {
            if (generatedFile.ends_with(extension))
            {
                return generatedFile;
            }
        }

        return {};
    }

    bool MaterialCanvasViewportContent::ApplyInMemoryMaterial(const AZ::Uuid& documentId)
    {
        if (!MaterialGraphCompiler::IsInMemoryPreviewMaterialEnabled())
        {
            return false;
        }

        const AZStd::string materialTypePath = GetGeneratedFilePath(documentId, ".materialtype");
        if (materialTypePath.empty())
        {
            AZ_TracePrintf("MaterialCanvas", "In-memory preview declined: the compile has generated no material type yet.\n");
            return false;
        }

        // Refuse an intermediate older than its source material type: PipelineStage hasn't rebuilt it for this edit yet.
        const AZStd::string intermediatePath =
            AZ::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath(materialTypePath);
        if (auto fileIO = AZ::IO::FileIOBase::GetInstance(); fileIO && !intermediatePath.empty())
        {
            const AZ::u64 intermediateModified = fileIO->ModificationTime(intermediatePath.c_str());
            const AZ::u64 sourceModified = fileIO->ModificationTime(materialTypePath.c_str());

            if (intermediateModified > 0 && sourceModified > 0 && intermediateModified < sourceModified)
            {
                AZ_TracePrintf(
                    "MaterialCanvas",
                    "In-memory preview declined at %.0f ms: the intermediate material type is from a previous edit.\n",
                    MillisecondsSinceCompile());
                return false;
            }
        }

        const AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> materialTypeAsset = CreateInMemoryMaterialTypeAsset(materialTypePath);
        if (!materialTypeAsset)
        {
            // Either the intermediate or its shaders are still being built; both are transient.
            AZ_TracePrintf(
                "MaterialCanvas",
                "In-memory preview declined at %.0f ms: the intermediate material type or its shaders are not ready.\n",
                MillisecondsSinceCompile());
            return false;
        }

        // Swap in shaders compiled for this edit; TryReplaceShaderAsset only accepts the same id, which cloning guarantees.
        m_appliedMaterialTypeAsset = materialTypeAsset;

        size_t replacedShaderCount = 0;
        {
            AZStd::scoped_lock lock(m_compiledShadersMutex);
            if (m_compiledShadersGeneration == m_compileGeneration.load())
            {
                for (const auto& [replacedAssetId, compiledShaderAsset] : m_compiledShaders)
                {
                    // Same call as the reload path; replaces every reference with this id, which the clone kept.
                    materialTypeAsset->ReinitializeAsset(compiledShaderAsset);
                    ++replacedShaderCount;
                }
            }
        }

        if (replacedShaderCount > 0)
        {
            AZ_TracePrintf(
                "MaterialCanvas", "Preview using %zu shader(s) compiled in process, %.0f ms after the compile finished.\n",
                replacedShaderCount, MillisecondsSinceCompile());
        }
        else
        {
            // Nothing compiled yet: start now and show the AP's shaders meanwhile; the job queues another apply when done.
            QueueInMemoryShaderCompile(materialTypeAsset, materialTypePath);
        }

        // The graph's current values live in the material (the type has placeholders), so set them on this asset.
        MaterialGraphCompilerNotifications::PropertyValueList propertyValues;
        {
            AZStd::scoped_lock lock(m_materialPropertyValuesMutex);
            const auto valuesIt = m_materialPropertyValuesByGraphPath.find(GetDocumentPath(documentId));
            if (valuesIt != m_materialPropertyValuesByGraphPath.end())
            {
                propertyValues = valuesIt->second;
            }
        }

        AZ::RPI::MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(AZ::Uuid::CreateRandom(), materialTypeAsset);

        // Resolve image paths to asset references via GetImageAssetReference, as MaterialSourceData does, or samplers read zero.
        const AZ::RPI::MaterialPropertiesLayout* propertiesLayout = materialTypeAsset->GetMaterialPropertiesLayout();

        for (const auto& [propertyId, propertyValue] : propertyValues)
        {
            bool resolvedAsImage = false;

            if (propertiesLayout && propertyValue.Is<AZStd::string>())
            {
                const AZ::RPI::MaterialPropertyIndex propertyIndex = propertiesLayout->FindPropertyIndex(propertyId);
                if (!propertyIndex.IsNull())
                {
                    const auto* propertyDescriptor = propertiesLayout->GetPropertyDescriptor(propertyIndex);
                    if (propertyDescriptor && propertyDescriptor->GetDataType() == AZ::RPI::MaterialPropertyDataType::Image)
                    {
                        // Paths are relative to the material type the values were written into before moving to the material.
                        AZ::Data::Asset<AZ::RPI::ImageAsset> imageAsset;
                        AZ::RPI::MaterialUtils::GetImageAssetReference(
                            imageAsset, materialTypePath, propertyValue.GetValue<AZStd::string>());

                        materialAssetCreator.SetPropertyValue(propertyId, imageAsset);
                        resolvedAsImage = true;
                    }
                }
            }

            if (!resolvedAsImage)
            {
                materialAssetCreator.SetPropertyValue(propertyId, propertyValue);
            }
        }

        AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset;
        if (!materialAssetCreator.End(materialAsset) || !materialAsset)
        {
            return false;
        }

        const AZ::Data::Instance<AZ::RPI::Material> materialInstance = AZ::RPI::Material::Create(materialAsset);
        if (!materialInstance)
        {
            return false;
        }

        // m_materialInstancePreCreated makes MaterialAssignment use this instance directly without resolving any asset id.
        AZ::Render::MaterialAssignment materialAssignment;
        materialAssignment.m_materialInstance = materialInstance;
        materialAssignment.m_materialInstancePreCreated = true;

        AZ::Render::MaterialAssignmentMap materialAssignmentMap;
        materialAssignmentMap.emplace(AZ::Render::DefaultMaterialAssignmentId, materialAssignment);
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materialAssignmentMap);

        // No ApplyMaterialPropertyValues: the values are already baked into the asset this instance was built from.
        return true;
    }

    void MaterialCanvasViewportContent::ClearMaterial()
    {
        // Mirrors ApplyMaterial's unresolved case so blanking takes the usual path; only the tracked document ID is kept.
        AZ::Render::MaterialAssignment materialAssignment;
        materialAssignment.m_materialAsset.Create(AZ::Data::AssetId(), AZ::Data::AssetLoadBehavior::PreLoad);
        materialAssignment.m_materialInstanceMustBeUnique = true;
        AZ::Render::MaterialAssignmentMap materialAssignmentMap;
        materialAssignmentMap.emplace(AZ::Render::DefaultMaterialAssignmentId, materialAssignment);
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materialAssignmentMap);
    }

    void MaterialCanvasViewportContent::ApplyMaterialPropertyValues()
    {
        // Only the on-screen document's values; others are kept under their own path until brought forward.
        const AZStd::string appliedGraphPath = GetDocumentPath(m_appliedDocumentId);
        if (appliedGraphPath.empty())
        {
            return;
        }

        // Copied out from under the lock so that the material component can be queried below without holding it.
        MaterialGraphCompilerNotifications::PropertyValueList propertyValues;
        {
            AZStd::scoped_lock lock(m_materialPropertyValuesMutex);
            const auto valuesIt = m_materialPropertyValuesByGraphPath.find(appliedGraphPath);
            if (valuesIt == m_materialPropertyValuesByGraphPath.end())
            {
                return;
            }

            propertyValues = valuesIt->second;
        }

        AZ::Render::MaterialPropertyOverrideMap propertyOverrides;
        propertyOverrides.reserve(propertyValues.size());

        for (const auto& [propertyId, propertyValue] : propertyValues)
        {
            AZStd::any propertyValueAsAny = AZ::RPI::MaterialPropertyValue::ToAny(propertyValue);

            // Skip values whose type doesn't match the live material (e.g. mid structural edit); only plain numeric types are compared.
            const bool propertyTypeIsComparable = propertyValue.Is<bool>() || propertyValue.Is<int32_t>() ||
                propertyValue.Is<uint32_t>() || propertyValue.Is<float>() || propertyValue.Is<AZ::Vector2>() ||
                propertyValue.Is<AZ::Vector3>() || propertyValue.Is<AZ::Vector4>() || propertyValue.Is<AZ::Color>();

            if (propertyTypeIsComparable)
            {
                AZStd::any currentValue;
                AZ::Render::MaterialComponentRequestBus::EventResult(
                    currentValue,
                    GetObjectEntityId(),
                    &AZ::Render::MaterialComponentRequests::GetPropertyValue,
                    AZ::Render::DefaultMaterialAssignmentId,
                    AZStd::string(propertyId.GetCStr()));

                // Empty means no such property (a rename); a differing type is e.g. float2 to float. Neither can be applied.
                if (currentValue.empty() || currentValue.type() != propertyValueAsAny.type())
                {
                    continue;
                }
            }

            propertyOverrides.emplace(propertyId, AZStd::move(propertyValueAsAny));
        }

        if (propertyOverrides.empty())
        {
            return;
        }

        // Values are baked into the material type as defaults, so also set them as overrides on the live instance to show them now.
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetPropertyValues,
            AZ::Render::DefaultMaterialAssignmentId, propertyOverrides);
    }

    void MaterialCanvasViewportContent::OnMaterialPropertyValuesChanged(
        const AZStd::string& graphPath, const MaterialGraphCompilerNotifications::PropertyValueList& propertyValues)
    {
        // CompileGraph requires a graph path; an empty one can't be attributed to a document, so don't store it.
        if (graphPath.empty())
        {
            return;
        }

        {
            AZStd::scoped_lock lock(m_materialPropertyValuesMutex);
            m_materialPropertyValuesByGraphPath[graphPath] = propertyValues;
        }

        // Raised on the job thread, so defer to the next tick; only overrides need applying, not a material rebuild.
        m_applyMaterialPropertyValuesQueued = true;
    }

    void MaterialCanvasViewportContent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        QueueApplyMaterialIfAffected(assetId);
    }

    void MaterialCanvasViewportContent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        QueueApplyMaterialIfAffected(assetId);
    }

    void MaterialCanvasViewportContent::QueueApplyMaterialIfAffected(const AZ::Data::AssetId& assetId)
    {
        if (m_appliedDocumentId.IsNull())
        {
            return;
        }

        // Until the material resolves any catalog update may help; afterwards only products sharing its or its type's GUID matter.
        const bool affected = !m_appliedMaterialAssetId.IsValid() || m_appliedMaterialAssetId.m_guid == assetId.m_guid ||
            (m_appliedMaterialTypeAssetId.IsValid() && m_appliedMaterialTypeAssetId.m_guid == assetId.m_guid);

        if (affected)
        {
            // Raised on the asset thread; defer to the main thread's next tick, collapsing the burst into one rebuild.
            m_applyMaterialQueued = true;
        }
    }

    void MaterialCanvasViewportContent::OnSystemTick()
    {
        const auto now = AZStd::chrono::steady_clock::now();

        // Cheap (overrides on an existing instance), so apply immediately without the debounce.
        if (m_applyMaterialPropertyValuesQueued.exchange(false))
        {
            ApplyMaterialPropertyValues();
        }

        if (m_applyMaterialQueued.exchange(false))
        {
            // Each notification extends the deadline to collapse bursts; the burst start caps how long it can be deferred.
            if (!m_applyMaterialPending)
            {
                m_applyMaterialPending = true;
                m_applyMaterialBurstStart = now;
            }

            // 100 ms quiet period keeps pace with the Editor viewport; the old 250 ms let trickling updates visibly lag the pane.
            const AZ::u64 quietPeriodMs = AtomToolsFramework::GetSettingsValue(
                "/O3DE/Atom/MaterialCanvas/Viewport/ApplyMaterialQuietPeriodMs", (AZ::u64)100);
            m_applyMaterialQuietDeadline = now + AZStd::chrono::milliseconds(quietPeriodMs);
        }

        if (!m_applyMaterialPending || m_appliedDocumentId.IsNull())
        {
            return;
        }

        // Hard ceiling on deferral: at worst two rebuilds a second, keeping the preview within half a second of the graph.
        const AZ::u64 maxDeferralMs = AtomToolsFramework::GetSettingsValue(
            "/O3DE/Atom/MaterialCanvas/Viewport/ApplyMaterialMaxDeferralMs", (AZ::u64)500);

        // A result this viewport produced itself skips the wait entirely.
        const bool applyImmediately = m_applyMaterialImmediately.exchange(false);

        const bool catalogWentQuiet = applyImmediately || now >= m_applyMaterialQuietDeadline;
        const bool deferredTooLong = (now - m_applyMaterialBurstStart) >= AZStd::chrono::milliseconds(maxDeferralMs);
        if (!catalogWentQuiet && !deferredTooLong)
        {
            return;
        }

        m_applyMaterialPending = false;
        m_applyMaterialBurstStart = now;
        ApplyMaterial(m_appliedDocumentId);
    }

    AZStd::string MaterialCanvasViewportContent::GetDocumentPath(const AZ::Uuid& documentId) const
    {
        if (documentId.IsNull())
        {
            return {};
        }

        AZStd::string absolutePath;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            absolutePath, documentId, &AtomToolsFramework::AtomToolsDocumentRequests::GetAbsolutePath);
        return absolutePath;
    }
} // namespace MaterialCanvas
