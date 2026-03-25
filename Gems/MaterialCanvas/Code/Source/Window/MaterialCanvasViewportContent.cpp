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
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Components/TransformComponent.h>
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
        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    MaterialCanvasViewportContent::~MaterialCanvasViewportContent()
    {
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

    void MaterialCanvasViewportContent::OnDocumentClosed([[maybe_unused]] const AZ::Uuid& documentId)
    {
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
            ApplyMaterial({});
        }
    }

    void MaterialCanvasViewportContent::OnCompileGraphCompleted(const AZ::Uuid& documentId)
    {
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

    void MaterialCanvasViewportContent::ApplyMaterial(const AZ::Uuid& documentId)
    {
        AZ::Data::AssetId assetId;

        AZStd::vector<AZStd::string> generatedFiles;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            generatedFiles, documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGeneratedFilePaths);

        for (const auto& generatedFile : generatedFiles)
        {
            if (generatedFile.ends_with(".material"))
            {
                if (auto assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(generatedFile, 0))
                {
                    assetId = assetIdOutcome.GetValue();
                    break;
                }
            }
        }

        // When material canvas generates assets, material input property values are assigned as default values in the material type instead
        // of overridden values in the material. The generated material asset is empty except for a single field referencing the material
        // type. Because the material asset never changes, it won't be reprocessed by the AP or treated as a unique asset in the asset
        // system. We force the viewport to create a unique material instance every time a change needs to be reflected in material canvas.
        AZ::Render::MaterialAssignment materialAssignment;
        materialAssignment.m_materialAsset.Create(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        materialAssignment.m_materialInstanceMustBeUnique = true;
        AZ::Render::MaterialAssignmentMap materialAssignmentMap;
        materialAssignmentMap.emplace(AZ::Render::DefaultMaterialAssignmentId, materialAssignment);
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materialAssignmentMap);
    }
} // namespace MaterialCanvas
