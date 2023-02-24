/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainSurfaceDataSystemComponent.h>
#include <Terrain/TerrainDataConstants.h>
#include <AzCore/Debug/Profiler.h> 
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <SurfaceData/SurfaceDataTagProviderRequestBus.h>

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
        SurfaceData::SurfaceDataTagProviderRequestBus::Handler::BusConnect();

        UpdateTerrainData(AZ::Aabb::CreateNull());
    }

    void TerrainSurfaceDataSystemComponent::Deactivate()
    {
        if (m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle)
        {
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataTagProviderRequestBus::Handler::BusDisconnect();
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
        GetSurfacePointsFromList(AZStd::span<const AZ::Vector3>(&inPosition, 1), surfacePointList);
    }

    void TerrainSurfaceDataSystemComponent::GetSurfacePointsFromList(
        AZStd::span<const AZ::Vector3> inPositions, SurfaceData::SurfacePointList& surfacePointList) const
    {
        if (!m_terrainBoundsIsValid)
        {
            return;
        }

        size_t inPositionIndex = 0;

        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequestBus::Events::QueryList, inPositions,
            AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::All,
            [this, inPositions, &inPositionIndex, &surfacePointList]
                (const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                AZ_Assert(inPositionIndex < inPositions.size(), "Too many points returned from QueryList");

                SurfaceData::SurfaceTagWeights weights(surfacePoint.m_surfaceTags);

                // Always add a "terrain" or "terrainHole" tag.
                const AZ::Crc32 terrainTag = terrainExists ? Constants::s_terrainTagCrc : Constants::s_terrainHoleTagCrc;
                weights.AddSurfaceTagWeight(terrainTag, 1.0f);

                surfacePointList.AddSurfacePoint(
                    GetEntityId(), inPositions[inPositionIndex], surfacePoint.m_position, surfacePoint.m_normal, weights);

                inPositionIndex++;
            },
            AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR);
    }


    AZ::Aabb TerrainSurfaceDataSystemComponent::GetSurfaceAabb() const
    {
        auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
        return terrain ? terrain->GetTerrainAabb() : AZ::Aabb::CreateNull();
    }

    SurfaceData::SurfaceTagVector TerrainSurfaceDataSystemComponent::GetSurfaceTags() const
    {
        SurfaceData::SurfaceTagVector tags;
        tags.push_back(Constants::s_terrainHoleTagCrc);
        tags.push_back(Constants::s_terrainTagCrc);
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
        registryEntry.m_maxPointsCreatedPerInput = 1;

        m_terrainBounds = registryEntry.m_bounds;
        m_terrainBoundsIsValid = m_terrainBounds.IsValid();

        terrainValidAfterUpdate = m_terrainBoundsIsValid;

        if (terrainValidBeforeUpdate && terrainValidAfterUpdate)
        {
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");

            // Our terrain was valid before and after, it just changed in some way.  If we have a valid dirty region, and the terrain
            // bounds themselves haven't changed, just notify that our terrain data has changed within the bounds.  Otherwise, notify
            // that the entire terrain provider needs to be updated, since it either has new bounds or the entire set of data is dirty.
            if (dirtyRegion.IsValid() && m_terrainBounds.IsClose(terrainBoundsBeforeUpdate))
            {
                AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RefreshSurfaceData(m_providerHandle, dirtyRegion);
            }
            else
            {
                AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UpdateSurfaceDataProvider(m_providerHandle, registryEntry);
            }
        }
        else if (!terrainValidBeforeUpdate && terrainValidAfterUpdate)
        {
            // Our terrain has become valid, so register as a provider and save off the registry handles
            AZ_Assert(
                (m_providerHandle == SurfaceData::InvalidSurfaceDataRegistryHandle),
                "Surface Provider data handle is initialized before our terrain became valid");
            m_providerHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataProvider(registryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
        }
        else if (terrainValidBeforeUpdate && !terrainValidAfterUpdate)
        {
            // Our terrain has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
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

    void TerrainSurfaceDataSystemComponent::GetRegisteredSurfaceTagNames(SurfaceData::SurfaceTagNameSet& names) const
    {
        names.insert(Constants::s_terrainHoleTagName);
        names.insert(Constants::s_terrainTagName);
    }
}
