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
    void SkyAtmosphereComponentController::Reflect(ReflectContext* context)
    {
        SkyAtmosphereComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SkyAtmosphereComponentController>()
                ->Version(1)
                ->Field("Configuration", &SkyAtmosphereComponentController::m_configuration);
        }
    }

    void SkyAtmosphereComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SkyAtmosphereService"));
    }

    void SkyAtmosphereComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void SkyAtmosphereComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService"));
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
        TransformNotificationBus::MultiHandler::BusDisconnect(m_entityId);
        TransformNotificationBus::MultiHandler::BusDisconnect(m_configuration.m_sun);
        AZ::EntityBus::Handler::BusDisconnect(m_configuration.m_sun);

        if (m_featureProcessorInterface)
        {
            m_featureProcessorInterface->ReleaseAtmosphere(m_atmosphereId);
            m_featureProcessorInterface = nullptr;
        }

        m_transformInterface = nullptr;
    }

    void SkyAtmosphereComponentController::SetConfiguration(const SkyAtmosphereComponentConfig& config)
    {
        m_configuration = config;
    }

    const SkyAtmosphereComponentConfig& SkyAtmosphereComponentController::GetConfiguration() const
    {
        return m_configuration;
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
