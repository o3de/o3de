/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientGI/GradientGIComponentController.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConstants.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        // =====================================================================
        // Reflect
        // =====================================================================

        void GradientGIComponentController::Reflect(ReflectContext* context)
        {
            GradientGIComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<GradientGIComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &GradientGIComponentController::m_configuration)
                    ;
            }

            if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->EBus<GradientGIComponentRequestBus>("GradientGIComponentRequestBus")
                    ->Event("SetLowColor", &GradientGIComponentRequestBus::Events::SetLowColor)
                    ->Event("GetLowColor", &GradientGIComponentRequestBus::Events::GetLowColor)
                    ->Event("SetMidColor", &GradientGIComponentRequestBus::Events::SetMidColor)
                    ->Event("GetMidColor", &GradientGIComponentRequestBus::Events::GetMidColor)
                    ->Event("SetHighColor", &GradientGIComponentRequestBus::Events::SetHighColor)
                    ->Event("GetHighColor", &GradientGIComponentRequestBus::Events::GetHighColor)
                    ->Event("SetExposure", &GradientGIComponentRequestBus::Events::SetExposure)
                    ->Event("GetExposure", &GradientGIComponentRequestBus::Events::GetExposure)
                    ->Event("SetFaceResolution", &GradientGIComponentRequestBus::Events::SetFaceResolution)
                    ->Event("GetFaceResolution", &GradientGIComponentRequestBus::Events::GetFaceResolution)
                    ;
            }
        }

        // =====================================================================
        // Service Descriptors
        // =====================================================================

        void GradientGIComponentController::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GradientGIService"));
        }

        void GradientGIComponentController::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("GradientGIService"));
        }

        // =====================================================================
        // Lifecycle
        // =====================================================================

        GradientGIComponentController::GradientGIComponentController(const GradientGIComponentConfig& config)
            : m_configuration(config)
        {
        }

        void GradientGIComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<GradientGIFeatureProcessorInterface>(m_entityId);
            AZ_Error("GradientGIComponentController", m_featureProcessor, "Unable to find GradientGIFeatureProcessorInterface on this entity's scene.");

            if (m_featureProcessor)
            {
                UpdateColors();
                m_featureProcessor->SetExposure(m_configuration.m_exposure);
                m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);

                GradientGIComponentRequestBus::Handler::BusConnect(m_entityId);
            }
        }

        void GradientGIComponentController::Deactivate()
        {
            GradientGIComponentRequestBus::Handler::BusDisconnect();

            if (m_featureProcessor)
            {
                m_featureProcessor->Reset();
                m_featureProcessor = nullptr;
            }

            m_entityId = EntityId(EntityId::InvalidEntityId);
        }

        void GradientGIComponentController::SetConfiguration(const GradientGIComponentConfig& config)
        {
            m_configuration = config;
        }

        const GradientGIComponentConfig& GradientGIComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        // =====================================================================
        // Bus Handlers
        // =====================================================================

        void GradientGIComponentController::SetLowColor(const Color& color)
        {
            m_configuration.m_lowColor = color;
            UpdateColors();
        }

        Color GradientGIComponentController::GetLowColor() const
        {
            return m_configuration.m_lowColor;
        }

        void GradientGIComponentController::SetMidColor(const Color& color)
        {
            m_configuration.m_midColor = color;
            UpdateColors();
        }

        Color GradientGIComponentController::GetMidColor() const
        {
            return m_configuration.m_midColor;
        }

        void GradientGIComponentController::SetHighColor(const Color& color)
        {
            m_configuration.m_highColor = color;
            UpdateColors();
        }

        Color GradientGIComponentController::GetHighColor() const
        {
            return m_configuration.m_highColor;
        }

        void GradientGIComponentController::SetExposure(float exposure)
        {
            m_configuration.m_exposure = exposure;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetExposure(exposure);
            }
        }

        float GradientGIComponentController::GetExposure() const
        {
            return m_configuration.m_exposure;
        }

        void GradientGIComponentController::SetFaceResolution(uint32_t resolution)
        {
            m_configuration.m_faceResolution = resolution;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetFaceResolution(resolution);
            }
        }

        uint32_t GradientGIComponentController::GetFaceResolution() const
        {
            return m_configuration.m_faceResolution;
        }

        // =====================================================================
        // Helpers
        // =====================================================================

        void GradientGIComponentController::UpdateColors()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->SetGradientColors(
                    m_configuration.m_lowColor,
                    m_configuration.m_midColor,
                    m_configuration.m_highColor);
            }
        }

    } // namespace Render
} // namespace AZ
