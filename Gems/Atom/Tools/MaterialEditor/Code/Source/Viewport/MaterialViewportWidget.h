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
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Viewport/InputController/MaterialEditorViewportInputController.h>
#include <Viewport/MaterialViewportNotificationBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QWidget>
AZ_POP_DISABLE_WARNING
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

namespace Ui
{
    class MaterialViewportWidget;
}

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
        MaterialViewportWidget(QWidget* parent = nullptr);
        ~MaterialViewportWidget();

    private:
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

        using DirectionalLightHandle = AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle;

        AZ::Data::Instance<AZ::RPI::SwapChainPass> m_swapChainPass;
        AZStd::string m_defaultPipelineAssetPath = "passes/MainRenderPipeline.azasset";
        AZ::RPI::RenderPipelinePtr m_renderPipeline;
        AZ::RPI::ScenePtr m_scene;
        AZ::Render::DirectionalLightFeatureProcessorInterface* m_directionalLightFeatureProcessor = {};
        AZ::Render::DisplayMapperFeatureProcessorInterface* m_displayMapperFeatureProcessor = {};

        AZ::Entity* m_cameraEntity = {};
        AZ::Component* m_cameraComponent = {};

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

        AZStd::shared_ptr<MaterialEditorViewportInputController> m_viewportController;

        QScopedPointer<Ui::MaterialViewportWidget> m_ui;
    };
} // namespace MaterialEditor
