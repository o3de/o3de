/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SlopeAlignmentModifierComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/Descriptor.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <Vegetation/InstanceData.h>
#include <AzCore/Debug/Profiler.h>

#include <VegetationProfiler.h>

namespace Vegetation
{
    void SlopeAlignmentModifierConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SlopeAlignmentModifierConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("AllowOverrides", &SlopeAlignmentModifierConfig::m_allowOverrides)
                ->Field("RangeMin", &SlopeAlignmentModifierConfig::m_rangeMin)
                ->Field("RangeMax", &SlopeAlignmentModifierConfig::m_rangeMax)
                ->Field("Gradient", &SlopeAlignmentModifierConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SlopeAlignmentModifierConfig>(
                    "Vegetation Slope Alignment Modifier", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SlopeAlignmentModifierConfig::m_allowOverrides, "Allow Per-Item Overrides", "Allow per-descriptor parameters to override component parameters.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SlopeAlignmentModifierConfig::m_rangeMin, "Alignment Coefficient Min", "Minimum slope alignment coefficient.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SlopeAlignmentModifierConfig::m_rangeMax, "Alignment Coefficient Max", "Maximum slope alignment coefficient.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &SlopeAlignmentModifierConfig::m_gradientSampler, "Gradient", "Gradient used as blend factor to lerp between ranges.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SlopeAlignmentModifierConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("allowOverrides", BehaviorValueProperty(&SlopeAlignmentModifierConfig::m_allowOverrides))
                ->Property("rangeMin", BehaviorValueProperty(&SlopeAlignmentModifierConfig::m_rangeMin))
                ->Property("rangeMax", BehaviorValueProperty(&SlopeAlignmentModifierConfig::m_rangeMax))
                ->Property("gradientSampler", BehaviorValueProperty(&SlopeAlignmentModifierConfig::m_gradientSampler))
                ;
        }
    }

    void SlopeAlignmentModifierComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationModifierService"));
        services.push_back(AZ_CRC_CE("VegetationAlignmentModifierService"));
    }

    void SlopeAlignmentModifierComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAlignmentModifierService"));
    }

    void SlopeAlignmentModifierComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void SlopeAlignmentModifierComponent::Reflect(AZ::ReflectContext* context)
    {
        SlopeAlignmentModifierConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SlopeAlignmentModifierComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SlopeAlignmentModifierComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SlopeAlignmentModifierComponentTypeId", BehaviorConstant(SlopeAlignmentModifierComponentTypeId));

            behaviorContext->Class<SlopeAlignmentModifierComponent>()->RequestBus("SlopeAlignmentModifierRequestBus");

            behaviorContext->EBus<SlopeAlignmentModifierRequestBus>("SlopeAlignmentModifierRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAllowOverrides", &SlopeAlignmentModifierRequestBus::Events::GetAllowOverrides)
                ->Event("SetAllowOverrides", &SlopeAlignmentModifierRequestBus::Events::SetAllowOverrides)
                ->VirtualProperty("AllowOverrides", "GetAllowOverrides", "SetAllowOverrides")
                ->Event("GetRangeMin", &SlopeAlignmentModifierRequestBus::Events::GetRangeMin)
                ->Event("SetRangeMin", &SlopeAlignmentModifierRequestBus::Events::SetRangeMin)
                ->VirtualProperty("RangeMin", "GetRangeMin", "SetRangeMin")
                ->Event("GetRangeMax", &SlopeAlignmentModifierRequestBus::Events::GetRangeMax)
                ->Event("SetRangeMax", &SlopeAlignmentModifierRequestBus::Events::SetRangeMax)
                ->VirtualProperty("RangeMax", "GetRangeMax", "SetRangeMax")
                ->Event("GetGradientSampler", &SlopeAlignmentModifierRequestBus::Events::GetGradientSampler)
                ;
        }
    }

    SlopeAlignmentModifierComponent::SlopeAlignmentModifierComponent(const SlopeAlignmentModifierConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SlopeAlignmentModifierComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependencies({ m_configuration.m_gradientSampler.m_gradientId });
        ModifierRequestBus::Handler::BusConnect(GetEntityId());
        SlopeAlignmentModifierRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SlopeAlignmentModifierComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        ModifierRequestBus::Handler::BusDisconnect();
        SlopeAlignmentModifierRequestBus::Handler::BusDisconnect();
    }

    bool SlopeAlignmentModifierComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SlopeAlignmentModifierConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SlopeAlignmentModifierComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SlopeAlignmentModifierConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void SlopeAlignmentModifierComponent::Execute(InstanceData& instanceData) const
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        const bool useOverrides = m_configuration.m_allowOverrides && instanceData.m_descriptorPtr && instanceData.m_descriptorPtr->m_surfaceAlignmentOverrideEnabled;
        const float min = useOverrides ? instanceData.m_descriptorPtr->m_surfaceAlignmentMin : m_configuration.m_rangeMin;
        const float max = useOverrides ? instanceData.m_descriptorPtr->m_surfaceAlignmentMax : m_configuration.m_rangeMax;

        const GradientSignal::GradientSampleParams sampleParams(instanceData.m_position);
        const float factor = m_configuration.m_gradientSampler.GetValue(sampleParams) * (max - min) + min;

        AZ::Vector3 r = AZ::Vector3(-1.0f, 0.0f, 0.0f);
        AZ::Vector3 f = AZ::Vector3(0.0f, 1.0f, 0.0f);
        AZ::Vector3 u = AZ::Vector3(0.0f, 0.0f, 1.0f);

        u = u.Lerp(instanceData.m_normal, factor);
        u.Normalize();
        f = r.Cross(u);
        f.Normalize();
        r = f.Cross(u);
        r.Normalize();

        instanceData.m_alignment = AZ::Quaternion::CreateFromMatrix3x3(AZ::Matrix3x3::CreateFromColumns(r, f, u));
    }


    bool SlopeAlignmentModifierComponent::GetAllowOverrides() const
    {
        return m_configuration.m_allowOverrides;
    }

    void SlopeAlignmentModifierComponent::SetAllowOverrides(bool value)
    {
        m_configuration.m_allowOverrides = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SlopeAlignmentModifierComponent::GetRangeMin() const
    {
        return m_configuration.m_rangeMin;
    }

    void SlopeAlignmentModifierComponent::SetRangeMin(float rangeMin)
    {
        m_configuration.m_rangeMin = rangeMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SlopeAlignmentModifierComponent::GetRangeMax() const
    {
        return m_configuration.m_rangeMax;
    }

    void SlopeAlignmentModifierComponent::SetRangeMax(float rangeMax)
    {
        m_configuration.m_rangeMax = rangeMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSignal::GradientSampler& SlopeAlignmentModifierComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
