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
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
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
#include <AzCore/Asset/AssetManager.h>
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
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    MaterialCanvasViewportContent::~MaterialCanvasViewportContent()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
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
        if (m_lastOpenedDocumentId == documentId)
        {
            m_lastOpenedDocumentId = {};
            ClearMaterial();
        }
        m_previewStates.erase(documentId);
        RebuildAssetBusConnections();
    }

    void MaterialCanvasViewportContent::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        if (m_lastOpenedDocumentId != documentId)
        {
            ClearMaterial();
        }
        m_lastOpenedDocumentId = documentId;
        if (documentId.IsNull())
        {
            return;
        }

        PreviewState& state = m_previewStates[documentId];
        UpdateMaterialIdentity(documentId, state, false);
        if (state.m_sourceGenerationAccepted && !state.m_updatePending && !state.m_updateFailed)
        {
            ApplyExpectedMaterialIfReady(state);
        }
    }

    void MaterialCanvasViewportContent::OnCompileGraphStarted(const AZ::Uuid& documentId)
    {
        const auto stateIt = m_previewStates.find(documentId);
        if (stateIt != m_previewStates.end())
        {
            stateIt->second.m_sourceGenerationAccepted = false;
        }

        if (m_lastOpenedDocumentId == documentId)
        {
            if (AtomToolsFramework::GetSettingsValue(
                    "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphStarted", false))
            {
                ClearMaterial();
            }
        }
    }

    void MaterialCanvasViewportContent::OnCompileGraphGeneratedFilesChanged(
        const AZ::Uuid& documentId, const AZStd::vector<AZStd::string>& modifiedGeneratedFiles)
    {
        const auto stateIt = m_previewStates.find(documentId);
        if (stateIt == m_previewStates.end())
        {
            return;
        }
        PreviewState& state = stateIt->second;
        UpdateMaterialIdentity(documentId, state, false);
        BeginMaterialUpdate(state, modifiedGeneratedFiles);
    }

    void MaterialCanvasViewportContent::OnCompileGraphCompleted(const AZ::Uuid& documentId)
    {
        const auto stateIt = m_previewStates.find(documentId);
        if (stateIt == m_previewStates.end())
        {
            return;
        }

        PreviewState& state = stateIt->second;
        UpdateMaterialIdentity(documentId, state, true);
        state.m_sourceGenerationAccepted = true;
        if (m_lastOpenedDocumentId == documentId
            && !state.m_updatePending
            && !state.m_updateFailed
            && (!m_materialAssigned || state.m_materialProductChanged))
        {
            ApplyExpectedMaterialIfReady(state);
        }
    }

    void MaterialCanvasViewportContent::OnCompileGraphFailed(const AZ::Uuid& documentId)
    {
        const auto stateIt = m_previewStates.find(documentId);
        if (stateIt != m_previewStates.end())
        {
            stateIt->second.m_sourceGenerationAccepted = false;
        }

        if (m_lastOpenedDocumentId == documentId)
        {
            if (AtomToolsFramework::GetSettingsValue(
                    "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphFailed", false))
            {
                ClearMaterial();
            }
        }
    }

    void MaterialCanvasViewportContent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        OnCatalogAssetChanged(assetId);
    }

    void MaterialCanvasViewportContent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZStd::string sourcePath;
        bool assetInfoLoaded = false;
        bool loadedAssetChecked = false;
        bool loadedAssetReady = false;
        bool subscriptionsChanged = false;

        for (auto& [documentId, state] : m_previewStates)
        {
            bool dependencyChanged = state.m_trackedAssetReloads.contains(assetId);
            if (dependencyChanged)
            {
                if (!loadedAssetChecked)
                {
                    const AZ::Data::Asset<AZ::Data::AssetData> loadedAsset =
                        AZ::Data::AssetManager::Instance().FindAsset(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                    loadedAssetReady = loadedAsset.IsReady();
                    loadedAssetChecked = true;
                }
                dependencyChanged = loadedAssetReady;
            }
            bool materialChanged = state.m_materialAssetId.IsValid() && state.m_materialAssetId == assetId;

            if (!dependencyChanged && !materialChanged && state.m_updatePending && !state.m_materialAssetId.IsValid())
            {
                if (!assetInfoLoaded)
                {
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
                    if (assetInfo.m_assetType == AZ::RPI::MaterialAsset::RTTI_Type())
                    {
                        sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId);
                    }
                    assetInfoLoaded = true;
                }

                if (!sourcePath.empty() && IsMaterialSourcePath(state, sourcePath))
                {
                    state.m_materialAssetId = assetId;
                    materialChanged = true;
                }
            }

            if (!dependencyChanged && !materialChanged)
            {
                continue;
            }

            state.m_updatePending = true;
            state.m_updateFailed = false;
            if (dependencyChanged)
            {
                if (!state.m_outstandingAssetReloads.insert(assetId).second)
                {
                    // This change may arrive during an earlier reload; wait for one more completion.
                    state.m_supersededAssetReloads.insert(assetId);
                }
                else
                {
                    subscriptionsChanged = true;
                }
                if (!assetInfoLoaded)
                {
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
                    assetInfoLoaded = true;
                }
                if (assetInfo.m_assetType == AZ::RPI::MaterialTypeAsset::RTTI_Type())
                {
                    // Existing instances may be incompatible with the reloaded material-type layout.
                    if (m_lastOpenedDocumentId == documentId)
                    {
                        ClearMaterial();
                    }
                    state.m_waitForMaterialProduct = true;
                }
                else if (!state.m_waitForMaterialProduct)
                {
                    state.m_materialProductChanged = true;
                }
            }
            if (materialChanged)
            {
                if (state.m_materialLoadState != MaterialLoadState::None)
                {
                    state.m_staleMaterialLoadState = state.m_materialLoadState;
                }
                state.m_materialProductChanged = true;
            }
        }

        if (subscriptionsChanged)
        {
            RebuildAssetBusConnections();
        }
    }

    void MaterialCanvasViewportContent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZStd::scoped_lock lock(m_assetNotificationMutex);
        m_assetNotifications.emplace_back(AZStd::move(asset), AssetNotification::Ready);
    }

    void MaterialCanvasViewportContent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZStd::scoped_lock lock(m_assetNotificationMutex);
        m_assetNotifications.emplace_back(AZStd::move(asset), AssetNotification::Reloaded);
    }

    void MaterialCanvasViewportContent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZStd::scoped_lock lock(m_assetNotificationMutex);
        m_assetNotifications.emplace_back(AZStd::move(asset), AssetNotification::Error);
    }

    void MaterialCanvasViewportContent::OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZStd::scoped_lock lock(m_assetNotificationMutex);
        m_assetNotifications.emplace_back(AZStd::move(asset), AssetNotification::Error);
    }

    void MaterialCanvasViewportContent::OnSystemTick()
    {
        AZStd::vector<AZStd::pair<AZ::Data::Asset<AZ::Data::AssetData>, AssetNotification>> assetNotifications;
        {
            AZStd::scoped_lock lock(m_assetNotificationMutex);
            assetNotifications.swap(m_assetNotifications);
        }

        bool subscriptionsChanged = false;
        AZStd::unordered_set<AZ::Data::AssetId> assetsToReload;
        for (const auto& [asset, notification] : assetNotifications)
        {
            for (auto& [documentId, state] : m_previewStates)
            {
                if (!state.m_updatePending)
                {
                    continue;
                }

                const AZ::Data::AssetId assetId = asset.GetId();
                if (state.m_outstandingAssetReloads.contains(assetId))
                {
                    if (notification == AssetNotification::Error)
                    {
                        FailMaterialUpdate(state);
                        subscriptionsChanged = true;
                        continue;
                    }
                    if (notification == AssetNotification::Reloaded)
                    {
                        if (state.m_supersededAssetReloads.erase(assetId) > 0)
                        {
                            assetsToReload.insert(assetId);
                        }
                        else
                        {
                            state.m_outstandingAssetReloads.erase(assetId);
                            subscriptionsChanged = true;
                        }
                    }
                }

                if (assetId != state.m_materialAssetId)
                {
                    continue;
                }

                MaterialLoadState completedLoadState = MaterialLoadState::None;
                if (notification == AssetNotification::Ready)
                {
                    completedLoadState = MaterialLoadState::Loading;
                }
                else if (notification == AssetNotification::Reloaded)
                {
                    completedLoadState = MaterialLoadState::Reloading;
                }

                if (completedLoadState != MaterialLoadState::None && completedLoadState == state.m_staleMaterialLoadState)
                {
                    state.m_staleMaterialLoadState = MaterialLoadState::None;
                    if (state.m_materialLoadState != MaterialLoadState::None)
                    {
                        if (state.m_sourceGenerationAccepted)
                        {
                            state.m_materialLoadState = MaterialLoadState::Reloading;
                            assetsToReload.insert(assetId);
                        }
                        else
                        {
                            state.m_materialLoadState = MaterialLoadState::None;
                        }
                    }
                    subscriptionsChanged = true;
                    continue;
                }

                if (state.m_materialLoadState != MaterialLoadState::None)
                {
                    if (completedLoadState == state.m_materialLoadState)
                    {
                        state.m_materialAsset = asset;
                        RefreshTrackedAssetReloads(state);
                        state.m_materialLoadState = MaterialLoadState::None;
                        if (state.m_outstandingAssetReloads.empty() && state.m_sourceGenerationAccepted)
                        {
                            ResetMaterialUpdate(state);
                            if (m_lastOpenedDocumentId == documentId)
                            {
                                ApplyMaterial(assetId);
                            }
                        }
                        subscriptionsChanged = true;
                    }
                    else if (notification == AssetNotification::Error)
                    {
                        FailMaterialUpdate(state);
                        subscriptionsChanged = true;
                    }
                }
            }
        }

        AZStd::vector<AZ::Uuid> materialReloads;
        for (auto& [documentId, state] : m_previewStates)
        {
            if (state.m_sourceGenerationAccepted
                && state.m_updatePending
                && state.m_materialProductChanged
                && state.m_outstandingAssetReloads.empty()
                && state.m_materialAssetId.IsValid()
                && state.m_materialLoadState == MaterialLoadState::None)
            {
                state.m_materialLoadState = MaterialLoadState::Loading;
                if (state.m_materialAsset.IsReady())
                {
                    state.m_materialLoadState = MaterialLoadState::Reloading;
                }
                materialReloads.push_back(documentId);
                subscriptionsChanged = true;
            }
        }

        if (subscriptionsChanged)
        {
            RebuildAssetBusConnections();
        }

        for (const AZ::Data::AssetId& assetId : assetsToReload)
        {
            AZ::Data::AssetManager::Instance().ReloadAsset(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        }

        for (const AZ::Uuid& documentId : materialReloads)
        {
            PreviewState& state = m_previewStates.at(documentId);
            if (state.m_materialLoadState == MaterialLoadState::Reloading)
            {
                AZ::Data::AssetManager::Instance().ReloadAsset(
                    state.m_materialAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            }
            else
            {
                state.m_materialAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(
                    state.m_materialAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            }
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

    void MaterialCanvasViewportContent::UpdateMaterialIdentity(
        const AZ::Uuid& documentId, PreviewState& state, bool clearIfEmpty)
    {
        AZStd::vector<AZStd::string> generatedFiles;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            generatedFiles, documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGeneratedFilePaths);

        AZStd::string materialSourcePath;
        for (const AZStd::string& generatedFile : generatedFiles)
        {
            if (generatedFile.ends_with(".material"))
            {
                materialSourcePath = generatedFile;
                break;
            }
        }

        if (state.m_materialSourcePath == materialSourcePath)
        {
            return;
        }

        if (materialSourcePath.empty())
        {
            if (clearIfEmpty)
            {
                const bool sourceGenerationAccepted = state.m_sourceGenerationAccepted;
                state = {};
                state.m_sourceGenerationAccepted = sourceGenerationAccepted;
                RebuildAssetBusConnections();
                if (m_lastOpenedDocumentId == documentId)
                {
                    ClearMaterial();
                }
            }
            return;
        }

        const bool sourceGenerationAccepted = state.m_sourceGenerationAccepted;
        state = {};
        state.m_sourceGenerationAccepted = sourceGenerationAccepted;
        state.m_materialSourcePath = AZStd::move(materialSourcePath);

        const auto assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(state.m_materialSourcePath, 0);
        if (assetIdOutcome)
        {
            state.m_materialAssetId = assetIdOutcome.GetValue();
            state.m_materialAsset = AZ::Data::AssetManager::Instance().FindAsset<AZ::RPI::MaterialAsset>(
                state.m_materialAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, state.m_materialAssetId);
            state.m_materialProductChanged = assetInfo.m_assetType == AZ::RPI::MaterialAsset::RTTI_Type();
        }
        RebuildAssetBusConnections();
    }

    void MaterialCanvasViewportContent::RefreshTrackedAssetReloads(PreviewState& state)
    {
        state.m_trackedAssetReloads.clear();
        if (!state.m_materialAssetId.IsValid())
        {
            return;
        }

        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> dependencies =
            AZ::Failure<AZStd::string>("No asset catalog response");
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            dependencies, &AZ::Data::AssetCatalogRequestBus::Events::GetAllProductDependencies, state.m_materialAssetId);
        if (!dependencies)
        {
            return;
        }

        for (const AZ::Data::ProductDependency& dependency : dependencies.GetValue())
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, dependency.m_assetId);
            if (assetInfo.m_assetType != AZ::RPI::MaterialTypeAsset::RTTI_Type()
                && assetInfo.m_assetType != AZ::RPI::ShaderAsset::RTTI_Type())
            {
                continue;
            }

            state.m_trackedAssetReloads.insert(dependency.m_assetId);
        }
    }

    bool MaterialCanvasViewportContent::IsMaterialSourcePath(
        const PreviewState& state, const AZStd::string& assetPath) const
    {
        if (state.m_materialSourcePath.empty())
        {
            return false;
        }

        AZStd::string normalizedAssetPath = assetPath;
        AZStd::string normalizedExpectedPath = state.m_materialSourcePath;
        AZ::StringFunc::Replace(normalizedAssetPath, '\\', '/');
        AZ::StringFunc::Replace(normalizedExpectedPath, '\\', '/');
        return AZ::StringFunc::Equal(normalizedAssetPath, normalizedExpectedPath);
    }

    void MaterialCanvasViewportContent::BeginMaterialUpdate(
        PreviewState& state, const AZStd::vector<AZStd::string>& modifiedGeneratedFiles)
    {
        bool updateRelevant = false;
        bool waitForMaterialProduct = false;
        for (const AZStd::string& modifiedGeneratedFile : modifiedGeneratedFiles)
        {
            const bool isMaterialType = modifiedGeneratedFile.ends_with(".materialtype");
            const bool isShader = modifiedGeneratedFile.ends_with(".shader");
            const bool isMaterial = modifiedGeneratedFile.ends_with(".material");
            if (isMaterial)
            {
                if (IsMaterialSourcePath(state, modifiedGeneratedFile))
                {
                    updateRelevant = true;
                    waitForMaterialProduct = true;
                }
                continue;
            }
            if (!isMaterialType && !isShader)
            {
                continue;
            }
            AZ::Data::AssetId modifiedAssetId;
            if (isMaterialType)
            {
                const auto assetIdOutcome =
                    AZ::RPI::MaterialUtils::GetFinalMaterialTypeAssetId(state.m_materialSourcePath, modifiedGeneratedFile);
                if (assetIdOutcome)
                {
                    modifiedAssetId = assetIdOutcome.GetValue();
                }
            }
            else
            {
                const auto assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(modifiedGeneratedFile, 0);
                if (assetIdOutcome)
                {
                    modifiedAssetId = assetIdOutcome.GetValue();
                }
            }
            if (!state.m_trackedAssetReloads.contains(modifiedAssetId))
            {
                continue;
            }
            updateRelevant = true;
            if (isMaterialType)
            {
                waitForMaterialProduct = true;
            }
        }

        if (!updateRelevant)
        {
            return;
        }

        const bool waitingForPreviousMaterialProduct =
            state.m_updatePending && state.m_waitForMaterialProduct && !state.m_materialProductChanged;
        if (state.m_materialLoadState != MaterialLoadState::None)
        {
            state.m_staleMaterialLoadState = state.m_materialLoadState;
        }
        state.m_updatePending = true;
        state.m_updateFailed = false;
        state.m_waitForMaterialProduct = waitingForPreviousMaterialProduct || waitForMaterialProduct;
        state.m_materialLoadState = MaterialLoadState::None;
        RebuildAssetBusConnections();
    }

    void MaterialCanvasViewportContent::ResetMaterialUpdate(PreviewState& state)
    {
        state.m_outstandingAssetReloads.clear();
        state.m_supersededAssetReloads.clear();
        state.m_updatePending = false;
        state.m_updateFailed = false;
        state.m_materialProductChanged = false;
        state.m_waitForMaterialProduct = false;
        state.m_staleMaterialLoadState = MaterialLoadState::None;
        state.m_materialLoadState = MaterialLoadState::None;
    }

    void MaterialCanvasViewportContent::FailMaterialUpdate(PreviewState& state)
    {
        ResetMaterialUpdate(state);
        state.m_updateFailed = true;
    }

    void MaterialCanvasViewportContent::RebuildAssetBusConnections()
    {
        AZStd::unordered_set<AZ::Data::AssetId> desiredAssetIds;
        for (const auto& statePair : m_previewStates)
        {
            const PreviewState& state = statePair.second;
            for (const AZ::Data::AssetId& assetId : state.m_outstandingAssetReloads)
            {
                desiredAssetIds.insert(assetId);
            }
            if (state.m_materialLoadState != MaterialLoadState::None
                || state.m_staleMaterialLoadState != MaterialLoadState::None)
            {
                desiredAssetIds.insert(state.m_materialAssetId);
            }
        }

        for (auto connectedAssetIdIt = m_connectedAssetIds.begin(); connectedAssetIdIt != m_connectedAssetIds.end();)
        {
            if (desiredAssetIds.contains(*connectedAssetIdIt))
            {
                ++connectedAssetIdIt;
                continue;
            }

            AZ::Data::AssetBus::MultiHandler::BusDisconnect(*connectedAssetIdIt);
            connectedAssetIdIt = m_connectedAssetIds.erase(connectedAssetIdIt);
        }

        for (const AZ::Data::AssetId& assetId : desiredAssetIds)
        {
            if (m_connectedAssetIds.insert(assetId).second)
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
            }
        }
    }

    void MaterialCanvasViewportContent::ApplyMaterial(const AZ::Data::AssetId& assetId)
    {
        AZ::Render::MaterialAssignment materialAssignment;
        materialAssignment.m_materialAsset =
            AZ::Data::Asset<AZ::RPI::MaterialAsset>(assetId, AZ::RPI::MaterialAsset::RTTI_Type());
        // Catalog updates reuse the asset ID, so the viewport needs a new material instance.
        materialAssignment.m_materialInstanceMustBeUnique = true;
        AZ::Render::MaterialAssignmentMap materialAssignmentMap;
        materialAssignmentMap.emplace(AZ::Render::DefaultMaterialAssignmentId, materialAssignment);
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materialAssignmentMap);
        m_materialAssigned = assetId.IsValid();
    }

    void MaterialCanvasViewportContent::ApplyExpectedMaterialIfReady(PreviewState& state)
    {
        if (!state.m_materialAssetId.IsValid())
        {
            return;
        }

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, state.m_materialAssetId);
        if (assetInfo.m_assetType == AZ::RPI::MaterialAsset::RTTI_Type())
        {
            state.m_materialAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(
                state.m_materialAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            RefreshTrackedAssetReloads(state);
            ApplyMaterial(state.m_materialAssetId);
            state.m_materialProductChanged = false;
        }
    }

    void MaterialCanvasViewportContent::ClearMaterial()
    {
        AZ::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialAssetIdOnDefaultSlot, AZ::Data::AssetId());
        m_materialAssigned = false;
    }
} // namespace MaterialCanvas
