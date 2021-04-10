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


#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <Atom/RPI.Reflect/System/SceneDescriptor.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Thumbnails/Rendering/ThumbnailRendererData.h>
#include <Thumbnails/Rendering/ThumbnailRendererContext.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/InitializeStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {

            InitializeStep::InitializeStep(ThumbnailRendererContext* context)
                : ThumbnailRendererStep(context)
            {
            }

            void InitializeStep::Start()
            {
                auto data = m_context->GetData();

                data->m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
                data->m_entityContext->InitContext();

                // Create and register a scene with minimum required feature processors
                RPI::SceneDescriptor sceneDesc;
                sceneDesc.m_featureProcessorNames.push_back("AZ::Render::TransformServiceFeatureProcessor");
                sceneDesc.m_featureProcessorNames.push_back("AZ::Render::MeshFeatureProcessor");
                sceneDesc.m_featureProcessorNames.push_back("AZ::Render::SimplePointLightFeatureProcessor");
                sceneDesc.m_featureProcessorNames.push_back("AZ::Render::SimpleSpotLightFeatureProcessor");
                sceneDesc.m_featureProcessorNames.push_back("AZ::Render::PointLightFeatureProcessor");
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

                data->m_scene = RPI::Scene::CreateScene(sceneDesc);

                // Setup scene srg modification callback (to push per-frame values to the shaders)
                RPI::ShaderResourceGroupCallback callback = [data](RPI::ShaderResourceGroup* srg)
                {
                    if (srg == nullptr)
                    {
                        return;
                    }
                    bool needCompile = false;
                    RHI::ShaderInputConstantIndex timeIndex = srg->FindShaderInputConstantIndex(Name{ "m_time" });
                    if (timeIndex.IsValid())
                    {
                        srg->SetConstant(timeIndex, aznumeric_cast<float>(data->m_simulateTime));
                        needCompile = true;
                    }
                    RHI::ShaderInputConstantIndex deltaTimeIndex = srg->FindShaderInputConstantIndex(Name{ "m_deltaTime" });
                    if (deltaTimeIndex.IsValid())
                    {
                        srg->SetConstant(deltaTimeIndex, data->m_deltaTime);
                        needCompile = true;
                    }

                    if (needCompile)
                    {
                        srg->Compile();
                    }
                };
                data->m_scene->SetShaderResourceGroupCallback(callback);

                // Bind m_defaultScene to the GameEntityContext's AzFramework::Scene
                auto* sceneSystem = AzFramework::SceneSystemInterface::Get();
                AZ_Assert(sceneSystem, "Thumbnail system failed to get scene system implementation.");
                Outcome<AzFramework::Scene*, AZStd::string> createSceneOutcome = sceneSystem->CreateScene(data->m_sceneName);
                AZ_Assert(createSceneOutcome, createSceneOutcome.GetError().c_str()); // This should never happen unless scene creation has changed.
                createSceneOutcome.GetValue()->SetSubsystem(data->m_scene);
                data->m_frameworkScene = createSceneOutcome.GetValue();
                data->m_frameworkScene->SetSubsystem(data->m_scene);

                data->m_frameworkScene->SetSubsystem(data->m_entityContext.get());
                // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene
                RPI::RenderPipelineDescriptor pipelineDesc;
                pipelineDesc.m_mainViewTagName = "MainCamera";
                pipelineDesc.m_name = data->m_pipelineName;
                pipelineDesc.m_rootPassTemplate = "MainPipelineRenderToTexture";
                // We have to set the samples to 4 to match the pipeline passes' setting, otherwise it may lead to device lost issue
                // [GFX TODO] [ATOM-13551] Default value sand validation required to prevent pipeline crash and device lost
                pipelineDesc.m_renderSettings.m_multisampleState.m_samples = 4;
                data->m_renderPipeline = RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);
                data->m_scene->AddRenderPipeline(data->m_renderPipeline);
                data->m_scene->Activate();
                RPI::RPISystemInterface::Get()->RegisterScene(data->m_scene);
                data->m_passHierarchy.push_back(data->m_pipelineName);
                data->m_passHierarchy.push_back("CopyToSwapChain");

                // Connect camera to pipeline's default view after camera entity activated
                Name viewName = Name("MainCamera");
                data->m_view = RPI::View::CreateView(viewName, RPI::View::UsageCamera);

                Matrix4x4 viewToClipMatrix;
                MakePerspectiveFovMatrixRH(viewToClipMatrix,
                    Constants::QuarterPi,
                    AspectRatio,
                    NearDist,
                    FarDist, true);
                data->m_view->SetViewToClipMatrix(viewToClipMatrix);

                data->m_renderPipeline->SetDefaultView(data->m_view);

                // Create lighting preset
                data->m_lightingPresetAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(ThumbnailRendererData::LightingPresetPath);
                if (data->m_lightingPresetAsset.IsReady())
                {
                    auto preset = data->m_lightingPresetAsset->GetDataAs<Render::LightingPreset>();
                    if (preset)
                    {
                        auto iblFeatureProcessor = data->m_scene->GetFeatureProcessor<Render::ImageBasedLightFeatureProcessorInterface>();
                        auto postProcessFeatureProcessor = data->m_scene->GetFeatureProcessor<Render::PostProcessFeatureProcessorInterface>();
                        auto exposureControlSettingInterface = postProcessFeatureProcessor->GetOrCreateSettingsInterface(EntityId())->GetOrCreateExposureControlSettingsInterface();
                        auto directionalLightFeatureProcessor = data->m_scene->GetFeatureProcessor<Render::DirectionalLightFeatureProcessorInterface>();
                        auto skyboxFeatureProcessor = data->m_scene->GetFeatureProcessor<Render::SkyBoxFeatureProcessorInterface>();
                        skyboxFeatureProcessor->Enable(true);
                        skyboxFeatureProcessor->SetSkyboxMode(Render::SkyBoxMode::Cubemap);

                        Camera::Configuration cameraConfig;
                        cameraConfig.m_fovRadians = Constants::HalfPi;
                        cameraConfig.m_nearClipDistance = NearDist;
                        cameraConfig.m_farClipDistance = FarDist;
                        cameraConfig.m_frustumWidth = 100.0f;
                        cameraConfig.m_frustumHeight = 100.0f;

                        AZStd::vector<Render::DirectionalLightFeatureProcessorInterface::LightHandle> lightHandles;

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
                AzFramework::EntityContextRequestBus::EventResult(data->m_modelEntity, data->m_entityContext->GetContextId(),
                    &AzFramework::EntityContextRequestBus::Events::CreateEntity, "ThumbnailPreviewModel");
                data->m_modelEntity->CreateComponent(Render::MeshComponentTypeId);
                data->m_modelEntity->CreateComponent(Render::MaterialComponentTypeId);
                data->m_modelEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
                data->m_modelEntity->Init();
                data->m_modelEntity->Activate();

                // preload default model
                Data::AssetId defaultModelAssetId;
                Data::AssetCatalogRequestBus::BroadcastResult(
                    defaultModelAssetId,
                    &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                    m_context->GetData()->DefaultModelPath,
                    RPI::ModelAsset::RTTI_Type(),
                    false);
                if (m_context->GetData()->m_assetsToLoad.emplace(defaultModelAssetId).second)
                {
                    data->m_defaultModelAsset.Create(defaultModelAssetId);
                    data->m_defaultModelAsset.QueueLoad();
                }

                // preload default material
                Data::AssetId defaultMaterialAssetId;
                Data::AssetCatalogRequestBus::BroadcastResult(
                    defaultMaterialAssetId,
                    &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                    m_context->GetData()->DefaultMaterialPath,
                    RPI::MaterialAsset::RTTI_Type(),
                    false);
                if (m_context->GetData()->m_assetsToLoad.emplace(defaultMaterialAssetId).second)
                {
                    data->m_defaultMaterialAsset.Create(defaultMaterialAssetId);
                    data->m_defaultMaterialAsset.QueueLoad();
                }

                m_context->SetStep(Step::FindThumbnailToRender);
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
