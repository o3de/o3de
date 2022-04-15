/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <SkyAtmosphere/SkyAtmosphereComponentController.h>

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

    void SkyAtmosphereComponentController::Activate(EntityId entityId)
    {
        m_featureProcessorInterface = RPI::Scene::GetFeatureProcessorForEntity<SkyAtmosphereFeatureProcessorInterface>(entityId);

        if (m_featureProcessorInterface)
        {
            m_entityId = entityId;

            m_transformInterface = TransformBus::FindFirstHandler(m_entityId);
            const AZ::Transform& transform = m_transformInterface ? m_transformInterface->GetWorldTM() : Transform::Identity();

            auto sunTransformInterface = TransformBus::FindFirstHandler(m_configuration.m_sun);
            AZ::Transform sunTransform = sunTransformInterface ? sunTransformInterface->GetWorldTM() : transform;

            m_atmosphereId = m_featureProcessorInterface->CreateAtmosphere();

            m_featureProcessorInterface->SetAbsorptionExtinction(m_atmosphereId, m_configuration.m_absorptionExtinction);
            m_featureProcessorInterface->SetAtmosphereRadius(m_atmosphereId, m_configuration.m_atmosphereRadius);
            m_featureProcessorInterface->SetGroundAlbedo(m_atmosphereId, m_configuration.m_groundAlbedo);
            m_featureProcessorInterface->SetMieScattering(m_atmosphereId, m_configuration.m_mieScattering);
            m_featureProcessorInterface->SetMinMaxSamples(m_atmosphereId, m_configuration.m_minSamples, m_configuration.m_maxSamples);
            m_featureProcessorInterface->SetOriginAtSurface(m_atmosphereId, m_configuration.m_originAtSurface);
            m_featureProcessorInterface->SetPlanetOrigin(m_atmosphereId, transform.GetTranslation());
            m_featureProcessorInterface->SetPlanetRadius(m_atmosphereId, m_configuration.m_planetRadius);
            m_featureProcessorInterface->SetRaleighScattering(m_atmosphereId, m_configuration.m_rayleighScattering);
            m_featureProcessorInterface->SetSunIlluminance(m_atmosphereId, m_configuration.m_sunIlluminance);
            m_featureProcessorInterface->SetSunDirection(m_atmosphereId, -sunTransform.GetBasisY());


            m_featureProcessorInterface->Enable(m_atmosphereId, true);

            AZ::TransformNotificationBus::MultiHandler::BusConnect(m_entityId);
            if (m_configuration.m_sun.IsValid())
            {
                AZ::TransformNotificationBus::MultiHandler::BusConnect(m_configuration.m_sun);
                if (!sunTransformInterface)
                {
                    AZ::EntityBus::Handler::BusConnect(m_configuration.m_sun);
                }
            }
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

    void SkyAtmosphereComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        const AZ::EntityId entityId = *AZ::TransformNotificationBus::GetCurrentBusId();
        if (m_configuration.m_sun.IsValid() && entityId == m_configuration.m_sun)
        {
            m_featureProcessorInterface->SetSunDirection(m_atmosphereId, -world.GetBasisY());
        }
        else
        {
            if (!m_configuration.m_sun.IsValid())
            {
                m_featureProcessorInterface->SetSunDirection(m_atmosphereId, -world.GetBasisY());
            }
            m_featureProcessorInterface->SetPlanetOrigin(m_atmosphereId, world.GetTranslation());
        }
    }

    void SkyAtmosphereComponentController::OnEntityActivated(const AZ::EntityId& entityId)
    {
        auto sunTransformInterface = TransformBus::FindFirstHandler(entityId);
        if (sunTransformInterface)
        {
            m_featureProcessorInterface->SetSunDirection(m_atmosphereId, -sunTransformInterface->GetWorldTM().GetBasisY());
        }
        AZ::EntityBus::Handler::BusDisconnect(m_configuration.m_sun);
    }

} // namespace AZ::Render
