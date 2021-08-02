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
#include <AtomLyIntegration/CommonFeatures/SkyBox/PhysicalSkyComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/SkyBox/PhysicalSkyBus.h>
#include <Atom/Feature/SkyBox/SkyBoxFogBus.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class PhysicalSkyComponentController final
            : public TransformNotificationBus::Handler
            , public PhysicalSkyRequestBus::Handler
            , public SkyBoxFogRequestBus::Handler
        {
        public:
            friend class EditorPhysicalSkyComponent;

            AZ_TYPE_INFO(AZ::Render::PhysicalSkyComponentController, "{C3EEB94D-AEB9-4727-9493-791F86924804}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            PhysicalSkyComponentController() = default;
            PhysicalSkyComponentController(const PhysicalSkyComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const PhysicalSkyComponentConfig& config);
            const PhysicalSkyComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(PhysicalSkyComponentController);

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // PhysicalSkyRequestBus::Handler overrides ...
            void SetTurbidity(int turbidity) override;
            int GetTurbidity() override;
            void SetSunRadiusFactor(float factor) override;
            float GetSunRadiusFactor() override;
            void SetSkyIntensity(float intensity, PhotometricUnit unit) override;
            void SetSkyIntensity(float intensity) override;
            float GetSkyIntensity(PhotometricUnit unit) override;
            float GetSkyIntensity() override;
            void SetSunIntensity(float intensity, PhotometricUnit unit) override;
            void SetSunIntensity(float intensity) override;
            float GetSunIntensity(PhotometricUnit unit) override;
            float GetSunIntensity() override;

            // SkyBoxFogRequestBus::Handler overrides ...
            void SetEnabled(bool enable) override;
            bool IsEnabled() const override;
            void SetColor(const AZ::Color& color) override;
            const AZ::Color& GetColor() const override;
            void SetTopHeight(float topHeight) override;
            float GetTopHeight() const override;
            void SetBottomHeight(float bottomHeight) override;
            float GetBottomHeight() const override;

            //! Get Sun azimuth and altitude from entity transform, without scale
            SunPosition GetSunTransform(const AZ::Transform& world);

            TransformInterface* m_transformInterface = nullptr;
            SkyBoxFeatureProcessorInterface* m_featureProcessorInterface = nullptr;
            PhysicalSkyComponentConfig m_configuration;
            EntityId m_entityId;
            bool m_isActive = false;

            // For light unit update on UI
            PhotometricValue m_skyPhotometricValue;
            PhotometricValue m_sunPhotometricValue;
        };
    }
}
