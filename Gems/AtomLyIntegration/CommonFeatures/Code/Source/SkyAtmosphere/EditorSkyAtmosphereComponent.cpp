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
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Environment")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
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
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::GroundAtWorldOrigin, "Ground at world origin")
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::GroundAtLocalOrigin, "Ground at local origin")
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::PlanetCenterAtLocalOrigin, "Planet center at local origin")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_groundRadius, "Ground radius", "Ground radius")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " km")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_groundAlbedo, "Ground albedo", "Additional light from the surface of the ground")
                        ->EndGroup()

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Atmosphere")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_atmosphereHeight, "Atmosphere height", "Atmosphere height")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " km")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_luminanceFactor, "Illuminance factor", "An additional factor to brighten or darken the overall atmosphere")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_rayleighScatteringScale, "Rayleigh scattering scale", "Raleigh scattering scale")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_rayleighScattering, "Rayleigh scattering", "Raleigh scattering coefficients from air molecules at surface of the planet")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_rayleighExponentialDistribution, "Rayleigh exponential distribution", "Altitude at which Rayleigh scattering is reduced to roughly 40%")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " km")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 400.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieScatteringScale, "Mie scattering scale", "Mie scattering scale")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_mieScattering, "Mie scattering", "Mie scattering coefficients from aerosole molecules at surface of the planet")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieAbsorptionScale, "Mie absorption scale", "Mie absorption scale")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_mieAbsorption, "Mie absorption", "Mie absorption coefficients from aerosole molecules at surface of the planet")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieExponentialDistribution, "Mie exponential distribution", "Altitude at which Mie scattering is reduced to roughly 40%")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " km")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 400.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_absorptionScale, "Ozone absorption scale", "Ozone molecule absorption scale")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_absorption, "Ozone absorption", "Absorption coefficients from ozone molecules in a layer most dense at roughly the middle height of the atmosphere")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_aerialDepthFactor, "Aerial depth factor", "A factor applied to object's depth to increase or decrease aeriel perspective")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->EndGroup()

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Sun")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_drawSun, "Show sun", "Whether to show the sun or not")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_sun, "Sun orientation", "Optional sun entity to use for orientation")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_sunColor, "Sun color", "Sun color")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_sunLuminanceFactor, "Sun luminance factor", "Sun luminance factor")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_sunLimbColor, "Sun limb color", "Sun limb color, for adjusting outer edge color of sun")
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
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_fastAerialPerspectiveEnabled, "Fast aerial perspective", "Enable to use a volume look-up-texture for faster performance but more memory")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_aerialPerspectiveEnabled, "Aerial perspective", "Enable to draw the effect of atmosphere scattering on objects in the scene.")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_shadowsEnabled, "Enable shadows", "Enable sampling of shadows in atmosphere")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_nearClip, "Near clip", "Distance at which to start drawing atmosphere")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_nearFadeDistance, "Near fade distance", "Distance over which to fade in the atmosphere")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
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

        if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorSkyAtmosphereComponent>()
                ->RequestBus("SkyAtmosphereRequestBus");
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
            featureProcessor->SetAtmosphereParams(m_controller.m_atmosphereId, m_controller.GetUpdatedSkyAtmosphereParams());
        }

        return Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void EditorSkyAtmosphereComponent::OnEntityVisibilityChanged(bool visibility)
    {
        if(auto featureProcessor = m_controller.m_featureProcessorInterface)
        {
            if (visibility && m_controller.m_atmosphereId.IsNull())
            {
                m_controller.m_atmosphereId = featureProcessor->CreateAtmosphere();
                featureProcessor->SetAtmosphereParams(m_controller.m_atmosphereId, m_controller.GetUpdatedSkyAtmosphereParams());
            }
            else if (!visibility && !m_controller.m_atmosphereId.IsNull())
            {
                featureProcessor->ReleaseAtmosphere(m_controller.m_atmosphereId);
                m_controller.m_atmosphereId.Reset();
            }
        }
    }
}
