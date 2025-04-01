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
#include <TerrainProfiler.h>

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
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<TerrainSurfaceGradientMapping>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Constructor()
                ->Property("GradientEntityId", BehaviorValueProperty(&TerrainSurfaceGradientMapping::m_gradientEntityId))
                ->Property("SurfaceTag", BehaviorValueProperty(&TerrainSurfaceGradientMapping::m_surfaceTag));
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
        services.push_back(AZ_CRC_CE("TerrainAreaService"));
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

        // Make sure we get update notifications whenever this entity or any dependent gradient entity changes in any way.
        // We'll use that to notify the terrain system that the surface information needs to be refreshed.
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.SetRegionChangedEntityNotificationFunction();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(GetEntityId());

        for (auto& surfaceMapping : m_configuration.m_gradientSurfaceMappings)
        {
            if (surfaceMapping.m_gradientEntityId != GetEntityId())
            {
                m_dependencyMonitor.ConnectDependency(surfaceMapping.m_gradientEntityId);
            }
        }

        Terrain::TerrainAreaSurfaceRequestBus::Handler::BusConnect(GetEntityId());

        // Notify that the area has changed.
        OnCompositionChanged();
    }

    void TerrainSurfaceGradientListComponent::Deactivate()
    {
        // Disconnect before doing any other teardown. This will guarantee that any active queries have finished before we proceed.
        Terrain::TerrainAreaSurfaceRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        // Since this surface data will no longer exist, notify the terrain system to refresh the area.
        TerrainSystemServiceRequestBus::Broadcast(
            &TerrainSystemServiceRequestBus::Events::RefreshArea, GetEntityId(),
            AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::SurfaceData);
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

        if (Terrain::TerrainAreaSurfaceRequestBus::HasReentrantEBusUseThisThread())
        {
            AZ_ErrorOnce("Terrain", false, "Detected cyclic dependencies with terrain surface entity references on entity '%s' (%s)",
                GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());
            return;
        }

        const GradientSignal::GradientSampleParams params(inPosition);

        for (const auto& mapping : m_configuration.m_gradientSurfaceMappings)
        {
            float weight = 0.0f;
            GradientSignal::GradientRequestBus::EventResult(weight,
                mapping.m_gradientEntityId, &GradientSignal::GradientRequestBus::Events::GetValue, params);

            outSurfaceWeights.emplace_back(mapping.m_surfaceTag, weight);
        }
    }

    void TerrainSurfaceGradientListComponent::GetSurfaceWeightsFromList(
        AZStd::span<const AZ::Vector3> inPositionList,
        AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList) const
    {
        TERRAIN_PROFILE_FUNCTION_VERBOSE

        AZ_Assert(
            inPositionList.size() == outSurfaceWeightsList.size(), "The position list size doesn't match the outSurfaceWeights list size.");

        if (Terrain::TerrainAreaSurfaceRequestBus::HasReentrantEBusUseThisThread())
        {
            AZ_ErrorOnce(
                "Terrain", false, "Detected cyclic dependencies with terrain surface entity references on entity '%s' (%s)",
                GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());
            return;
        }

        AZStd::vector<float> gradientValues;

        gradientValues.resize_no_construct(inPositionList.size());

        for (const auto& mapping : m_configuration.m_gradientSurfaceMappings)
        {
            // Clear out the gradient values before every GetValues call to ensure we don't accidentally end up with stale data.
            AZStd::fill(gradientValues.begin(), gradientValues.end(), 0.0f);

            GradientSignal::GradientRequestBus::Event(
                mapping.m_gradientEntityId, &GradientSignal::GradientRequestBus::Events::GetValues, inPositionList, gradientValues);

            for (size_t index = 0; index < outSurfaceWeightsList.size(); index++)
            {
                outSurfaceWeightsList[index].emplace_back(mapping.m_surfaceTag, gradientValues[index]);
            }
        }
    }

    void TerrainSurfaceGradientListComponent::OnCompositionChanged()
    {
        OnCompositionRegionChanged(AZ::Aabb::CreateNull());
    }

    void TerrainSurfaceGradientListComponent::OnCompositionRegionChanged(const AZ::Aabb& dirtyRegion)
    {
        if (dirtyRegion.IsValid())
        {
            TerrainSystemServiceRequestBus::Broadcast(
                &TerrainSystemServiceRequestBus::Events::RefreshRegion,
                dirtyRegion,
                AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::SurfaceData);
        }
        else
        {
            TerrainSystemServiceRequestBus::Broadcast(
                &TerrainSystemServiceRequestBus::Events::RefreshArea,
                GetEntityId(),
                AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::SurfaceData);
        }
    }

} // namespace Terrain
