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

#pragma once

#include <AtomLyIntegration/CommonFeatures/CoreLights/SpotLightBus.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/SpotLightComponentConfig.h>
#include <Atom/Feature/CoreLights/SpotLightFeatureProcessorInterface.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace Render
    {
        class SpotLightComponentController final
            : public TransformNotificationBus::Handler
            , public SpotLightRequestBus::Handler
        {
        public:
            friend class EditorSpotLightComponent;

            AZ_TYPE_INFO(AZ::Render::SpotLightComponentController, "{2B37DC8C-BE9E-481C-A53B-FCBFFAB425E0}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            SpotLightComponentController() = default;
            SpotLightComponentController(const SpotLightComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const SpotLightComponentConfig& config);
            const SpotLightComponentConfig& GetConfiguration() const;

        private:

            AZ_DISABLE_COPY(SpotLightComponentController);

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // SpotLightRequestBus::Handler overrides ...
            const Color& GetColor() const override;
            void SetColor(const Color& color) override;
            float GetIntensity() const override;
            void SetIntensity(float intensity) override;
            float GetBulbRadius() const override;
            void SetBulbRadius(float bulbRadius) override;
            float GetInnerConeAngleInDegrees() const override;
            void SetInnerConeAngleInDegrees(float degrees) override;
            float GetOuterConeAngleInDegrees() const override;
            void SetOuterConeAngleInDegrees(float degrees) override;
            float GetPenumbraBias() const override;
            void SetPenumbraBias(float penumbraBias) override;
            float GetAttenuationRadius() const override;
            void SetAttenuationRadius(float radius) override;
            LightAttenuationRadiusMode GetAttenuationRadiusMode() const override;
            void SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode) override;
            bool GetEnableShadow() const override;
            void SetEnableShadow(bool enabled) override;
            ShadowmapSize GetShadowmapSize() const override;
            void SetShadowmapSize(ShadowmapSize size) override;
            ShadowFilterMethod GetShadowFilterMethod() const override;
            void SetShadowFilterMethod(ShadowFilterMethod method) override;
            float GetSofteningBoundaryWidthAngle() const override;
            void SetSofteningBoundaryWidthAngle(float width) override;
            uint32_t GetPredictionSampleCount() const override;
            void SetPredictionSampleCount(uint32_t count) override;
            uint32_t GetFilteringSampleCount() const override;
            void SetFilteringSampleCount(uint32_t count) override;
            PcfMethod GetPcfMethod() const override;
            void SetPcfMethod(PcfMethod method) override;

            void ConfigurationChanged();
            void ColorIntensityChanged();
            void ConeAnglesChanged();
            void AttenuationRadiusChanged();
            void PenumbraBiasChanged();

            void AutoCalculateAttenuationRadius();

            SpotLightComponentConfig m_configuration;
            PhotometricValue m_photometricValue;
            SpotLightFeatureProcessorInterface* m_featureProcessor = nullptr;
            SpotLightFeatureProcessorInterface::LightHandle m_lightHandle;
            EntityId m_entityId;
        };

    } // namespace Render
} // AZ namespace
