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
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        //! Controls behavior of an image based light that affects a scene
        class ImageBasedLightComponentController final
            : ImageBasedLightComponentRequestBus::Handler
            , Data::AssetBus::MultiHandler
            , TransformNotificationBus::Handler
        {
        public:
            friend class EditorImageBasedLightComponent;

            AZ_CLASS_ALLOCATOR(ImageBasedLightComponentController, SystemAllocator);
            AZ_RTTI(AZ::Render::ImageBasedLightComponentController, "{73DBD008-4E77-471C-B7DE-F2217A256FE2}");

            static void Reflect(ReflectContext* context);
            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);

            ImageBasedLightComponentController() = default;
            ImageBasedLightComponentController(const ImageBasedLightComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ImageBasedLightComponentConfig& config);
            const ImageBasedLightComponentConfig& GetConfiguration() const;

        private:

            AZ_DISABLE_COPY(ImageBasedLightComponentController);

            //! Data::AssetBus interface
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;

            //! ImageBasedLightComponentRequestBus overrides...
            void SetSpecularImageAsset(const Data::Asset<RPI::StreamingImageAsset>& imageAsset) override;
            void SetDiffuseImageAsset(const Data::Asset<RPI::StreamingImageAsset>& imageAsset) override;
            Data::Asset<RPI::StreamingImageAsset> GetSpecularImageAsset() const override;
            Data::Asset<RPI::StreamingImageAsset> GetDiffuseImageAsset() const override;
            void SetSpecularImageAssetId(const Data::AssetId imageAssetId) override;
            void SetDiffuseImageAssetId(const Data::AssetId imageAssetId) override;
            Data::AssetId GetSpecularImageAssetId() const override;
            Data::AssetId GetDiffuseImageAssetId() const override;
            void SetSpecularImageAssetPath(const AZStd::string path) override;
            void SetDiffuseImageAssetPath(const AZStd::string path) override;
            AZStd::string GetSpecularImageAssetPath() const override;
            AZStd::string GetDiffuseImageAssetPath() const override;
            void SetExposure(float exposure) override;
            float GetExposure() const override;

            // TransformNotificationBus::Handler overrides...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            //! Queues a load of an image Asset
            void LoadImage(Data::Asset<RPI::StreamingImageAsset>& imageAsset);
            
            //! Releases all image assest.
            void ReleaseImages();
            
            //! Handles all Data::AssetBus calls in a unified way
            void UpdateWithAsset(Data::Asset<Data::AssetData> updatedAsset);
            
            //! Validates individual asset updates, returns true for valid ready assets.
            bool HandleAssetUpdate(Data::Asset<Data::AssetData> updatedAsset, Data::Asset<RPI::StreamingImageAsset>& configAsset);

            EntityId m_entityId;
            ImageBasedLightComponentConfig m_configuration;
            ImageBasedLightFeatureProcessorInterface* m_featureProcessor = nullptr;
        };
    } // namespace Render
} // namespace AZ
