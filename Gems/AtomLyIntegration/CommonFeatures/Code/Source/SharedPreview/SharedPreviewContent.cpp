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
#include <SharedPreview/SharedPreviewContent.h>

namespace AZ
{
    namespace LyIntegration
    {
        SharedPreviewContent::SharedPreviewContent(
            RPI::ScenePtr scene,
            RPI::ViewPtr view,
            AZ::Uuid entityContextId,
            const Data::Asset<RPI::ModelAsset>& modelAsset,
            const Data::Asset<RPI::MaterialAsset>& materialAsset,
            const Data::Asset<RPI::AnyAsset>& lightingPresetAsset,
            const Render::MaterialPropertyOverrideMap& materialPropertyOverrides)
            : m_scene(scene)
            , m_view(view)
            , m_entityContextId(entityContextId)
            , m_modelAsset(modelAsset)
            , m_materialAsset(materialAsset)
            , m_lightingPresetAsset(lightingPresetAsset)
            , m_materialPropertyOverrides(materialPropertyOverrides)
            , m_meshHandle(nullptr)
            , m_meshDrawPacketUpdateCount(0)
        {
            // Create preview model
            AzFramework::EntityContextRequestBus::EventResult(
                m_modelEntity, m_entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "SharedPreviewContentModel");
            m_modelEntity->CreateComponent(Render::MeshComponentTypeId);
            m_modelEntity->CreateComponent(Render::MaterialComponentTypeId);
            m_modelEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
            m_modelEntity->Init();
            m_modelEntity->Activate();

            // Create the mesh update handler, this handler will become active once @m_meshHandle becomes valid.
            m_meshUpdatedHandler = AZ::Render::ModelDataInstanceInterface::MeshDrawPacketUpdatedEvent::Handler{
                [this](
                    const AZ::Render::ModelDataInstanceInterface& meshHandleIface,
                    uint32_t lodIndex,
                    uint32_t meshIndex,
                    const AZ::RPI::MeshDrawPacket& meshDrawPacket)
                {
                    OnMeshDrawPacketUpdated(meshHandleIface, lodIndex, meshIndex, meshDrawPacket);
                }
            };
        }

        SharedPreviewContent::~SharedPreviewContent()
        {
            if (m_modelEntity)
            {
                AZ::Render::MeshHandleStateNotificationBus::Handler::BusDisconnect();

                m_modelEntity->Deactivate();
                AzFramework::EntityContextRequestBus::Event(
                    m_entityContextId, &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_modelEntity);
                m_modelEntity = nullptr;
            }
        }

        void SharedPreviewContent::Load()
        {
            m_modelAsset.QueueLoad();
            m_materialAsset.QueueLoad();
            m_lightingPresetAsset.QueueLoad();
        }

        bool SharedPreviewContent::IsReady() const
        {
            return (!m_modelAsset.GetId().IsValid() || m_modelAsset.IsReady()) &&
                (!m_materialAsset.GetId().IsValid() || m_materialAsset.IsReady()) &&
                (!m_lightingPresetAsset.GetId().IsValid() || m_lightingPresetAsset.IsReady());
        }

        bool SharedPreviewContent::IsError() const
        {
            return m_modelAsset.IsError() || m_materialAsset.IsError() || m_lightingPresetAsset.IsError();
        }

        void SharedPreviewContent::ReportErrors()
        {
            AZ_Warning(
                "SharedPreviewContent", !m_modelAsset.GetId().IsValid() || m_modelAsset.IsReady(), "Asset failed to load in time: %s",
                m_modelAsset.ToString<AZStd::string>().c_str());
            AZ_Warning(
                "SharedPreviewContent", !m_materialAsset.GetId().IsValid() || m_materialAsset.IsReady(), "Asset failed to load in time: %s",
                m_materialAsset.ToString<AZStd::string>().c_str());
            AZ_Warning(
                "SharedPreviewContent", !m_lightingPresetAsset.GetId().IsValid() || m_lightingPresetAsset.IsReady(),
                "Asset failed to load in time: %s", m_lightingPresetAsset.ToString<AZStd::string>().c_str());
        }

        void SharedPreviewContent::Update()
        {
            UpdateModel();
            UpdateLighting();
            UpdateCamera();
        }

        void SharedPreviewContent::UpdateModel()
        {
            Render::MeshComponentRequestBus::Event(
                m_modelEntity->GetId(), &Render::MeshComponentRequestBus::Events::SetModelAsset, m_modelAsset);
            
            // This Ebus will give us the @m_meshHandle created by the mesh feature processor on behalf of
            // m_modelEntity->GetId(). With @m_meshHandle we can get accurate updates from the MeshFeatureProcessor
            // and know exactly when the MeshDrawPackets have been updated, which means we are ready to render the scene.
            AZ::Render::MeshHandleStateNotificationBus::Handler::BusConnect(m_modelEntity->GetId());

            Render::MaterialComponentRequestBus::Event(
                m_modelEntity->GetId(), &Render::MaterialComponentRequestBus::Events::SetMaterialAssetId,
                Render::DefaultMaterialAssignmentId, m_materialAsset.GetId());

            Render::MaterialComponentRequestBus::Event(
                m_modelEntity->GetId(), &Render::MaterialComponentRequestBus::Events::SetPropertyValues,
                Render::DefaultMaterialAssignmentId, m_materialPropertyOverrides);
        }

        void SharedPreviewContent::UpdateLighting()
        {
            if (m_lightingPresetAsset.IsReady())
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
                        cameraConfig, lightHandles, false);
                }
            }
        }

        void SharedPreviewContent::UpdateCamera()
        {
            // Get bounding sphere of the model asset and estimate how far the camera needs to be see all of it
            Vector3 center = {};
            float radius = {};
            if (m_modelAsset.IsReady())
            {
                m_modelAsset->GetAabb().GetAsSphere(center, radius);
            }

            const auto distance = fabsf(radius / sinf(FieldOfView)) + NearDist;
            const auto cameraRotation = Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), -CameraRotationAngle);
            const auto cameraPosition = center + cameraRotation.TransformVector(-Vector3::CreateAxisY() * distance);
            const auto cameraTransform = Transform::CreateLookAt(cameraPosition, center);
            m_view->SetCameraTransform(Matrix3x4::CreateFromTransform(cameraTransform));
        }

        ////////////////////////////////////////////////////////////////
        // AZ::Render::MeshHandleStateNotificationBus::Handler overrides
        void SharedPreviewContent::OnMeshHandleSet(const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* meshHandle)
        {
            m_meshHandle = meshHandle;
            (*m_meshHandle)->ConnectMeshDrawPacketUpdatedHandler(m_meshUpdatedHandler);
        }
        /////////////////////////////////////////////////////////////////

        // Function called by the callback defined in @m_meshUpdatedHandler
        void SharedPreviewContent::OnMeshDrawPacketUpdated(
            [[maybe_unused]] const AZ::Render::ModelDataInstanceInterface& meshHandleIface,
            [[maybe_unused]] uint32_t lodIndex,
            [[maybe_unused]] uint32_t meshIndex,
            [[maybe_unused]] const AZ::RPI::MeshDrawPacket& meshDrawPacket)
        {
            // Typically we only get two updates.
            // The first update is triggered by the creation of the Mesh with all its defaults.
            // The second update is triggered by setting m_materialAsset.
            m_meshDrawPacketUpdateCount++;
        }

        bool SharedPreviewContent::IsReadyToRender()
        {
            // We need at least two updates to have assurance that m_materialAsset
            // have applied to the mesh and it is fully loaded in GPU.
            return (m_meshDrawPacketUpdateCount > 1);
        }
    } // namespace LyIntegration
} // namespace AZ
