/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TerrainComponent.h"
#include "TerrainFeatureProcessor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>


namespace Terrain
{
    void TerrainConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("Wireframe", &TerrainConfig::m_debugWireframeEnabled)
                ->Field("Gradient", &TerrainConfig::m_gradientSampler)
                ;

            if (AZ::EditContext* edit = serialize->GetEditContext())
            {
                edit->Class<TerrainConfig>(
                    "Terrain", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainConfig::m_debugWireframeEnabled, "Wireframe", "Enable wireframe")
                    ->DataElement(0, &TerrainConfig::m_gradientSampler, "Gradient", "Gradient mapped to range between 0 and total combined weight of all descriptors.")
                    ;
            }
        }
    }

    void TerrainComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService"));
        services.push_back(AZ_CRC("TerrainService"));
    }

    void TerrainComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void TerrainComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("ShapeService"));
        //services.push_back(AZ_CRC("SurfaceDataSystemService"));
    }

    void TerrainComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainConfig::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainComponent::m_configuration)
                ;
        }
    }

    TerrainComponent::TerrainComponent(const TerrainConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainComponent::Activate()
    {
        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(m_configuration.m_gradientSampler.m_gradientId);
        AZ::TickBus::Handler::BusConnect();
        m_dirty = true;
    }

    void TerrainComponent::Deactivate()
    {
        if (m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle)
        {
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();

        DestroyTerrainData();
        AZ::TickBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        // Clear the cached terrain bounds data
        {
            m_terrainBounds = AZ::Aabb::CreateNull();
            m_terrainBoundsIsValid = false;
        }
    }

    TerrainComponent::~TerrainComponent()
    {
        DestroyTerrainData();
    }

    void TerrainComponent::DestroyTerrainData()
    {
        const AZ::RPI::Scene* scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene().get();
        auto terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            terrainFeatureProcessor->RemoveTerrainData(GetEntityId());
        }
    }

    bool TerrainComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        BuildTerrainData();
    }

    void TerrainComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        m_dirty = true;
    }

    void TerrainComponent::OnShapeChanged([[maybe_unused]] ShapeChangeReasons changeReason)
    {
        m_dirty = true;
    }

    void TerrainComponent::OnCompositionChanged()
    {
        m_dirty = true;
    }

    void TerrainComponent::BuildTerrainData()
    {
        if (m_dirty)
        {
            m_dirty = false;

            AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            AZ::EntityId gradient = GetEntityId();
            float sampleSpacing = 1.0f;
            UpdateTerrainData(worldBounds);

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);


            uint32_t width = aznumeric_cast<uint32_t>((float)worldBounds.GetXExtent() / sampleSpacing);
            uint32_t height = aznumeric_cast<uint32_t>((float)worldBounds.GetYExtent() / sampleSpacing);
            AZStd::vector<float> pixels;
            pixels.resize(width * height);
            const uint32_t pixelDataSize = width * height * sizeof(float);

            for (uint32_t y = 0; y < height; y++)
            {
                for (uint32_t x = 0; x < width; x++)
                {
                    GradientSignal::GradientSampleParams sampleParams;
                    sampleParams.m_position = AZ::Vector3((x * sampleSpacing) + worldBounds.GetMin().GetX(),
                        (y * sampleSpacing) + worldBounds.GetMin().GetY(),worldBounds.GetMin().GetZ());
                    float desiredHeight = m_configuration.m_gradientSampler.GetValue(sampleParams);
                    pixels[(y*width) + x] = desiredHeight;
                }
            }

            const AZ::RPI::Scene* scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene().get();
            auto terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();

            AZ_Assert(terrainFeatureProcessor, "Unable to find a TerrainFeatureProcessor.");
            if (terrainFeatureProcessor)
            {
                terrainFeatureProcessor->UpdateTerrainData(GetEntityId(), transform, worldBounds, sampleSpacing, width, height, pixels);
                terrainFeatureProcessor->SetDebugDrawWireframe(GetEntityId(), m_configuration.m_debugWireframeEnabled);
            }
        }
    }

    void TerrainComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const
    {
        if (m_terrainBoundsIsValid && m_terrainBounds.Contains(inPosition))
        {
            bool isTerrainValidAtPoint = true;
            GradientSignal::GradientSampleParams sampleParams;
            sampleParams.m_position = inPosition;
            float terrainHeight = AZ::Lerp(m_terrainBounds.GetMin().GetZ(), m_terrainBounds.GetMax().GetZ(), m_configuration.m_gradientSampler.GetValue(sampleParams));
            const bool isHole = !isTerrainValidAtPoint;

            SurfaceData::SurfacePoint point;
            point.m_entityId = GetEntityId();
            point.m_position = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), terrainHeight);
            //point.m_normal = terrain->GetNormal(inPosition);
            point.m_normal = AZ::Vector3(0.0f, 0.0f, 1.0f);
            const AZ::Crc32 terrainTag = isHole ? SurfaceData::Constants::s_terrainHoleTagCrc : SurfaceData::Constants::s_terrainTagCrc;
            SurfaceData::AddMaxValueForMasks(point.m_masks, terrainTag, 1.0f);
            surfacePointList.push_back(point);
        }
    }

    AZ::Aabb TerrainComponent::GetSurfaceAabb() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        return worldBounds;
    }

    SurfaceData::SurfaceTagVector TerrainComponent::GetSurfaceTags() const
    {
        SurfaceData::SurfaceTagVector tags;
        tags.push_back(SurfaceData::Constants::s_terrainHoleTagCrc);
        tags.push_back(SurfaceData::Constants::s_terrainTagCrc);
        return tags;
    }

    void TerrainComponent::UpdateTerrainData(const AZ::Aabb& dirtyRegion)
    {
        bool terrainValidBeforeUpdate = m_terrainBoundsIsValid;
        bool terrainValidAfterUpdate = false;
        AZ::Aabb terrainBoundsBeforeUpdate = GetSurfaceAabb();

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



}
