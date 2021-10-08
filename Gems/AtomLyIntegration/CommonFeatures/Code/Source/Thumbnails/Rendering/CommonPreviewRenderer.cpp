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
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Thumbnails/Rendering/CommonPreviewRenderer.h>
#include <Thumbnails/Rendering/CommonPreviewRendererCaptureState.h>
#include <Thumbnails/Rendering/CommonPreviewRendererIdleState.h>
#include <Thumbnails/Rendering/CommonPreviewRendererLoadState.h>
#include <Thumbnails/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CommonPreviewRenderer::CommonPreviewRenderer()
            {
                // CommonPreviewRenderer supports both models and materials
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::MaterialAsset::RTTI_Type());
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::ModelAsset::RTTI_Type());
                PreviewerFeatureProcessorProviderBus::Handler::BusConnect();
                SystemTickBus::Handler::BusConnect();

                m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
                m_entityContext->InitContext();

                // Create and register a scene with all required feature processors
                AZStd::unordered_set<AZStd::string> featureProcessors;
                PreviewerFeatureProcessorProviderBus::Broadcast(
                    &PreviewerFeatureProcessorProviderBus::Handler::GetRequiredFeatureProcessors, featureProcessors);

                RPI::SceneDescriptor sceneDesc;
                sceneDesc.m_featureProcessorNames.assign(featureProcessors.begin(), featureProcessors.end());
                m_scene = RPI::Scene::CreateScene(sceneDesc);

                // Bind m_frameworkScene to the entity context's AzFramework::Scene
                auto sceneSystem = AzFramework::SceneSystemInterface::Get();
                AZ_Assert(sceneSystem, "Failed to get scene system implementation.");

                Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> createSceneOutcome = sceneSystem->CreateScene(m_sceneName);
                AZ_Assert(createSceneOutcome, createSceneOutcome.GetError().c_str());

                m_frameworkScene = createSceneOutcome.TakeValue();
                m_frameworkScene->SetSubsystem(m_scene);
                m_frameworkScene->SetSubsystem(m_entityContext.get());

                // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene
                RPI::RenderPipelineDescriptor pipelineDesc;
                pipelineDesc.m_mainViewTagName = "MainCamera";
                pipelineDesc.m_name = m_pipelineName;
                pipelineDesc.m_rootPassTemplate = "MainPipelineRenderToTexture";

                // We have to set the samples to 4 to match the pipeline passes' setting, otherwise it may lead to device lost issue
                // [GFX TODO] [ATOM-13551] Default value sand validation required to prevent pipeline crash and device lost
                pipelineDesc.m_renderSettings.m_multisampleState.m_samples = 4;
                m_renderPipeline = RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);
                m_scene->AddRenderPipeline(m_renderPipeline);
                m_scene->Activate();
                RPI::RPISystemInterface::Get()->RegisterScene(m_scene);
                m_passHierarchy.push_back(m_pipelineName);
                m_passHierarchy.push_back("CopyToSwapChain");

                // Connect camera to pipeline's default view after camera entity activated
                Matrix4x4 viewToClipMatrix;
                MakePerspectiveFovMatrixRH(viewToClipMatrix, FieldOfView, AspectRatio, NearDist, FarDist, true);
                m_view = RPI::View::CreateView(Name("MainCamera"), RPI::View::UsageCamera);
                m_view->SetViewToClipMatrix(viewToClipMatrix);
                m_renderPipeline->SetDefaultView(m_view);

                m_states[CommonPreviewRenderer::State::IdleState] = AZStd::make_shared<CommonPreviewRendererIdleState>(this);
                m_states[CommonPreviewRenderer::State::LoadState] = AZStd::make_shared<CommonPreviewRendererLoadState>(this);
                m_states[CommonPreviewRenderer::State::CaptureState] = AZStd::make_shared<CommonPreviewRendererCaptureState>(this);
                SetState(CommonPreviewRenderer::State::IdleState);
            }

            CommonPreviewRenderer::~CommonPreviewRenderer()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusDisconnect();
                SystemTickBus::Handler::BusDisconnect();
                PreviewerFeatureProcessorProviderBus::Handler::BusDisconnect();

                SetState(CommonPreviewRenderer::State::None);
                m_currentCaptureRequest = {};
                m_captureRequestQueue = {};

                m_scene->Deactivate();
                m_scene->RemoveRenderPipeline(m_renderPipeline->GetId());
                RPI::RPISystemInterface::Get()->UnregisterScene(m_scene);
                m_frameworkScene->UnsetSubsystem(m_scene);
                m_frameworkScene->UnsetSubsystem(m_entityContext.get());
            }

            void CommonPreviewRenderer::AddCaptureRequest(const CaptureRequest& captureRequest)
            {
                m_captureRequestQueue.push(captureRequest);
            }

            void CommonPreviewRenderer::SetState(State state)
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

            CommonPreviewRenderer::State CommonPreviewRenderer::GetState() const
            {
                return m_currentState;
            }

            void CommonPreviewRenderer::SelectCaptureRequest()
            {
                if (!m_captureRequestQueue.empty())
                {
                    // pop the next request to be rendered from the queue
                    m_currentCaptureRequest = m_captureRequestQueue.front();
                    m_captureRequestQueue.pop();

                    SetState(CommonPreviewRenderer::State::LoadState);
                }
            }

            void CommonPreviewRenderer::CancelCaptureRequest()
            {
                m_currentCaptureRequest.m_captureFailedCallback();
                SetState(CommonPreviewRenderer::State::IdleState);
            }

            void CommonPreviewRenderer::CompleteCaptureRequest()
            {
                SetState(CommonPreviewRenderer::State::IdleState);
            }

            void CommonPreviewRenderer::LoadAssets()
            {
                m_currentCaptureRequest.m_content->Load();
            }

            void CommonPreviewRenderer::UpdateLoadAssets()
            {
                if (m_currentCaptureRequest.m_content->IsReady())
                {
                    SetState(CommonPreviewRenderer::State::CaptureState);
                    return;
                }

                if (m_currentCaptureRequest.m_content->IsError())
                {
                    CancelLoadAssets();
                    return;
                }
            }

            void CommonPreviewRenderer::CancelLoadAssets()
            {
                m_currentCaptureRequest.m_content->ReportErrors();
                CancelCaptureRequest();
            }

            void CommonPreviewRenderer::UpdateScene()
            {
                m_currentCaptureRequest.m_content->UpdateScene();
            }

            bool CommonPreviewRenderer::StartCapture()
            {
                auto captureCallback =
                    [currentCaptureRequest = m_currentCaptureRequest](const RPI::AttachmentReadback::ReadbackResult& result)
                {
                    if (result.m_dataBuffer)
                    {
                        currentCaptureRequest.m_captureCompleteCallback(QImage(
                            result.m_dataBuffer.get()->data(), result.m_imageDescriptor.m_size.m_width,
                            result.m_imageDescriptor.m_size.m_height, QImage::Format_RGBA8888));
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
                Render::FrameCaptureRequestBus::BroadcastResult(
                    startedCapture, &Render::FrameCaptureRequestBus::Events::CapturePassAttachmentWithCallback, m_passHierarchy,
                    AZStd::string("Output"), captureCallback, RPI::PassAttachmentReadbackOption::Output);
                return startedCapture;
            }

            void CommonPreviewRenderer::EndCapture()
            {
                m_renderPipeline->RemoveFromRenderTick();
            }

            void CommonPreviewRenderer::OnSystemTick()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
            }

            void CommonPreviewRenderer::GetRequiredFeatureProcessors(AZStd::unordered_set<AZStd::string>& featureProcessors) const
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

            void CommonPreviewRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
            {
                AddCaptureRequest(
                    { thumbnailSize,
                      AZStd::make_shared<CommonPreviewContent>(
                          m_scene, m_view, m_entityContext->GetContextId(),
                          GetAssetId(thumbnailKey, RPI::ModelAsset::RTTI_Type()),
                          GetAssetId(thumbnailKey, RPI::MaterialAsset::RTTI_Type()),
                          GetAssetId(thumbnailKey, RPI::AnyAsset::RTTI_Type())),
                          [thumbnailKey]()
                          {
                              AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                                  thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                          },
                          [thumbnailKey](const QImage& image)
                          {
                              AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                                  thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered,
                                  QPixmap::fromImage(image));
                          } });
            }

            bool CommonPreviewRenderer::Installed() const
            {
                return true;
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
