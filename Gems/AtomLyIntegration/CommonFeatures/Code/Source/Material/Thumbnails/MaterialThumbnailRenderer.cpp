/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzFramework/Components/TransformComponent.h>

#include <AzCore/Math/MatrixUtils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Mesh/MeshComponent.h>
#include <Material/MaterialComponent.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <Source/Material/Thumbnails/MaterialThumbnailRenderer.h>
#include "Atom/Feature/Utils/FrameCaptureBus.h"
#include "Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h"

namespace AZ
{
    namespace LyIntegration
    {
        using namespace AzToolsFramework::Thumbnailer;

        static constexpr const char* ModelPath = "materialeditor/viewportmodels/quadsphere.azmodel";
        static constexpr const char* LightingPresetPath = "lightingpresets/default.lightingpreset.azasset";
        static constexpr float AspectRatio = 1.0f;
        static constexpr float NearDist = 0.1f;
        static constexpr float FarDist = 100.0f;

        MaterialThumbnailRenderer::MaterialThumbnailRenderer()
        {
            m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
            m_entityContext->InitContext();

            Data::AssetType materialAssetType = AzTypeInfo<RPI::MaterialAsset>::Uuid();
            ThumbnailerRendererRequestBus::Handler::BusConnect(materialAssetType);
            SystemTickBus::Handler::BusConnect();

            m_shouldPullNextAsset = true;
        }

        MaterialThumbnailRenderer::~MaterialThumbnailRenderer()
        {
            ThumbnailerRendererRequestBus::Handler::BusDisconnect();
            SystemTickBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusDisconnect();
            Render::MaterialComponentNotificationBus::Handler::BusDisconnect();
            m_materialAssetToRender.Release();

            if (m_initialized)
            {
                Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
                TickBus::Handler::BusDisconnect();

                if (m_modelEntity)
                {
                    AzFramework::EntityContextRequestBus::Event(m_entityContext->GetContextId(),
                        &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_modelEntity);
                    m_modelEntity = nullptr;
                }

                m_frameworkScene->UnsetSubsystem<RPI::Scene>();

                m_scene->Deactivate();
                m_scene->RemoveRenderPipeline(m_renderPipeline->GetId());
                RPI::RPISystemInterface::Get()->UnregisterScene(m_scene);
                bool sceneRemovedSuccessfully = false;
                AzFramework::SceneSystemRequestBus::BroadcastResult(sceneRemovedSuccessfully, &AzFramework::SceneSystemRequests::RemoveScene, m_sceneName);
                m_scene = nullptr;
                m_renderPipeline = nullptr;
            }
        }

        bool MaterialThumbnailRenderer::Installed() const
        {
            return true;
        }

        void MaterialThumbnailRenderer::OnSystemTick()
        {
            ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
        }

        void MaterialThumbnailRenderer::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            m_deltaTime = deltaTime;
            m_simulateTime = time.GetSeconds();

            if (m_shouldPullNextAsset && !m_assetIdQueue.empty())
            {
                m_shouldPullNextAsset = false;

                const auto assetId = m_assetIdQueue.front();
                m_assetIdQueue.pop();

                m_materialAssetToRender.Release();
                AZ::Data::AssetBus::Handler::BusDisconnect();

                if (assetId.IsValid())
                {
                    m_materialAssetToRender.Create(assetId);
                    m_materialAssetToRender.QueueLoad();
                    AZ::Data::AssetBus::Handler::BusConnect(assetId);
                }
            }
            else if (m_readyToCapture)
            {
                m_renderPipeline->AddToRenderTickOnce();

                RPI::AttachmentReadback::CallbackFunction readbackCallback = [&](const RPI::AttachmentReadback::ReadbackResult& result)
                    {
                        uchar* data = result.m_dataBuffer.get()->data();
                        QImage image(data, result.m_imageDescriptor.m_size.m_width, result.m_imageDescriptor.m_size.m_height, QImage::Format_RGBA8888);
                        QPixmap pixmap;
                        pixmap.convertFromImage(image);
                        ThumbnailerRendererNotificationBus::Event(m_materialAssetToRender.GetId(),
                            &ThumbnailerRendererNotifications::ThumbnailRendered, pixmap);
                    };

                Render::FrameCaptureNotificationBus::Handler::BusConnect();
                bool startedCapture = false;
                Render::FrameCaptureRequestBus::BroadcastResult(
                    startedCapture,
                    &Render::FrameCaptureRequestBus::Events::CapturePassAttachmentWithCallback,
                    m_passHierarchy, AZStd::string("Output"), readbackCallback);
                // Reset the capture flag the capture requst was successful. Otherwise try capture it again next tick.
                if (startedCapture)
                {
                    m_readyToCapture = false;
                }
            }
        }

        void MaterialThumbnailRenderer::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_materialAssetToRender = asset;
            AZ::Data::AssetBus::Handler::BusDisconnect();

            Render::MaterialComponentRequestBus::Event(m_modelEntity->GetId(),
                &Render::MaterialComponentRequestBus::Events::SetDefaultMaterialOverride,
                m_materialAssetToRender.GetId());

            // listen to material override finished notification.
            Render::MaterialComponentNotificationBus::Handler::BusConnect(m_modelEntity->GetId());
        }

        void MaterialThumbnailRenderer::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetCanceled(asset.GetId());
        }

        void MaterialThumbnailRenderer::OnAssetCanceled([[maybe_unused]] AZ::Data::AssetId assetId)
        {
            m_readyToCapture = false;
            m_shouldPullNextAsset = true;
            m_materialAssetToRender.Release();
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }

        void MaterialThumbnailRenderer::OnCaptureFinished([[maybe_unused]] Render::FrameCaptureResult result, [[maybe_unused]] const AZStd::string& info)
        {
            m_shouldPullNextAsset = true;
            m_renderPipeline->RemoveFromRenderTick();
            Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
            if (m_assetIdQueue.empty())
            {
                TickBus::Handler::BusDisconnect();
            }
        }

        void MaterialThumbnailRenderer::Init()
        {
            using namespace Data;

            // Create and register a scene with minimum required feature processors
            RPI::SceneDescriptor sceneDesc;
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::TransformServiceFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::MeshFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::PointLightFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::SpotLightFeatureProcessor");
            // There is currently a bug where having multiple DirectionalLightFeatureProcessors active can result in shadow flickering [ATOM-13568]
            // as well as continually rebuilding MeshDrawPackets [ATOM-13633]. Lets just disable the directional light FP for now.
            // Possibly re-enable with [GFX TODO][ATOM-13639] 
            // sceneDesc.m_featureProcessorNames.push_back("AZ::Render::DirectionalLightFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::DiskLightFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::CapsuleLightFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::QuadLightFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::DecalTextureArrayFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::ImageBasedLightFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::PostProcessFeatureProcessor");
            sceneDesc.m_featureProcessorNames.push_back("AZ::Render::SkyBoxFeatureProcessor");
            m_scene = RPI::Scene::CreateScene(sceneDesc);

            // Setup scene srg modification callback (to push per-frame values to the shaders)
            RPI::ShaderResourceGroupCallback callback = [this](RPI::ShaderResourceGroup* srg)
                {
                    if (srg == nullptr)
                    {
                        return;
                    }
                    bool needCompile = false;
                    RHI::ShaderInputConstantIndex timeIndex = srg->FindShaderInputConstantIndex(Name{ "m_time" });
                    if (timeIndex.IsValid())
                    {
                        srg->SetConstant(timeIndex, (float)m_simulateTime);
                        needCompile = true;
                    }
                    RHI::ShaderInputConstantIndex deltaTimeIndex = srg->FindShaderInputConstantIndex(Name{ "m_deltaTime" });
                    if (deltaTimeIndex.IsValid())
                    {
                        srg->SetConstant(deltaTimeIndex, m_deltaTime);
                        needCompile = true;
                    }

                    if (needCompile)
                    {
                        srg->Compile();
                    }
                };
            m_scene->SetShaderResourceGroupCallback(callback);

            // Bind m_defaultScene to the GameEntityContext's AzFramework::Scene
            Outcome<AzFramework::Scene*, AZStd::string> createSceneOutcome;
            AzFramework::SceneSystemRequestBus::BroadcastResult(createSceneOutcome, &AzFramework::SceneSystemRequests::CreateScene,
                m_sceneName);
            AZ_Assert(createSceneOutcome, createSceneOutcome.GetError().c_str()); // This should never happen unless scene creation has changed.
            createSceneOutcome.GetValue()->SetSubsystem(m_scene.get());
            m_frameworkScene = createSceneOutcome.GetValue();
            m_frameworkScene->SetSubsystem(m_scene.get());

            m_entityContext->InitContext();

            bool success = false;
            AzFramework::SceneSystemRequestBus::BroadcastResult(success, &AzFramework::SceneSystemRequests::SetSceneForEntityContextId,
                m_entityContext->GetContextId(), m_frameworkScene);
            AZ_Assert(success, "Unable to set entity context on AzFramework::Scene: %s", m_sceneName.c_str());

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
            Name viewName = Name("MainCamera");
            m_view = RPI::View::CreateView(viewName, RPI::View::UsageCamera);
            m_transform = Transform::CreateFromQuaternionAndTranslation(Quaternion::CreateIdentity(), Vector3::CreateZero());
            m_view->SetCameraTransform(Matrix3x4::CreateFromTransform(m_transform));

            Matrix4x4 viewToClipMatrix;
            MakePerspectiveFovMatrixRH(viewToClipMatrix,
                Constants::HalfPi,
                AspectRatio,
                NearDist,
                FarDist, true);
            m_view->SetViewToClipMatrix(viewToClipMatrix);

            m_renderPipeline->SetDefaultView(m_view);

            // Create lighting preset
            m_lightingPresetAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(LightingPresetPath);
            if (m_lightingPresetAsset.IsReady())
            {
                auto preset = m_lightingPresetAsset->GetDataAs<AZ::Render::LightingPreset>();
                if (preset)
                {
                    auto iblFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::ImageBasedLightFeatureProcessorInterface>();
                    auto postProcessFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::PostProcessFeatureProcessorInterface>();
                    auto exposureControlSettingInterface = postProcessFeatureProcessor->GetOrCreateSettingsInterface(AZ::EntityId())->GetOrCreateExposureControlSettingsInterface();
                    auto directionalLightFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>();
                    auto skyboxFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
                    skyboxFeatureProcessor->Enable(true);
                    skyboxFeatureProcessor->SetSkyboxMode(AZ::Render::SkyBoxMode::Cubemap);

                    Camera::Configuration cameraConfig;
                    cameraConfig.m_fovRadians = Constants::HalfPi;
                    cameraConfig.m_nearClipDistance = NearDist;
                    cameraConfig.m_farClipDistance = FarDist;
                    cameraConfig.m_frustumWidth = 100.0f;
                    cameraConfig.m_frustumHeight = 100.0f;

                    AZStd::vector<AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle> lightHandles;

                    preset->ApplyLightingPreset(
                        iblFeatureProcessor,
                        skyboxFeatureProcessor,
                        exposureControlSettingInterface,
                        directionalLightFeatureProcessor,
                        cameraConfig,
                        lightHandles);
                }
            }

            // Create preview model
            AzFramework::EntityContextRequestBus::EventResult(m_modelEntity, m_entityContext->GetContextId(),
                &AzFramework::EntityContextRequestBus::Events::CreateEntity, "PreviewModel");
            m_modelEntity->CreateComponent(Render::MeshComponentTypeId);
            m_modelEntity->CreateComponent(Render::MaterialComponentTypeId);
            m_modelEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
            m_modelEntity->Init();
            m_modelEntity->Activate();
            TransformBus::Event(m_modelEntity->GetId(), &TransformBus::Events::SetLocalTM,
                Transform::CreateTranslation(Vector3(0, 0.8f, -0.5f)));

            Render::MeshComponentRequestBus::Event(m_modelEntity->GetId(), &Render::MeshComponentRequestBus::Events::SetModelAssetPath, ModelPath);
        }

        void MaterialThumbnailRenderer::RenderThumbnail(Data::AssetId assetId, [[maybe_unused]] int thumbnailSize)
        {
            if (!m_initialized)
            {
                Init();
                m_initialized = true;
            }

            if (m_assetIdQueue.empty())
            {
                m_shouldPullNextAsset = true;
                TickBus::Handler::BusConnect();
            }

            m_assetIdQueue.emplace(assetId);
        }

        void MaterialThumbnailRenderer::OnMaterialsUpdated([[maybe_unused]] const Render::MaterialAssignmentMap& materials)
        {
            m_readyToCapture = true;
            Render::MaterialComponentNotificationBus::Handler::BusDisconnect();
        }

    } // namespace LyIntegration
} // namespace AZ
