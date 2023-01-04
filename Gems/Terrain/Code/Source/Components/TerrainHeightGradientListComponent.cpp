/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainHeightGradientListComponent.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <TerrainProfiler.h>

namespace Terrain
{
    void TerrainHeightGradientListConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainHeightGradientListConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("GradientEntities", &TerrainHeightGradientListConfig::m_gradientEntities)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainHeightGradientListConfig>(
                    "Terrain Height Gradient List Component", "Provide height data for a region of the world")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(0, &TerrainHeightGradientListConfig::m_gradientEntities, "Gradient Entities", "Ordered list of gradients to use as height providers.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("GradientService"))
                ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<TerrainHeightGradientListConfig>()
                    ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                    ->Constructor()
                    ->Property("gradientEntities", BehaviorValueProperty(&TerrainHeightGradientListConfig::m_gradientEntities))
                ;
            }
        }
    }

    void TerrainHeightGradientListComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainHeightProviderService"));
    }

    void TerrainHeightGradientListComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainHeightProviderService"));
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void TerrainHeightGradientListComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainAreaService"));
        services.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void TerrainHeightGradientListComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainHeightGradientListConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainHeightGradientListComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainHeightGradientListComponent::m_configuration)
            ;
        }
    }

    TerrainHeightGradientListComponent::TerrainHeightGradientListComponent(const TerrainHeightGradientListConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainHeightGradientListComponent::Activate()
    {
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        // Make sure we get update notifications whenever this entity or any dependent gradient entity changes in any way.
        // We'll use that to notify the terrain system that the height information needs to be refreshed.
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.SetRegionChangedEntityNotificationFunction();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(GetEntityId());

        for (auto& entityId : m_configuration.m_gradientEntities)
        {
            if (entityId != GetEntityId())
            {
                m_dependencyMonitor.ConnectDependency(entityId);
            }
        }

        Terrain::TerrainAreaHeightRequestBus::Handler::BusConnect(GetEntityId());

        // Cache any height data needed and notify that the area has changed.
        OnCompositionChanged();
    }

    void TerrainHeightGradientListComponent::Deactivate()
    {
        // Disconnect before doing any other teardown. This will guarantee that any active queries have finished before we proceed.
        Terrain::TerrainAreaHeightRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        // Since this height data will no longer exist, notify the terrain system to refresh the area.
        TerrainSystemServiceRequestBus::Broadcast(
            &TerrainSystemServiceRequestBus::Events::RefreshArea, GetEntityId(),
            AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::HeightData);
    }

    bool TerrainHeightGradientListComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainHeightGradientListConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainHeightGradientListComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainHeightGradientListConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainHeightGradientListComponent::GetHeight(
        const AZ::Vector3& inPosition,
        AZ::Vector3& outPosition,
        bool& terrainExists)
    {
        // Make sure we don't run queries simultaneously with changing any of the cached data.
        AZStd::shared_lock lock(m_queryMutex);

        float maxSample = 0.0f;
        terrainExists = false;

        AZ_ErrorOnce(
            "Terrain", !Terrain::TerrainAreaHeightRequestBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with terrain height entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        if (!Terrain::TerrainAreaHeightRequestBus::HasReentrantEBusUseThisThread())
        {
            GradientSignal::GradientSampleParams params(inPosition);

            // Right now, when the list contains multiple entries, we will use the highest point from each gradient.
            // This is needed in part because gradients don't really have world bounds, so they exist everywhere but generally have a value
            // of 0 outside their data bounds if they're using bounded data.  We should examine the possibility of extending the gradient
            // API to provide actual bounds so that it's possible to detect if the gradient even 'exists' in an area, at which point we
            // could just make this list a prioritized list from top to bottom for any points that overlap.
            for (auto& gradientId : m_configuration.m_gradientEntities)
            {
                if (gradientId.IsValid())
                {
                    // If gradients ever provide bounds, or if we add a value threshold in this component, it would be possible for terrain
                    // to *not* exist at a specific point.
                    terrainExists = true;

                    float sample = 0.0f;
                    GradientSignal::GradientRequestBus::EventResult(
                        sample, gradientId, &GradientSignal::GradientRequestBus::Events::GetValue, params);
                    maxSample = AZ::GetMax(maxSample, sample);
                }
            }
        }

        const float height = AZ::Lerp(m_cachedShapeBounds.GetMin().GetZ(), m_cachedShapeBounds.GetMax().GetZ(), maxSample);
        outPosition.Set(inPosition.GetX(), inPosition.GetY(), AZ::GetClamp(height, m_cachedHeightBounds.m_min, m_cachedHeightBounds.m_max));
    }

    void TerrainHeightGradientListComponent::GetHeights(
        AZStd::span<AZ::Vector3> inOutPositionList, AZStd::span<bool> terrainExistsList)
    {
        TERRAIN_PROFILE_FUNCTION_VERBOSE

        // Make sure we don't run queries simultaneously with changing any of the cached data.
        AZStd::shared_lock lock(m_queryMutex);

        AZ_Assert(
            inOutPositionList.size() == terrainExistsList.size(), "The position list size doesn't match the terrainExists list size.");

        AZ_ErrorOnce(
            "Terrain", !Terrain::TerrainAreaHeightRequestBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with terrain height entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        if (!Terrain::TerrainAreaHeightRequestBus::HasReentrantEBusUseThisThread())
        {
            // Start by initializing all our terrainExists flags to false.
            AZStd::fill(terrainExistsList.begin(), terrainExistsList.end(), false);

            // Create a temporary buffer for storing all the gradient values for the currently-queried gradient.
            AZStd::vector<float> curGradientSamples(inOutPositionList.size());

            // Create a temporary buffer for storing all the max gradient values.
            AZStd::vector<float> maxValueSamples(inOutPositionList.size());

            // Right now, when the list contains multiple entries, we will use the highest point from each gradient.
            // This is needed in part because gradients don't really have world bounds, so they exist everywhere but generally have a
            // value of 0 outside their data bounds if they're using bounded data.  We should examine the possibility of extending the
            // gradient API to provide actual bounds so that it's possible to detect if the gradient even 'exists' in an area, at which
            // point we could just make this list a prioritized list from top to bottom for any points that overlap.
            for (auto& gradientId : m_configuration.m_gradientEntities)
            {
                if (gradientId.IsValid())
                {
                    GradientSignal::GradientRequestBus::Event(
                        gradientId, &GradientSignal::GradientRequestBus::Events::GetValues, inOutPositionList, curGradientSamples);

                    for (size_t index = 0; index < maxValueSamples.size(); index++)
                    {
                        maxValueSamples[index] = AZ::GetMax(maxValueSamples[index], curGradientSamples[index]);

                        // If gradients ever provide bounds, or if we add a value threshold in this component, it would be possible for
                        // terrain to *not* exist at a specific point.
                        terrainExistsList[index] = true;
                    }
                }
            }

            for (size_t index = 0; index < inOutPositionList.size(); index++)
            {
                if (terrainExistsList[index])
                {
                    const float height =
                        AZ::Lerp(m_cachedShapeBounds.GetMin().GetZ(), m_cachedShapeBounds.GetMax().GetZ(), maxValueSamples[index]);
                    inOutPositionList[index].SetZ(AZ::GetClamp(height, m_cachedHeightBounds.m_min, m_cachedHeightBounds.m_max));
                }
            }
        }
    }

    void TerrainHeightGradientListComponent::OnCompositionChanged()
    {
        OnCompositionRegionChanged(AZ::Aabb::CreateNull());
    }

    void TerrainHeightGradientListComponent::OnCompositionRegionChanged(const AZ::Aabb& dirtyRegion)
    {
        // We query the shape and world bounds prior to locking the queryMutex to help reduce the chances of deadlocks between
        // threads due to the EBus call mutexes.

        // Get the height range of our height provider based on the shape component.
        AZ::Aabb shapeBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            shapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        // Get the height range of the entire world
        AzFramework::Terrain::FloatRange heightBounds = AzFramework::Terrain::FloatRange::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            heightBounds, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightBounds);

        // Ensure that we only change our cached data and terrain registration status when no queries are actively running.
        {
            AZStd::unique_lock lock(m_queryMutex);

            m_cachedShapeBounds = shapeBounds;

            // Save off the min/max heights so that we don't have to re-query them on every single height query.
            m_cachedHeightBounds = heightBounds;
        }

        // We specifically refresh this outside of the queryMutex lock to avoid lock inversion deadlocks. These can occur if one thread
        // is calling TerrainHeightGradientListComponent::OnCompositionChanged -> TerrainSystem::RefreshArea, and another thread
        // is running a query like TerrainSystem::GetHeights -> TerrainHeightGradientListComponent::GetHeights.
        // It's ok if a query is able to run in-between the cache change and the RefreshArea call, because the RefreshArea should cause
        // the querying system to refresh and achieve eventual consistency.
        if (dirtyRegion.IsValid())
        {
            // Only send a terrain update if the dirty region overlaps the bounds of the terrain spawner
            if (dirtyRegion.Overlaps(shapeBounds))
            {
                AZ::Aabb clampedDirtyRegion = dirtyRegion.GetClamped(shapeBounds);

                TerrainSystemServiceRequestBus::Broadcast(
                    &TerrainSystemServiceRequestBus::Events::RefreshRegion,
                    clampedDirtyRegion,
                    AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::HeightData);
            }
        }
        else
        {
            TerrainSystemServiceRequestBus::Broadcast(
                &TerrainSystemServiceRequestBus::Events::RefreshArea,
                GetEntityId(),
                AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::HeightData);
        }
    }

    void TerrainHeightGradientListComponent::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & TerrainDataChangedMask::Settings) == TerrainDataChangedMask::Settings)
        {
            // If the terrain system settings changed, it's possible that the world bounds have changed, which can affect our height data.
            // Refresh the min/max heights and notify that the height data for this area needs to be refreshed.
            OnCompositionRegionChanged(dirtyRegion);
        }
    }

}
