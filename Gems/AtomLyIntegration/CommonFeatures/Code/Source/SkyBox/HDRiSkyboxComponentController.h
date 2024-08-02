/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/SkyBox/HDRiSkyboxComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/SkyBox/HDRiSkyboxBus.h>

namespace AZ
{
    namespace Render
    {
        class HDRiSkyboxComponentController final
            : public TransformNotificationBus::Handler
            , public HDRiSkyboxRequestBus::Handler
            , Data::AssetBus::MultiHandler
        {
        public:
            friend class EditorHDRiSkyboxComponent;

            AZ_TYPE_INFO(AZ::Render::HDRiSkyboxComponentController, "{D01C123D-4EA1-4A9B-A7D9-47EF26A55CD0}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            HDRiSkyboxComponentController() = default;
            HDRiSkyboxComponentController(const HDRiSkyboxComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const HDRiSkyboxComponentConfig& config);
            const HDRiSkyboxComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(HDRiSkyboxComponentController);

            //! Data::AssetBus interface
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // HDRiSkyboxRequestBus::Handler overrides ...
            Data::Asset<RPI::StreamingImageAsset> GetCubemapAsset() const override;
            void SetCubemapAsset(const Data::Asset<RPI::StreamingImageAsset>& cubemapAsset) override;
            void SetCubemapAssetPath(const AZStd::string& path) override;
            void SetCubemapAssetId(AZ::Data::AssetId) override;
            AZ::Data::AssetId GetCubemapAssetId() const override;
            AZStd::string GetCubemapAssetPath() const override;
            void SetExposure(float exposure) override;
            float GetExposure() const override;

            //! Queues a load of an image Asset
            void LoadImage(Data::Asset<RPI::StreamingImageAsset>& imageAsset);

            //! Handles all Data::AssetBus calls in a unified way
            void UpdateWithAsset(Data::Asset<Data::AssetData> updatedAsset);

            //! Validates individual asset updates, returns true for valid ready assets.
            bool IsAssetValid(Data::Asset<RPI::StreamingImageAsset>& asset);

            //! Get the inverse matrix from entity transfom, without scale and translate
            AZ::Matrix4x4 GetInverseTransform(const AZ::Transform& world);

            TransformInterface* m_transformInterface = nullptr;
            SkyBoxFeatureProcessorInterface* m_featureProcessorInterface = nullptr;
            HDRiSkyboxComponentConfig m_configuration;
            EntityId m_entityId;
            bool m_isActive = false;
        };
    }
}
