/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainSystem/TerrainSystem.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>

using namespace Terrain;

TerrainSystem::TerrainSystem()
{
    Terrain::TerrainSystemServiceRequestBus::Handler::BusConnect();
    AZ::TickBus::Handler::BusConnect();

    m_currentSettings.m_systemActive = false;
    m_currentSettings.m_worldBounds = AZ::Aabb::CreateNull();

    m_requestedSettings = m_currentSettings;
    m_requestedSettings.m_worldBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(4096.0f, 4096.0f, 2048.0f));
}

TerrainSystem::~TerrainSystem()
{
    AZ::TickBus::Handler::BusDisconnect();
    Terrain::TerrainSystemServiceRequestBus::Handler::BusDisconnect();

    Deactivate();
}

void TerrainSystem::Activate()
{
    m_requestedSettings.m_systemActive = true;
    m_terrainSettingsDirty = true;
}

void TerrainSystem::Deactivate()
{
    m_requestedSettings.m_systemActive = false;
    m_terrainSettingsDirty = true;
}

void TerrainSystem::SetWorldMin(AZ::Vector3 worldOrigin)
{
    m_requestedSettings.m_worldBounds.SetMin(worldOrigin);
    m_terrainSettingsDirty = true;
}

void TerrainSystem::SetWorldMax(AZ::Vector3 worldBounds)
{
    m_requestedSettings.m_worldBounds.SetMax(worldBounds);
    m_terrainSettingsDirty = true;
}

void TerrainSystem::SetHeightQueryResolution(AZ::Vector2 queryResolution)
{
    m_requestedSettings.m_heightQueryResolution = queryResolution;
    m_terrainSettingsDirty = true;
}

void TerrainSystem::SetDebugWireframe(bool wireframeEnabled)
{
    m_requestedSettings.m_debugWireframeEnabled = wireframeEnabled;
    m_terrainSettingsDirty = true;
}


AZ::Aabb TerrainSystem::GetTerrainAabb() const
{
    return m_currentSettings.m_worldBounds;
}

AZ::Vector2 TerrainSystem::GetTerrainGridResolution() const
{
    return m_currentSettings.m_heightQueryResolution;
}

float TerrainSystem::GetHeightSynchronous(float x, float y) const
{
    AZ::Vector3 inPosition((float)x, (float)y, m_currentSettings.m_worldBounds.GetMin().GetZ());
    AZ::Vector3 outPosition((float)x, (float)y, m_currentSettings.m_worldBounds.GetMin().GetZ());

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    if (!m_registeredAreas.empty())
    {
        for (auto& [areaId, areaBounds] : m_registeredAreas)
        {
            inPosition.SetZ(areaBounds.GetMin().GetZ());
            if (areaBounds.Contains(inPosition))
            {
                Terrain::TerrainAreaHeightRequestBus::Event(
                    areaId, &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition,
                    Terrain::TerrainAreaHeightRequestBus::Events::Sampler::DEFAULT);
            }
        }
    }

    return AZ::GetClamp(
        outPosition.GetZ(), m_currentSettings.m_worldBounds.GetMin().GetZ(), m_currentSettings.m_worldBounds.GetMax().GetZ());
}

float TerrainSystem::GetHeight(AZ::Vector3 position, [[maybe_unused]] Sampler sampler, [[maybe_unused]] bool* terrainExistsPtr) const
{
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    return GetHeightSynchronous(position.GetX(), position.GetY());
}

float TerrainSystem::GetHeightFromFloats(
    float x, float y, [[maybe_unused]] Sampler sampler, [[maybe_unused]] bool* terrainExistsPtr) const
{
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    return GetHeightSynchronous(x, y);
}

bool TerrainSystem::GetIsHoleFromFloats(
    [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] Sampler sampleFilter) const
{
    return false;
}

AZ::Vector3 TerrainSystem::GetNormalSynchronous([[maybe_unused]] float x, [[maybe_unused]] float y) const
{
    return AZ::Vector3::CreateAxisZ();
}

AZ::Vector3 TerrainSystem::GetNormal(
    AZ::Vector3 position, [[maybe_unused]] Sampler sampleFilter, [[maybe_unused]] bool* terrainExistsPtr) const
{
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    return GetNormalSynchronous(position.GetX(), position.GetY());
}

AZ::Vector3 TerrainSystem::GetNormalFromFloats(
    float x, float y, [[maybe_unused]] Sampler sampleFilter, [[maybe_unused]] bool* terrainExistsPtr) const
{
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    return GetNormalSynchronous(x, y);
}


AzFramework::SurfaceData::SurfaceTagWeight TerrainSystem::GetMaxSurfaceWeight(
    [[maybe_unused]] AZ::Vector3 position, [[maybe_unused]] Sampler sampleFilter, [[maybe_unused]] bool* terrainExistsPtr) const
{
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    return AzFramework::SurfaceData::SurfaceTagWeight();
}

AzFramework::SurfaceData::SurfaceTagWeight TerrainSystem::GetMaxSurfaceWeightFromFloats(
    [[maybe_unused]] float x,
    [[maybe_unused]] float y,
    [[maybe_unused]] Sampler sampleFilter,
    [[maybe_unused]] bool* terrainExistsPtr) const
{
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    return AzFramework::SurfaceData::SurfaceTagWeight();
}

const char* TerrainSystem::GetMaxSurfaceName(
    [[maybe_unused]] AZ::Vector3 position, [[maybe_unused]] Sampler sampleFilter, [[maybe_unused]] bool* terrainExistsPtr) const
{
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    return "";
}

/*
void TerrainSystem::GetSurfaceWeights(
    [[maybe_unused]] const AZ::Vector3& inPosition,
    [[maybe_unused]] Sampler sampleFilter,
    [[maybe_unused]] SurfaceData::SurfaceTagWeightMap& outSurfaceWeights)
{
    // TODO: implement
}

void TerrainSystem::GetSurfacePoint(
    const AZ::Vector3& inPosition, [[maybe_unused]] Sampler sampleFilter, SurfaceData::SurfacePoint& outSurfacePoint)
{
    // TODO: Handle sampleFilter

    float sampleX = inPosition.GetX();
    float sampleY = inPosition.GetY();

    GetHeight(inPosition, sampleFilter, outSurfacePoint.m_position);
    //outSurfacePoint.m_position = AZ::Vector3(sampleX, sampleY, GetHeightSynchronous(sampleX, sampleY));
    outSurfacePoint.m_normal = GetNormalSynchronous(sampleX, sampleY);
}




void TerrainSystem::ProcessHeightsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete)
{
    // Don't bother processing if we don't have a callback
    if (!perPositionCallback)
    {
        return;
    }

    uint32_t numSamplesX = static_cast<uint32_t>((inRegion.GetMax().GetX() - inRegion.GetMin().GetX()) / stepSize.GetX());
    uint32_t numSamplesY = static_cast<uint32_t>((inRegion.GetMax().GetY() - inRegion.GetMin().GetY()) / stepSize.GetY());

    for (uint32_t y = 0; y < numSamplesY; y++)
    {
        for (uint32_t x = 0; x < numSamplesX; x++)
        {
            float fx = (float)(inRegion.GetMin().GetX() + (x * stepSize.GetX()));
            float fy = (float)(inRegion.GetMin().GetY() + (y * stepSize.GetY()));

            SurfaceData::SurfacePoint surfacePoint;
            GetHeight(AZ::Vector3(fx, fy, 0.0f), sampleFilter, surfacePoint.m_position);
            perPositionCallback(surfacePoint, x, y);
        }
    }

    if (onComplete)
    {
        onComplete();
    }
}


void TerrainSystem::ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete)
{
    // Don't bother processing if we don't have a callback
    if (!perPositionCallback)
    {
        return;
    }

    uint32_t numSamplesX = static_cast<uint32_t>((inRegion.GetMax().GetX() - inRegion.GetMin().GetX()) / stepSize.GetX());
    uint32_t numSamplesY = static_cast<uint32_t>((inRegion.GetMax().GetY() - inRegion.GetMin().GetY()) / stepSize.GetY());

    for (uint32_t y = 0; y < numSamplesY; y++)
    {
        for (uint32_t x = 0; x < numSamplesX; x++)
        {
            float fx = (float)(inRegion.GetMin().GetX() + (x * stepSize.GetX()));
            float fy = (float)(inRegion.GetMin().GetY() + (y * stepSize.GetY()));

            SurfaceData::SurfacePoint surfacePoint;
            GetSurfacePoint(AZ::Vector3(fx, fy, inRegion.GetMin().GetZ()), sampleFilter, surfacePoint);
            perPositionCallback(surfacePoint, x, y);
        }
    }

    if (onComplete)
    {
        onComplete();
    }
}
*/

void TerrainSystem::SystemActivate()
{
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);
        m_registeredAreas.clear();
    }

    AzFramework::Terrain::TerrainDataRequestBus::Handler::BusConnect();

    TerrainAreaRequestBus::Broadcast(&TerrainAreaRequestBus::Events::RegisterArea);
}

void TerrainSystem::SystemDeactivate()
{
    AzFramework::Terrain::TerrainDataRequestBus::Handler::BusDisconnect();

    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);
        m_registeredAreas.clear();
    }

    const AZ::RPI::Scene* scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene().get();
    auto terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
    if (terrainFeatureProcessor)
    {
        terrainFeatureProcessor->RemoveTerrainData();
    }
}

void TerrainSystem::RegisterArea(AZ::EntityId areaId)
{
    {
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        m_registeredAreas[areaId] = aabb;
    }

    RefreshArea(areaId);
}

void TerrainSystem::UnregisterArea(AZ::EntityId areaId)
{
    {
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        m_registeredAreas.erase(areaId);
    }

    RefreshArea(areaId);
}

void TerrainSystem::RefreshArea(AZ::EntityId areaId)
{
    AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);

    auto areaAabb = m_registeredAreas.find(areaId);

    AZ::Aabb oldAabb = (areaAabb != m_registeredAreas.end()) ? areaAabb->second : AZ::Aabb::CreateNull();
    AZ::Aabb newAabb = AZ::Aabb::CreateNull();
    LmbrCentral::ShapeComponentRequestsBus::EventResult(newAabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

    m_registeredAreas[areaId] = newAabb;

    AZ::Aabb expandedAabb = oldAabb;
    expandedAabb.AddAabb(newAabb);

    m_dirtyRegion.AddAabb(expandedAabb);
    m_terrainHeightDirty = true;
}

void TerrainSystem::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
{
    bool terrainSettingsChanged = false;

    if (m_terrainSettingsDirty)
    {
        m_terrainSettingsDirty = false;

        // This needs to happen before the "system active" check below, because activating the system will cause the various
        // terrain layer areas to request the current world bounds.
        if (m_requestedSettings.m_worldBounds != m_currentSettings.m_worldBounds)
        {
            m_dirtyRegion = m_currentSettings.m_worldBounds;
            m_dirtyRegion.AddAabb(m_requestedSettings.m_worldBounds);
            m_terrainHeightDirty = true;
            m_currentSettings.m_worldBounds = m_requestedSettings.m_worldBounds;
            terrainSettingsChanged = true;
        }

        if (m_requestedSettings.m_debugWireframeEnabled != m_currentSettings.m_debugWireframeEnabled)
        {
            m_dirtyRegion = AZ::Aabb::CreateNull();
            m_terrainHeightDirty = true;
            terrainSettingsChanged = true;
        }

        if (m_requestedSettings.m_heightQueryResolution != m_currentSettings.m_heightQueryResolution)
        {
            m_dirtyRegion = AZ::Aabb::CreateNull();
            m_terrainHeightDirty = true;
            terrainSettingsChanged = true;
        }

        if (m_requestedSettings.m_systemActive != m_currentSettings.m_systemActive)
        {
            m_requestedSettings.m_systemActive ? SystemActivate() : SystemDeactivate();

            // Null dirty region will be interpreted as updating everything
            m_dirtyRegion = AZ::Aabb::CreateNull();
            m_terrainHeightDirty = true;
            terrainSettingsChanged = true;
        }

        m_currentSettings = m_requestedSettings;
    }

    if (m_currentSettings.m_systemActive && m_terrainHeightDirty)
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

        AZ::EntityId entityId(0);
        AZ::Transform transform = AZ::Transform::CreateTranslation(m_currentSettings.m_worldBounds.GetCenter());

        uint32_t width = aznumeric_cast<uint32_t>(
            (float)m_currentSettings.m_worldBounds.GetXExtent() / m_currentSettings.m_heightQueryResolution.GetX());
        uint32_t height = aznumeric_cast<uint32_t>(
            (float)m_currentSettings.m_worldBounds.GetYExtent() / m_currentSettings.m_heightQueryResolution.GetY());
        AZStd::vector<float> pixels;
        pixels.resize(width * height);
        const uint32_t pixelDataSize = width * height * sizeof(float);
        memset(pixels.data(), 0, pixelDataSize);

        for (auto& [areaId, areaBounds] : m_registeredAreas)
        {
            for (uint32_t y = 0; y < height; y++)
            {
                for (uint32_t x = 0; x < width; x++)
                {
                    AZ::Vector3 inPosition(
                        (x * m_currentSettings.m_heightQueryResolution.GetX()) + m_currentSettings.m_worldBounds.GetMin().GetX(),
                        (y * m_currentSettings.m_heightQueryResolution.GetY()) + m_currentSettings.m_worldBounds.GetMin().GetY(),
                        areaBounds.GetMin().GetZ());
                    if (areaBounds.Contains(inPosition))
                    {
                        AZ::Vector3 outPosition;
                        const Terrain::TerrainAreaHeightRequests::Sampler sampleFilter =
                            Terrain::TerrainAreaHeightRequests::Sampler::DEFAULT;

                        Terrain::TerrainAreaHeightRequestBus::Event(
                            areaId, &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition, sampleFilter);

                        pixels[(y * width) + x] = (outPosition.GetZ() - m_currentSettings.m_worldBounds.GetMin().GetZ()) /
                            m_currentSettings.m_worldBounds.GetExtents().GetZ();
                    }
                }
            }
        }

        const AZ::RPI::Scene* scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene().get();
        auto terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();

        AZ_Assert(terrainFeatureProcessor, "Unable to find a TerrainFeatureProcessor.");
        if (terrainFeatureProcessor)
        {
            terrainFeatureProcessor->UpdateTerrainData(
                entityId, transform, m_currentSettings.m_worldBounds, m_currentSettings.m_heightQueryResolution.GetX(), width, height,
                pixels);
            terrainFeatureProcessor->SetDebugDrawWireframe(entityId, m_currentSettings.m_debugWireframeEnabled);
        }
    }

    if (terrainSettingsChanged || m_terrainHeightDirty)
    {
        AZ::Aabb dirtyRegion = m_dirtyRegion;
        m_terrainHeightDirty = false;
        m_dirtyRegion = AZ::Aabb::CreateNull();

        AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask changeMask =
            AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::None;

        if (terrainSettingsChanged)
        {
            changeMask = static_cast<AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask>(
                changeMask | AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::Settings);
        }
        if (m_terrainHeightDirty)
        {
            changeMask = static_cast<AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask>(
                changeMask | AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::HeightData);
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(
            &AzFramework::Terrain::TerrainDataNotificationBus::Events::OnTerrainDataChanged, dirtyRegion,
            changeMask);
    }

}
