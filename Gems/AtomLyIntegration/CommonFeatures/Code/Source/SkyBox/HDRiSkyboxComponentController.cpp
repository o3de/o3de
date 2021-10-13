/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <SkyBox/HDRiSkyboxComponentController.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void HDRiSkyboxComponentController::Reflect(ReflectContext* context)
        {
            HDRiSkyboxComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<HDRiSkyboxComponentController>()
                    ->Version(1)
                    ->Field("Configuration", &HDRiSkyboxComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<HDRiSkyboxRequestBus>("HDRiSkyboxRequestBus")
                    ->Event("SetExposure", &HDRiSkyboxRequestBus::Events::SetExposure)
                    ->Event("GetExposure", &HDRiSkyboxRequestBus::Events::GetExposure)
                    ->VirtualProperty("Exposure", "GetExposure", "SetExposure")
                    ;
            }
        }

        void HDRiSkyboxComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SkyBoxService", 0x8169a709));
        }

        void HDRiSkyboxComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SkyBoxService", 0x8169a709));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        void HDRiSkyboxComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService"));
        }

        HDRiSkyboxComponentController::HDRiSkyboxComponentController(const HDRiSkyboxComponentConfig& config)
            : m_configuration(config)
        {
        }

        void HDRiSkyboxComponentController::Activate(EntityId entityId)
        {
            m_featureProcessorInterface = RPI::Scene::GetFeatureProcessorForEntity<SkyBoxFeatureProcessorInterface>(entityId);

            // only activate if there is no other skybox activate
            if (m_featureProcessorInterface && !m_featureProcessorInterface->IsEnabled())
            {
                m_featureProcessorInterface->SetSkyboxMode(SkyBoxMode::Cubemap);
                m_featureProcessorInterface->Enable(true);

                m_entityId = entityId;

                SetCubemapAsset(m_configuration.m_cubemapAsset);
                SetExposure(m_configuration.m_exposure);

                m_transformInterface = TransformBus::FindFirstHandler(m_entityId);
                AZ_Assert(m_transformInterface, "Unable to attach to a TransformBus handler. Entity transform will not affect to skybox.");

                const AZ::Transform& transform = m_transformInterface ? m_transformInterface->GetWorldTM() : Transform::Identity();
                m_featureProcessorInterface->SetCubemapRotationMatrix(GetInverseTransform(transform));

                HDRiSkyboxRequestBus::Handler::BusConnect(m_entityId);
                TransformNotificationBus::Handler::BusConnect(m_entityId);

                m_isActive = true;
            }
            else
            {
                m_featureProcessorInterface = nullptr;
                AZ_Warning("HDRiSkyboxComponentController", false, "There is already another HDRi Skybox or Physical Sky component in the scene!");
            }
        }

        void HDRiSkyboxComponentController::Deactivate()
        {
            // Run deactivate if this skybox is activate
            if (m_isActive)
            {
                HDRiSkyboxRequestBus::Handler::BusDisconnect(m_entityId);
                TransformNotificationBus::Handler::BusDisconnect(m_entityId);

                Data::AssetBus::MultiHandler::BusDisconnect();
                m_configuration.m_cubemapAsset.Release();

                m_featureProcessorInterface->Enable(false);
                m_featureProcessorInterface = nullptr;

                m_transformInterface = nullptr;

                m_isActive = false;
            }
        }

        void HDRiSkyboxComponentController::SetConfiguration(const HDRiSkyboxComponentConfig& config)
        {
            m_configuration = config;
        }

        const HDRiSkyboxComponentConfig& HDRiSkyboxComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void HDRiSkyboxComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            UpdateWithAsset(asset);
        }

        void HDRiSkyboxComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            UpdateWithAsset(asset);
        }

        void HDRiSkyboxComponentController::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            UpdateWithAsset(asset);
        }

        void HDRiSkyboxComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            m_featureProcessorInterface->SetCubemapRotationMatrix(GetInverseTransform(world));
        }


        Data::Asset<RPI::StreamingImageAsset> HDRiSkyboxComponentController::GetCubemapAsset() const
        {
            return m_configuration.m_cubemapAsset;
        }

        void HDRiSkyboxComponentController::SetCubemapAsset(const Data::Asset<RPI::StreamingImageAsset>& cubemapAsset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_cubemapAsset.GetId());
            m_configuration.m_cubemapAsset = cubemapAsset;
            LoadImage(m_configuration.m_cubemapAsset);
        }

        void HDRiSkyboxComponentController::SetExposure(float exposure)
        {
            m_configuration.m_exposure = exposure;
            m_featureProcessorInterface->SetCubemapExposure(exposure);
        }

        float HDRiSkyboxComponentController::GetExposure() const
        {
            return m_configuration.m_exposure;
        }

        void HDRiSkyboxComponentController::LoadImage(Data::Asset<RPI::StreamingImageAsset>& imageAsset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect(imageAsset.GetId());

            if (imageAsset.GetId().IsValid())
            {
                // If the asset is already loaded it'll call OnAssetReady() immediately on BusConnect().
                Data::AssetBus::MultiHandler::BusConnect(imageAsset.GetId());
                imageAsset.QueueLoad();
            }
            else
            {
                // Call update for invalid assets so current assets can be cleared if necessary.
                UpdateWithAsset(imageAsset);
            }
        }

        void HDRiSkyboxComponentController::UpdateWithAsset(Data::Asset<Data::AssetData> updatedAsset)
        {
            if (m_configuration.m_cubemapAsset.GetId() == updatedAsset.GetId())
            {
                m_configuration.m_cubemapAsset = updatedAsset;

                if (IsAssetValid(m_configuration.m_cubemapAsset))
                {
                    m_featureProcessorInterface->SetCubemap(RPI::StreamingImage::FindOrCreate(m_configuration.m_cubemapAsset));
                }
                else
                {
                    // If this asset didn't load or isn't a cubemap, release it.
                    m_configuration.m_cubemapAsset.Release();
                    m_featureProcessorInterface->SetCubemap(nullptr);
                }
            }
        }

        bool HDRiSkyboxComponentController::IsAssetValid(Data::Asset<RPI::StreamingImageAsset>& asset)
        {
            if (asset.GetId().IsValid() && asset->IsReady())
            {
                auto& descriptor = asset->GetImageDescriptor();
                bool isCubemap = descriptor.m_isCubemap || descriptor.m_arraySize == 6;

                if (isCubemap)
                {
                    return true;
                }
            }
            return false;
        }

        AZ::Matrix4x4 HDRiSkyboxComponentController::GetInverseTransform(const AZ::Transform& world)
        {
            float matrix[16];

            // remove scale
            Transform worldNoScale = world;
            worldNoScale.ExtractUniformScale();
            
            AZ::Matrix3x4 transformMatrix = AZ::Matrix3x4::CreateFromTransform(worldNoScale);
            transformMatrix.StoreToRowMajorFloat12(matrix);

            // remove translation
            matrix[3] = 0;
            matrix[7] = 0;
            matrix[11] = 0;

            matrix[12] = 0;
            matrix[13] = 0;
            matrix[14] = 0;
            matrix[15] = 1;

            return AZ::Matrix4x4::CreateFromRowMajorFloat16(matrix).GetInverseFast();
        }

    } // namespace Render
} // namespace AZ
