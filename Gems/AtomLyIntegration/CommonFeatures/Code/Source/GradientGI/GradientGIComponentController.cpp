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
#include <AzCore/Script/ScriptContextAttributes.h>
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
                // Expose the update mode enum values for Script Canvas
                behaviorContext
                    ->Enum<static_cast<uint32_t>(GradientGIUpdateMode::Static)>("GradientGIUpdateMode_Static")
                    ->Enum<static_cast<uint32_t>(GradientGIUpdateMode::Dynamic)>("GradientGIUpdateMode_Dynamic")
                    ;

                behaviorContext->EBus<GradientGIComponentRequestBus>("GradientGIComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    // Single-layer color setters/getters
                    ->Event("SetLowColor",    &GradientGIComponentRequestBus::Events::SetLowColor)
                    ->Event("GetLowColor",    &GradientGIComponentRequestBus::Events::GetLowColor)
                    ->Event("SetMidColor",    &GradientGIComponentRequestBus::Events::SetMidColor)
                    ->Event("GetMidColor",    &GradientGIComponentRequestBus::Events::GetMidColor)
                    ->Event("SetHighColor",   &GradientGIComponentRequestBus::Events::SetHighColor)
                    ->Event("GetHighColor",   &GradientGIComponentRequestBus::Events::GetHighColor)
                    // Set all three gradient layers at once
                    ->Event("SetGradientColors", &GradientGIComponentRequestBus::Events::SetGradientColors)
                    // Exposure
                    ->Event("SetExposure",    &GradientGIComponentRequestBus::Events::SetExposure)
                    ->Event("GetExposure",    &GradientGIComponentRequestBus::Events::GetExposure)
                    // Resolution
                    ->Event("SetFaceResolution", &GradientGIComponentRequestBus::Events::SetFaceResolution)
                    ->Event("GetFaceResolution", &GradientGIComponentRequestBus::Events::GetFaceResolution)
                    // Update mode (Static / Dynamic)
                    ->Event("SetUpdateMode",  &GradientGIComponentRequestBus::Events::SetUpdateMode)
                    ->Event("GetUpdateMode",  &GradientGIComponentRequestBus::Events::GetUpdateMode)
                    // Virtual properties for Script Canvas property nodes
                    ->VirtualProperty("LowColor",       "GetLowColor",       "SetLowColor")
                    ->VirtualProperty("MidColor",       "GetMidColor",       "SetMidColor")
                    ->VirtualProperty("HighColor",      "GetHighColor",      "SetHighColor")
                    ->VirtualProperty("Exposure",       "GetExposure",       "SetExposure")
                    ->VirtualProperty("FaceResolution", "GetFaceResolution", "SetFaceResolution")
                    ->VirtualProperty("UpdateMode",     "GetUpdateMode",     "SetUpdateMode")
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

            AZ_TracePrintf("GradientGI", "=== Controller::Activate() entity=%llu, mode=%d ===\n",
                static_cast<AZ::u64>(m_entityId), static_cast<int>(m_configuration.m_updateMode));

            m_featureProcessor =
                RPI::Scene::GetFeatureProcessorForEntity<GradientGIFeatureProcessorInterface>(m_entityId);
            AZ_Error("GradientGIComponentController", m_featureProcessor,
                "Unable to find GradientGIFeatureProcessorInterface on this entity's scene.");

            AZ_TracePrintf("GradientGI", "  FP ptr = %p\n", m_featureProcessor);

            if (m_featureProcessor)
            {
                // Push all initial configuration into the FP.
                AZ_TracePrintf("GradientGI", "  Pushing config: mode=%d, exposure=%.2f, resolution=%u\n",
                    static_cast<int>(m_configuration.m_updateMode), m_configuration.m_exposure, m_configuration.m_faceResolution);

                m_featureProcessor->SetUpdateMode(
                    static_cast<GradientGIFeatureProcessorInterface::UpdateMode>(m_configuration.m_updateMode));
                UpdateColors();
                m_featureProcessor->SetExposure(m_configuration.m_exposure);
                m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);

                GradientGIComponentRequestBus::Handler::BusConnect(m_entityId);
            }
        }

        void GradientGIComponentController::Deactivate()
        {
            AZ_TracePrintf("GradientGI", "=== Controller::Deactivate() entity=%llu ===\n",
                static_cast<AZ::u64>(m_entityId));

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

        void GradientGIComponentController::SetGradientColors(
            const Color& low, const Color& mid, const Color& high)
        {
            m_configuration.m_lowColor  = low;
            m_configuration.m_midColor  = mid;
            m_configuration.m_highColor = high;
            UpdateColors();
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

        void GradientGIComponentController::SetUpdateMode(GradientGIUpdateMode mode)
        {
            AZ_TracePrintf("GradientGI", "Controller::SetUpdateMode(%d) current=%d, entity=%llu\n",
                static_cast<int>(mode), static_cast<int>(m_configuration.m_updateMode),
                static_cast<AZ::u64>(m_entityId));

            m_configuration.m_updateMode = mode;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetUpdateMode(
                    static_cast<GradientGIFeatureProcessorInterface::UpdateMode>(mode));
            }
        }

        GradientGIUpdateMode GradientGIComponentController::GetUpdateMode() const
        {
            return m_configuration.m_updateMode;
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
