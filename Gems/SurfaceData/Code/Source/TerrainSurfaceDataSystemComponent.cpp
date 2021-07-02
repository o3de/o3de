/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SurfaceData_precompiled.h"
#include "TerrainSurfaceDataSystemComponent.h"
#include <AzCore/Debug/Profiler.h> 
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <MathConversion.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <ISystem.h>

namespace SurfaceData
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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
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
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("TerrainSurfaceDataProviderService", 0xa1ac7717));
    }

    void TerrainSurfaceDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainSurfaceDataProviderService", 0xa1ac7717));
    }

    void TerrainSurfaceDataSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataSystemService", 0x1d44d25f));
    }

    void TerrainSurfaceDataSystemComponent::Activate()
    {
        m_providerHandle = InvalidSurfaceDataRegistryHandle;
        m_system = GetISystem();
        CrySystemEventBus::Handler::BusConnect();
        AZ::HeightmapUpdateNotificationBus::Handler::BusConnect();

        UpdateTerrainData(AZ::Aabb::CreateNull());
    }

    void TerrainSurfaceDataSystemComponent::Deactivate()
    {
        if (m_providerHandle != InvalidSurfaceDataRegistryHandle)
        {
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;
        }

        SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        AZ::HeightmapUpdateNotificationBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        m_system = nullptr;

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

    void TerrainSurfaceDataSystemComponent::OnCrySystemInitialized(ISystem& system, [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
    }

    void TerrainSurfaceDataSystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
        m_system = nullptr;
    }

    void TerrainSurfaceDataSystemComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        if (m_terrainBoundsIsValid)
        {
            auto enumerationCallback = [&](AzFramework::Terrain::TerrainDataRequests* terrain) -> bool
            {
                if (terrain->GetTerrainAabb().Contains(inPosition))
                {
                    bool isTerrainValidAtPoint = false;
                    const float terrainHeight = terrain->GetHeight(inPosition, AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, &isTerrainValidAtPoint);
                    const bool isHole = !isTerrainValidAtPoint;

                    SurfacePoint point;
                    point.m_entityId = GetEntityId();
                    point.m_position = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), terrainHeight);
                    point.m_normal = terrain->GetNormal(inPosition);
                    const AZ::Crc32 terrainTag = isHole ? Constants::s_terrainHoleTagCrc : Constants::s_terrainTagCrc;
                    AddMaxValueForMasks(point.m_masks, terrainTag, 1.0f);
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

    SurfaceTagVector TerrainSurfaceDataSystemComponent::GetSurfaceTags() const
    {
        SurfaceTagVector tags;
        tags.push_back(Constants::s_terrainHoleTagCrc);
        tags.push_back(Constants::s_terrainTagCrc);
        return tags;
    }

    void TerrainSurfaceDataSystemComponent::UpdateTerrainData(const AZ::Aabb& dirtyRegion)
    {
        bool terrainValidBeforeUpdate = m_terrainBoundsIsValid;
        bool terrainValidAfterUpdate = false;
        AZ::Aabb terrainBoundsBeforeUpdate = m_terrainBounds;

        SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = GetSurfaceAabb();
        registryEntry.m_tags = GetSurfaceTags();

        m_terrainBounds = registryEntry.m_bounds;
        m_terrainBoundsIsValid = m_terrainBounds.IsValid();

        terrainValidAfterUpdate = m_terrainBoundsIsValid;

        if (terrainValidBeforeUpdate && terrainValidAfterUpdate)
        {
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");

            // Our terrain was valid before and after, it just changed in some way.  If we have a valid dirty region passed in
            // then it's possible that the heightmap has been modified in the Editor.  Otherwise, just notify that the entire
            // terrain has changed in some way.
            if (dirtyRegion.IsValid())
            {
                SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::RefreshSurfaceData, dirtyRegion);
            }
            else
            {
                SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry);
            }
        }
        else if (!terrainValidBeforeUpdate && terrainValidAfterUpdate)
        {
            // Our terrain has become valid, so register as a provider and save off the registry handles
            AZ_Assert((m_providerHandle == InvalidSurfaceDataRegistryHandle), "Surface Provider data handle is initialized before our terrain became valid");
            SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
        }
        else if (terrainValidBeforeUpdate && !terrainValidAfterUpdate)
        {
            // Our terrain has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;

            SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        }
        else
        {
            // We didn't have a valid terrain before or after running this, so do nothing.
        }

    }

    void TerrainSurfaceDataSystemComponent::HeightmapModified(const AZ::Aabb& bounds)
    {
        UpdateTerrainData(bounds);
    }
}
