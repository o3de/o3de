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
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <Thumbnails/Rendering/CommonPreviewContent.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CommonPreviewContent::CommonPreviewContent(
                RPI::ScenePtr scene,
                RPI::ViewPtr view,
                AZ::Uuid entityContextId,
                const Data::AssetId& modelAssetId,
                const Data::AssetId& materialAssetId,
                const Data::AssetId& lightingPresetAssetId)
                : m_scene(scene)
                , m_view(view)
                , m_entityContextId(entityContextId)
            {
                // Connect camera to pipeline's default view after camera entity activated
                Matrix4x4 viewToClipMatrix;
                MakePerspectiveFovMatrixRH(viewToClipMatrix, FieldOfView, AspectRatio, NearDist, FarDist, true);
                m_view->SetViewToClipMatrix(viewToClipMatrix);

                // Create preview model
                AzFramework::EntityContextRequestBus::EventResult(
                    m_modelEntity, m_entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "ThumbnailPreviewModel");
                m_modelEntity->CreateComponent(Render::MeshComponentTypeId);
                m_modelEntity->CreateComponent(Render::MaterialComponentTypeId);
                m_modelEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
                m_modelEntity->Init();
                m_modelEntity->Activate();

                m_defaultModelAsset.Create(DefaultModelAssetId, true);
                m_defaultMaterialAsset.Create(DefaultMaterialAssetId, true);
                m_defaultLightingPresetAsset.Create(DefaultLightingPresetAssetId, true);

                m_modelAsset.Create(modelAssetId.IsValid() ? modelAssetId : DefaultModelAssetId, false);
                m_materialAsset.Create(materialAssetId.IsValid() ? materialAssetId : DefaultMaterialAssetId, false);
                m_lightingPresetAsset.Create(lightingPresetAssetId.IsValid() ? lightingPresetAssetId : DefaultLightingPresetAssetId, false);
            }

            CommonPreviewContent::~CommonPreviewContent()
            {
                if (m_modelEntity)
                {
                    AzFramework::EntityContextRequestBus::Event(
                        m_entityContextId, &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_modelEntity);
                    m_modelEntity = nullptr;
                }
            }

            void CommonPreviewContent::Load()
            {
                m_modelAsset.QueueLoad();
                m_materialAsset.QueueLoad();
                m_lightingPresetAsset.QueueLoad();
            }

            bool CommonPreviewContent::IsReady() const
            {
                return m_modelAsset.IsReady() && m_materialAsset.IsReady() && m_lightingPresetAsset.IsReady();
            }

            bool CommonPreviewContent::IsError() const
            {
                return m_modelAsset.IsError() || m_materialAsset.IsError() || m_lightingPresetAsset.IsError();
            }

            void CommonPreviewContent::ReportErrors()
            {
                AZ_Warning(
                    "CommonPreviewContent", m_modelAsset.IsReady(), "Asset failed to load in time: %s",
                    m_modelAsset.ToString<AZStd::string>().c_str());
                AZ_Warning(
                    "CommonPreviewContent", m_materialAsset.IsReady(), "Asset failed to load in time: %s",
                    m_materialAsset.ToString<AZStd::string>().c_str());
                AZ_Warning(
                    "CommonPreviewContent", m_lightingPresetAsset.IsReady(), "Asset failed to load in time: %s",
                    m_lightingPresetAsset.ToString<AZStd::string>().c_str());
            }

            void CommonPreviewContent::UpdateScene()
            {
                UpdateModel();
                UpdateLighting();
                UpdateCamera();
            }

            void CommonPreviewContent::UpdateModel()
            {
                Render::MeshComponentRequestBus::Event(
                    m_modelEntity->GetId(), &Render::MeshComponentRequestBus::Events::SetModelAsset, m_modelAsset);

                Render::MaterialComponentRequestBus::Event(
                    m_modelEntity->GetId(), &Render::MaterialComponentRequestBus::Events::SetDefaultMaterialOverride,
                    m_materialAsset.GetId());
            }

            void CommonPreviewContent::UpdateLighting()
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

            void CommonPreviewContent::UpdateCamera()
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
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
