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
#include <Document/InMemoryShaderCompiler.h>
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

        // The viewport has to keep watching the asset catalog after a graph compile reports completion. See ApplyMaterial for why the
        // assets it needs are frequently not registered yet at that point.
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
        // The path has to be read before the document is gone, and only this document's values are dropped. Another document's values
        // stay put so that it still has them if it is brought back to the front.
        if (const AZStd::string graphPath = GetDocumentPath(documentId); !graphPath.empty())
        {
            AZStd::scoped_lock lock(m_materialPropertyValuesMutex);
            m_materialPropertyValuesByGraphPath.erase(graphPath);
        }

        // Closing a document that is not the one on screen must not blank the preview. Every other member below describes the applied
        // document only, so touching them for an unrelated close would clear a preview that is still perfectly valid.
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
        if (m_lastOpenedDocumentId == documentId &&
            AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphStarted", true))
        {
            // Blank the object but keep tracking the document. Clearing the tracked ID here would disarm the catalog handler for the
            // duration of the compile, and a compile that never reports completion, which is what o3de/o3de#19642 describes, would leave
            // the viewport blank with nothing able to recover it.
            ClearMaterial();
        }
    }

    double MaterialCanvasViewportContent::MillisecondsSinceCompile() const
    {
        return AZStd::chrono::duration<double, AZStd::milli>(AZStd::chrono::steady_clock::now() - m_compileCompletedAt).count();
    }

    void MaterialCanvasViewportContent::OnCompileGraphCompleted(const AZ::Uuid& documentId)
    {
        m_compileCompletedAt = AZStd::chrono::steady_clock::now();

        if (m_lastOpenedDocumentId == documentId)
        {
            ApplyMaterial(documentId);
        }
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

        // Prefer the preview output set over the production one. A compile triggered by a save writes both, they share a file name and
        // differ only by folder, and both resolve, so without an explicit preference the viewport would display whichever one the Asset
        // Processor happened to publish first. The production set stays in the list as the fallback, which is what a graph compiled with
        // preview output turned off has, and what a graph has before its first save.
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
                // TraceLevel::None because an unresolved file is an expected, transient state while the Asset Processor catches up with
                // the files the graph compiler just wrote. QueueApplyMaterialIfAffected retries until it resolves, and reporting an error
                // on every attempt would fill the log during ordinary editing.
                if (auto assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(generatedFile, 0, AZ::RPI::AssetUtils::TraceLevel::None))
                {
                    // MakeAssetId only maps the source file to its GUID. It succeeds as soon as the Asset Processor knows the source
                    // exists and says nothing about whether the product has actually been built, so on its own it will happily hand back
                    // an ID for an asset that is still being generated. Loading that leaves the material referencing shader assets from
                    // a half finished build, which surfaces as "OptionGroup for specialization is different to the one in the
                    // ShaderAsset" and, further along, as material functors initialised from data that was never written.
                    //
                    // Requiring the product to be registered in the catalog turns that into a miss instead. The catalog handler re-applies
                    // once the Asset Processor publishes it, which is the same path that recovers from a compile whose status wait timed
                    // out, so nothing is lost by waiting.
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
        // Record what the viewport is showing so that later asset catalog updates can rebuild it.
        //
        // This function routinely runs before the assets it needs exist. GraphCompiler::ReportGeneratedFileStatus waits on the Asset
        // Processor jobs for the files the graph compiler wrote, but the generated material type is abstract: MaterialTypeBuilder's
        // pipeline stage expands it into intermediate azsl, shader and material type sources, and those are queued as a second wave of
        // jobs that the original source paths know nothing about. AssetStatusReporter therefore reports success, and the compile is
        // announced as complete, while the shaders are still building. Resolving the material here can fail outright, or succeed against
        // products that are about to be replaced.
        m_appliedDocumentId = documentId;
        m_appliedMaterialAssetId = GetGeneratedAssetId(documentId, ".material");
        m_appliedMaterialTypeAssetId = GetGeneratedAssetId(documentId, ".materialtype");

        // When material canvas generates assets, material input property values are assigned as default values in the material type instead
        // of overridden values in the material. The generated material asset is empty except for a single field referencing the material
        // type. Because the material asset never changes, it won't be reprocessed by the AP or treated as a unique asset in the asset
        // system. We force the viewport to create a unique material instance every time a change needs to be reflected in material canvas.
        // Build the material here rather than waiting for the Asset Processor to finish building it, when that is enabled.
        //
        // The two jobs this replaces, FinalStage and MaterialBuilder, cost 300 ms and 268 ms of Asset Processor time for 16 ms and
        // roughly 20 ms of actual work. The rest is hashing, dependency fingerprinting, product copies and catalog updates around
        // builders that barely do anything. Nothing is reimplemented to skip them: CreateMaterialTypeAsset and MaterialAssetCreator
        // are the same public calls those builders make.
        //
        // The shader is still built by the Asset Processor, and this depends on that having finished, because the material type
        // resolves its shader references through the asset system. Every way this can fail is a way of saying "not ready yet", so
        // failure falls through to the path below, which waits.
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

        AZ::Render::MaterialAssignment materialAssignment;
        materialAssignment.m_materialAsset.Create(m_appliedMaterialAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        materialAssignment.m_materialInstanceMustBeUnique = true;
        AZ::Render::MaterialAssignmentMap materialAssignmentMap;
        materialAssignmentMap.emplace(AZ::Render::DefaultMaterialAssignmentId, materialAssignment);
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materialAssignmentMap);

        // SetMaterialMap replaces the assignment, and with it any property overrides that were on it, so the values from the last compile
        // have to go back on afterwards.
        ApplyMaterialPropertyValues();
    }

    AZStd::string MaterialCanvasViewportContent::GetGeneratedFilePath(
        const AZ::Uuid& documentId, AZStd::string_view extension) const
    {
        AZStd::vector<AZStd::string> generatedFiles;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            generatedFiles, documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGeneratedFilePaths);

        // Preview first, for the same reason GetGeneratedAssetId prefers it: after a save both sets exist and the viewport shows the
        // preview one.
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

        // Refuse an intermediate the Asset Processor has not rebuilt for this edit. Without this the first attempt, which runs the
        // instant the compile reports complete, happily builds a material from the previous edit's intermediate: the viewport shows
        // stale content until a later attempt corrects it, and the log records a success that measured nothing.
        //
        // Both sides of the comparison are FileIOBase modification times, so whatever units and epoch that uses cancel out. An
        // earlier version of this compared one of them against a system_clock epoch, which is not the same base and never rejected
        // anything.
        //
        // The source material type is what the compiler wrote for this edit and the intermediate is what PipelineStage derives from
        // it, so an intermediate older than its own source has not been rebuilt yet. When the compiler leaves the source untouched
        // because nothing changed, the intermediate is legitimately newer and this correctly allows it.
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
            // Either PipelineStage has not produced the intermediate yet, or it has and the shaders it references are still
            // building. Both are transient; the distinction matters only for knowing which job the viewport is really waiting on.
            AZ_TracePrintf(
                "MaterialCanvas",
                "In-memory preview declined at %.0f ms: the intermediate material type or its shaders are not ready.\n",
                MillisecondsSinceCompile());
            return false;
        }

        // The values the graph currently describes. These live in the material rather than the material type, which is why the
        // generated material type carries placeholders, so they have to be set on the asset being built here.
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

        // An image property's value is a path, not an asset. MaterialSourceData turns those into asset references in
        // ApplyPropertiesToAssetCreator before handing them to this creator, but that function is private and driving the creator
        // directly skips it: the property is then set to a bare string, the sampler finds no image, and it reads zero. On an opaque
        // material that shows up as a black texture; on a transparent one the alpha is zero and the mesh disappears entirely, which
        // is why a graph using only a noise node looked fine.
        //
        // GetImageAssetReference is the same public helper MaterialSourceData uses, so this resolves paths the way the Asset
        // Processor path does rather than inventing a second set of rules.
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
                        // Paths are relative to the material type the values came out of, which is the file the compiler wrote
                        // them into before moving them to the material.
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

        // m_materialInstancePreCreated is what makes this legitimate rather than a trick. MaterialAssignment checks it in the three
        // places that matter: RequiresLoading returns false, RebuildInstance leaves the instance alone, and Release does not null it.
        // The mesh consumes m_materialInstance directly through ConvertToCustomMaterialMap, so no asset id is ever resolved.
        AZ::Render::MaterialAssignment materialAssignment;
        materialAssignment.m_materialInstance = materialInstance;
        materialAssignment.m_materialInstancePreCreated = true;

        AZ::Render::MaterialAssignmentMap materialAssignmentMap;
        materialAssignmentMap.emplace(AZ::Render::DefaultMaterialAssignmentId, materialAssignment);
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materialAssignmentMap);

        // Deliberately no ApplyMaterialPropertyValues call. The values are already in the asset this instance was built from, so
        // setting them again as overrides would be redundant.
        return true;
    }

    void MaterialCanvasViewportContent::ClearMaterial()
    {
        // Mirrors what ApplyMaterial produces when it cannot resolve an asset, so that blanking the object goes through the same path it
        // always has. Only the tracked document ID is left alone.
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
        // Only the values belonging to the document currently on screen. Anything another document compiled is kept under its own path
        // and applied if and when that document is brought forward.
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

            // These values describe the graph as it is now, but the material instance in the viewport was built from whatever material
            // type asset the Asset Processor has finished, which during a structural edit is the previous one. Renaming a material input
            // node, or changing its type from float2 to float, produces values for a layout the live material does not have, and setting
            // one of those installs an override the material cannot use: MaterialPropertyCollection reports "Accessed as type Float but
            // is type Vector2", the shader parameter write that follows reads a size out of the wrong layout, and the material renders
            // black until the component is recreated, which is why it took an editor restart to clear.
            //
            // Skipping is free. The values are resent in full after every compile and reapplied after every rebuild, so a value that is
            // correct but early is simply applied a moment later instead.
            //
            // Only the plain numeric types are checked. MaterialComponentController::GetPropertyValue passes asset values through
            // ConvertAssetsForSerialization, so an image comes back as a different type than the one that went in and comparing against
            // it would reject every image. Images keep the behaviour they had before this check existed.
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

                // An empty result means the live material has no property by that name at all, which is what a rename looks like from
                // here. A differing type is the float2 to float case. Neither can be applied to this instance.
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

        // Material Canvas bakes material input values into the material type as property defaults rather than into the material as
        // overrides, so a value edit would otherwise only become visible once the material type asset, and every shader built from it, had
        // been rebuilt. Setting the same values as overrides on the live instance shows them immediately and is independent of whether the
        // Asset Processor has caught up. Overrides that already match the defaults are harmless.
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetPropertyValues,
            AZ::Render::DefaultMaterialAssignmentId, propertyOverrides);
    }

    void MaterialCanvasViewportContent::OnMaterialPropertyValuesChanged(
        const AZStd::string& graphPath, const MaterialGraphCompilerNotifications::PropertyValueList& propertyValues)
    {
        // GraphCompiler::CompileGraph refuses to run without a graph path, so an empty one here means something upstream changed and the
        // values cannot be attributed to a document. Storing them under an empty key would hand them to whichever document asked next.
        if (graphPath.empty())
        {
            return;
        }

        {
            AZStd::scoped_lock lock(m_materialPropertyValuesMutex);
            m_materialPropertyValuesByGraphPath[graphPath] = propertyValues;
        }

        // Raised from the graph compilation job thread. Applying the values touches entity component buses, so it is deferred to the next
        // system tick, which decides whose values are actually on screen.
        //
        // This asks for a property apply, not a material rebuild. The values are already excluded from the generated material type, so
        // nothing about the assets has changed and the live instance only needs the overrides set on it. A structural change still
        // reaches the viewport through the asset catalog and queues a rebuild there in the usual way.
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

        // While the generated material has not resolved there is no ID to compare against, so every catalog update is a reason to try
        // again: the material becomes resolvable as soon as the Asset Processor registers its source file, and nothing announces that
        // specifically. Once it has resolved, only its own products and those of its material type matter. Every product carries the GUID
        // of the source it was built from, so comparing GUIDs covers the intermediate and final material type assets alike.
        const bool affected = !m_appliedMaterialAssetId.IsValid() || m_appliedMaterialAssetId.m_guid == assetId.m_guid ||
            (m_appliedMaterialTypeAssetId.IsValid() && m_appliedMaterialTypeAssetId.m_guid == assetId.m_guid);

        if (affected)
        {
            // Catalog notifications are raised from the asset system thread, and rebuilding the material touches entity component buses.
            // Deferring to the next system tick keeps that work on the main thread and collapses the burst of notifications that arrives
            // as the Asset Processor drains its queue into a single rebuild.
            m_applyMaterialQueued = true;
        }
    }

    void MaterialCanvasViewportContent::OnSystemTick()
    {
        const auto now = AZStd::chrono::steady_clock::now();

        // Cheap enough to run as soon as it is asked for: it sets property overrides on a material instance that already exists, with no
        // asset resolution and no new instance, so there is nothing for the debounce below to protect against.
        if (m_applyMaterialPropertyValuesQueued.exchange(false))
        {
            ApplyMaterialPropertyValues();
        }

        if (m_applyMaterialQueued.exchange(false))
        {
            // Each notification pushes the deadline back so that a burst collapses into a single rebuild once the catalog settles. The
            // start of the burst is remembered separately so a catalog that never goes quiet cannot defer the rebuild forever.
            if (!m_applyMaterialPending)
            {
                m_applyMaterialPending = true;
                m_applyMaterialBurstStart = now;
            }

            // Short enough that the preview keeps pace with the Editor viewport, which reacts to the material asset reloading and does
            // not wait for the catalog to settle at all. The original 250ms, paired with a 2s ceiling, meant a rebuild that emitted a
            // steady trickle of catalog updates kept pushing the deadline out and the pane visibly lagged the main viewport. The point of
            // the debounce is only to collapse a per-frame storm into something sane, and 100ms already achieves that.
            const AZ::u64 quietPeriodMs = AtomToolsFramework::GetSettingsValue(
                "/O3DE/Atom/MaterialCanvas/Viewport/ApplyMaterialQuietPeriodMs", (AZ::u64)100);
            m_applyMaterialQuietDeadline = now + AZStd::chrono::milliseconds(quietPeriodMs);
        }

        if (!m_applyMaterialPending || m_appliedDocumentId.IsNull())
        {
            return;
        }

        // Hard ceiling on how long a burst may defer the rebuild. Worst case is now two rebuilds a second rather than one per frame,
        // which is still far below what caused the runaway, while keeping the preview within half a second of the graph.
        const AZ::u64 maxDeferralMs = AtomToolsFramework::GetSettingsValue(
            "/O3DE/Atom/MaterialCanvas/Viewport/ApplyMaterialMaxDeferralMs", (AZ::u64)500);

        const bool catalogWentQuiet = now >= m_applyMaterialQuietDeadline;
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
