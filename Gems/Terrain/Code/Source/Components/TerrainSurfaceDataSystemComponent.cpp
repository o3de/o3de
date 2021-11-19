/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainSurfaceDataSystemComponent.h>
#include <AzCore/Debug/Profiler.h> 
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

namespace Terrain
{
    //////////////////////////////////////////////////////////////////////////
    // TerrainSurfaceDataSystemConfig

    void TerrainSurfaceDataSystemConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TerrainSurfaceDataSystemConfig, AZ::ComponentConfig>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TerrainSurfaceDataSystemConfig>(
                    "Terrain Surface Data System", "Configures management of surface data requests against legacy terrain")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // TerrainSurfaceDataSystemComponent
    void TerrainSurfaceDataSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceDataSystemConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceDataSystemComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainSurfaceDataSystemComponent::m_configuration)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<TerrainSurfaceDataSystemComponent>("Terrain Surface Data System", "Manages surface data requests against legacy terrain")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Surface Data")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &TerrainSurfaceDataSystemComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    TerrainSurfaceDataSystemComponent::TerrainSurfaceDataSystemComponent(const TerrainSurfaceDataSystemConfig& configuration)
        : m_configuration(configuration)
    {
    }

    TerrainSurfaceDataSystemComponent::TerrainSurfaceDataSystemComponent()
    {
    }

    void TerrainSurfaceDataSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
        services.push_back(AZ_CRC_CE("TerrainSurfaceDataProviderService"));
    }

    void TerrainSurfaceDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainSurfaceDataProviderService"));
    }

    void TerrainSurfaceDataSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataSystemService"));
    }

    void TerrainSurfaceDataSystemComponent::Activate()
    {
        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        UpdateTerrainData(AZ::Aabb::CreateNull());
    }

    void TerrainSurfaceDataSystemComponent::Deactivate()
    {
        if (m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle)
        {
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
                &SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();

        // Clear the cached terrain bounds data
        {
            m_terrainBounds = AZ::Aabb::CreateNull();
            m_terrainBoundsIsValid = false;
        }
    }

    bool TerrainSurfaceDataSystemComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const TerrainSurfaceDataSystemConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainSurfaceDataSystemComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainSurfaceDataSystemConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainSurfaceDataSystemComponent::GetSurfacePoints(
        const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const
    {
        if (m_terrainBoundsIsValid)
        {
            auto enumerationCallback = [&](AzFramework::Terrain::TerrainDataRequests* terrain) -> bool
            {
                if (terrain->GetTerrainAabb().Contains(inPosition))
                {
                    bool isTerrainValidAtPoint = false;
                    AzFramework::SurfaceData::SurfacePoint terrainSurfacePoint;
                    terrain->GetSurfacePoint(
                        inPosition, terrainSurfacePoint, AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR,
                        &isTerrainValidAtPoint);

                    const bool isHole = !isTerrainValidAtPoint;

                    SurfaceData::SurfacePoint point;
                    point.m_entityId = GetEntityId();
                    point.m_position = terrainSurfacePoint.m_position;
                    point.m_normal = terrainSurfacePoint.m_normal;

                    // Always add a "terrain" or "terrainHole" tag.
                    const AZ::Crc32 terrainTag =
                        isHole ? SurfaceData::Constants::s_terrainHoleTagCrc : SurfaceData::Constants::s_terrainTagCrc;
                    SurfaceData::AddMaxValueForMasks(point.m_masks, terrainTag, 1.0f);

                    // Add all of the surface tags that the terrain has at this point.
                    for (auto& tag : terrainSurfacePoint.m_surfaceTags)
                    {
                        SurfaceData::AddMaxValueForMasks(point.m_masks, tag.m_surfaceType, tag.m_weight);
                    }
                    surfacePointList.push_back(point);
                }
                // Only one handler should exist.
                return false;
            };
            AzFramework::Terrain::TerrainDataRequestBus::EnumerateHandlers(enumerationCallback);
        }
    }

    AZ::Aabb TerrainSurfaceDataSystemComponent::GetSurfaceAabb() const
    {
        auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
        return terrain ? terrain->GetTerrainAabb() : AZ::Aabb::CreateNull();
    }

    SurfaceData::SurfaceTagVector TerrainSurfaceDataSystemComponent::GetSurfaceTags() const
    {
        SurfaceData::SurfaceTagVector tags;
        tags.push_back(SurfaceData::Constants::s_terrainHoleTagCrc);
        tags.push_back(SurfaceData::Constants::s_terrainTagCrc);
        return tags;
    }

    void TerrainSurfaceDataSystemComponent::UpdateTerrainData(const AZ::Aabb& dirtyRegion)
    {
        bool terrainValidBeforeUpdate = m_terrainBoundsIsValid;
        bool terrainValidAfterUpdate = false;
        AZ::Aabb terrainBoundsBeforeUpdate = m_terrainBounds;

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = GetSurfaceAabb();
        registryEntry.m_tags = GetSurfaceTags();

        m_terrainBounds = registryEntry.m_bounds;
        m_terrainBoundsIsValid = m_terrainBounds.IsValid();

        terrainValidAfterUpdate = m_terrainBoundsIsValid;

        if (terrainValidBeforeUpdate && terrainValidAfterUpdate)
        {
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");

            // Our terrain was valid before and after, it just changed in some way.  If we have a valid dirty region passed in
            // then it's possible that the heightmap has been modified in the Editor.  Otherwise, just notify that the entire
            // terrain has changed in some way.
            if (dirtyRegion.IsValid())
            {
                SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
                    &SurfaceData::SurfaceDataSystemRequestBus::Events::RefreshSurfaceData, dirtyRegion);
            }
            else
            {
                SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
                    &SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry);
            }
        }
        else if (!terrainValidBeforeUpdate && terrainValidAfterUpdate)
        {
            // Our terrain has become valid, so register as a provider and save off the registry handles
            AZ_Assert(
                (m_providerHandle == SurfaceData::InvalidSurfaceDataRegistryHandle),
                "Surface Provider data handle is initialized before our terrain became valid");
            SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(
                m_providerHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
        }
        else if (terrainValidBeforeUpdate && !terrainValidAfterUpdate)
        {
            // Our terrain has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
                &SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        }
        else
        {
            // We didn't have a valid terrain before or after running this, so do nothing.
        }

    }

    void TerrainSurfaceDataSystemComponent::OnTerrainDataChanged(
        const AZ::Aabb& dirtyRegion, [[maybe_unused]] TerrainDataChangedMask dataChangedMask)
    {
        UpdateTerrainData(dirtyRegion);
    }
}
