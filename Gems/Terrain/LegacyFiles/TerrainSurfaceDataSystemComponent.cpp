/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "SurfaceData_precompiled.h"
#include "TerrainSurfaceDataSystemComponent.h"
#include <AzCore/Debug/Profiler.h> 
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Math/MathUtils.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <MathConversion.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <ISystem.h>
#include <I3DEngine.h>
#include <Terrain/Bus/TerrainBus.h>
#include <Terrain/Bus/TerrainProviderBus.h>

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
        m_system = GetISystem();
        CrySystemEventBus::Handler::BusConnect();
        AZ::HeightmapUpdateNotificationBus::Handler::BusConnect();

        SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = GetSurfaceAabb();
        registryEntry.m_tags = GetSurfaceTags();

        m_providerHandle = InvalidSurfaceDataRegistryHandle;
        SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);

        SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
    }

    void TerrainSurfaceDataSystemComponent::Deactivate()
    {
        SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
        m_providerHandle = InvalidSurfaceDataRegistryHandle;

        SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        AZ::HeightmapUpdateNotificationBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        m_system = nullptr;
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

    void TerrainSurfaceDataSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
    }

    void TerrainSurfaceDataSystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        m_system = nullptr;
    }

    void TerrainSurfaceDataSystemComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        AZ::Aabb surfaceAabb = GetSurfaceAabb();

        auto engine = m_system ? m_system->GetI3DEngine() : nullptr;
        auto enumerationCallback = [&](Terrain::TerrainDataRequests* terrain) -> bool
        {
            AZ::Vector3 adjustedInPosition(inPosition.GetX(), inPosition.GetY(), surfaceAabb.GetMax().GetZ());
            if (surfaceAabb.Contains(adjustedInPosition))
            {
                SurfacePoint point;
                point.m_entityId = GetEntityId();
                terrain->GetSurfacePoint(inPosition, Terrain::TerrainDataRequests::Sampler::BILINEAR, point);
                AddMaxValueForMasks(point.m_masks, Constants::s_terrainTagCrc, 1.0f);
                surfacePointList.push_back(point);
            }
            // Only one handler should exist.
            return false;
        };
        if (engine)
        {
            Terrain::TerrainDataRequestBus::EnumerateHandlers(enumerationCallback);
        }
    }

    AZ::Aabb TerrainSurfaceDataSystemComponent::GetSurfaceAabb() const
    {
        auto terrain = Terrain::TerrainProviderRequestBus::FindFirstHandler();
        return terrain ? terrain->GetWorldBounds() : AZ::Aabb::CreateNull();
    }

    SurfaceTagVector TerrainSurfaceDataSystemComponent::GetSurfaceTags() const
    {
        SurfaceTagVector tags;
        tags.push_back(Constants::s_terrainHoleTagCrc);
        tags.push_back(Constants::s_terrainTagCrc);
        return tags;
    }

    void TerrainSurfaceDataSystemComponent::HeightmapModified(const AZ::Aabb& bounds)
    {
        SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = GetSurfaceAabb();
        registryEntry.m_tags = GetSurfaceTags();
        SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry, bounds);
    }
}
