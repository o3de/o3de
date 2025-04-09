/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScaleModifierComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/Descriptor.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <Vegetation/InstanceData.h>
#include <VegetationProfiler.h>

namespace Vegetation
{
    void ScaleModifierConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ScaleModifierConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("AllowOverrides", &ScaleModifierConfig::m_allowOverrides)
                ->Field("RangeMin", &ScaleModifierConfig::m_rangeMin)
                ->Field("RangeMax", &ScaleModifierConfig::m_rangeMax)
                ->Field("Gradient", &ScaleModifierConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ScaleModifierConfig>(
                    "Vegetation Scale Modifier", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ScaleModifierConfig::m_allowOverrides, "Allow Per-Item Overrides", "Allow per-descriptor parameters to override component parameters.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ScaleModifierConfig::m_rangeMin, "Range Min", "Minimum scale.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.125f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ScaleModifierConfig::m_rangeMax, "Range Max", "Maximum scale.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.125f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->DataElement(0, &ScaleModifierConfig::m_gradientSampler, "Gradient", "Gradient used as blend factor to lerp between ranges.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ScaleModifierConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("allowOverrides", BehaviorValueProperty(&ScaleModifierConfig::m_allowOverrides))
                ->Property("rangeMin", BehaviorValueProperty(&ScaleModifierConfig::m_rangeMin))
                ->Property("rangeMax", BehaviorValueProperty(&ScaleModifierConfig::m_rangeMax))
                ->Property("gradientSampler", BehaviorValueProperty(&ScaleModifierConfig::m_gradientSampler))
                ;
        }
    }

    void ScaleModifierComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationModifierService"));
        services.push_back(AZ_CRC_CE("VegetationScaleModifierService"));
    }

    void ScaleModifierComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationScaleModifierService"));
    }

    void ScaleModifierComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void ScaleModifierComponent::Reflect(AZ::ReflectContext* context)
    {
        ScaleModifierConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ScaleModifierComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &ScaleModifierComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("ScaleModifierComponentTypeId", BehaviorConstant(ScaleModifierComponentTypeId));

            behaviorContext->Class<ScaleModifierComponent>()->RequestBus("ScaleModifierRequestBus");

            behaviorContext->EBus<ScaleModifierRequestBus>("ScaleModifierRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAllowOverrides", &ScaleModifierRequestBus::Events::GetAllowOverrides)
                ->Event("SetAllowOverrides", &ScaleModifierRequestBus::Events::SetAllowOverrides)
                ->VirtualProperty("AllowOverrides", "GetAllowOverrides", "SetAllowOverrides")
                ->Event("GetRangeMin", &ScaleModifierRequestBus::Events::GetRangeMin)
                ->Event("SetRangeMin", &ScaleModifierRequestBus::Events::SetRangeMin)
                ->VirtualProperty("RangeMin", "GetRangeMin", "SetRangeMin")
                ->Event("GetRangeMax", &ScaleModifierRequestBus::Events::GetRangeMax)
                ->Event("SetRangeMax", &ScaleModifierRequestBus::Events::SetRangeMax)
                ->VirtualProperty("RangeMax", "GetRangeMax", "SetRangeMax")
                ->Event("GetGradientSampler", &ScaleModifierRequestBus::Events::GetGradientSampler)
                ;
        }
    }

    ScaleModifierComponent::ScaleModifierComponent(const ScaleModifierConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void ScaleModifierComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependencies({ m_configuration.m_gradientSampler.m_gradientId });
        ModifierRequestBus::Handler::BusConnect(GetEntityId());
        ScaleModifierRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ScaleModifierComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        ModifierRequestBus::Handler::BusDisconnect();
        ScaleModifierRequestBus::Handler::BusDisconnect();
    }

    bool ScaleModifierComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const ScaleModifierConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool ScaleModifierComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<ScaleModifierConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void ScaleModifierComponent::Execute(InstanceData& instanceData) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        const GradientSignal::GradientSampleParams sampleParams(instanceData.m_position);
        float factor = m_configuration.m_gradientSampler.GetValue(sampleParams);

        const bool useOverrides = m_configuration.m_allowOverrides && instanceData.m_descriptorPtr && instanceData.m_descriptorPtr->m_scaleOverrideEnabled;
        const float min = useOverrides ? instanceData.m_descriptorPtr->m_scaleMin : m_configuration.m_rangeMin;
        const float max = useOverrides ? instanceData.m_descriptorPtr->m_scaleMax : m_configuration.m_rangeMax;
        instanceData.m_scale = AZ::GetMax(instanceData.m_scale * (factor * (max - min) + min), 0.01f);
    }

    bool ScaleModifierComponent::GetAllowOverrides() const
    {
        return m_configuration.m_allowOverrides;
    }

    void ScaleModifierComponent::SetAllowOverrides(bool value)
    {
        m_configuration.m_allowOverrides = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float ScaleModifierComponent::GetRangeMin() const
    {
        return m_configuration.m_rangeMin;
    }

    void ScaleModifierComponent::SetRangeMin(float rangeMin)
    {
        m_configuration.m_rangeMin = rangeMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float ScaleModifierComponent::GetRangeMax() const
    {
        return m_configuration.m_rangeMax;
    }

    void ScaleModifierComponent::SetRangeMax(float rangeMax)
    {
        m_configuration.m_rangeMax = rangeMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSignal::GradientSampler& ScaleModifierComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
