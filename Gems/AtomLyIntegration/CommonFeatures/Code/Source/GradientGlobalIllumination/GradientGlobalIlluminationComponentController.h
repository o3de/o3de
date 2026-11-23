/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/GradientGlobalIllumination/GradientGlobalIlluminationFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentConfig.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

namespace UnitTest
{
    class GradientGlobalIlluminationComponentControllerFixture;
}

namespace AZ::Render
{
    class GradientGlobalIlluminationComponentController final
        : GradientGlobalIlluminationComponentRequestBus::Handler
        , Data::AssetBus::MultiHandler
        , AZ::TickBus::Handler
    {
    public:
        friend class EditorGradientGlobalIlluminationComponent;
        friend class UnitTest::GradientGlobalIlluminationComponentControllerFixture;

        AZ_CLASS_ALLOCATOR(GradientGlobalIlluminationComponentController, SystemAllocator);
        AZ_RTTI(AZ::Render::GradientGlobalIlluminationComponentController, "{D4E5F6A7-B8C9-0123-DEF0-123456789ABC}");

        static void Reflect(ReflectContext* context);
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);

        GradientGlobalIlluminationComponentController() = default;
        GradientGlobalIlluminationComponentController(const GradientGlobalIlluminationComponentConfig& config);

        void Activate(EntityId entityId);
        void Deactivate();
        void SetConfiguration(const GradientGlobalIlluminationComponentConfig& config);
        const GradientGlobalIlluminationComponentConfig& GetConfiguration() const;

    private:
        AZ_DISABLE_COPY(GradientGlobalIlluminationComponentController);

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
        void SetUpdateMode(GradientGlobalIlluminationUpdateMode mode) override;
        GradientGlobalIlluminationUpdateMode GetUpdateMode() const override;

        void SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) override;
        Data::Asset<RPI::StreamingImageAsset> GetDetailTexture() const override;
        void SetDetailTextureAssetId(const Data::AssetId& id) override;
        Data::AssetId GetDetailTextureAssetId() const override;
        void SetDetailTextureAssetPath(const AZStd::string& path) override;
        AZStd::string GetDetailTextureAssetPath() const override;
        void SetDetailMapping(GradientGlobalIlluminationTextureMapping mapping) override;
        GradientGlobalIlluminationTextureMapping GetDetailMapping() const override;
        void SetDetailBlend(GradientGlobalIlluminationBlendMode blend) override;
        GradientGlobalIlluminationBlendMode GetDetailBlend() const override;
        void SetDetailStrength(float strength) override;
        float GetDetailStrength() const override;

        void SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) override;
        Data::Asset<RPI::StreamingImageAsset> GetSpecularTexture() const override;
        void SetSpecularTextureAssetId(const Data::AssetId& id) override;
        Data::AssetId GetSpecularTextureAssetId() const override;
        void SetSpecularTextureAssetPath(const AZStd::string& path) override;
        AZStd::string GetSpecularTextureAssetPath() const override;
        void SetSpecularMapping(GradientGlobalIlluminationTextureMapping mapping) override;
        GradientGlobalIlluminationTextureMapping GetSpecularMapping() const override;
        void SetSpecularBlend(GradientGlobalIlluminationBlendMode blend) override;
        GradientGlobalIlluminationBlendMode GetSpecularBlend() const override;
        void SetSpecularStrength(float strength) override;
        float GetSpecularStrength() const override;

        void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
        void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
        void OnAssetError(Data::Asset<Data::AssetData> asset) override;

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void UpdateColors();
        bool IsActiveOwner() const;
        void ApplyConfiguration();
        void OnOwnerChanged(EntityId owner);

        //! Store the power-of-two CPU size, including when the requested GPU mode falls back.
        void SnapResolutionForCurrentMode();

        //! Subscribe and request asynchronous texture loading.
        void LoadDetailTexture();
        void LoadSpecularTexture();

        void PushDetailToFeatureProcessor();
        void PushSpecularToFeatureProcessor();

        void PushDetailParams();
        void PushSpecularParams();

    private:
        EntityId m_entityId;
        GradientGlobalIlluminationComponentConfig m_configuration;
        GradientGlobalIlluminationFeatureProcessorInterface* m_featureProcessor = nullptr;

        GradientGlobalIlluminationFeatureProcessorInterface::OwnerChangedEvent::Handler m_ownerChangedHandler
        {
            [this](EntityId owner)
            {
                OnOwnerChanged(owner);
            }
        };

        bool m_detailTextureNeedsUpdate = false;
        bool m_specularTextureNeedsUpdate = false;
    };
} // namespace AZ::Render
