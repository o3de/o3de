/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PosterizeGradientComponent.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GradientSignal
{
    void PosterizeGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PosterizeGradientConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Mode", &PosterizeGradientConfig::m_mode)
                ->Field("Bands", &PosterizeGradientConfig::m_bands)
                ->Field("Gradient", &PosterizeGradientConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<PosterizeGradientConfig>(
                    "Posterize Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &PosterizeGradientConfig::m_mode, "Mode", "")
                    ->EnumAttribute(ModeType::Ceiling, "Ceiling")
                    ->EnumAttribute(ModeType::Floor, "Floor")
                    ->EnumAttribute(ModeType::Round, "Round")
                    ->EnumAttribute(ModeType::Ps, "PS")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PosterizeGradientConfig::m_bands, "Bands", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 2)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->DataElement(0, &PosterizeGradientConfig::m_gradientSampler, "Gradient", "Input gradient whose values will be transformed in relation to threshold.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PosterizeGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("bands", BehaviorValueProperty(&PosterizeGradientConfig::m_bands))
                ->Property("mode", 
                    [](PosterizeGradientConfig* config) { return (AZ::u8&)(config->m_mode); },
                    [](PosterizeGradientConfig* config, const AZ::u8& i) { config->m_mode = (PosterizeGradientConfig::ModeType)i; })
                ->Property("gradientSampler", BehaviorValueProperty(&PosterizeGradientConfig::m_gradientSampler))
                ;
        }
    }

    void PosterizeGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void PosterizeGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void PosterizeGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void PosterizeGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        PosterizeGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PosterizeGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &PosterizeGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("PosterizeGradientComponentTypeId", BehaviorConstant(PosterizeGradientComponentTypeId));

            behaviorContext->Class<PosterizeGradientComponent>()->RequestBus("PosterizeGradientRequestBus");

            behaviorContext->EBus<PosterizeGradientRequestBus>("PosterizeGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetBands", &PosterizeGradientRequestBus::Events::GetBands)
                ->Event("SetBands", &PosterizeGradientRequestBus::Events::SetBands)
                ->VirtualProperty("Bands", "GetBands", "SetBands")
                ->Event("GetModeType", &PosterizeGradientRequestBus::Events::GetModeType)
                ->Event("SetModeType", &PosterizeGradientRequestBus::Events::SetModeType)
                ->VirtualProperty("Bands", "GetBands", "SetBands")
                ->Event("GetGradientSampler", &PosterizeGradientRequestBus::Events::GetGradientSampler)
                ;
        }
    }

    PosterizeGradientComponent::PosterizeGradientComponent(const PosterizeGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void PosterizeGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        PosterizeGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void PosterizeGradientComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        GradientRequestBus::Handler::BusDisconnect();
        PosterizeGradientRequestBus::Handler::BusDisconnect();
    }

    bool PosterizeGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const PosterizeGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool PosterizeGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<PosterizeGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float PosterizeGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        const float bands = AZ::GetMax(static_cast<float>(m_configuration.m_bands), 2.0f);
        const float input = AZ::GetClamp(m_configuration.m_gradientSampler.GetValue(sampleParams), 0.0f, 1.0f);
        float output = 0.0f;

        // "quantize" the input down to a number that goes from 0 to (bands-1)
        const float band = AZ::GetClamp(floorf(input * bands), 0.0f, bands - 1.0f);

        // Given our quantized band, produce the right output for that band range.
        switch (m_configuration.m_mode)
        {
            default:
            case PosterizeGradientConfig::ModeType::Floor:
                // Floor:  the output range should be the lowest value of each band, or (0 to bands-1) / bands
                output = (band + 0.0f) / bands;
                break;
            case PosterizeGradientConfig::ModeType::Round:
                // Round:  the output range should be the midpoint of each band, or (0.5 to bands-0.5) / bands
                output = (band + 0.5f) / bands;
                break;
            case PosterizeGradientConfig::ModeType::Ceiling:
                // Ceiling:  the output range should be the highest value of each band, or (1 to bands) / bands
                output = (band + 1.0f) / bands;
                break;
            case PosterizeGradientConfig::ModeType::Ps:
                // Ps:  the output range should be equally distributed from 0-1, or (0 to bands-1) / (bands-1)
                output = band / (bands - 1.0f);
                break;
        }
        return AZ::GetClamp(output, 0.0f, 1.0f);
    }

    bool PosterizeGradientComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    AZ::s32 PosterizeGradientComponent::GetBands() const
    {
        return m_configuration.m_bands;
    }

    void PosterizeGradientComponent::SetBands(AZ::s32 bands)
    {
        m_configuration.m_bands = bands;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u8 PosterizeGradientComponent::GetModeType() const
    {
        return static_cast<AZ::u8>(m_configuration.m_mode);
    }

    void PosterizeGradientComponent::SetModeType(AZ::u8 modeType)
    {
        m_configuration.m_mode = static_cast<PosterizeGradientConfig::ModeType>(modeType);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSampler& PosterizeGradientComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
