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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/PointLightFeatureProcessorInterface.h>

#include <AtomLyIntegration/CommonFeatures/CoreLights/PointLightBus.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/PointLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class PointLightComponentController final
            : private TransformNotificationBus::Handler
            , public PointLightRequestBus::Handler
        {
        public:
            friend class EditorPointLightComponent;

            AZ_TYPE_INFO(AZ::Render::PointLightComponentController, "{23F82E30-2E1F-45FE-A9A7-B15632ED9EBD}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            PointLightComponentController() = default;
            PointLightComponentController(const PointLightComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const PointLightComponentConfig& config);
            const PointLightComponentConfig& GetConfiguration() const;

        private:

            AZ_DISABLE_COPY(PointLightComponentController);

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // PointLightRequestBus::Handler overrides ...
            const Color& GetColor() const override;
            void SetColor(const Color& color) override;
            float GetIntensity() const override;
            PhotometricUnit GetIntensityMode() const override;
            void SetIntensity(float intensity) override;
            void SetIntensity(float intensity, PhotometricUnit intensityMode) override;

            float GetAttenuationRadius() const override;
            void SetAttenuationRadius(float radius) override;
            bool GetAttenuationRadiusIsAutomatic() const override;
            void SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode) override;
            float GetBulbRadius() const override;
            void SetBulbRadius(float bulbRadius) override;
            void ConvertToIntensityMode(PhotometricUnit intensityMode) override;

            void ConfigurationChanged();
            void ColorIntensityChanged();
            void AttenuationRadiusChanged();
            void BulbRadiusChanged();

            void AutoCalculateAttenuationRadius();

            PointLightComponentConfig m_configuration;
            PhotometricValue m_photometricValue;
            PointLightFeatureProcessorInterface* m_featureProcessor = nullptr;
            PointLightFeatureProcessorInterface::LightHandle m_lightHandle;
            EntityId m_entityId;
        };

    } // namespace Render
} // AZ namespace
