/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <SkyAtmosphere/SkyAtmosphereComponentController.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/DirectionalLightBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render
{
    // The {{{#_Property}}} magic is giving a friendly name to the Set Event parameter
#define SKY_VIRTUAL_PROPERTY(_Property, _Param) \
    ->Event("Get" #_Property, &SkyAtmosphereRequestBus::Events::Get##_Property) \
    ->Event("Set" #_Property, &SkyAtmosphereRequestBus::Events::Set##_Property, {{{#_Param}}}) \
    ->VirtualProperty(#_Property, "Get" #_Property, "Set" #_Property)


    void SkyAtmosphereComponentController::Reflect(ReflectContext* context)
    {
        SkyAtmosphereComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SkyAtmosphereComponentController>()
                ->Version(1)
                ->Field("Configuration", &SkyAtmosphereComponentController::m_configuration);
        }

        if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->EBus<SkyAtmosphereRequestBus>("SkyAtmosphereRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Render")
                ->Attribute(AZ::Script::Attributes::Module, "Render")
                SKY_VIRTUAL_PROPERTY(Enabled, Enabled)
                SKY_VIRTUAL_PROPERTY(PlanetOriginMode, Mode)
                SKY_VIRTUAL_PROPERTY(AtmosphereHeight, Height)
                SKY_VIRTUAL_PROPERTY(PlanetRadius, RadiusKm)
                SKY_VIRTUAL_PROPERTY(GroundAlbedo, Albedo)
                SKY_VIRTUAL_PROPERTY(LuminanceFactor, Factor)
                SKY_VIRTUAL_PROPERTY(MieAbsorption, Absorption)
                SKY_VIRTUAL_PROPERTY(MieExpDistribution, Distribution)
                SKY_VIRTUAL_PROPERTY(MieScattering, Scattering)
                SKY_VIRTUAL_PROPERTY(RayleighExpDistribution, Distribution)
                SKY_VIRTUAL_PROPERTY(RayleighScattering, Scattering)
                SKY_VIRTUAL_PROPERTY(MaxSamples, Samples)
                SKY_VIRTUAL_PROPERTY(MinSamples, Samples)
                SKY_VIRTUAL_PROPERTY(SunEnabled, Enabled)
                SKY_VIRTUAL_PROPERTY(SunEntityId, EntityId)
                SKY_VIRTUAL_PROPERTY(SunColor, Color)
                SKY_VIRTUAL_PROPERTY(SunLuminanceFactor, Factor)
                SKY_VIRTUAL_PROPERTY(SunLimbColor, Color)
                SKY_VIRTUAL_PROPERTY(SunFalloffFactor, Factor)
                SKY_VIRTUAL_PROPERTY(SunRadiusFactor, Factor)
                SKY_VIRTUAL_PROPERTY(SunDirection, Direction)
                SKY_VIRTUAL_PROPERTY(FastSkyEnabled, Enabled)
                SKY_VIRTUAL_PROPERTY(FastAerialPerspectiveEnabled, Enabled)
                SKY_VIRTUAL_PROPERTY(AerialPerspectiveEnabled, Enabled)
                SKY_VIRTUAL_PROPERTY(NearClip, NearClip)
                SKY_VIRTUAL_PROPERTY(NearFadeDistance, Distance)
                SKY_VIRTUAL_PROPERTY(AerialDepthFactor, Factor)
                SKY_VIRTUAL_PROPERTY(ShadowsEnabled, Enabled)
                ;

        }
    }

    void SkyAtmosphereComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SkyAtmosphereService"));
    }

    void SkyAtmosphereComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void SkyAtmosphereComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    SkyAtmosphereComponentController::SkyAtmosphereComponentController(const SkyAtmosphereComponentConfig& config)
        : m_configuration(config)
    {
    }

    const SkyAtmosphereParams& SkyAtmosphereComponentController::GetUpdatedSkyAtmosphereParams()
    {
        UpdateSkyAtmosphereParams(m_atmosphereParams);
        return m_atmosphereParams;
    }

    void SkyAtmosphereComponentController::UpdateSkyAtmosphereParams(SkyAtmosphereParams& params)
    {
        // general params
        params.m_absorption = m_configuration.m_absorption * m_configuration.m_absorptionScale;
        params.m_atmosphereRadius = m_configuration.m_groundRadius + m_configuration.m_atmosphereHeight;
        params.m_fastSkyEnabled = m_configuration.m_fastSkyEnabled;
        params.m_fastAerialPerspectiveEnabled = m_configuration.m_fastAerialPerspectiveEnabled;
        params.m_aerialPerspectiveEnabled = m_configuration.m_aerialPerspectiveEnabled;
        params.m_aerialDepthFactor = m_configuration.m_aerialDepthFactor;
        params.m_groundAlbedo = m_configuration.m_groundAlbedo;
        params.m_luminanceFactor = m_configuration.m_luminanceFactor;
        params.m_mieAbsorption = m_configuration.m_mieAbsorption * m_configuration.m_mieAbsorptionScale;
        params.m_mieExpDistribution = m_configuration.m_mieExponentialDistribution;
        params.m_mieScattering = m_configuration.m_mieScattering * m_configuration.m_mieScatteringScale;
        params.m_minSamples = m_configuration.m_minSamples;
        params.m_maxSamples = m_configuration.m_maxSamples;
        params.m_planetRadius = m_configuration.m_groundRadius;
        params.m_rayleighScattering = m_configuration.m_rayleighScattering * m_configuration.m_rayleighScatteringScale;
        params.m_rayleighExpDistribution = m_configuration.m_rayleighExponentialDistribution;
        params.m_shadowsEnabled = m_configuration.m_shadowsEnabled;
        params.m_nearClip = m_configuration.m_nearClip;
        params.m_nearFadeDistance = m_configuration.m_nearFadeDistance;

        // sun params
        params.m_sunEnabled = m_configuration.m_drawSun;
        params.m_sunColor = m_configuration.m_sunColor * m_configuration.m_sunLuminanceFactor;
        params.m_sunLimbColor = m_configuration.m_sunLimbColor * m_configuration.m_sunLuminanceFactor;
        params.m_sunFalloffFactor = m_configuration.m_sunFalloffFactor;
        params.m_sunRadiusFactor = m_configuration.m_sunRadiusFactor;

        if (m_configuration.m_sun != m_sunEntityId)
        {
            if(m_sunEntityId.IsValid())
            {
                AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_sunEntityId);
                m_sunEntityId.SetInvalid();
            }

            if (m_configuration.m_sun.IsValid() && m_configuration.m_sun != m_entityId)
            {
                m_sunEntityId = m_configuration.m_sun;

                AZ::TransformNotificationBus::MultiHandler::BusConnect(m_configuration.m_sun);
                if (auto handler = TransformBus::FindFirstHandler(m_configuration.m_sun); handler == nullptr)
                {
                    // connect to the entity bus so we can be notified when this entity activates so we can get
                    // notified when the transform changes
                    AZ::EntityBus::Handler::BusConnect(m_configuration.m_sun);
                }
            }
        }

        // sun direction using own transform or sun entity
        const AZ::Transform& transform = m_transformInterface ? m_transformInterface->GetWorldTM() : Transform::Identity();
        auto sunTransformInterface = TransformBus::FindFirstHandler(m_configuration.m_sun);
        AZ::Transform sunTransform = sunTransformInterface ? sunTransformInterface->GetWorldTM() : transform;
        params.m_sunDirection = -sunTransform.GetBasisY();

        if (auto directionalLightInterface = DirectionalLightRequestBus::FindFirstHandler(m_configuration.m_sun);
            directionalLightInterface != nullptr)
        {
            params.m_sunShadowsFarClip = directionalLightInterface->GetShadowFarClipDistance();
        }
        else
        {
            // use the first directional light in the scene
            DirectionalLightRequestBus::BroadcastResult(params.m_sunShadowsFarClip, &DirectionalLightRequests::GetShadowFarClipDistance);
        }

        if (m_transformInterface)
        {
            switch (m_configuration.m_originMode)
            {
            case SkyAtmosphereComponentConfig::AtmosphereOrigin::PlanetCenterAtLocalOrigin:
                params.m_planetOrigin = m_transformInterface->GetWorldTranslation() * 0.001f;
                break;
            case SkyAtmosphereComponentConfig::AtmosphereOrigin::GroundAtLocalOrigin:
                params.m_planetOrigin = m_transformInterface->GetWorldTranslation() * 0.001f - AZ::Vector3(0.0, 0.0, m_configuration.m_groundRadius);
                break;
            default:
            case SkyAtmosphereComponentConfig::AtmosphereOrigin::GroundAtWorldOrigin:
                params.m_planetOrigin = -AZ::Vector3(0.0, 0.0, m_configuration.m_groundRadius);
                break;
            }
        }
        else
        {
            params.m_planetOrigin = Vector3::CreateZero();
        }
    }

    void SkyAtmosphereComponentController::Activate(EntityId entityId)
    {
        SkyAtmosphereRequestBus::Handler::BusConnect(entityId);
        m_featureProcessorInterface = RPI::Scene::GetFeatureProcessorForEntity<SkyAtmosphereFeatureProcessorInterface>(entityId);

        if (m_featureProcessorInterface)
        {
            m_entityId = entityId;

            m_transformInterface = TransformBus::FindFirstHandler(m_entityId);
            m_atmosphereId = m_featureProcessorInterface->CreateAtmosphere();
            m_featureProcessorInterface->SetAtmosphereParams(m_atmosphereId, GetUpdatedSkyAtmosphereParams());

            AZ::TransformNotificationBus::MultiHandler::BusConnect(m_entityId);
        }
    }

    void SkyAtmosphereComponentController::Deactivate()
    {
        SkyAtmosphereRequestBus::Handler::BusDisconnect();
        TransformNotificationBus::MultiHandler::BusDisconnect(m_entityId);
        TransformNotificationBus::MultiHandler::BusDisconnect(m_configuration.m_sun);
        AZ::EntityBus::Handler::BusDisconnect(m_configuration.m_sun);

        if (m_featureProcessorInterface)
        {
            m_featureProcessorInterface->ReleaseAtmosphere(m_atmosphereId);
            m_featureProcessorInterface = nullptr;
            m_atmosphereId.Reset();
        }

        m_transformInterface = nullptr;

        // set the sun entity as invalid so we re-connect to the transform interface if we get re-activated 
        m_sunEntityId.SetInvalid();
    }

    void SkyAtmosphereComponentController::SetConfiguration(const SkyAtmosphereComponentConfig& config)
    {
        m_configuration = config;
    }

    const SkyAtmosphereComponentConfig& SkyAtmosphereComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void SkyAtmosphereComponentController::OnParamUpdated()
    {
        if (m_featureProcessorInterface && m_atmosphereId.IsValid())
        {
            m_featureProcessorInterface->SetAtmosphereParams(m_atmosphereId, GetUpdatedSkyAtmosphereParams());
        }
    }

    void SkyAtmosphereComponentController::SetEnabled(bool enabled)
    {
        if (m_featureProcessorInterface && m_atmosphereId.IsValid())
        {
            m_featureProcessorInterface->SetAtmosphereEnabled(m_atmosphereId, enabled);
        }
    }

    bool SkyAtmosphereComponentController::GetEnabled()
    {
        if (m_featureProcessorInterface && m_atmosphereId.IsValid())
        {
            return m_featureProcessorInterface->GetAtmosphereEnabled(m_atmosphereId);
        }

        return false; 
    }


    void SkyAtmosphereComponentController::SetPlanetOriginMode(SkyAtmosphereComponentConfig::AtmosphereOrigin mode)
    {
        m_configuration.m_originMode = mode;
        OnParamUpdated();
    }

    SkyAtmosphereComponentConfig::AtmosphereOrigin SkyAtmosphereComponentController::GetPlanetOriginMode()
    {
        return m_configuration.m_originMode;
    }


    void SkyAtmosphereComponentController::SetAtmosphereHeight(float atmosphereHeightKm)
    {
        m_configuration.m_atmosphereHeight = atmosphereHeightKm;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetAtmosphereHeight()
    {
        return m_configuration.m_atmosphereHeight;
    }


    void SkyAtmosphereComponentController::SetPlanetRadius(float planetRadiusKm)
    {
        m_configuration.m_groundRadius = planetRadiusKm;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetPlanetRadius()
    {
        return m_configuration.m_groundRadius;
    }


    void SkyAtmosphereComponentController::SetGroundAlbedo(const AZ::Vector3& groundAlbedo)
    {
        m_configuration.m_groundAlbedo = groundAlbedo;
        OnParamUpdated();
    }

    AZ::Vector3 SkyAtmosphereComponentController::GetGroundAlbedo()
    {
        return m_configuration.m_groundAlbedo;
    }


    void SkyAtmosphereComponentController::SetLuminanceFactor(const AZ::Vector3& luminanceFactor)
    {
        m_configuration.m_luminanceFactor = luminanceFactor;
        OnParamUpdated();
    }

    AZ::Vector3 SkyAtmosphereComponentController::GetLuminanceFactor()
    {
        return m_configuration.m_luminanceFactor;
    }


    void SkyAtmosphereComponentController::SetMieAbsorption(const AZ::Vector3& mieAbsorption)
    {
        if (m_configuration.m_mieAbsorptionScale > 0.f)
        {
            m_configuration.m_mieAbsorption = mieAbsorption / m_configuration.m_mieAbsorptionScale;
            OnParamUpdated();
        }
    }

    AZ::Vector3 SkyAtmosphereComponentController::GetMieAbsorption()
    {
        return m_configuration.m_mieAbsorption * m_configuration.m_mieAbsorptionScale;
    }


    void SkyAtmosphereComponentController::SetMieExpDistribution(float mieExpDistribution)
    {
        m_configuration.m_mieExponentialDistribution = mieExpDistribution;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetMieExpDistribution()
    {
        return m_configuration.m_mieExponentialDistribution;
    }


    void SkyAtmosphereComponentController::SetMieScattering(const AZ::Vector3& mieScattering)
    {
        if (m_configuration.m_mieScatteringScale > 0.f)
        {
            m_configuration.m_mieScattering = mieScattering / m_configuration.m_mieScatteringScale;
            OnParamUpdated();
        }
    }

    AZ::Vector3 SkyAtmosphereComponentController::GetMieScattering()
    {
        return m_configuration.m_mieScattering * m_configuration.m_mieScatteringScale;
    }


    void SkyAtmosphereComponentController::SetRayleighExpDistribution(float rayleighExpDistribution)
    {
        m_configuration.m_rayleighExponentialDistribution = rayleighExpDistribution;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetRayleighExpDistribution()
    {
        return m_configuration.m_rayleighExponentialDistribution;
    }


    void SkyAtmosphereComponentController::SetRayleighScattering(const AZ::Vector3& rayleighScattering)
    {
        if (m_configuration.m_rayleighScatteringScale > 0.f)
        {
            m_configuration.m_rayleighScattering = rayleighScattering / m_configuration.m_rayleighScatteringScale;
            OnParamUpdated();
        }
    }

    AZ::Vector3 SkyAtmosphereComponentController::GetRayleighScattering()
    {
        return m_configuration.m_rayleighScattering * m_configuration.m_rayleighScatteringScale;
    }


    void SkyAtmosphereComponentController::SetMaxSamples(uint8_t maxSamples)
    {
        m_configuration.m_maxSamples = maxSamples;
        OnParamUpdated();
    }

    uint8_t SkyAtmosphereComponentController::GetMaxSamples()
    {
        return m_configuration.m_maxSamples;
    }


    void SkyAtmosphereComponentController::SetMinSamples(uint8_t minSamples)
    {
        m_configuration.m_minSamples = minSamples;
        OnParamUpdated();
    }

    uint8_t SkyAtmosphereComponentController::GetMinSamples()
    {
        return m_configuration.m_minSamples;
    }


    void SkyAtmosphereComponentController::SetSunEnabled(bool enabled)
    {
        m_configuration.m_drawSun = enabled;
        OnParamUpdated();
    }

    bool SkyAtmosphereComponentController::GetSunEnabled()
    {
        return m_configuration.m_drawSun;
    }


    void SkyAtmosphereComponentController::SetSunEntityId(AZ::EntityId entityId)
    {
        m_configuration.m_sun = entityId;
        OnParamUpdated();
    }

    AZ::EntityId SkyAtmosphereComponentController::GetSunEntityId()
    {
        return m_configuration.m_sun;
    }

    void SkyAtmosphereComponentController::SetSunColor(const AZ::Color sunColor)
    {
        m_configuration.m_sunColor = sunColor;
        OnParamUpdated();
    }

    AZ::Color SkyAtmosphereComponentController::GetSunColor()
    {
        return m_configuration.m_sunColor;
    }

    void SkyAtmosphereComponentController::SetSunLuminanceFactor(float factor)
    {
        m_configuration.m_sunLuminanceFactor = factor;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetSunLuminanceFactor()
    {
        return m_configuration.m_sunLuminanceFactor;
    }

    void SkyAtmosphereComponentController::SetSunLimbColor(const AZ::Color sunLimbColor)
    {
        m_configuration.m_sunLimbColor = sunLimbColor;
        OnParamUpdated();
    }

    AZ::Color SkyAtmosphereComponentController::GetSunLimbColor()
    {
        return m_configuration.m_sunLimbColor;
    }


    void SkyAtmosphereComponentController::SetSunFalloffFactor(float factor)
    {
        m_configuration.m_sunFalloffFactor = factor;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetSunFalloffFactor()
    {
        return m_configuration.m_sunFalloffFactor;
    }


    void SkyAtmosphereComponentController::SetSunRadiusFactor(float factor)
    {
        m_configuration.m_sunRadiusFactor = factor;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetSunRadiusFactor()
    {
        return m_configuration.m_sunRadiusFactor;
    }


    void SkyAtmosphereComponentController::SetSunDirection(const AZ::Vector3& sunDirection)
    {
        if (m_configuration.m_sun.IsValid())
        {
            AZ_WarningOnce("SkyAtmosphereComponentController", false, "Cannot set the sun direction when a sun entity exists, rotate the sun entity instead.");
        }
        else if (m_transformInterface)
        {
            const Vector3 up = Vector3::CreateAxisZ();
            const Vector3 right = sunDirection.Cross(up);
            const AZ::Quaternion lookAt = AZ::Quaternion::CreateFromBasis(right, sunDirection, up);
            m_transformInterface->SetWorldRotationQuaternion(lookAt);
            OnParamUpdated();
        }
    }

    AZ::Vector3 SkyAtmosphereComponentController::GetSunDirection()
    {
        if (m_configuration.m_sun.IsValid())
        {
            AZ::Transform worldTm;
            AZ::TransformBus::EventResult(worldTm, m_configuration.m_sun, &AZ::TransformBus::Events::GetWorldTM);
            return worldTm.GetBasisY();
        }
        else if (m_transformInterface)
        {
            return m_transformInterface->GetWorldTM().GetBasisY();
        }
        return AZ::Vector3::CreateAxisY();
    }


    void SkyAtmosphereComponentController::SetFastSkyEnabled(bool enabled)
    {
        m_configuration.m_fastSkyEnabled = enabled;
        OnParamUpdated();
    }

    bool SkyAtmosphereComponentController::GetFastSkyEnabled()
    {
        return m_configuration.m_fastSkyEnabled;
    }


    void SkyAtmosphereComponentController::SetFastAerialPerspectiveEnabled(bool enabled)
    {
        m_configuration.m_fastAerialPerspectiveEnabled = enabled;
        OnParamUpdated();
    }

    bool SkyAtmosphereComponentController::GetFastAerialPerspectiveEnabled()
    {
        return m_configuration.m_fastAerialPerspectiveEnabled;
    }


    void SkyAtmosphereComponentController::SetAerialPerspectiveEnabled(bool enabled)
    {
        m_configuration.m_aerialPerspectiveEnabled = enabled;
        OnParamUpdated();
    }

    bool SkyAtmosphereComponentController::GetAerialPerspectiveEnabled()
    {
        return m_configuration.m_aerialPerspectiveEnabled;
    }


    void SkyAtmosphereComponentController::SetNearClip(float nearClip)
    {
        m_configuration.m_nearClip = nearClip;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetNearClip()
    {
        return m_configuration.m_nearClip;
    }


    void SkyAtmosphereComponentController::SetNearFadeDistance(float nearFadeDistance)
    {
        m_configuration.m_nearFadeDistance = nearFadeDistance;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetNearFadeDistance()
    {
        return m_configuration.m_nearFadeDistance;
    }


    void SkyAtmosphereComponentController::SetAerialDepthFactor(float aerialDepthFactor)
    {
        m_configuration.m_aerialDepthFactor = aerialDepthFactor;
        OnParamUpdated();
    }

    float SkyAtmosphereComponentController::GetAerialDepthFactor()
    {
        return m_configuration.m_aerialDepthFactor;
    }


    void SkyAtmosphereComponentController::SetShadowsEnabled(bool enabled)
    {
        m_configuration.m_shadowsEnabled = enabled;
        OnParamUpdated();
    }

    bool SkyAtmosphereComponentController::GetShadowsEnabled()
    {
        return m_configuration.m_shadowsEnabled;
    }


    void SkyAtmosphereComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local,[[maybe_unused]] const AZ::Transform& world)
    {
        m_featureProcessorInterface->SetAtmosphereParams(m_atmosphereId, GetUpdatedSkyAtmosphereParams());
    }

    void SkyAtmosphereComponentController::OnEntityActivated(const AZ::EntityId& entityId)
    {
        m_featureProcessorInterface->SetAtmosphereParams(m_atmosphereId, GetUpdatedSkyAtmosphereParams());

        AZ::EntityBus::Handler::BusDisconnect(entityId);
    }

} // namespace AZ::Render
