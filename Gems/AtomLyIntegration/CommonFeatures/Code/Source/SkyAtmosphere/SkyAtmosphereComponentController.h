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
#include <AzCore/Component/EntityBus.h>
#include <Atom/Feature/SkyAtmosphere/SkyAtmosphereFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/SkyAtmosphere/SkyAtmosphereBus.h>

namespace AZ::Render
{
    class SkyAtmosphereComponentController final
        : public TransformNotificationBus::MultiHandler
        , public SkyAtmosphereRequestBus::Handler
        , public EntityBus::Handler
    {
    public:
        friend class EditorSkyAtmosphereComponent;

        AZ_TYPE_INFO(AZ::Render::SkyAtmosphereComponentController, "{CB3DC903-ADAD-4127-9740-2D28AA890C2F}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        SkyAtmosphereComponentController() = default;
        SkyAtmosphereComponentController(const SkyAtmosphereComponentConfig& config);

        void Activate(EntityId entityId);
        void Deactivate();
        void SetConfiguration(const SkyAtmosphereComponentConfig& config);
        const SkyAtmosphereComponentConfig& GetConfiguration() const;

    protected:
        const SkyAtmosphereParams& GetUpdatedSkyAtmosphereParams();

    private:
        AZ_DISABLE_COPY(SkyAtmosphereComponentController);

        //! SkyAtmosphereRequestBus
        void SetEnabled(bool enabled) override;
        bool GetEnabled() override;

        void SetPlanetOriginMode(SkyAtmosphereComponentConfig::AtmosphereOrigin mode) override;
        SkyAtmosphereComponentConfig::AtmosphereOrigin GetPlanetOriginMode() override;

        void SetAtmosphereHeight(float atmosphereHeightKm) override;
        float GetAtmosphereHeight() override;

        void SetPlanetRadius(float planetRadiusKm) override;
        float GetPlanetRadius() override;

        void SetGroundAlbedo(const AZ::Vector3& groundAlbedo) override;
        AZ::Vector3 GetGroundAlbedo() override;

        //! Atmosphere 
        void SetLuminanceFactor(const AZ::Vector3& luminanceFactor) override;
        AZ::Vector3 GetLuminanceFactor() override;

        void SetMieAbsorption(const AZ::Vector3& mieAbsorption) override;
        AZ::Vector3 GetMieAbsorption() override;

        void SetMieExpDistribution(float mieExpDistribution) override;
        float GetMieExpDistribution() override;

        void SetMieScattering(const AZ::Vector3& mieScattering) override;
        AZ::Vector3 GetMieScattering() override;

        void SetRayleighExpDistribution(float rayleighExpDistribution) override;
        float GetRayleighExpDistribution() override;

        void SetRayleighScattering(const AZ::Vector3& rayleighScattering) override;
        AZ::Vector3 GetRayleighScattering() override;

        void SetMaxSamples(uint8_t maxSamples) override;
        uint8_t GetMaxSamples() override;

        void SetMinSamples(uint8_t minSamples) override;
        uint8_t GetMinSamples() override;

        //! Sun
        void SetSunEnabled(bool enabled) override;
        bool GetSunEnabled() override;

        void SetSunEntityId(AZ::EntityId entityId) override;
        AZ::EntityId GetSunEntityId() override;

        void SetSunColor(const AZ::Color sunColor) override;
        AZ::Color GetSunColor() override;

        void SetSunLuminanceFactor(float factor) override;
        float GetSunLuminanceFactor() override;

        void SetSunLimbColor(const AZ::Color sunLimbColor) override;
        AZ::Color GetSunLimbColor() override;

        void SetSunFalloffFactor(float factor) override;
        float GetSunFalloffFactor() override;

        void SetSunRadiusFactor(float factor) override;
        float GetSunRadiusFactor() override;

        void SetSunDirection(const AZ::Vector3& sunDirection) override;
        AZ::Vector3 GetSunDirection() override;

        //! Advanced
        void SetFastSkyEnabled(bool enabled) override;
        bool GetFastSkyEnabled() override;

        void SetFastAerialPerspectiveEnabled(bool enabled) override;
        bool GetFastAerialPerspectiveEnabled() override;

        void SetAerialPerspectiveEnabled(bool enabled) override;
        bool GetAerialPerspectiveEnabled() override;

        void SetNearClip(float nearClip) override;
        float GetNearClip() override;

        void SetNearFadeDistance(float nearFadeDistance) override;
        float GetNearFadeDistance() override;

        void SetAerialDepthFactor(float aerialDepthFactor) override;
        float GetAerialDepthFactor() override;

        void SetShadowsEnabled(bool enabled) override;
        bool GetShadowsEnabled() override;

        void OnParamUpdated();

        //! TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        //! EntityBus
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        void UpdateSkyAtmosphereParams(SkyAtmosphereParams& params);

        TransformInterface* m_transformInterface = nullptr;
        SkyAtmosphereFeatureProcessorInterface* m_featureProcessorInterface = nullptr;
        SkyAtmosphereFeatureProcessorInterface::AtmosphereId m_atmosphereId;
        SkyAtmosphereComponentConfig m_configuration;
        SkyAtmosphereParams m_atmosphereParams;
        EntityId m_entityId;
        EntityId m_sunEntityId;
    };
} // namespace AZ::Render
