/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainSurfaceGradientListComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <TerrainSystem/TerrainSystemBus.h>

namespace Terrain
{
    void TerrainSurfaceGradientMapping::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainSurfaceGradientMapping>()
                ->Version(1)
                ->Field("Gradient Entity", &TerrainSurfaceGradientMapping::m_gradientEntityId)
                ->Field("Surface Tag", &TerrainSurfaceGradientMapping::m_surfaceTag)
            ;

            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<TerrainSurfaceGradientMapping>("Terrain Surface Gradient Mapping", "Mapping between a gradient and a surface.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientMapping::m_gradientEntityId,
                        "Gradient Entity", "ID of Entity providing a gradient.")
                       ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->UIElement("GradientPreviewer", "Previewer")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientEntity"), &TerrainSurfaceGradientMapping::m_gradientEntityId)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientMapping::m_surfaceTag, "Surface Tag",
                        "Surface type to map to this gradient.")
                ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<TerrainSurfaceGradientMapping>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Constructor()
                ->Property("gradientEntityId", BehaviorValueProperty(&TerrainSurfaceGradientMapping::m_gradientEntityId))
                ->Property("surfaceTag", BehaviorValueProperty(&TerrainSurfaceGradientMapping::m_surfaceTag));
        }
    }

    void TerrainSurfaceGradientListConfig::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceGradientMapping::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceGradientListConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Mappings", &TerrainSurfaceGradientListConfig::m_gradientSurfaceMappings)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainSurfaceGradientListConfig>(
                    "Terrain Surface Gradient List Component", "Provide mapping between gradients and surfaces.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientListConfig::m_gradientSurfaceMappings,
                        "Gradient to Surface Mappings", "Maps Gradient Entities to Surfaces.")
                    ;
            }
        }
    }

    void TerrainSurfaceGradientListComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainSurfaceProviderService"));
    }

    void TerrainSurfaceGradientListComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainSurfaceProviderService"));
    }

    void TerrainSurfaceGradientListComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainAreaService"));
    }

    void TerrainSurfaceGradientListComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceGradientListConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceGradientListComponent, AZ::Component>()
                ->Version(0)->Field("Configuration", &TerrainSurfaceGradientListComponent::m_configuration)
            ;
        }
    }

    TerrainSurfaceGradientListComponent::TerrainSurfaceGradientListComponent(const TerrainSurfaceGradientListConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainSurfaceGradientListComponent::Activate()
    {
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        Terrain::TerrainAreaSurfaceRequestBus::Handler::BusConnect(GetEntityId());

        // Make sure we get update notifications whenever this entity or any dependent gradient entity changes in any way.
        // We'll use that to notify the terrain system that the surface information needs to be refreshed.
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(GetEntityId());

        for (auto& surfaceMapping : m_configuration.m_gradientSurfaceMappings)
        {
            if (surfaceMapping.m_gradientEntityId != GetEntityId())
            {
                m_dependencyMonitor.ConnectDependency(surfaceMapping.m_gradientEntityId);
            }
        }

        // Notify that the area has changed.
        OnCompositionChanged();
    }

    void TerrainSurfaceGradientListComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();

        Terrain::TerrainAreaSurfaceRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        // Since this surface data will no longer exist, notify the terrain system to refresh the area.
        OnCompositionChanged();
    }

    bool TerrainSurfaceGradientListComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainSurfaceGradientListConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainSurfaceGradientListComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainSurfaceGradientListConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }
    
    void TerrainSurfaceGradientListComponent::GetSurfaceWeights(
        const AZ::Vector3& inPosition,
        AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights) const
    {
        outSurfaceWeights.clear();

        const GradientSignal::GradientSampleParams params(AZ::Vector3(inPosition.GetX(), inPosition.GetY(), 0.0f));

        for (const auto& mapping : m_configuration.m_gradientSurfaceMappings)
        {
            float weight = 0.0f;
            GradientSignal::GradientRequestBus::EventResult(weight,
                mapping.m_gradientEntityId, &GradientSignal::GradientRequestBus::Events::GetValue, params);

            outSurfaceWeights.emplace_back(mapping.m_surfaceTag, weight);
        }
    }

    void TerrainSurfaceGradientListComponent::OnCompositionChanged()
    {
        TerrainSystemServiceRequestBus::Broadcast(
            &TerrainSystemServiceRequestBus::Events::RefreshArea, GetEntityId(),
            AzFramework::Terrain::TerrainDataNotifications::SurfaceData);
    }

} // namespace Terrain
