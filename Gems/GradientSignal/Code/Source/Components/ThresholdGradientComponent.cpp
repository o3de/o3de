/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "ThresholdGradientComponent.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GradientSignal
{
    void ThresholdGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ThresholdGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("Threshold", &ThresholdGradientConfig::m_threshold)
                ->Field("Gradient", &ThresholdGradientConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ThresholdGradientConfig>(
                    "Threshold Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ThresholdGradientConfig::m_threshold, "Threshold", "Specifies the value used to convert lower or higher gradient samples to 0 or 1 respectively.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &ThresholdGradientConfig::m_gradientSampler, "Gradient", "Input gradient whose values will be transformed in relation to threshold.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ThresholdGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("threshold", BehaviorValueProperty(&ThresholdGradientConfig::m_threshold))
                ->Property("gradientSampler", BehaviorValueProperty(&ThresholdGradientConfig::m_gradientSampler))
                ;
        }
    }

    void ThresholdGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void ThresholdGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void ThresholdGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void ThresholdGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        ThresholdGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ThresholdGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &ThresholdGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("ThresholdGradientComponentTypeId", BehaviorConstant(ThresholdGradientComponentTypeId));

            behaviorContext->Class<ThresholdGradientComponent>()->RequestBus("ThresholdGradientRequestBus");

            behaviorContext->EBus<ThresholdGradientRequestBus>("ThresholdGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetThreshold", &ThresholdGradientRequestBus::Events::GetThreshold)
                ->Event("SetThreshold", &ThresholdGradientRequestBus::Events::SetThreshold)
                ->VirtualProperty("Threshold", "GetThreshold", "SetThreshold")
                ->Event("GetGradientSampler", &ThresholdGradientRequestBus::Events::GetGradientSampler)
                ;
        }
    }

    ThresholdGradientComponent::ThresholdGradientComponent(const ThresholdGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void ThresholdGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        ThresholdGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ThresholdGradientComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        GradientRequestBus::Handler::BusDisconnect();
        ThresholdGradientRequestBus::Handler::BusDisconnect();
    }

    bool ThresholdGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const ThresholdGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool ThresholdGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<ThresholdGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float ThresholdGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        float output = 0.0f;

        output = m_configuration.m_gradientSampler.GetValue(sampleParams) <= m_configuration.m_threshold ? 0.0f : 1.0f;

        return output;
    }

    bool ThresholdGradientComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    float ThresholdGradientComponent::GetThreshold() const
    {
        return m_configuration.m_threshold;
    }

    void ThresholdGradientComponent::SetThreshold(float threshold)
    {
        m_configuration.m_threshold = threshold;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSampler& ThresholdGradientComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
