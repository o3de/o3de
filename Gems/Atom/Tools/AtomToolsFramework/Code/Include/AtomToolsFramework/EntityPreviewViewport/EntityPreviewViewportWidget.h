/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/RPI.Public/Base.h>
#include <AtomCore/Instance/Instance.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/Windowing/WindowBus.h>
#endif

namespace AZ
{
    class Entity;
    class Component;

    namespace RPI
    {
        class SwapChainPass;
        class WindowContext;
    } // namespace RPI
} // namespace AZ

namespace AtomToolsFramework
{
    class EntityPreviewViewportWidget
        : public RenderViewportWidget
        , public AZ::TransformNotificationBus::MultiHandler
        , public EntityPreviewViewportSettingsNotificationBus::Handler
    {
    public:
        EntityPreviewViewportWidget(
            const AZ::Crc32& toolId,
            const AZStd::string& sceneName = "EntityPreviewViewport",
            const AZStd::string pipelineAssetPath = "passes/MainRenderPipeline.azasset",
            QWidget* parent = nullptr);
        ~EntityPreviewViewportWidget();

        virtual void Init();
        virtual AZ::Aabb GetObjectLocalBounds() const;
        virtual AZ::Aabb GetObjectWorldBounds() const;
        virtual AZ::EntityId GetObjectEntityId() const;
        virtual AZ::EntityId GetCameraEntityId() const;
        virtual AZ::EntityId GetEnvironmentEntityId() const;
        virtual AZ::EntityId GetPostFxEntityId() const;

        AZ::Entity* CreateEntity(const AZStd::string& name, const AZStd::vector<AZ::Uuid>& componentTypeIds);
        void DestroyEntity(AZ::Entity*& entity);

        virtual void CreateScene();
        virtual void DestroyScene();

        virtual void CreateEntities();
        virtual void DestroyEntities();

        virtual void CreateInputController();

    protected:
        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AZ::TransformNotificationBus::MultiHandler overrides...
        void OnTransformChanged(const AZ::Transform&, const AZ::Transform&) override;

        const AZ::Crc32 m_toolId = {};
        const AZStd::string m_sceneName;
        const AZStd::string m_pipelineAssetPath;

        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;

        AZ::RPI::ScenePtr m_scene;
        AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;
        AZ::RPI::RenderPipelinePtr m_renderPipeline;
        AZ::Data::Instance<AZ::RPI::SwapChainPass> m_swapChainPass = {};

        AZ::Entity* m_cameraEntity = {};
        AZStd::vector<AZ::Entity*> m_entities;
        AZ::Aabb m_objectBoundsOld = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
        using DirectionalLightHandle = AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle;
        AZStd::vector<DirectionalLightHandle> m_lightHandles;

        AZStd::shared_ptr<ViewportInputBehaviorController> m_viewportController;
    };
} // namespace AtomToolsFramework
