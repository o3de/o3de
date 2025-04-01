/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "SurfaceSlopeFilterComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/Descriptor.h>
#include <Vegetation/InstanceData.h>
#include <AzCore/Debug/Profiler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

#include <Vegetation/Ebuses/DebugNotificationBus.h>

namespace Vegetation
{
    void SurfaceSlopeFilterConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceSlopeFilterConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("FilterStage", &SurfaceSlopeFilterConfig::m_filterStage)
                ->Field("AllowOverrides", &SurfaceSlopeFilterConfig::m_allowOverrides)
                ->Field("SlopeMin", &SurfaceSlopeFilterConfig::m_slopeMin)
                ->Field("SlopeMax", &SurfaceSlopeFilterConfig::m_slopeMax)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceSlopeFilterConfig>(
                    "Vegetation Slope Filter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SurfaceSlopeFilterConfig::m_filterStage, "Filter Stage", "Determines if filter is applied before (PreProcess) or after (PostProcess) modifiers.")
                    ->EnumAttribute(FilterStage::Default, "Default")
                    ->EnumAttribute(FilterStage::PreProcess, "PreProcess")
                    ->EnumAttribute(FilterStage::PostProcess, "PostProcess")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SurfaceSlopeFilterConfig::m_allowOverrides, "Allow Per-Item Overrides", "Allow per-descriptor parameters to override component parameters.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SurfaceSlopeFilterConfig::m_slopeMin, "Slope Min", "Minimum surface slope angle in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SurfaceSlopeFilterConfig::m_slopeMax, "Slope Max", "Maximum surface slope angle in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceSlopeFilterConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("filterStage",
                    [](SurfaceSlopeFilterConfig* config) { return (AZ::u8&)(config->m_filterStage); },
                    [](SurfaceSlopeFilterConfig* config, const AZ::u8& i) { config->m_filterStage = (FilterStage)i; })
                ->Property("allowOverrides", BehaviorValueProperty(&SurfaceSlopeFilterConfig::m_allowOverrides))
                ->Property("slopeMin", BehaviorValueProperty(&SurfaceSlopeFilterConfig::m_slopeMin))
                ->Property("slopeMax", BehaviorValueProperty(&SurfaceSlopeFilterConfig::m_slopeMax))
                ;
        }
    }

    void SurfaceSlopeFilterComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationFilterService"));
        services.push_back(AZ_CRC_CE("VegetationSurfaceSlopeFilterService"));
    }

    void SurfaceSlopeFilterComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationSurfaceSlopeFilterService"));
    }

    void SurfaceSlopeFilterComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void SurfaceSlopeFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceSlopeFilterConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceSlopeFilterComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceSlopeFilterComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceSlopeFilterComponentTypeId", BehaviorConstant(SurfaceSlopeFilterComponentTypeId));

            behaviorContext->Class<SurfaceSlopeFilterComponent>()->RequestBus("SurfaceSlopeFilterRequestBus");

            behaviorContext->EBus<SurfaceSlopeFilterRequestBus>("SurfaceSlopeFilterRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAllowOverrides", &SurfaceSlopeFilterRequestBus::Events::GetAllowOverrides)
                ->Event("SetAllowOverrides", &SurfaceSlopeFilterRequestBus::Events::SetAllowOverrides)
                ->VirtualProperty("AllowOverrides", "GetAllowOverrides", "SetAllowOverrides")
                ->Event("GetSlopeMin", &SurfaceSlopeFilterRequestBus::Events::GetSlopeMin)
                ->Event("SetSlopeMin", &SurfaceSlopeFilterRequestBus::Events::SetSlopeMin)
                ->VirtualProperty("SlopeMin", "GetSlopeMin", "SetSlopeMin")
                ->Event("GetSlopeMax", &SurfaceSlopeFilterRequestBus::Events::GetSlopeMax)
                ->Event("SetSlopeMax", &SurfaceSlopeFilterRequestBus::Events::SetSlopeMax)
                ->VirtualProperty("SlopeMax", "GetSlopeMax", "SetSlopeMax")
                ;
        }
    }

    SurfaceSlopeFilterComponent::SurfaceSlopeFilterComponent(const SurfaceSlopeFilterConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceSlopeFilterComponent::Activate()
    {
        FilterRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceSlopeFilterRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SurfaceSlopeFilterComponent::Deactivate()
    {
        FilterRequestBus::Handler::BusDisconnect();
        SurfaceSlopeFilterRequestBus::Handler::BusDisconnect();
    }

    bool SurfaceSlopeFilterComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceSlopeFilterConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceSlopeFilterComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceSlopeFilterConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool SurfaceSlopeFilterComponent::Evaluate(const InstanceData& instanceData) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        const bool useOverrides = m_configuration.m_allowOverrides && instanceData.m_descriptorPtr && instanceData.m_descriptorPtr->m_slopeFilterOverrideEnabled;
        const float min = useOverrides ? instanceData.m_descriptorPtr->m_slopeFilterMin : m_configuration.m_slopeMin;
        const float max = useOverrides ? instanceData.m_descriptorPtr->m_slopeFilterMax : m_configuration.m_slopeMax;

        const AZ::Vector3 up = AZ::Vector3(0.0f, 0.0f, 1.0f);
        const float c = instanceData.m_normal.Dot(up);
        const float c1 = cosf(AZ::DegToRad(AZ::GetMin(min, max)));
        const float c2 = cosf(AZ::DegToRad(AZ::GetMax(min, max)));
        const bool result = (c <= c1) && (c >= c2);

        if (!result)
        {
            VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FilterInstance, instanceData.m_id, AZStd::string_view("SurfaceSlopeFilter")));
        }
        return result;
    }

    FilterStage SurfaceSlopeFilterComponent::GetFilterStage() const
    {
        return m_configuration.m_filterStage;
    }

    void SurfaceSlopeFilterComponent::SetFilterStage(FilterStage filterStage)
    {
        m_configuration.m_filterStage = filterStage;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool SurfaceSlopeFilterComponent::GetAllowOverrides() const
    {
        return m_configuration.m_allowOverrides;
    }

    void SurfaceSlopeFilterComponent::SetAllowOverrides(bool value)
    {
        m_configuration.m_allowOverrides = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceSlopeFilterComponent::GetSlopeMin() const
    {
        return m_configuration.m_slopeMin;
    }

    void SurfaceSlopeFilterComponent::SetSlopeMin(float slopeMin)
    {
        m_configuration.m_slopeMin = slopeMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceSlopeFilterComponent::GetSlopeMax() const
    {
        return m_configuration.m_slopeMax;
    }

    void SurfaceSlopeFilterComponent::SetSlopeMax(float slopeMax)
    {
        m_configuration.m_slopeMax = slopeMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
