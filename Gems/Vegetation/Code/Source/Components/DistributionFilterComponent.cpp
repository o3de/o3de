/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "DistributionFilterComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <Vegetation/InstanceData.h>
#include <AzCore/Debug/Profiler.h>

#include <Vegetation/Ebuses/DebugNotificationBus.h>

namespace Vegetation
{
    void DistributionFilterConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DistributionFilterConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("FilterStage", &DistributionFilterConfig::m_filterStage)
                ->Field("ThresholdMin", &DistributionFilterConfig::m_thresholdMin)
                ->Field("ThresholdMax", &DistributionFilterConfig::m_thresholdMax)
                ->Field("Gradient", &DistributionFilterConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<DistributionFilterConfig>(
                    "Vegetation Distribution Filter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->UIElement("GradientPreviewer", "Previewer")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientSampler"), &DistributionFilterConfig::GetSampler)
                    ->Attribute(AZ_CRC_CE("GradientFilter"), &DistributionFilterConfig::GetFilterFunc)
                    ->EndGroup()

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DistributionFilterConfig::m_filterStage, "Filter Stage", "Determines if filter is applied before (PreProcess) or after (PostProcess) modifiers.")
                    ->EnumAttribute(FilterStage::Default, "Default")
                    ->EnumAttribute(FilterStage::PreProcess, "PreProcess")
                    ->EnumAttribute(FilterStage::PostProcess, "PostProcess")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DistributionFilterConfig::m_thresholdMin, "Threshold Min", "Minimum value accepted from input gradient that allows the distribution filter to pass.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DistributionFilterConfig::m_thresholdMax, "Threshold Max", "Maximum value accepted from input gradient that allows the distribution filter to pass.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &DistributionFilterConfig::m_gradientSampler, "Gradient", "Gradient used as input signal tested against threshold range.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DistributionFilterConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("filterStage",
                    [](DistributionFilterConfig* config) { return (AZ::u8&)(config->m_filterStage); },
                    [](DistributionFilterConfig* config, const AZ::u8& i) { config->m_filterStage = (FilterStage)i; })
                ->Property("thresholdMin", BehaviorValueProperty(&DistributionFilterConfig::m_thresholdMin))
                ->Property("thresholdMax", BehaviorValueProperty(&DistributionFilterConfig::m_thresholdMax))
                ->Property("gradientSampler", BehaviorValueProperty(&DistributionFilterConfig::m_gradientSampler))
                ;
        }
    }

    AZStd::function<float(float, const GradientSignal::GradientSampleParams&)> DistributionFilterConfig::GetFilterFunc()
    {
        return [this](float sampleValue, const GradientSignal::GradientSampleParams& /*params*/)
        {
            return ((sampleValue >= m_thresholdMin) && (sampleValue <= m_thresholdMax)) ? 1.0f : 0.0f;
        };
    }

    GradientSignal::GradientSampler* DistributionFilterConfig::GetSampler()
    {
        return &m_gradientSampler;
    }

    void DistributionFilterComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationFilterService"));
        services.push_back(AZ_CRC_CE("VegetationDistributionFilterService"));
    }

    void DistributionFilterComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationDistributionFilterService"));
    }

    void DistributionFilterComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void DistributionFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        DistributionFilterConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DistributionFilterComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &DistributionFilterComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("DistributionFilterComponentTypeId", BehaviorConstant(DistributionFilterComponentTypeId));

            behaviorContext->Class<DistributionFilterComponent>()->RequestBus("DistributionFilterRequestBus");

            behaviorContext->EBus<DistributionFilterRequestBus>("DistributionFilterRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetThresholdMin", &DistributionFilterRequestBus::Events::GetThresholdMin)
                ->Event("SetThresholdMin", &DistributionFilterRequestBus::Events::SetThresholdMin)
                ->VirtualProperty("ThresholdMin", "GetThresholdMin", "SetThresholdMin")
                ->Event("GetThresholdMax", &DistributionFilterRequestBus::Events::GetThresholdMax)
                ->Event("SetThresholdMax", &DistributionFilterRequestBus::Events::SetThresholdMax)
                ->VirtualProperty("ThresholdMax", "GetThresholdMax", "SetThresholdMax")
                ->Event("GetGradientSampler", &DistributionFilterRequestBus::Events::GetGradientSampler);
        }
    }

    DistributionFilterComponent::DistributionFilterComponent(const DistributionFilterConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void DistributionFilterComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());

        if (m_configuration.m_gradientSampler.m_gradientId.IsValid())
        {
            m_dependencyMonitor.ConnectDependencies({ m_configuration.m_gradientSampler.m_gradientId });
            FilterRequestBus::Handler::BusConnect(GetEntityId());
        }

        DistributionFilterRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DistributionFilterComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        FilterRequestBus::Handler::BusDisconnect();
        DistributionFilterRequestBus::Handler::BusDisconnect();
    }

    bool DistributionFilterComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const DistributionFilterConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool DistributionFilterComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<DistributionFilterConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool DistributionFilterComponent::Evaluate(const InstanceData& instanceData) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        const GradientSignal::GradientSampleParams sampleParams(instanceData.m_position);
        const float noise = m_configuration.m_gradientSampler.GetValue(sampleParams);
        const bool result = (noise >= m_configuration.m_thresholdMin) && (noise <= m_configuration.m_thresholdMax);
        if (!result)
        {
            VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FilterInstance, instanceData.m_id, AZStd::string_view("DistributionFilter")));
        }
        return result;
    }

    FilterStage DistributionFilterComponent::GetFilterStage() const
    {
        return m_configuration.m_filterStage;
    }

    void DistributionFilterComponent::SetFilterStage(FilterStage filterStage)
    {
        m_configuration.m_filterStage = filterStage;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float DistributionFilterComponent::GetThresholdMin() const
    {
        return m_configuration.m_thresholdMin;
    }

    void DistributionFilterComponent::SetThresholdMin(float thresholdMin)
    {
        m_configuration.m_thresholdMin = thresholdMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float DistributionFilterComponent::GetThresholdMax() const
    {
        return m_configuration.m_thresholdMax;
    }

    void DistributionFilterComponent::SetThresholdMax(float thresholdMax)
    {
        m_configuration.m_thresholdMax = thresholdMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSignal::GradientSampler& DistributionFilterComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
