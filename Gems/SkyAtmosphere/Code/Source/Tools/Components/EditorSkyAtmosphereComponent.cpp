/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tools/Components/EditorSkyAtmosphereComponent.h>
#include <SkyAtmosphere/SkyAtmosphereComponentConfig.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace SkyAtmosphere
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
                    QT_TRANSLATE_NOOP("SkyAtmosphere", "Sky Atmosphere"),
                    QT_TRANSLATE_NOOP("SkyAtmosphere", "Sky atmosphere component that renders a physical atmosphere "))
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
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentController::m_configuration,
                        QT_TRANSLATE_NOOP("SkyAtmosphere", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<SkyAtmosphereComponentConfig>(
                    "SkyAtmosphereComponentConfig", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Planet"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SkyAtmosphereComponentConfig::m_originMode,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Origin"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "The origin to use for the atmosphere"))
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::GroundAtWorldOrigin,
                                QT_TRANSLATE_NOOP("SkyAtmosphere", "Ground at world origin"))
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::GroundAtLocalOrigin,
                                QT_TRANSLATE_NOOP("SkyAtmosphere", "Ground at local origin"))
                            ->EnumAttribute(SkyAtmosphereComponentConfig::AtmosphereOrigin::PlanetCenterAtLocalOrigin,
                                QT_TRANSLATE_NOOP("SkyAtmosphere", "Planet center at local origin"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_groundRadius,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Ground radius"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Ground radius"))
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                QT_TRANSLATE_NOOP("SkyAtmosphere", " km"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_groundAlbedo,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Ground albedo"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Additional light from the surface of the ground"))
                        ->EndGroup()

                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Atmosphere"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_atmosphereHeight,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Atmosphere height"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Atmosphere height"))
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                QT_TRANSLATE_NOOP("SkyAtmosphere", " km"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_luminanceFactor,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Illuminance factor"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "An additional factor to brighten or darken the overall atmosphere"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_rayleighScatteringScale,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Rayleigh scattering scale"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Raleigh scattering scale"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_rayleighScattering,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Rayleigh scattering"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Raleigh scattering coefficients from air molecules at surface of the planet"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_rayleighExponentialDistribution,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Rayleigh exponential distribution"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Altitude at which Rayleigh scattering is reduced to roughly 40%"))
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                QT_TRANSLATE_NOOP("SkyAtmosphere", " km"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 400.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieScatteringScale,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie scattering scale"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie scattering scale"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_mieScattering,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie scattering"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie scattering coefficients from aerosole molecules at surface of the planet"))

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieAbsorptionScale,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie absorption scale"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie absorption scale"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_mieAbsorption,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie absorption"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie absorption coefficients from aerosole molecules at surface of the planet"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieExponentialDistribution,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Mie exponential distribution"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Altitude at which Mie scattering is reduced to roughly 40%"))
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                QT_TRANSLATE_NOOP("SkyAtmosphere", " km"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 400.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_absorptionScale,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Ozone absorption scale"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Ozone molecule absorption scale"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_absorption,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Ozone absorption"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Absorption coefficients from ozone molecules in a layer most dense at roughly the middle height of the atmosphere"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_aerialDepthFactor,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Aerial depth factor"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "A factor applied to object's depth to increase or decrease aeriel perspective"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->EndGroup()

                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_drawSun,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Show sun"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Whether to show the sun or not"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_sun,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun orientation"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Optional sun entity to use for orientation"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_sunColor,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun color"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun color"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_sunLuminanceFactor,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun luminance factor"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun luminance factor"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.00f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SkyAtmosphereComponentConfig::m_sunLimbColor,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun limb color"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun limb color, for adjusting outer edge color of sun"))
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_sunRadiusFactor,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun radius factor"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun radius factor"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_sunFalloffFactor,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun falloff factor"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Sun falloff factor"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 200.0f)
                        ->EndGroup()

                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Advanced"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_fastSkyEnabled,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Fast sky"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Enable to use a less accurate but faster performing sky algorithm"))
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_fastAerialPerspectiveEnabled,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Fast aerial perspective"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Enable to use a volume look-up-texture for faster performance but more memory"))
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_aerialPerspectiveEnabled,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Aerial perspective"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Enable to draw the effect of atmosphere scattering on objects in the scene."))
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SkyAtmosphereComponentConfig::m_shadowsEnabled,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Enable shadows"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Enable sampling of shadows in atmosphere"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_nearClip,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Near clip"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Distance at which to start drawing atmosphere"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_nearFadeDistance,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Near fade distance"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Distance over which to fade in the atmosphere"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_minSamples,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Min samples"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Minimum number of samples when tracing"))
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, 64)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_maxSamples,
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Max samples"),
                            QT_TRANSLATE_NOOP("SkyAtmosphere", "Maximum number of samples when tracing"))
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, 64)
                        ->EndGroup()
                    ;
            }
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
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

        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
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
