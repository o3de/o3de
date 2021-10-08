/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/EBus/Results.h>

#include <AzFramework/Components/TransformComponent.h>

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
#include <AtomLyIntegration/CommonFeatures/Thumbnails/ThumbnailFeatureProcessorProviderBus.h>

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

                // Create and register a scene with all required feature processors
                RPI::SceneDescriptor sceneDesc;

                AZ::EBusAggregateResults<AZStd::vector<AZStd::string>> results;
                ThumbnailFeatureProcessorProviderBus::BroadcastResult(results, &ThumbnailFeatureProcessorProviderBus::Handler::GetCustomFeatureProcessors);

                AZStd::set<AZStd::string> featureProcessorNames;
                for (auto& resultCollection : results.values)
                {
                    for (auto& featureProcessorName : resultCollection)
                    {
                        if (featureProcessorNames.emplace(featureProcessorName).second)
                        {
                            sceneDesc.m_featureProcessorNames.push_back(featureProcessorName);
                        }
                    }
                }

                data->m_scene = RPI::Scene::CreateScene(sceneDesc);

                // Bind m_defaultScene to the GameEntityContext's AzFramework::Scene
                auto* sceneSystem = AzFramework::SceneSystemInterface::Get();
                AZ_Assert(sceneSystem, "Thumbnail system failed to get scene system implementation.");
                Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> createSceneOutcome =
                    sceneSystem->CreateScene(data->m_sceneName);
                AZ_Assert(createSceneOutcome, createSceneOutcome.GetError().c_str()); // This should never happen unless scene creation has changed.
                data->m_frameworkScene = createSceneOutcome.TakeValue();
                data->m_frameworkScene->SetSubsystem(data->m_scene);

                data->m_frameworkScene->SetSubsystem(data->m_entityContext.get());
                // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene
                RPI::RenderPipelineDescriptor pipelineDesc;
                pipelineDesc.m_mainViewTagName = "MainCamera";
                pipelineDesc.m_name = data->m_pipelineName;
                pipelineDesc.m_rootPassTemplate = "ThumbnailPipelineRenderToTexture";
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
                AZ_Error("ThumbnailRenderer", defaultModelAssetId.IsValid(), "Default model asset is invalid. Verify the asset %s exists.", m_context->GetData()->DefaultModelPath);
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
                AZ_Error("ThumbnailRenderer", defaultMaterialAssetId.IsValid(), "Default material asset is invalid. Verify the asset %s exists.", m_context->GetData()->DefaultMaterialPath);
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
