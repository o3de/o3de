/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Base.h>
#include <AtomCore/Instance/Instance.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Viewport/MaterialViewportNotificationBus.h>
#endif

namespace AZ
{
    namespace Render
    {
        class DisplayMapperFeatureProcessorInterface;
    }

    class Entity;
    class Component;

    namespace RPI
    {
        class SwapChainPass;
        class WindowContext;
    } // namespace RPI
} // namespace AZ

namespace MaterialEditor
{
    class MaterialViewportWidget
        : public AtomToolsFramework::RenderViewportWidget
        , public AZ::Data::AssetBus::Handler
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , public MaterialViewportNotificationBus::Handler
        , public AZ::TransformNotificationBus::MultiHandler
    {
    public:
        MaterialViewportWidget(const AZ::Crc32& toolId, QWidget* parent = nullptr);
        ~MaterialViewportWidget();

    private:
        AZ::Entity* CreateEntity(const AZStd::string& name, const AZStd::vector<AZ::Uuid>& componentTypeIds);
        void DestroyEntity(AZ::Entity*& entity);
        void SetupInputController();

        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler interface overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // MaterialViewportNotificationBus::Handler interface overrides...
        void OnLightingPresetSelected(AZ::Render::LightingPresetPtr preset) override;
        void OnLightingPresetChanged(AZ::Render::LightingPresetPtr preset) override;
        void OnModelPresetSelected(AZ::Render::ModelPresetPtr preset) override;
        void OnModelPresetChanged(AZ::Render::ModelPresetPtr preset) override;
        void OnShadowCatcherEnabledChanged(bool enable) override;
        void OnGridEnabledChanged(bool enable) override;
        void OnAlternateSkyboxEnabledChanged(bool enable) override;
        void OnFieldOfViewChanged(float fieldOfView) override;
        void OnDisplayMapperOperationTypeChanged(AZ::Render::DisplayMapperOperationType operationType) override;

        // AZ::Data::AssetBus::Handler interface overrides...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AZ::TransformNotificationBus::MultiHandler overrides...
        void OnTransformChanged(const AZ::Transform&, const AZ::Transform&) override;

        const AZ::Crc32 m_toolId = {};

        using DirectionalLightHandle = AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle;

        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;

        AZ::RPI::ScenePtr m_scene;
        AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;
        AZ::RPI::RenderPipelinePtr m_renderPipeline;
        AZ::Data::Instance<AZ::RPI::SwapChainPass> m_swapChainPass;
        AZStd::string m_mainPipelineAssetPath = "passes/MainRenderPipeline.azasset";

        AZ::Render::DirectionalLightFeatureProcessorInterface* m_directionalLightFeatureProcessor = {};
        AZ::Render::DisplayMapperFeatureProcessorInterface* m_displayMapperFeatureProcessor = {};

        AZ::Entity* m_cameraEntity = {};
        AZ::Entity* m_postProcessEntity = {};

        AZ::Entity* m_modelEntity = {};
        AZ::Data::AssetId m_modelAssetId;

        AZ::Entity* m_gridEntity = {};

        AZ::Entity* m_shadowCatcherEntity = {};
        AZ::Data::Instance<AZ::RPI::Material> m_shadowCatcherMaterial;
        AZ::RPI::MaterialPropertyIndex m_shadowCatcherOpacityPropertyIndex;

        AZStd::vector<DirectionalLightHandle> m_lightHandles;

        AZ::Entity* m_iblEntity = {};
        AZ::Render::SkyBoxFeatureProcessorInterface* m_skyboxFeatureProcessor = {};

        AZStd::shared_ptr<AtomToolsFramework::ViewportInputBehaviorController> m_viewportController;
    };
} // namespace MaterialEditor
