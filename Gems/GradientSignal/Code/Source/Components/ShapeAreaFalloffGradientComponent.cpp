/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShapeAreaFalloffGradientComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Debug/Profiler.h>

namespace GradientSignal
{
    void ShapeAreaFalloffGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ShapeAreaFalloffGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ShapeEntityId", &ShapeAreaFalloffGradientConfig::m_shapeEntityId)
                ->Field("FalloffWidth", &ShapeAreaFalloffGradientConfig::m_falloffWidth)
                ->Field("FalloffType", &ShapeAreaFalloffGradientConfig::m_falloffType)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ShapeAreaFalloffGradientConfig>(
                    "Shape Falloff Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &ShapeAreaFalloffGradientConfig::m_shapeEntityId, "Shape Entity Id", "Entity with shape component to test distance against.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ShapeService", 0xe86aa5fe))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ShapeAreaFalloffGradientConfig::m_falloffWidth, "Falloff Width", "Maximum distance used to calculate gradient in meters.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->DataElement(0, &ShapeAreaFalloffGradientConfig::m_falloffType, "Falloff Type", "Function for calculating falloff")

                    // Inner falloff isn't supported yet.
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                    //->EnumAttribute(ShapeAreaFalloffGradientConfig::FalloffType::Inner, "Inner")
                    ->EnumAttribute(FalloffType::Outer, "Outer")
                    //->EnumAttribute(ShapeAreaFalloffGradientConfig::FalloffType::InnerOuter, "InnerOuter")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ShapeAreaFalloffGradientConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("shapeEntityId", BehaviorValueProperty(&ShapeAreaFalloffGradientConfig::m_shapeEntityId))
                ->Property("falloffWidth", BehaviorValueProperty(&ShapeAreaFalloffGradientConfig::m_falloffWidth))
                ->Property("falloffType",
                    [](ShapeAreaFalloffGradientConfig* config) { return (AZ::u8&)(config->m_falloffType); },
                    [](ShapeAreaFalloffGradientConfig* config, const AZ::u8& i) { config->m_falloffType = (FalloffType)i; })
                ;
        }
    }

    void ShapeAreaFalloffGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void ShapeAreaFalloffGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void ShapeAreaFalloffGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void ShapeAreaFalloffGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        ShapeAreaFalloffGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ShapeAreaFalloffGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &ShapeAreaFalloffGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("ShapeAreaFalloffGradientComponentTypeId", BehaviorConstant(ShapeAreaFalloffGradientComponentTypeId));

            behaviorContext->Class<ShapeAreaFalloffGradientComponent>()->RequestBus("ShapeAreaFalloffGradientRequestBus");

            behaviorContext->EBus<ShapeAreaFalloffGradientRequestBus>("ShapeAreaFalloffGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetShapeEntityId", &ShapeAreaFalloffGradientRequestBus::Events::GetShapeEntityId)
                ->Event("SetShapeEntityId", &ShapeAreaFalloffGradientRequestBus::Events::SetShapeEntityId)
                ->VirtualProperty("ShapeEntityId", "GetShapeEntityId", "SetShapeEntityId")
                ->Event("GetFalloffWidth", &ShapeAreaFalloffGradientRequestBus::Events::GetFalloffWidth)
                ->Event("SetFalloffWidth", &ShapeAreaFalloffGradientRequestBus::Events::SetFalloffWidth)
                ->VirtualProperty("FalloffWidth", "GetFalloffWidth", "SetFalloffWidth")
                ->Event("GetFalloffType", &ShapeAreaFalloffGradientRequestBus::Events::GetFalloffType)
                ->Event("SetFalloffType", &ShapeAreaFalloffGradientRequestBus::Events::SetFalloffType)
                ->VirtualProperty("FalloffType", "GetFalloffType", "SetFalloffType")
                ;
        }
    }

    ShapeAreaFalloffGradientComponent::ShapeAreaFalloffGradientComponent(const ShapeAreaFalloffGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void ShapeAreaFalloffGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_shapeEntityId);
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        ShapeAreaFalloffGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ShapeAreaFalloffGradientComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        GradientRequestBus::Handler::BusDisconnect();
        ShapeAreaFalloffGradientRequestBus::Handler::BusDisconnect();
    }

    bool ShapeAreaFalloffGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const ShapeAreaFalloffGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool ShapeAreaFalloffGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<ShapeAreaFalloffGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float ShapeAreaFalloffGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        float distance = 0.0f;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceFromPoint, sampleParams.m_position);

        // In the special case of 0 falloff, make sure that all points inside the shape (0 distance) return 
        // 1.0, and all points outside the shape return 0.
        if (m_configuration.m_falloffWidth == 0.0f)
        {
            return (distance > 0.0f) ? 0.0f : 1.0f;
        }

        // Since this is outer falloff, distance should give us values from 1.0 at the minimum distance
        // to 0.0 at the maximum distance.
        return GetRatio(m_configuration.m_falloffWidth, 0.0f, distance);
    }

    AZ::EntityId ShapeAreaFalloffGradientComponent::GetShapeEntityId() const
    {
        return m_configuration.m_shapeEntityId;
    }

    void ShapeAreaFalloffGradientComponent::SetShapeEntityId(AZ::EntityId entityId)
    {
        m_configuration.m_shapeEntityId = entityId;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float ShapeAreaFalloffGradientComponent::GetFalloffWidth() const
    {
        return m_configuration.m_falloffWidth;
    }

    void ShapeAreaFalloffGradientComponent::SetFalloffWidth(float falloffWidth)
    {
        m_configuration.m_falloffWidth = falloffWidth;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    FalloffType ShapeAreaFalloffGradientComponent::GetFalloffType() const
    {
        return m_configuration.m_falloffType;
    }

    void ShapeAreaFalloffGradientComponent::SetFalloffType(FalloffType type)
    {
        m_configuration.m_falloffType = type;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
