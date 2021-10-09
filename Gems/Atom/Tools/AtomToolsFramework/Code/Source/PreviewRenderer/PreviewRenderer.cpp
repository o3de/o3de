/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/RPI.Public/Pass/Specific/RenderToTexturePass.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <Atom/RPI.Reflect/System/SceneDescriptor.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewRenderer.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <PreviewRenderer/PreviewRendererCaptureState.h>
#include <PreviewRenderer/PreviewRendererIdleState.h>
#include <PreviewRenderer/PreviewRendererLoadState.h>

#include <QImage>
#include <QPixmap>

namespace AtomToolsFramework
{
    PreviewRenderer::PreviewRenderer(const AZStd::string& sceneName, const AZStd::string& pipelineName)
    {
        PreviewerFeatureProcessorProviderBus::Handler::BusConnect();

        m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
        m_entityContext->InitContext();

        // Create and register a scene with all required feature processors
        AZStd::unordered_set<AZStd::string> featureProcessors;
        PreviewerFeatureProcessorProviderBus::Broadcast(
            &PreviewerFeatureProcessorProviderBus::Handler::GetRequiredFeatureProcessors, featureProcessors);

        AZ::RPI::SceneDescriptor sceneDesc;
        sceneDesc.m_featureProcessorNames.assign(featureProcessors.begin(), featureProcessors.end());
        m_scene = AZ::RPI::Scene::CreateScene(sceneDesc);

        // Bind m_frameworkScene to the entity context's AzFramework::Scene
        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "Failed to get scene system implementation.");

        AZ::Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> createSceneOutcome = sceneSystem->CreateScene(sceneName);
        AZ_Assert(createSceneOutcome, createSceneOutcome.GetError().c_str());

        m_frameworkScene = createSceneOutcome.TakeValue();
        m_frameworkScene->SetSubsystem(m_scene);
        m_frameworkScene->SetSubsystem(m_entityContext.get());

        // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene
        AZ::RPI::RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_mainViewTagName = "MainCamera";
        pipelineDesc.m_name = pipelineName;
        pipelineDesc.m_rootPassTemplate = "MainPipelineRenderToTexture";

        // We have to set the samples to 4 to match the pipeline passes' setting, otherwise it may lead to device lost issue
        // [GFX TODO] [ATOM-13551] Default value sand validation required to prevent pipeline crash and device lost
        pipelineDesc.m_renderSettings.m_multisampleState.m_samples = 4;
        m_renderPipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);
        m_scene->AddRenderPipeline(m_renderPipeline);
        m_scene->Activate();
        AZ::RPI::RPISystemInterface::Get()->RegisterScene(m_scene);
        m_passHierarchy.push_back(pipelineName);
        m_passHierarchy.push_back("CopyToSwapChain");

        // Connect camera to pipeline's default view after camera entity activated
        AZ::Matrix4x4 viewToClipMatrix;
        AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, FieldOfView, AspectRatio, NearDist, FarDist, true);
        m_view = AZ::RPI::View::CreateView(AZ::Name("MainCamera"), AZ::RPI::View::UsageCamera);
        m_view->SetViewToClipMatrix(viewToClipMatrix);
        m_renderPipeline->SetDefaultView(m_view);

        m_states[PreviewRenderer::State::IdleState] = AZStd::make_shared<PreviewRendererIdleState>(this);
        m_states[PreviewRenderer::State::LoadState] = AZStd::make_shared<PreviewRendererLoadState>(this);
        m_states[PreviewRenderer::State::CaptureState] = AZStd::make_shared<PreviewRendererCaptureState>(this);
        SetState(PreviewRenderer::State::IdleState);
    }

    PreviewRenderer::~PreviewRenderer()
    {
        PreviewerFeatureProcessorProviderBus::Handler::BusDisconnect();

        SetState(PreviewRenderer::State::None);
        m_currentCaptureRequest = {};
        m_captureRequestQueue = {};

        m_scene->Deactivate();
        m_scene->RemoveRenderPipeline(m_renderPipeline->GetId());
        AZ::RPI::RPISystemInterface::Get()->UnregisterScene(m_scene);
        m_frameworkScene->UnsetSubsystem(m_scene);
        m_frameworkScene->UnsetSubsystem(m_entityContext.get());
    }

    void PreviewRenderer::AddCaptureRequest(const CaptureRequest& captureRequest)
    {
        m_captureRequestQueue.push(captureRequest);
    }

    AZ::RPI::ScenePtr PreviewRenderer::GetScene() const
    {
        return m_scene;
    }

    AZ::RPI::ViewPtr PreviewRenderer::GetView() const
    {
        return m_view;
    }

    AZ::Uuid PreviewRenderer::GetEntityContextId() const
    {
        return m_entityContext->GetContextId();
    }

    void PreviewRenderer::SetState(State state)
    {
        auto stepItr = m_states.find(m_currentState);
        if (stepItr != m_states.end())
        {
            stepItr->second->Stop();
        }

        m_currentState = state;

        stepItr = m_states.find(m_currentState);
        if (stepItr != m_states.end())
        {
            stepItr->second->Start();
        }
    }

    PreviewRenderer::State PreviewRenderer::GetState() const
    {
        return m_currentState;
    }

    void PreviewRenderer::SelectCaptureRequest()
    {
        if (!m_captureRequestQueue.empty())
        {
            // pop the next request to be rendered from the queue
            m_currentCaptureRequest = m_captureRequestQueue.front();
            m_captureRequestQueue.pop();

            SetState(PreviewRenderer::State::LoadState);
        }
    }

    void PreviewRenderer::CancelCaptureRequest()
    {
        m_currentCaptureRequest.m_captureFailedCallback();
        SetState(PreviewRenderer::State::IdleState);
    }

    void PreviewRenderer::CompleteCaptureRequest()
    {
        SetState(PreviewRenderer::State::IdleState);
    }

    void PreviewRenderer::LoadAssets()
    {
        m_currentCaptureRequest.m_content->Load();
    }

    void PreviewRenderer::UpdateLoadAssets()
    {
        if (m_currentCaptureRequest.m_content->IsReady())
        {
            SetState(PreviewRenderer::State::CaptureState);
            return;
        }

        if (m_currentCaptureRequest.m_content->IsError())
        {
            CancelLoadAssets();
            return;
        }
    }

    void PreviewRenderer::CancelLoadAssets()
    {
        m_currentCaptureRequest.m_content->ReportErrors();
        CancelCaptureRequest();
    }

    void PreviewRenderer::UpdateScene()
    {
        m_currentCaptureRequest.m_content->UpdateScene();
    }

    bool PreviewRenderer::StartCapture()
    {
        auto captureCallback = [currentCaptureRequest = m_currentCaptureRequest](const AZ::RPI::AttachmentReadback::ReadbackResult& result)
        {
            if (result.m_dataBuffer)
            {
                currentCaptureRequest.m_captureCompleteCallback(QPixmap::fromImage(QImage(
                    result.m_dataBuffer.get()->data(), result.m_imageDescriptor.m_size.m_width, result.m_imageDescriptor.m_size.m_height,
                    QImage::Format_RGBA8888)));
            }
            else
            {
                currentCaptureRequest.m_captureFailedCallback();
            }
        };

        if (auto renderToTexturePass = azrtti_cast<AZ::RPI::RenderToTexturePass*>(m_renderPipeline->GetRootPass().get()))
        {
            renderToTexturePass->ResizeOutput(m_currentCaptureRequest.m_size, m_currentCaptureRequest.m_size);
        }

        m_renderPipeline->AddToRenderTickOnce();

        bool startedCapture = false;
        AZ::Render::FrameCaptureRequestBus::BroadcastResult(
            startedCapture, &AZ::Render::FrameCaptureRequestBus::Events::CapturePassAttachmentWithCallback, m_passHierarchy,
            AZStd::string("Output"), captureCallback, AZ::RPI::PassAttachmentReadbackOption::Output);
        return startedCapture;
    }

    void PreviewRenderer::EndCapture()
    {
        m_renderPipeline->RemoveFromRenderTick();
    }

    void PreviewRenderer::GetRequiredFeatureProcessors(AZStd::unordered_set<AZStd::string>& featureProcessors) const
    {
        featureProcessors.insert({
            "AZ::Render::TransformServiceFeatureProcessor",
            "AZ::Render::MeshFeatureProcessor",
            "AZ::Render::SimplePointLightFeatureProcessor",
            "AZ::Render::SimpleSpotLightFeatureProcessor",
            "AZ::Render::PointLightFeatureProcessor",
            // There is currently a bug where having multiple DirectionalLightFeatureProcessors active can result in shadow
            // flickering [ATOM-13568]
            // as well as continually rebuilding MeshDrawPackets [ATOM-13633]. Lets just disable the directional light FP for now.
            // Possibly re-enable with [GFX TODO][ATOM-13639]
            // "AZ::Render::DirectionalLightFeatureProcessor",
            "AZ::Render::DiskLightFeatureProcessor",
            "AZ::Render::CapsuleLightFeatureProcessor",
            "AZ::Render::QuadLightFeatureProcessor",
            "AZ::Render::DecalTextureArrayFeatureProcessor",
            "AZ::Render::ImageBasedLightFeatureProcessor",
            "AZ::Render::PostProcessFeatureProcessor",
            "AZ::Render::SkyBoxFeatureProcessor" });
    }
} // namespace AtomToolsFramework
