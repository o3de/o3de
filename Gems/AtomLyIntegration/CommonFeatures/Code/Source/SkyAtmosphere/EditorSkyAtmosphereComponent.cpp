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
    AZ::u32 SkyAtmosphereComponentConfig::OnLUTConfigurationChanged()
    {
        m_lutPropertyChanged = true;
        return Edit::PropertyRefreshLevels::AttributesAndValues;
    }

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

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Planet")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SkyAtmosphereComponentConfig::m_originMode, "Origin", "The origin to use for the atmosphere")
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::GroundAtWorldOrigin, "Ground at World Origin")
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::GroundAtLocalOrigin, "Ground at Local Origin")
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::PlanetCenterAtLocalOrigin, "Planet center at local origin")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_groundRadius, "Ground radius", "Ground radius")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " km")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_groundAlbedo, "Ground albedo", "Additional light from the surface of the ground")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                        ->EndGroup()

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Atmosphere")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_atmosphereHeight, "Atmosphere height", "Atmosphere height")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " km")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_luminanceFactor, "Illuminance factor", "An additional factor to brighten or darken the overall atmosphere.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_rayleighScatteringScale, "Rayleigh scattering Scale", "Raleigh scattering scale")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_rayleighScattering, "Rayleigh scattering", "Raleigh scattering coefficients from air molecules at surface of the planet.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_rayleighExponentialDistribution, "Rayleigh exponential distribution", "Altitude at which Rayleigh scattering is reduced to roughly 40%.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " km")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 400.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieScatteringScale, "Mie scattering Scale", "Mie scattering scale")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_mieScattering, "Mie scattering", "Mie scattering coefficients from aerosole molecules at surface of the planet.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieAbsorptionScale, "Mie absorption Scale", "Mie absorption scale")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_mieAbsorption, "Mie absorption", "Mie absorption coefficients from aerosole molecules at surface of the planet.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieExponentialDistribution, "Mie exponential distribution", "Altitude at which Mie scattering is reduced to roughly 40%.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " km")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 400.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_absorptionScale, "Ozone Absorption Scale", "Ozone molecule absorption scale")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_absorption, "Ozone Absorption", "Absorption coefficients from ozone molecules in a layer most dense at roughly the middle height of the atmosphere.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkyAtmosphereComponentConfig::OnLUTConfigurationChanged)
                        ->EndGroup()

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Sun")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_drawSun, "Show sun", "Whether to show the sun or not")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_sun, "Sun orientation", "Optional sun entity to use for orientation")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_sunColor, "Sun color", "Sun color")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_sunLuminanceFactor, "Sun luminance factor", "Sun luminance factor")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_sunLimbColor, "Sun limb color", "Sun limb color, for adjusting outer edge color of sun.")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_sunRadiusFactor, "Sun radius factor", "Sun radius factor")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_sunFalloffFactor, "Sun falloff factor", "Sun falloff factor")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 200.0f)
                        ->EndGroup()

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_fastSkyEnabled, "Fast sky", "Enable to use a less accurate but faster performing sky algorithm")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_shadowsEnabled, "Enable shadows", "Enable sampling of shadows in atmosphere")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_minSamples, "Min samples", "Minimum number of samples when tracing")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, 64)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_maxSamples, "Max samples", "Maximum number of samples when tracing")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, 64)
                        ->EndGroup()
                    ;
            }
        }
    }

    EditorSkyAtmosphereComponent::EditorSkyAtmosphereComponent(const SkyAtmosphereComponentConfig& config)
        : BaseClass(config)
    {
    }

    AZ::u32 EditorSkyAtmosphereComponent::OnConfigurationChanged()
    {
        if(auto featureProcessor = m_controller.m_featureProcessorInterface)
        {
            auto id = m_controller.m_atmosphereId;
            auto config = m_controller.m_configuration;

            if (config.m_lutPropertyChanged)
            {
                featureProcessor->SetAtmosphereRadius(id, config.m_groundRadius + config.m_atmosphereHeight);
                featureProcessor->SetAbsorption(id, config.m_absorption * config.m_absorptionScale);
                featureProcessor->SetGroundAlbedo(id, config.m_groundAlbedo);
                featureProcessor->SetLuminanceFactor(id, config.m_luminanceFactor);
                featureProcessor->SetMieScattering(id, config.m_mieScattering * config.m_mieScatteringScale);
                featureProcessor->SetMieAbsorption(id, config.m_mieAbsorption * config.m_mieAbsorptionScale);
                featureProcessor->SetMieExpDistribution(id, config.m_mieExponentialDistribution);
                featureProcessor->SetPlanetRadius(id, config.m_groundRadius);
                featureProcessor->SetRayleighScattering(id, config.m_rayleighScattering * config.m_rayleighScatteringScale);
                featureProcessor->SetRayleighExpDistribution(id, config.m_rayleighExponentialDistribution);

                config.m_lutPropertyChanged = false;
            }

            featureProcessor->SetFastSkyEnabled(id, config.m_fastSkyEnabled);
            featureProcessor->SetShadowsEnabled(id, config.m_shadowsEnabled);
            featureProcessor->SetSunEnabled(id, config.m_drawSun);
            featureProcessor->SetSunColor(id, config.m_sunColor * config.m_sunLuminanceFactor);
            featureProcessor->SetSunLimbColor(id, config.m_sunLimbColor * config.m_sunLuminanceFactor);
            featureProcessor->SetSunRadiusFactor(id, config.m_sunRadiusFactor);
            featureProcessor->SetSunFalloffFactor(id, config.m_sunFalloffFactor);
            featureProcessor->SetMinMaxSamples(id, config.m_minSamples, config.m_maxSamples);
            
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
