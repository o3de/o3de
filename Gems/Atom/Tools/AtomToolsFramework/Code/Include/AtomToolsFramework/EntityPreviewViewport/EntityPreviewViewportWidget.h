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
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportContent.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportInputController.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportScene.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/Windowing/WindowBus.h>
#endif

namespace AtomToolsFramework
{
    //! EntityPreviewViewportWidget is a viewport render widget that can be set up to display lighting and model presets, entities and
    //! components, and other rendering features. The lighting and model presets and other viewport content will be updated as notifications
    //! are received for viewport settings changes.
    class EntityPreviewViewportWidget final
        : public RenderViewportWidget
        , public EntityPreviewViewportSettingsNotificationBus::Handler
    {
    public:
        EntityPreviewViewportWidget(const AZ::Crc32& toolId, QWidget* parent = nullptr);
        ~EntityPreviewViewportWidget();

        //! Initializes the input controller and other content after the render widget has been created. This is not done on construction
        //! because multiple objects require the widget to be allocated beforehand.
        void Init(
            AZStd::shared_ptr<AzFramework::EntityContext> entityContext,
            AZStd::shared_ptr<EntityPreviewViewportScene> viewportScene,
            AZStd::shared_ptr<EntityPreviewViewportContent> viewportContent,
            AZStd::shared_ptr<EntityPreviewViewportInputController> viewportController);

    private:
        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        const AZ::Crc32 m_toolId = {};
        AZStd::shared_ptr<AzFramework::EntityContext> m_entityContext;
        AZStd::shared_ptr<EntityPreviewViewportScene> m_viewportScene;
        AZStd::shared_ptr<EntityPreviewViewportContent> m_viewportContent;
        AZStd::shared_ptr<EntityPreviewViewportInputController> m_viewportController;

        // Last recorded local object bounds used to check for object changes
        AZ::Aabb m_objectLocalBoundsOld = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);

        // Last recorded camera transform used to update directional lights for the lighting preset
        AZ::Transform m_cameraTransformOld = AZ::Transform::CreateIdentity();

        // Directional light handles produced by the lighting preset
        using DirectionalLightHandle = AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle;
        AZStd::vector<DirectionalLightHandle> m_lightHandles;
    };
} // namespace AtomToolsFramework
