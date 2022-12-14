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
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#endif

namespace AtomToolsFramework
{
    class RenderViewportWidget;

    //! EntityPreviewViewportScene configures and initializes the atom scene and render pipeline for the render viewport widget
    class EntityPreviewViewportScene final
    {
    public:
        EntityPreviewViewportScene(
            const AZ::Crc32& toolId,
            RenderViewportWidget* widget,
            AZStd::shared_ptr<AzFramework::EntityContext> entityContext,
            const AZStd::string& sceneName = "EntityPreviewViewportScene",
            const AZStd::string& defaultRenderPipelineAssetPath = "passes/mainrenderpipeline.azasset");
        ~EntityPreviewViewportScene();

        bool ActivateRenderPipeline(const AZStd::string& pipelineAssetPath);
        bool ActivateRenderPipeline(const AZ::Data::AssetId& pipelineAssetId);

        //! Returns a pointer to the scene used for rendering the viewport content
        AZ::RPI::ScenePtr GetScene() const;

        //! Returns a pointer to the pipeline used for rendering the viewport content
        AZ::RPI::RenderPipelinePtr GetPipeline() const;

        //! Returns the AssetId of the active render pipeline 
        AZ::Data::AssetId GetPipelineAssetId() const;

    private:
        using RenderPipelineMap = AZStd::unordered_map<AZ::Data::AssetId, AZ::RPI::RenderPipelinePtr>;

        RenderPipelineMap::iterator AddRenderPipeline(const AZ::Data::AssetId& pipelineAssetId);

        const AZ::Crc32 m_toolId = {};
        AZ::RPI::ScenePtr m_scene;
        AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;

        RenderPipelineMap m_renderPipelines;
        AZ::Data::AssetId m_activeRenderPipelineId;
        AZ::RPI::RenderPipelinePtr m_activeRenderPipeline;
        AZStd::string m_viewportIdSuffix;
        AZ::RPI::WindowContextSharedPtr m_windowContext;

        AZ::Data::Instance<AZ::RPI::SwapChainPass> m_swapChainPass;
        AZStd::shared_ptr<AzFramework::EntityContext> m_entityContext;
    };
} // namespace AtomToolsFramework
