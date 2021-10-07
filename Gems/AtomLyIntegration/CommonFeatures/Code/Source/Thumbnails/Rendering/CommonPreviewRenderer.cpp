/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/RPI.Public/Pass/Specific/RenderToTexturePass.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <Atom/RPI.Reflect/System/SceneDescriptor.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Thumbnails/Rendering/CommonThumbnailRenderer.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/CaptureStep.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/FindThumbnailToRenderStep.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/WaitForAssetsToLoadStep.h>
#include <Thumbnails/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CommonThumbnailRenderer::CommonThumbnailRenderer()
            {
                // CommonThumbnailRenderer supports both models and materials, but we connect on materialAssetType
                // since MaterialOrModelThumbnail dispatches event on materialAssetType address too
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::MaterialAsset::RTTI_Type());
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::ModelAsset::RTTI_Type());
                ThumbnailFeatureProcessorProviderBus::Handler::BusConnect();
                SystemTickBus::Handler::BusConnect();

                m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
                m_entityContext->InitContext();

                // Create and register a scene with all required feature processors
                AZStd::unordered_set<AZStd::string> featureProcessors;
                ThumbnailFeatureProcessorProviderBus::Broadcast(
                    &ThumbnailFeatureProcessorProviderBus::Handler::GetCustomFeatureProcessors, featureProcessors);

                RPI::SceneDescriptor sceneDesc;
                sceneDesc.m_featureProcessorNames.assign(featureProcessors.begin(), featureProcessors.end());
                m_scene = RPI::Scene::CreateScene(sceneDesc);

                // Bind m_frameworkScene to the entity context's AzFramework::Scene
                auto sceneSystem = AzFramework::SceneSystemInterface::Get();
                AZ_Assert(sceneSystem, "Thumbnail system failed to get scene system implementation.");

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

                // Create preview model
                AzFramework::EntityContextRequestBus::EventResult(
                    m_modelEntity, m_entityContext->GetContextId(), &AzFramework::EntityContextRequestBus::Events::CreateEntity,
                    "ThumbnailPreviewModel");
                m_modelEntity->CreateComponent(Render::MeshComponentTypeId);
                m_modelEntity->CreateComponent(Render::MaterialComponentTypeId);
                m_modelEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
                m_modelEntity->Init();
                m_modelEntity->Activate();

                m_defaultLightingPresetAsset.Create(DefaultLightingPresetAssetId, true);
                m_defaultMaterialAsset.Create(DefaultMaterialAssetId, true);
                m_defaultModelAsset.Create(DefaultModelAssetId, true);

                m_steps[CommonThumbnailRenderer::Step::FindThumbnailToRender] = AZStd::make_shared<FindThumbnailToRenderStep>(this);
                m_steps[CommonThumbnailRenderer::Step::WaitForAssetsToLoad] = AZStd::make_shared<WaitForAssetsToLoadStep>(this);
                m_steps[CommonThumbnailRenderer::Step::Capture] = AZStd::make_shared<CaptureStep>(this);
                SetStep(CommonThumbnailRenderer::Step::FindThumbnailToRender);
            }

            CommonThumbnailRenderer::~CommonThumbnailRenderer()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusDisconnect();
                SystemTickBus::Handler::BusDisconnect();
                ThumbnailFeatureProcessorProviderBus::Handler::BusDisconnect();

                SetStep(CommonThumbnailRenderer::Step::None);

                if (m_modelEntity)
                {
                    AzFramework::EntityContextRequestBus::Event(
                        m_entityContext->GetContextId(), &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_modelEntity);
                    m_modelEntity = nullptr;
                }

                m_scene->Deactivate();
                m_scene->RemoveRenderPipeline(m_renderPipeline->GetId());
                RPI::RPISystemInterface::Get()->UnregisterScene(m_scene);
                m_frameworkScene->UnsetSubsystem(m_scene);
                m_frameworkScene->UnsetSubsystem(m_entityContext.get());
            }

            void CommonThumbnailRenderer::SetStep(Step step)
            {
                auto stepItr = m_steps.find(m_currentStep);
                if (stepItr != m_steps.end())
                {
                    stepItr->second->Stop();
                }

                m_currentStep = step;

                stepItr = m_steps.find(m_currentStep);
                if (stepItr != m_steps.end())
                {
                    stepItr->second->Start();
                }
            }

            CommonThumbnailRenderer::Step CommonThumbnailRenderer::GetStep() const
            {
                return m_currentStep;
            }

            void CommonThumbnailRenderer::SelectThumbnail()
            {
                if (!m_thumbnailInfoQueue.empty())
                {
                    // pop the next thumbnailkey to be rendered from the queue
                    m_currentThubnailInfo = m_thumbnailInfoQueue.front();
                    m_thumbnailInfoQueue.pop();

                    SetStep(CommonThumbnailRenderer::Step::WaitForAssetsToLoad);
                }
            }

            void CommonThumbnailRenderer::CancelThumbnail()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                    m_currentThubnailInfo.m_key, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                SetStep(CommonThumbnailRenderer::Step::FindThumbnailToRender);
            }

            void CommonThumbnailRenderer::CompleteThumbnail()
            {
                SetStep(CommonThumbnailRenderer::Step::FindThumbnailToRender);
            }

            void CommonThumbnailRenderer::LoadAssets()
            {
                // Determine if thumbnailkey contains a material asset or set a default material
                const Data::AssetId materialAssetId = GetAssetId(m_currentThubnailInfo.m_key, RPI::MaterialAsset::RTTI_Type());
                m_materialAsset.Create(materialAssetId.IsValid() ? materialAssetId : DefaultMaterialAssetId, true);

                // Determine if thumbnailkey contains a model asset or set a default model
                const Data::AssetId modelAssetId = GetAssetId(m_currentThubnailInfo.m_key, RPI::ModelAsset::RTTI_Type());
                m_modelAsset.Create(modelAssetId.IsValid() ? modelAssetId : DefaultModelAssetId, true);

                // Determine if thumbnailkey contains a lighting preset asset or set a default lighting preset
                const Data::AssetId lightingPresetAssetId = GetAssetId(m_currentThubnailInfo.m_key, RPI::AnyAsset::RTTI_Type());
                m_lightingPresetAsset.Create(lightingPresetAssetId.IsValid() ? lightingPresetAssetId : DefaultLightingPresetAssetId, true);
            }

            void CommonThumbnailRenderer::UpdateLoadAssets()
            {
                if (m_materialAsset.IsReady() && m_modelAsset.IsReady() && m_lightingPresetAsset.IsReady())
                {
                    SetStep(CommonThumbnailRenderer::Step::Capture);
                    return;
                }

                if (m_materialAsset.IsError() || m_modelAsset.IsError() || m_lightingPresetAsset.IsError())
                {
                    CancelLoadAssets();
                    return;
                }
            }

            void CommonThumbnailRenderer::CancelLoadAssets()
            {
                AZ_Warning(
                    "CommonThumbnailRenderer", m_materialAsset.IsReady(), "Asset failed to load in time: %s",
                    m_materialAsset.ToString<AZStd::string>().c_str());
                AZ_Warning(
                    "CommonThumbnailRenderer", m_modelAsset.IsReady(), "Asset failed to load in time: %s",
                    m_modelAsset.ToString<AZStd::string>().c_str());
                AZ_Warning(
                    "CommonThumbnailRenderer", m_lightingPresetAsset.IsReady(), "Asset failed to load in time: %s",
                    m_lightingPresetAsset.ToString<AZStd::string>().c_str());
                CancelThumbnail();
            }

            void CommonThumbnailRenderer::UpdateScene()
            {
                UpdateModel();
                UpdateLighting();
                UpdateCamera();
            }

            void CommonThumbnailRenderer::UpdateModel()
            {
                Render::MaterialComponentRequestBus::Event(
                    m_modelEntity->GetId(), &Render::MaterialComponentRequestBus::Events::SetDefaultMaterialOverride,
                    m_materialAsset.GetId());

                Render::MeshComponentRequestBus::Event(
                    m_modelEntity->GetId(), &Render::MeshComponentRequestBus::Events::SetModelAsset, m_modelAsset);
            }

            void CommonThumbnailRenderer::UpdateLighting()
            {
                auto preset = m_lightingPresetAsset->GetDataAs<Render::LightingPreset>();
                if (preset)
                {
                    auto iblFeatureProcessor = m_scene->GetFeatureProcessor<Render::ImageBasedLightFeatureProcessorInterface>();
                    auto postProcessFeatureProcessor = m_scene->GetFeatureProcessor<Render::PostProcessFeatureProcessorInterface>();
                    auto postProcessSettingInterface = postProcessFeatureProcessor->GetOrCreateSettingsInterface(EntityId());
                    auto exposureControlSettingInterface = postProcessSettingInterface->GetOrCreateExposureControlSettingsInterface();
                    auto directionalLightFeatureProcessor =
                        m_scene->GetFeatureProcessor<Render::DirectionalLightFeatureProcessorInterface>();
                    auto skyboxFeatureProcessor = m_scene->GetFeatureProcessor<Render::SkyBoxFeatureProcessorInterface>();
                    skyboxFeatureProcessor->Enable(true);
                    skyboxFeatureProcessor->SetSkyboxMode(Render::SkyBoxMode::Cubemap);

                    Camera::Configuration cameraConfig;
                    cameraConfig.m_fovRadians = FieldOfView;
                    cameraConfig.m_nearClipDistance = NearDist;
                    cameraConfig.m_farClipDistance = FarDist;
                    cameraConfig.m_frustumWidth = 100.0f;
                    cameraConfig.m_frustumHeight = 100.0f;

                    AZStd::vector<Render::DirectionalLightFeatureProcessorInterface::LightHandle> lightHandles;

                    preset->ApplyLightingPreset(
                        iblFeatureProcessor, skyboxFeatureProcessor, exposureControlSettingInterface, directionalLightFeatureProcessor,
                        cameraConfig, lightHandles);
                }
            }

            void CommonThumbnailRenderer::UpdateCamera()
            {
                // Get bounding sphere of the model asset and estimate how far the camera needs to be see all of it
                Vector3 center = {};
                float radius = {};
                m_modelAsset->GetAabb().GetAsSphere(center, radius);

                const auto distance = radius + NearDist;
                const auto cameraRotation = Quaternion::CreateFromAxisAngle(Vector3::CreateAxisZ(), CameraRotationAngle);
                const auto cameraPosition = center - cameraRotation.TransformVector(Vector3(0.0f, distance, 0.0f));
                const auto cameraTransform = Transform::CreateFromQuaternionAndTranslation(cameraRotation, cameraPosition);
                m_view->SetCameraTransform(Matrix3x4::CreateFromTransform(cameraTransform));
            }

            RPI::AttachmentReadback::CallbackFunction CommonThumbnailRenderer::GetCaptureCallback()
            {
                return [this](const RPI::AttachmentReadback::ReadbackResult& result)
                {
                    if (result.m_dataBuffer)
                    {
                        QImage image(
                            result.m_dataBuffer.get()->data(), result.m_imageDescriptor.m_size.m_width,
                            result.m_imageDescriptor.m_size.m_height, QImage::Format_RGBA8888);

                        AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                            m_currentThubnailInfo.m_key,
                            &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered, QPixmap::fromImage(image));
                    }
                    else
                    {
                        AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                            m_currentThubnailInfo.m_key,
                            &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                    }
                };
            }

            bool CommonThumbnailRenderer::StartCapture()
            {
                if (auto renderToTexturePass = azrtti_cast<AZ::RPI::RenderToTexturePass*>(m_renderPipeline->GetRootPass().get()))
                {
                    renderToTexturePass->ResizeOutput(m_currentThubnailInfo.m_size, m_currentThubnailInfo.m_size);
                }

                m_renderPipeline->AddToRenderTickOnce();

                bool startedCapture = false;
                Render::FrameCaptureRequestBus::BroadcastResult(
                    startedCapture, &Render::FrameCaptureRequestBus::Events::CapturePassAttachmentWithCallback, m_passHierarchy,
                    AZStd::string("Output"), GetCaptureCallback(), RPI::PassAttachmentReadbackOption::Output);
                return startedCapture;
            }

            void CommonThumbnailRenderer::EndCapture()
            {
                m_renderPipeline->RemoveFromRenderTick();
            }

            bool CommonThumbnailRenderer::Installed() const
            {
                return true;
            }

            void CommonThumbnailRenderer::OnSystemTick()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
            }

            void CommonThumbnailRenderer::GetCustomFeatureProcessors(AZStd::unordered_set<AZStd::string>& featureProcessors) const
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
            
            void CommonThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
            {
                m_thumbnailInfoQueue.push({ thumbnailKey, thumbnailSize });
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
