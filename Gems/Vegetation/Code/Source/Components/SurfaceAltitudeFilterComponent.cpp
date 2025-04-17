/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "SurfaceAltitudeFilterComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Vegetation/Descriptor.h>
#include <Vegetation/InstanceData.h>
#include <AzCore/Debug/Profiler.h>

#include <Vegetation/Ebuses/DebugNotificationBus.h>

namespace Vegetation
{
    void SurfaceAltitudeFilterConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceAltitudeFilterConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("FilterStage", &SurfaceAltitudeFilterConfig::m_filterStage)
                ->Field("AllowOverrides", &SurfaceAltitudeFilterConfig::m_allowOverrides)
                ->Field("ShapeEntityId", &SurfaceAltitudeFilterConfig::m_shapeEntityId)
                ->Field("AltitudeMin", &SurfaceAltitudeFilterConfig::m_altitudeMin)
                ->Field("AltitudeMax", &SurfaceAltitudeFilterConfig::m_altitudeMax)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceAltitudeFilterConfig>(
                    "Vegetation Altitude Filter", "Vegetation altitude filter")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SurfaceAltitudeFilterConfig::m_filterStage, "Filter Stage", "Determines if filter is applied before (PreProcess) or after (PostProcess) modifiers.")
                    ->EnumAttribute(FilterStage::Default, "Default")
                    ->EnumAttribute(FilterStage::PreProcess, "PreProcess")
                    ->EnumAttribute(FilterStage::PostProcess, "PostProcess")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SurfaceAltitudeFilterConfig::m_allowOverrides, "Allow Per-Item Overrides", "Allow per-descriptor parameters to override component parameters.")
                    ->DataElement(0, &SurfaceAltitudeFilterConfig::m_shapeEntityId, "Pin To Shape Entity Id", "Shape bounds override min/max altitude if specified.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("ShapeService"))
                    ->DataElement(0, &SurfaceAltitudeFilterConfig::m_altitudeMin, "Altitude Min", "Minimum acceptable surface altitude.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &SurfaceAltitudeFilterConfig::IsShapeValid)
                    ->DataElement(0, &SurfaceAltitudeFilterConfig::m_altitudeMax, "Altitude Max", "Maximum acceptable surface altitude.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &SurfaceAltitudeFilterConfig::IsShapeValid)
                    ;
            }

        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceAltitudeFilterConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("filterStage",
                    [](SurfaceAltitudeFilterConfig* config) { return (AZ::u8&)(config->m_filterStage); },
                    [](SurfaceAltitudeFilterConfig* config, const AZ::u8& i) { config->m_filterStage = (FilterStage)i; })
                ->Property("allowOverrides", BehaviorValueProperty(&SurfaceAltitudeFilterConfig::m_allowOverrides))
                ->Property("shapeEntityId", BehaviorValueProperty(&SurfaceAltitudeFilterConfig::m_shapeEntityId))
                ->Property("altitudeMin", BehaviorValueProperty(&SurfaceAltitudeFilterConfig::m_altitudeMin))
                ->Property("altitudeMax", BehaviorValueProperty(&SurfaceAltitudeFilterConfig::m_altitudeMax))
                ;
        }
    }

    bool SurfaceAltitudeFilterConfig::IsShapeValid() const
    {
        return m_shapeEntityId.IsValid();
    }

    void SurfaceAltitudeFilterComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationFilterService"));
        services.push_back(AZ_CRC_CE("VegetationSurfaceAltitudeFilterService"));
    }

    void SurfaceAltitudeFilterComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationSurfaceAltitudeFilterService"));
    }

    void SurfaceAltitudeFilterComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void SurfaceAltitudeFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceAltitudeFilterConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceAltitudeFilterComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceAltitudeFilterComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceAltitudeFilterComponentTypeId", BehaviorConstant(SurfaceAltitudeFilterComponentTypeId));

            behaviorContext->Class<SurfaceAltitudeFilterComponent>()->RequestBus("SurfaceAltitudeFilterRequestBus");

            behaviorContext->EBus<SurfaceAltitudeFilterRequestBus>("SurfaceAltitudeFilterRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAllowOverrides", &SurfaceAltitudeFilterRequestBus::Events::GetAllowOverrides)
                ->Event("SetAllowOverrides", &SurfaceAltitudeFilterRequestBus::Events::SetAllowOverrides)
                ->VirtualProperty("AllowOverrides", "GetAllowOverrides", "SetAllowOverrides")
                ->Event("GetShapeEntityId", &SurfaceAltitudeFilterRequestBus::Events::GetShapeEntityId)
                ->Event("SetShapeEntityId", &SurfaceAltitudeFilterRequestBus::Events::SetShapeEntityId)
                ->VirtualProperty("ShapeEntityId", "GetShapeEntityId", "SetShapeEntityId")
                ->Event("GetAltitudeMin", &SurfaceAltitudeFilterRequestBus::Events::GetAltitudeMin)
                ->Event("SetAltitudeMin", &SurfaceAltitudeFilterRequestBus::Events::SetAltitudeMin)
                ->VirtualProperty("AltitudeMin", "GetAltitudeMin", "SetAltitudeMin")
                ->Event("GetAltitudeMax", &SurfaceAltitudeFilterRequestBus::Events::GetAltitudeMax)
                ->Event("SetAltitudeMax", &SurfaceAltitudeFilterRequestBus::Events::SetAltitudeMax)
                ->VirtualProperty("AltitudeMax", "GetAltitudeMax", "SetAltitudeMax");
        }
    }

    SurfaceAltitudeFilterComponent::SurfaceAltitudeFilterComponent(const SurfaceAltitudeFilterConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceAltitudeFilterComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_shapeEntityId);
        FilterRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceAltitudeFilterRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SurfaceAltitudeFilterComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        FilterRequestBus::Handler::BusDisconnect();
        SurfaceAltitudeFilterRequestBus::Handler::BusDisconnect();
    }

    bool SurfaceAltitudeFilterComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceAltitudeFilterConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceAltitudeFilterComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceAltitudeFilterConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool SurfaceAltitudeFilterComponent::Evaluate(const InstanceData& instanceData) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        const bool useOverrides = m_configuration.m_allowOverrides && instanceData.m_descriptorPtr && instanceData.m_descriptorPtr->m_altitudeFilterOverrideEnabled;
        const float min = useOverrides ? instanceData.m_descriptorPtr->m_altitudeFilterMin : m_configuration.m_altitudeMin;
        const float max = useOverrides ? instanceData.m_descriptorPtr->m_altitudeFilterMax : m_configuration.m_altitudeMax;

        AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(
            AZ::Vector3(-AZ::Constants::FloatMax, -AZ::Constants::FloatMax, AZ::GetMin(min, max)),
            AZ::Vector3(AZ::Constants::FloatMax, AZ::Constants::FloatMax, AZ::GetMax(min, max)));

        LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        const bool result = bounds.IsValid() && (instanceData.m_position.GetZ() >= bounds.GetMin().GetZ()) && (instanceData.m_position.GetZ() <= bounds.GetMax().GetZ());
        if (!result)
        {
            VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FilterInstance, instanceData.m_id, AZStd::string_view("SurfaceAltitudeFilter")));
        }
        return result;
    }

    FilterStage SurfaceAltitudeFilterComponent::GetFilterStage() const
    {
        return m_configuration.m_filterStage;
    }

    void SurfaceAltitudeFilterComponent::SetFilterStage(FilterStage filterStage)
    {
        m_configuration.m_filterStage = filterStage;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool SurfaceAltitudeFilterComponent::GetAllowOverrides() const
    {
        return m_configuration.m_allowOverrides;
    }

    void SurfaceAltitudeFilterComponent::SetAllowOverrides(bool value)
    {
        m_configuration.m_allowOverrides = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::EntityId SurfaceAltitudeFilterComponent::GetShapeEntityId() const
    {
        return m_configuration.m_shapeEntityId;
    }

    void SurfaceAltitudeFilterComponent::SetShapeEntityId(AZ::EntityId shapeEntityId)
    {
        m_configuration.m_shapeEntityId = shapeEntityId;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceAltitudeFilterComponent::GetAltitudeMin() const
    {
        return m_configuration.m_altitudeMin;
    }

    void SurfaceAltitudeFilterComponent::SetAltitudeMin(float altitudeMin)
    {
        m_configuration.m_altitudeMin = altitudeMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceAltitudeFilterComponent::GetAltitudeMax() const
    {
        return m_configuration.m_altitudeMax;
    }

    void SurfaceAltitudeFilterComponent::SetAltitudeMax(float altitudeMax)
    {
        m_configuration.m_altitudeMax = altitudeMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
