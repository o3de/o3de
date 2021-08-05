/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainProvider.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <TerrainFeatureProcessor.h>

using namespace Terrain;

TerrainProvider::TerrainProvider()
{
    m_dirtyRegion = AZ::Aabb::CreateNull();
    m_worldBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(4096.0f, 4096.0f, 2048.0f));

    Terrain::TerrainDataRequestBus::Handler::BusConnect();

    Terrain::TerrainSystemServiceRequestBus::Handler::BusConnect();
    AZ::TickBus::Handler::BusConnect();

    TerrainAreaRequestBus::Broadcast(&TerrainAreaRequestBus::Events::RegisterArea);

}

TerrainProvider::~TerrainProvider()
{
    AZ::TickBus::Handler::BusDisconnect();

    Terrain::TerrainSystemServiceRequestBus::Handler::BusDisconnect();

    Terrain::TerrainDataRequestBus::Handler::BusDisconnect();
}

AZ::Aabb TerrainProvider::GetTerrainAabb() const
{
    return m_worldBounds;
}

AZ::Vector2 TerrainProvider::GetTerrainGridResolution() const
{
    return m_heightQueryResolution;
}

float TerrainProvider::GetHeightSynchronous(float x, float y)
{
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
}

AZ::Vector3 TerrainProvider::GetNormalSynchronous([[maybe_unused]] float x, [[maybe_unused]] float y)
{
    return AZ::Vector3::CreateAxisZ();
}

void TerrainProvider::GetHeight(const AZ::Vector3& inPosition, [[maybe_unused]] Sampler sampleFilter, AZ::Vector3& outPosition)
{
    outPosition = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), GetHeightSynchronous(inPosition.GetX(), inPosition.GetY()));
}

void TerrainProvider::GetNormal(
    [[maybe_unused]] const AZ::Vector3& inPosition, [[maybe_unused]] Sampler sampleFilter, [[maybe_unused]] AZ::Vector3& outNormal)
{
    // TODO: implement
}

void TerrainProvider::GetSurfaceWeights(
    [[maybe_unused]] const AZ::Vector3& inPosition,
    [[maybe_unused]] Sampler sampleFilter,
    [[maybe_unused]] SurfaceData::SurfaceTagWeightMap& outSurfaceWeights)
{
    // TODO: implement
}


void TerrainProvider::GetSurfacePoint(
    const AZ::Vector3& inPosition, [[maybe_unused]] Sampler sampleFilter, SurfaceData::SurfacePoint& outSurfacePoint)
{
    // TODO: Handle sampleFilter

    float sampleX = inPosition.GetX();
    float sampleY = inPosition.GetY();

    GetHeight(inPosition, sampleFilter, outSurfacePoint.m_position);
    //outSurfacePoint.m_position = AZ::Vector3(sampleX, sampleY, GetHeightSynchronous(sampleX, sampleY));
    outSurfacePoint.m_normal = GetNormalSynchronous(sampleX, sampleY);
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

void TerrainProvider::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
{
    if (m_terrainVersionDirty)
    {
        m_terrainVersionDirty = false;

        // Null dirty region will be interpreted as updating everything
        m_dirtyRegion = AZ::Aabb::CreateNull();
        m_terrainHeightDirty = true;
    }

    if (m_terrainHeightDirty)
    {
        AZ::EntityId entityId(0);
        AZ::Transform transform = AZ::Transform::CreateTranslation(m_worldBounds.GetCenter());

        uint32_t width = aznumeric_cast<uint32_t>((float)m_worldBounds.GetXExtent() / m_heightQueryResolution.GetX());
        uint32_t height = aznumeric_cast<uint32_t>((float)m_worldBounds.GetYExtent() / m_heightQueryResolution.GetY());
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
                        (x * m_heightQueryResolution.GetX()) + m_worldBounds.GetMin().GetX(),
                        (y * m_heightQueryResolution.GetY()) + m_worldBounds.GetMin().GetY(),
                        areaBounds.GetMin().GetZ());
                    if (areaBounds.Contains(inPosition))
                    {
                        AZ::Vector3 outPosition;
                        const Terrain::TerrainAreaHeightRequests::Sampler sampleFilter =
                            Terrain::TerrainAreaHeightRequests::Sampler::DEFAULT;

                        Terrain::TerrainAreaHeightRequestBus::Event(
                            areaId, &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition, sampleFilter);

                        pixels[(y * width) + x] = (outPosition.GetZ() - m_worldBounds.GetMin().GetZ()) / m_worldBounds.GetExtents().GetZ();
                    }
                }
            }
        }

        const AZ::RPI::Scene* scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene().get();
        auto terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();

        AZ_Assert(terrainFeatureProcessor, "Unable to find a TerrainFeatureProcessor.");
        if (terrainFeatureProcessor)
        {
            terrainFeatureProcessor->UpdateTerrainData(entityId, transform, m_worldBounds, m_heightQueryResolution.GetX(), width, height, pixels);
            terrainFeatureProcessor->SetDebugDrawWireframe(entityId, m_debugWireframeEnabled);
        }


        m_terrainHeightDirty = false;
        m_dirtyRegion = AZ::Aabb::CreateNull();
    }
}
