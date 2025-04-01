/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Device.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportScene.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>

namespace AtomToolsFramework
{
    EntityPreviewViewportScene::EntityPreviewViewportScene(
        const AZ::Crc32& toolId,
        RenderViewportWidget* widget,
        AZStd::shared_ptr<AzFramework::EntityContext> entityContext,
        const AZStd::string& sceneName,
        const AZStd::string& defaultRenderPipelineAssetPath)
        : m_toolId(toolId)
        , m_entityContext(entityContext)
    {
        AZ::RPI::ViewportContextPtr viewportContext = widget->GetViewportContext();
        const AzFramework::ViewportId viewportId = viewportContext->GetId();
        m_viewportIdSuffix = AZStd::string::format("_%i", viewportId);
        const AZStd::string uniqueSceneName = AZStd::string::format("%s%s", sceneName.c_str(), m_viewportIdSuffix.c_str());
        m_windowContext = viewportContext->GetWindowContext();

        // The viewport context created by RenderViewportWidget has no name.
        // Systems like frame capturing and post FX expect there to be a context with DefaultViewportContextName
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        const AZ::Name defaultContextName = viewportContextManager->GetDefaultViewportContextName();
        viewportContextManager->RenameViewportContext(viewportContext, defaultContextName);

        // Create and register a scene with all available feature processors
        AZ::RPI::SceneDescriptor sceneDesc;
        sceneDesc.m_nameId = AZ::Name(uniqueSceneName);
        m_scene = AZ::RPI::Scene::CreateScene(sceneDesc);
        m_scene->EnableAllFeatureProcessors();

        // Bind m_frameworkScene to the entity context's AzFramework::Scene
        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "EntityPreviewViewportWidget was unable to get the scene system during construction.");

        AZ::Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> createSceneOutcome = sceneSystem->CreateScene(uniqueSceneName);
        AZ_Assert(createSceneOutcome, createSceneOutcome.GetError().c_str());

        m_frameworkScene = createSceneOutcome.TakeValue();
        m_frameworkScene->SetSubsystem(m_scene);
        m_frameworkScene->SetSubsystem(m_entityContext.get());

        // Create the BRDF texture generation pipeline
        AZ::RPI::RenderPipelineDescriptor brdfPipelineDesc;
        brdfPipelineDesc.m_mainViewTagName = "MainCamera";
        brdfPipelineDesc.m_rootPassTemplate = "BRDFTexturePipeline";
        brdfPipelineDesc.m_name = AZStd::string::format("%s_%s", uniqueSceneName.c_str(), brdfPipelineDesc.m_rootPassTemplate.c_str());
        brdfPipelineDesc.m_renderSettings.m_multisampleState = AZ::RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();
        brdfPipelineDesc.m_executeOnce = true;

        AZ::RPI::RenderPipelinePtr brdfTexturePipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(brdfPipelineDesc);
        m_scene->AddRenderPipeline(brdfTexturePipeline);
        m_scene->Activate();

        AZ::RPI::RPISystemInterface::Get()->RegisterScene(m_scene);

        // activate the render pipeline
        // Note: this is done after the scene is registered in order to properly update the MSAA state
        ActivateRenderPipeline(defaultRenderPipelineAssetPath);
    }

    EntityPreviewViewportScene::~EntityPreviewViewportScene()
    {
        m_scene->Deactivate();
        m_scene->RemoveRenderPipeline(m_activeRenderPipeline->GetId());
        AZ::RPI::RPISystemInterface::Get()->UnregisterScene(m_scene);
        m_frameworkScene->UnsetSubsystem(m_scene);
        m_frameworkScene->UnsetSubsystem(m_entityContext.get());

        if (auto sceneSystem = AzFramework::SceneSystemInterface::Get())
        {
            sceneSystem->RemoveScene(m_frameworkScene->GetName());
        }
    }

    EntityPreviewViewportScene::RenderPipelineMap::iterator EntityPreviewViewportScene::AddRenderPipeline(const AZ::Data::AssetId& pipelineAssetId)
    {
        // Load the render pipeline asset
        AZStd::optional<AZ::RPI::RenderPipelineDescriptor> pipelineDesc =
            AZ::RPI::GetRenderPipelineDescriptorFromAsset(pipelineAssetId, m_viewportIdSuffix);
        if (!pipelineDesc.has_value())
        {
            AZ_Error("EntityPreviewViewportScene", false, "Invalid render pipeline descriptor from asset %s", pipelineAssetId.ToFixedString().c_str());
            return m_renderPipelines.end();
        }

        // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene
        AZ::RPI::RenderPipelinePtr renderPipeline = AZ::RPI::RenderPipeline::CreateRenderPipelineForWindow(pipelineDesc.value(), *m_windowContext.get());

        if (renderPipeline)
        {
            return m_renderPipelines.emplace(pipelineAssetId, renderPipeline).first;
        }

        return m_renderPipelines.end();
    }

    bool EntityPreviewViewportScene::ActivateRenderPipeline(const AZ::Data::AssetId& pipelineAssetId)
    {
        if (!pipelineAssetId.IsValid())
        {
            return false;
        }

        auto iter = m_renderPipelines.find(pipelineAssetId);
        if (iter == m_renderPipelines.end())
        {
            iter = AddRenderPipeline(pipelineAssetId);
            if (iter == m_renderPipelines.end())
            {
                // The pipeline was not found and could not be loaded
                return false;
            }
        }

        if (iter->first != m_activeRenderPipelineId)
        {
            iter->second->GetRootPass()->SetEnabled(true); // PassSystem::RemoveRenderPipeline was calling SetEnabled(false)
            m_scene->AddRenderPipeline(iter->second);

            if (m_activeRenderPipeline)
            {
                iter->second->SetDefaultView(m_activeRenderPipeline->GetDefaultView());
                m_scene->RemoveRenderPipeline(m_activeRenderPipeline->GetId());
            }

            m_activeRenderPipelineId = iter->first;
            m_activeRenderPipeline = iter->second;

            // TODO SetApplicationMultisampleState should only be called once per application and will need to consider multiple viewports
            // and pipelines. The default pipeline determines the initial MSAA state for the application.
            AZ::RPI::RPISystemInterface::Get()->SetApplicationMultisampleState(m_activeRenderPipeline->GetRenderSettings().m_multisampleState);
        }

        return true;
    }

    bool EntityPreviewViewportScene::ActivateRenderPipeline(const AZStd::string& pipelineAssetPath)
    {
        using namespace AZ::RPI;
        AZ::Data::AssetId assetId = AssetUtils::GetAssetIdForProductPath(pipelineAssetPath.c_str(), AssetUtils::TraceLevel::Error);
        return ActivateRenderPipeline(assetId);
    }

    AZ::RPI::ScenePtr EntityPreviewViewportScene::GetScene() const
    {
        return m_scene;
    }

    AZ::RPI::RenderPipelinePtr EntityPreviewViewportScene::GetPipeline() const
    {
        return m_activeRenderPipeline;
    }

    AZ::Data::AssetId EntityPreviewViewportScene::GetPipelineAssetId() const
    {
        return m_activeRenderPipelineId;
    }

} // namespace AtomToolsFramework
