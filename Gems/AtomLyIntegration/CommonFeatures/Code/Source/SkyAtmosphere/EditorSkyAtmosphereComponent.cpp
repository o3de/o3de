/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/EditorSkyAtmosphereComponent.h>
#include <AtomLyIntegration/CommonFeatures/SkyAtmosphere/SkyAtmosphereComponentConfig.h>

namespace AZ::Render
{
    void EditorSkyAtmosphereComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSkyAtmosphereComponent, BaseClass>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSkyAtmosphereComponent>(
                    "Sky Atmosphere", "Sky atmosphere component that renders a physical atmosphere ")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                editContext->Class<SkyAtmosphereComponentController>(
                    "SkyAtmosphereComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<SkyAtmosphereComponentConfig>(
                    "SkyAtmosphereComponentConfig", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_atmosphereRadius, "Atmosphere radius", "Atmosphere radius")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10000.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_planetRadius, "Planet radius", "Planet radius")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10000.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_sun, "Sun orientation", "Optional sun entity to use for orientation")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_drawSun, "Draw sun", "Whether to draw the sun or not")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_sunRadiusFactor, "Sun radius factor", "Sun radius factor")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_sunFalloffFactor, "Sun falloff factor", "Sun falloff factor")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_sunIlluminance, "Sun illuminance", "Sun illuminance")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSkyAtmosphereComponent::OnLUTConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_minSamples, "Min samples", "Minimum number of samples when tracing")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 20.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 64.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_maxSamples, "Max samples", "Maximum number of samples when tracing")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 20.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 64.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_rayleighScattering, "Raleigh scattering", "Raleigh scattering")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSkyAtmosphereComponent::OnLUTConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieScattering, "Mie scattering", "Mie scattering")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSkyAtmosphereComponent::OnLUTConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieExtinction, "Mie extinction", "Mie extinction")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSkyAtmosphereComponent::OnLUTConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_absorptionExtinction, "Absorption extinction", "Absorption extinction")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSkyAtmosphereComponent::OnLUTConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_groundAlbedo, "Ground albedo", "Additional light from the surface of the ground")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSkyAtmosphereComponent::OnLUTConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SkyAtmosphereComponentConfig::m_originMode, "Origin", "The origin to use for the atmosphere")
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::CenterAtLocalOrigin, "Center at Local Origin")
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::SurfaceAtLocalOrigin, "Surface at Local Origin")
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::SurfaceAtWorldOrigin, "Surface at World Origin")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_fastSkyEnabled, "Fast sky", "Enable to use a less accurate but faster performing sky algorithm")
                    ;
            }
        }
    }

    EditorSkyAtmosphereComponent::EditorSkyAtmosphereComponent(const SkyAtmosphereComponentConfig& config)
        : BaseClass(config)
    {
    }

    AZ::u32 EditorSkyAtmosphereComponent::OnLUTConfigurationChanged()
    {
        if (auto featureProcessor = m_controller.m_featureProcessorInterface)
        {
            auto id = m_controller.m_atmosphereId;
            auto config = m_controller.m_configuration;

            featureProcessor->SetAbsorptionExtinction(id, config.m_absorptionExtinction);
            featureProcessor->SetGroundAlbedo(id, config.m_groundAlbedo);
            featureProcessor->SetMieScattering(id, config.m_mieScattering);
            featureProcessor->SetRaleighScattering(id, config.m_rayleighScattering);
            featureProcessor->SetSunIlluminance(id, config.m_sunIlluminance);
        }

        return Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::u32 EditorSkyAtmosphereComponent::OnConfigurationChanged()
    {
        if(auto featureProcessor = m_controller.m_featureProcessorInterface)
        {
            auto id = m_controller.m_atmosphereId;
            auto config = m_controller.m_configuration;

            featureProcessor->SetAtmosphereRadius(id, config.m_atmosphereRadius);
            featureProcessor->SetMinMaxSamples(id, config.m_minSamples, config.m_maxSamples);
            featureProcessor->SetPlanetRadius(id, config.m_planetRadius);
            
            // update the transform again in case the sun entity changed
            const AZ::Transform& transform = m_controller.m_transformInterface ? m_controller.m_transformInterface->GetWorldTM() : Transform::Identity();
            auto sunTransformInterface = TransformBus::FindFirstHandler(m_controller.m_configuration.m_sun);
            AZ::Transform sunTransform = sunTransformInterface ? sunTransformInterface->GetWorldTM() : transform;
            featureProcessor->SetSunDirection(id, -sunTransform.GetBasisY());

            m_controller.UpdatePlanetOrigin();
        }

        return Edit::PropertyRefreshLevels::AttributesAndValues;
    }
}
