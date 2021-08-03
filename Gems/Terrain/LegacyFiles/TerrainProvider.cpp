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

#include <Terrain/TerrainProvider.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

using namespace Terrain;

// NEW-TERRAIN LY-103280:  This flag is temporary to help test the new terrain system by feeding it
// height data from the legacy terrain system.  Long-term, there should be no dependency between
// the new terrain system and the legacy terrain system.
#define USE_LEGACY_TERRAIN_HEIGHTS 0
TerrainProvider::TerrainProvider()
{
#if USE_LEGACY_TERRAIN_HEIGHTS
    // Cached pointers to I3DEngine and ITerrain - the calls to retrieve them use EBuses that expect to be used from the main thread,
    // but our access is from multiple threads, so we retrieve and cache them on the main thread.
    m_3dEngine = GetISystem()->GetI3DEngine();
    m_terrain = m_3dEngine->GetITerrain();
#endif

    m_dirtyRegion = AZ::Aabb::CreateNull();
    m_worldBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(4096.0f, 4096.0f, 2048.0f));

    Terrain::TerrainDataRequestBus::Handler::BusConnect();
    Terrain::TerrainProviderRequestBus::Handler::BusConnect();
    Terrain::TerrainProviderNotificationBus::Handler::BusConnect();

    ShaderRequestBus::Broadcast(&ShaderRequests::LoadShader, "TerrainHeightGenerator", m_terrainHeightGenerator);
    ShaderRequestBus::Broadcast(&ShaderRequests::LoadShader, "TerrainTileCompositeSystem", m_terrainMaterialCompositing);

    Terrain::TerrainSystemServiceRequestBus::Handler::BusConnect();

    TerrainAreaRequestBus::Broadcast(&TerrainAreaRequestBus::Events::RegisterArea);

}

TerrainProvider::~TerrainProvider()
{
    Terrain::TerrainSystemServiceRequestBus::Handler::BusDisconnect();

    Terrain::TerrainProviderNotificationBus::Handler::BusDisconnect();
    Terrain::TerrainProviderRequestBus::Handler::BusDisconnect();
    Terrain::TerrainDataRequestBus::Handler::BusDisconnect();

    ShaderRequestBus::Broadcast(&ShaderRequests::UnloadShader, m_terrainHeightGenerator);
    ShaderRequestBus::Broadcast(&ShaderRequests::UnloadShader, m_terrainMaterialCompositing);
}

AZ::Vector3 TerrainProvider::GetRegionSize()
{
    return m_regionBounds;
}


AZ::Aabb TerrainProvider::GetWorldBounds()
{
    return m_worldBounds;
}

void TerrainProvider::GetRegionIndex(const AZ::Vector2& worldMin, const AZ::Vector2& worldMax, int& regionIndexX, int& regionIndexY)
{
    const AZ::Vector3 worldOrigin = GetWorldBounds().GetMin();
    const AZ::Vector3 regionSize = GetRegionSize();
    float tileCenterX = ((worldMax.GetX() - worldMin.GetX()) * 0.5f + worldMin.GetX()) - worldOrigin.GetX();
    float tileCenterY = ((worldMax.GetY() - worldMin.GetY()) * 0.5f + worldMin.GetY()) - worldOrigin.GetY();

    regionIndexX = static_cast<int>(floorf(tileCenterX / static_cast<float>(regionSize.GetX())));
    regionIndexY = static_cast<int>(floorf(tileCenterY / static_cast<float>(regionSize.GetY())));
}

float TerrainProvider::GetHeightmapCellSize()
{
#if USE_LEGACY_TERRAIN_HEIGHTS
    return static_cast<float>(m_3dEngine->GetHeightMapUnitSize());
#else
    return m_heightmapCellSize;
#endif
}

float TerrainProvider::GetHeightSynchronous(float x, float y)
{
#if USE_LEGACY_TERRAIN_HEIGHTS
    return m_terrain->GetBilinearZ(x, y);
#else

    AZ::Vector3 inPosition((float)x, (float)y, 0.0f);
    AZ::Vector3 outPosition((float)x, (float)y, m_worldBounds.GetMin().GetZ());

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    if (!m_registeredAreas.empty())
    {
        for (auto& entry : m_registeredAreas)
        {
            AZ_Assert(m_registeredAreas.size() > 0, "???");
            Terrain::TerrainAreaHeightRequestBus::Event(entry.first, &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition, Terrain::TerrainAreaHeightRequestBus::Events::Sampler::DEFAULT);
        }
    }

    return AZ::GetClamp(outPosition.GetZ(), m_worldBounds.GetMin().GetZ(), m_worldBounds.GetMax().GetZ());
#endif
}

AZ::Vector3 TerrainProvider::GetNormalSynchronous(float x, float y)
{
#if USE_LEGACY_TERRAIN_HEIGHTS
    return LYVec3ToAZVec3(m_3dEngine->GetTerrainSurfaceNormal(Vec3(x, y, 0.0f)));
#else
    return AZ::Vector3::CreateAxisZ();
#endif
}

void TerrainProvider::GetHeight(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outPosition)
{
#if USE_LEGACY_TERRAIN_HEIGHTS
    switch (sampleFilter)
    {
        case Sampler::BILINEAR:
            outPosition = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_terrain->GetBilinearZ(inPosition.GetX(), inPosition.GetY()));
            break;
        case Sampler::EXACT:
        case Sampler::CLAMP:
            // TODO: Should actually get the legacy terrain meters per pixel and shift by that.
            int x = (int)(inPosition.GetX());
            int y = (int)(inPosition.GetY());
            outPosition = AZ::Vector3((float)x, (float)y, m_terrain->GetZ(x, y));
            break;
    }
#else
    outPosition = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), GetHeightSynchronous(inPosition.GetX(), inPosition.GetY()));
#endif
}

void TerrainProvider::GetNormal(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outNormal)
{
    // TODO: implement
}

void TerrainProvider::GetSurfaceWeights(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfaceTagWeightMap& outSurfaceWeights)
{
    // TODO: implement
}


void TerrainProvider::GetSurfacePoint(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfacePoint& outSurfacePoint)
{
    // TODO: Handle sampleFilter

    float sampleX = inPosition.GetX();
    float sampleY = inPosition.GetY();

    GetHeight(inPosition, sampleFilter, outSurfacePoint.m_position);
    //outSurfacePoint.m_position = AZ::Vector3(sampleX, sampleY, GetHeightSynchronous(sampleX, sampleY));
    outSurfacePoint.m_normal = GetNormalSynchronous(sampleX, sampleY);

#if USE_LEGACY_TERRAIN_HEIGHTS
    ITerrain::SurfaceWeight weight = m_terrain->GetSurfaceWeight((int)sampleX, (int)sampleY);
    for (int i = 0; i < ITerrain::SurfaceWeight::WeightCount; i++)
    {
        // TODO: Convert legacy tag IDs to new system CRCs
        if (weight.Ids[i] != ITerrain::SurfaceWeight::Undefined)
        {
            const AZ::Crc32 tag = weight.Ids[i];
            const float value = weight.Weights[i] / 255.0f;
            const auto maskItr = outSurfacePoint.m_masks.find(tag);
            const float valueOld = maskItr != outSurfacePoint.m_masks.end() ? maskItr->second : 0.0f;
            outSurfacePoint.m_masks[tag] = AZ::GetMax(value, valueOld);
        }
    }
#endif
    
}

void TerrainProvider::ProcessHeightsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete)
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


void TerrainProvider::ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete)
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


CShader* TerrainProvider::GetTerrainHeightGeneratorShader() const
{
    return m_terrainHeightGenerator;
}

CShader* TerrainProvider::GetTerrainMaterialCompositingShader() const
{
    return m_terrainMaterialCompositing;
}

float TerrainProvider::GetHeightAtIndexedPosition(int ix, int iy)
{
    // Shim back to legacy terrain heightmap data
#if USE_LEGACY_TERRAIN_HEIGHTS
    int unitSize = m_3dEngine->GetHeightMapUnitSize();
#else
    int unitSize = static_cast<int>(m_heightmapCellSize);
#endif
    float x = static_cast<float>(ix * unitSize);
    float y = static_cast<float>(iy * unitSize);

    return GetHeightAtWorldPosition(x, y);
}

float TerrainProvider::GetHeightAtWorldPosition(float fx, float fy)
{
    // Shim back to legacy terrain heightmap data
    float fZ = 0;

#if USE_LEGACY_TERRAIN_HEIGHTS
    if (m_terrain)
    {
        fZ = m_terrain->GetBilinearZ(fx, fy);
    }
#else
    fZ = GetHeightSynchronous(fx, fy);
#endif

    return fZ;
}

unsigned char TerrainProvider::GetSurfaceTypeAtIndexedPosition(int ix, int iy)
{
    return 0;
}

void TerrainProvider::SynchronizeSettings(Terrain::CRETerrainContext* context)
{
    if (m_terrainVersionDirty)
    {
        m_terrainVersionDirty = false;

        // Load the new worldmat asset
        // NEW-TERRAIN LY-103284: inconsistent handling of region bounds.  Single value here, but vector3 for GetRegionSize.
        Terrain::WorldMaterialRequestBus::Broadcast(&Terrain::WorldMaterialRequestBus::Events::LoadWorld, m_worldMaterialName, static_cast<int>(m_regionBounds.GetX()));

        // Null dirty region will be interpreted as updating everything
        m_dirtyRegion = AZ::Aabb::CreateNull();
        m_terrainHeightDirty = true;

        if (context != nullptr)
        {
            context->OnTractVersionUpdate();
        }
    }

    if (m_terrainHeightDirty)
    {
        Terrain::HeightmapDataNotificationBus::Broadcast(&Terrain::HeightmapDataNotificationBus::Events::OnTerrainHeightDataChanged, m_dirtyRegion);
        m_terrainHeightDirty = false;
        m_dirtyRegion = AZ::Aabb::CreateNull();
    }
}

void TerrainProvider::RegisterArea(AZ::EntityId areaId)
{
    {
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        m_registeredAreas[areaId] = aabb;
    }

    RefreshArea(areaId);
}

void TerrainProvider::UnregisterArea(AZ::EntityId areaId)
{
    {
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        m_registeredAreas.erase(areaId);
    }

    RefreshArea(areaId);
}

void TerrainProvider::RefreshArea(AZ::EntityId areaId)
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

