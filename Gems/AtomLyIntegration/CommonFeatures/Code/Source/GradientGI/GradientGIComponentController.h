/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/Feature/GradientGI/GradientGIFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class GradientGIComponentController final
            : GradientGIComponentRequestBus::Handler
            , Data::AssetBus::MultiHandler
        {
        public:
            friend class EditorGradientGIComponent;

            AZ_CLASS_ALLOCATOR(GradientGIComponentController, SystemAllocator);
            AZ_RTTI(AZ::Render::GradientGIComponentController, "{D4E5F6A7-B8C9-0123-DEF0-123456789ABC}");

            static void Reflect(ReflectContext* context);
            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);

            GradientGIComponentController() = default;
            GradientGIComponentController(const GradientGIComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const GradientGIComponentConfig& config);
            const GradientGIComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(GradientGIComponentController);

            // =====================================================================
            // GradientGIComponentRequestBus
            // =====================================================================

            void SetLowColor(const Color& color) override;
            Color GetLowColor() const override;
            void SetMidColor(const Color& color) override;
            Color GetMidColor() const override;
            void SetHighColor(const Color& color) override;
            Color GetHighColor() const override;
            void SetGradientColors(const Color& low, const Color& mid, const Color& high) override;
            void SetExposure(float exposure) override;
            float GetExposure() const override;
            void SetFaceResolution(float resolution) override;
            float GetFaceResolution() const override;
            void SetUpdateMode(GradientGIUpdateMode mode) override;
            GradientGIUpdateMode GetUpdateMode() const override;

            void SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) override;
            Data::Asset<RPI::StreamingImageAsset> GetDetailTexture() const override;
            void SetDetailTextureAssetId(const Data::AssetId& id) override;
            Data::AssetId GetDetailTextureAssetId() const override;
            void SetDetailTextureAssetPath(const AZStd::string& path) override;
            AZStd::string GetDetailTextureAssetPath() const override;
            void SetDetailMapping(GradientGITextureMapping mapping) override;
            GradientGITextureMapping GetDetailMapping() const override;
            void SetDetailBlend(GradientGIBlendMode blend) override;
            GradientGIBlendMode GetDetailBlend() const override;
            void SetDetailStrength(float strength) override;
            float GetDetailStrength() const override;

            void SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) override;
            Data::Asset<RPI::StreamingImageAsset> GetSpecularTexture() const override;
            void SetSpecularTextureAssetId(const Data::AssetId& id) override;
            Data::AssetId GetSpecularTextureAssetId() const override;
            void SetSpecularTextureAssetPath(const AZStd::string& path) override;
            AZStd::string GetSpecularTextureAssetPath() const override;
            void SetSpecularMapping(GradientGITextureMapping mapping) override;
            GradientGITextureMapping GetSpecularMapping() const override;
            void SetSpecularBlend(GradientGIBlendMode blend) override;
            GradientGIBlendMode GetSpecularBlend() const override;
            void SetSpecularStrength(float strength) override;
            float GetSpecularStrength() const override;

            // =====================================================================
            // Data::AssetBus (detail/specular texture loading)
            // =====================================================================

            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;

            // =====================================================================
            // Helpers
            // =====================================================================

            void UpdateColors();

            //! Queue an async load of the detail/specular texture assets (connects AssetBus).
            void LoadDetailTexture();
            void LoadSpecularTexture();
            //! Push the resolved textures to the feature processor.
            void PushDetailToFeatureProcessor();
            void PushSpecularToFeatureProcessor();
            //! Push the (mapping, blend, strength) params for each layer to the feature processor.
            void PushDetailParams();
            void PushSpecularParams();

            EntityId                         m_entityId;
            GradientGIComponentConfig        m_configuration;
            GradientGIFeatureProcessorInterface* m_featureProcessor = nullptr;
        };

    } // namespace Render
} // namespace AZ
