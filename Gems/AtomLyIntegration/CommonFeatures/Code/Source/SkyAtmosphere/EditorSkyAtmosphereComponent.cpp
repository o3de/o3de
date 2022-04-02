/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/EditorSkyAtmosphereComponent.h>

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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_sun, "Sun", "Optional sun entity to use for transform")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyAtmosphereComponentConfig::m_sunIlluminance, "Sun illuminance", "Sun illuminance")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieScattering, "Mie scattering", "Mie scattering")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_mieExtinction, "Mie extinction", "Mie extinction")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_absorptionExtinction, "Absorption extinction", "Absorption extinction. Useful for thi")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkyAtmosphereComponentConfig::m_groundAlbedo, "Ground albedo", "Additional light from the surface of the ground")
                    ;
            }
        }
    }

    EditorSkyAtmosphereComponent::EditorSkyAtmosphereComponent(const SkyAtmosphereComponentConfig& config)
        : BaseClass(config)
    {
    }
}
