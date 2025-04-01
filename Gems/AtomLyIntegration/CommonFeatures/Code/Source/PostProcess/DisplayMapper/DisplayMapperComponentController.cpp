/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomLyIntegration/CommonFeatures/CoreLights/AreaLightBus.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/DisplayMapper/DisplayMapperComponentController.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>

namespace AZ
{
    namespace Render
    {
        void DisplayMapperComponentController::Reflect(ReflectContext* context)
        {
            DisplayMapperComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DisplayMapperComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &DisplayMapperComponentController::m_configuration);
            }

             if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                 behaviorContext->EBus<DisplayMapperComponentRequestBus>("DisplayMapperComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    // LoadPreset
                    ->Event("LoadPreset", &DisplayMapperComponentRequestBus::Events::LoadPreset)
                    // DisplayMapperOperationType
                    ->Event("SetDisplayMapperOperationType", &DisplayMapperComponentRequestBus::Events::SetDisplayMapperOperationType)
                    ->Event("GetDisplayMapperOperationType", &DisplayMapperComponentRequestBus::Events::GetDisplayMapperOperationType)
                    ->VirtualProperty("DisplayMapperOperationType", "GetDisplayMapperOperationType", "SetDisplayMapperOperationType")
                    // AcesParameterOverrides
                    ->Event("SetAcesParameterOverrides", &DisplayMapperComponentRequestBus::Events::SetAcesParameterOverrides)
                    ->Event("GetAcesParameterOverrides", &DisplayMapperComponentRequestBus::Events::GetAcesParameterOverrides)
                    ->VirtualProperty("AcesParameterOverrides", "GetAcesParameterOverrides", "SetAcesParameterOverrides")
                    // OverrideAcesParameters
                    ->Event("SetOverrideAcesParameters", &DisplayMapperComponentRequestBus::Events::SetOverrideAcesParameters)
                    ->Event("GetOverrideAcesParameters", &DisplayMapperComponentRequestBus::Events::GetOverrideAcesParameters)
                    ->VirtualProperty("OverrideAcesParameters", "GetOverrideAcesParameters", "SetOverrideAcesParameters")
                    // AlterSurround
                    ->Event("SetAlterSurround", &DisplayMapperComponentRequestBus::Events::SetAlterSurround)
                    ->Event("GetAlterSurround", &DisplayMapperComponentRequestBus::Events::GetAlterSurround)
                    ->VirtualProperty("AlterSurround", "GetAlterSurround", "SetAlterSurround")
                    // ApplyDesaturation
                    ->Event("SetApplyDesaturation", &DisplayMapperComponentRequestBus::Events::SetApplyDesaturation)
                    ->Event("GetApplyDesaturation", &DisplayMapperComponentRequestBus::Events::GetApplyDesaturation)
                    ->VirtualProperty("ApplyDesaturation", "GetApplyDesaturation", "SetApplyDesaturation")
                    // ApplyCATD60toD65
                    ->Event("SetApplyCATD60toD65", &DisplayMapperComponentRequestBus::Events::SetApplyCATD60toD65)
                    ->Event("GetApplyCATD60toD65", &DisplayMapperComponentRequestBus::Events::GetApplyCATD60toD65)
                    ->VirtualProperty("ApplyCATD60toD65", "GetApplyCATD60toD65", "SetApplyCATD60toD65")
                    // CinemaLimitsBlack
                    ->Event("SetCinemaLimitsBlack", &DisplayMapperComponentRequestBus::Events::SetCinemaLimitsBlack)
                    ->Event("GetCinemaLimitsBlack", &DisplayMapperComponentRequestBus::Events::GetCinemaLimitsBlack)
                    ->VirtualProperty("CinemaLimitsBlack", "GetCinemaLimitsBlack", "SetCinemaLimitsBlack")
                    // CinemaLimitsWhite
                    ->Event("SetCinemaLimitsWhite", &DisplayMapperComponentRequestBus::Events::SetCinemaLimitsWhite)
                    ->Event("GetCinemaLimitsWhite", &DisplayMapperComponentRequestBus::Events::GetCinemaLimitsWhite)
                    ->VirtualProperty("CinemaLimitsWhite", "GetCinemaLimitsWhite", "SetCinemaLimitsWhite")
                    // MinPoint
                    ->Event("SetMinPoint", &DisplayMapperComponentRequestBus::Events::SetMinPoint)
                    ->Event("GetMinPoint", &DisplayMapperComponentRequestBus::Events::GetMinPoint)
                    ->VirtualProperty("MinPoint", "GetMinPoint", "SetMinPoint")
                    // MidPoint
                    ->Event("SetMidPoint", &DisplayMapperComponentRequestBus::Events::SetMidPoint)
                    ->Event("GetMidPoint", &DisplayMapperComponentRequestBus::Events::GetMidPoint)
                    ->VirtualProperty("MidPoint", "GetMidPoint", "SetMidPoint")
                    // MaxPoint
                    ->Event("SetMaxPoint", &DisplayMapperComponentRequestBus::Events::SetMaxPoint)
                    ->Event("GetMaxPoint", &DisplayMapperComponentRequestBus::Events::GetMaxPoint)
                    ->VirtualProperty("MaxPoint", "GetMaxPoint", "SetMaxPoint")
                    // SurroundGamma
                    ->Event("SetSurroundGamma", &DisplayMapperComponentRequestBus::Events::SetSurroundGamma)
                    ->Event("GetSurroundGamma", &DisplayMapperComponentRequestBus::Events::GetSurroundGamma)
                    ->VirtualProperty("SurroundGamma", "GetSurroundGamma", "SetSurroundGamma")
                    // Gamma
                    ->Event("SetGamma", &DisplayMapperComponentRequestBus::Events::SetGamma)
                    ->Event("GetGamma", &DisplayMapperComponentRequestBus::Events::GetGamma)
                    ->VirtualProperty("Gamma", "GetGamma", "SetGamma")
                ;
            }
        }

        void DisplayMapperComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ToneMapperService"));
        }

        void DisplayMapperComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ToneMapperService"));
        }

        void DisplayMapperComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        DisplayMapperComponentController::DisplayMapperComponentController(const DisplayMapperComponentConfig& config)
            : m_configuration(config)
        {
        }

        void DisplayMapperComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            DisplayMapperComponentRequestBus::Handler::BusConnect(m_entityId);
            OnConfigChanged();
        }

        void DisplayMapperComponentController::Deactivate()
        {
            DisplayMapperFeatureProcessorInterface* fp =
                AZ::RPI::Scene::GetFeatureProcessorForEntity<DisplayMapperFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                fp->UnregisterDisplayMapperConfiguration();
            }
            DisplayMapperComponentRequestBus::Handler::BusDisconnect(m_entityId);

            m_postProcessInterface = nullptr;
            m_entityId.SetInvalid();
        }

        void DisplayMapperComponentController::SetConfiguration(const DisplayMapperComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const DisplayMapperComponentConfig& DisplayMapperComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void DisplayMapperComponentController::LoadPreset(OutputDeviceTransformType preset)
        {
            AcesParameterOverrides propertyOverrides;
            propertyOverrides.m_preset = preset;
            propertyOverrides.m_overrideDefaults = true;
            propertyOverrides.LoadPreset();
            SetAcesParameterOverrides(propertyOverrides);
        }

        void DisplayMapperComponentController::SetDisplayMapperOperationType(DisplayMapperOperationType displayMapperOperationType)
        {
            if (m_configuration.m_displayMapperOperation != displayMapperOperationType)
            {
                m_configuration.m_displayMapperOperation = displayMapperOperationType;
                OnConfigChanged();
                DisplayMapperComponentNotificationBus::Broadcast(
                    &DisplayMapperComponentNotificationBus::Handler::OnDisplayMapperOperationTypeUpdated,
                    m_configuration.m_displayMapperOperation);
            }
        }

        DisplayMapperOperationType DisplayMapperComponentController::GetDisplayMapperOperationType() const
        {
            return m_configuration.m_displayMapperOperation;
        }

        void DisplayMapperComponentController::SetAcesParameterOverrides(const AcesParameterOverrides& parameterOverrides)
        {
            m_configuration.m_acesParameterOverrides = parameterOverrides;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
            DisplayMapperComponentNotificationBus::Broadcast(
                &DisplayMapperComponentNotificationBus::Handler::OnAcesParameterOverridesUpdated,
                m_configuration.m_acesParameterOverrides);
        }

        const AcesParameterOverrides& DisplayMapperComponentController::GetAcesParameterOverrides() const
        {
            return m_configuration.m_acesParameterOverrides;
        }

        void DisplayMapperComponentController::SetOverrideAcesParameters(bool value)
        {
            if (m_configuration.m_acesParameterOverrides.m_overrideDefaults == value)
            {
                return; // prevents flickering when set via TrackView
            }
            m_configuration.m_acesParameterOverrides.m_overrideDefaults = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        bool DisplayMapperComponentController::GetOverrideAcesParameters() const
        {
            return m_configuration.m_acesParameterOverrides.m_overrideDefaults;
        }

        void DisplayMapperComponentController::SetAlterSurround(bool value)
        {
            if (m_configuration.m_acesParameterOverrides.m_alterSurround != value)
            {
                return; // prevents flickering when set via TrackView
            }
            m_configuration.m_acesParameterOverrides.m_alterSurround = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        bool DisplayMapperComponentController::GetAlterSurround() const
        {
            return m_configuration.m_acesParameterOverrides.m_alterSurround;
        }

        void DisplayMapperComponentController::SetApplyDesaturation(bool value)
        {
            if (m_configuration.m_acesParameterOverrides.m_applyDesaturation != value)
            {
                return; // prevents flickering when set via TrackView
            }
            m_configuration.m_acesParameterOverrides.m_applyDesaturation = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        bool DisplayMapperComponentController::GetApplyDesaturation() const
        {
            return m_configuration.m_acesParameterOverrides.m_applyDesaturation;
        }

        void DisplayMapperComponentController::SetApplyCATD60toD65(bool value)
        {
            if (m_configuration.m_acesParameterOverrides.m_applyCATD60toD65 != value)
            {
                return; // prevents flickering when set via TrackView
            }
            m_configuration.m_acesParameterOverrides.m_applyCATD60toD65 = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        bool DisplayMapperComponentController::GetApplyCATD60toD65() const
        {
            return m_configuration.m_acesParameterOverrides.m_applyCATD60toD65;
        }

        void DisplayMapperComponentController::SetCinemaLimitsBlack(float value)
        {
            m_configuration.m_acesParameterOverrides.m_cinemaLimitsBlack = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        float DisplayMapperComponentController::GetCinemaLimitsBlack() const
        {
            return m_configuration.m_acesParameterOverrides.m_cinemaLimitsBlack;
        }

        void DisplayMapperComponentController::SetCinemaLimitsWhite(float value)
        {
            m_configuration.m_acesParameterOverrides.m_cinemaLimitsWhite = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        float DisplayMapperComponentController::GetCinemaLimitsWhite() const
        {
            return m_configuration.m_acesParameterOverrides.m_cinemaLimitsWhite;
        }

        void DisplayMapperComponentController::SetMinPoint(float value)
        {
            m_configuration.m_acesParameterOverrides.m_minPoint = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        float DisplayMapperComponentController::GetMinPoint() const
        {
            return m_configuration.m_acesParameterOverrides.m_minPoint;
        }

        void DisplayMapperComponentController::SetMidPoint(float value)
        {
            m_configuration.m_acesParameterOverrides.m_midPoint = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        float DisplayMapperComponentController::GetMidPoint() const
        {
            return m_configuration.m_acesParameterOverrides.m_midPoint;
        }

        void DisplayMapperComponentController::SetMaxPoint(float value)
        {
            m_configuration.m_acesParameterOverrides.m_maxPoint = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        float DisplayMapperComponentController::GetMaxPoint() const
        {
            return m_configuration.m_acesParameterOverrides.m_maxPoint;
        }

        void DisplayMapperComponentController::SetSurroundGamma(float value)
        {
            m_configuration.m_acesParameterOverrides.m_surroundGamma = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        float DisplayMapperComponentController::GetSurroundGamma() const
        {
            return m_configuration.m_acesParameterOverrides.m_surroundGamma;
        }

        void DisplayMapperComponentController::SetGamma(float value)
        {
            m_configuration.m_acesParameterOverrides.m_gamma = value;
            if (m_configuration.m_displayMapperOperation == DisplayMapperOperationType::Aces)
            {
                OnConfigChanged();
            }
        }

        float DisplayMapperComponentController::GetGamma() const
        {
            return m_configuration.m_acesParameterOverrides.m_gamma;
        }

        void DisplayMapperComponentController::OnConfigChanged()
        {
            // Register the configuration with the  AcesDisplayMapperFeatureProcessor for this scene.
            DisplayMapperFeatureProcessorInterface* fp = AZ::RPI::Scene::GetFeatureProcessorForEntity<DisplayMapperFeatureProcessorInterface>(m_entityId);
            DisplayMapperConfigurationDescriptor desc;
            desc.m_operationType = m_configuration.m_displayMapperOperation;
            desc.m_ldrGradingLutEnabled = m_configuration.m_ldrColorGradingLutEnabled;
            desc.m_ldrColorGradingLut = m_configuration.m_ldrColorGradingLut;
            desc.m_acesParameterOverrides = m_configuration.m_acesParameterOverrides;
            fp->RegisterDisplayMapperConfiguration(desc);
        }

    } // namespace Render
} // namespace AZ
