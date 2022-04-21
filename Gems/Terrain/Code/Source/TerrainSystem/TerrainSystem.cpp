/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainSystem/TerrainSystem.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/sort.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>

#include <Terrain/Ebuses/TerrainAreaSurfaceRequestBus.h>

using namespace Terrain;

AZ_DEFINE_BUDGET(Terrain);

bool TerrainLayerPriorityComparator::operator()(const AZ::EntityId& layer1id, const AZ::EntityId& layer2id) const
{
    // Comparator for insertion/keylookup.
    // Sorts into layer/priority order, highest priority first.
    AZ::u32 priority1 = 0;
    AZ::u32 layer1 = 0;
    Terrain::TerrainSpawnerRequestBus::Event(layer1id, &Terrain::TerrainSpawnerRequestBus::Events::GetPriority, layer1, priority1);

    AZ::u32 priority2 = 0;
    AZ::u32 layer2 = 0;
    Terrain::TerrainSpawnerRequestBus::Event(layer2id, &Terrain::TerrainSpawnerRequestBus::Events::GetPriority, layer2, priority2);

    if (layer1 < layer2)
    {
        return false;
    }
    else if (layer1 > layer2)
    {
        return true;
    }

    if (priority1 != priority2)
    {
        return priority1 > priority2;
    }

    return layer1id > layer2id;
}

TerrainSystem::TerrainSystem()
    : m_terrainRaycastContext(*this)
{
    Terrain::TerrainSystemServiceRequestBus::Handler::BusConnect();
    AZ::TickBus::Handler::BusConnect();

    m_currentSettings.m_systemActive = false;
    m_currentSettings.m_worldBounds = AZ::Aabb::CreateNull();

    m_requestedSettings = m_currentSettings;
    m_requestedSettings.m_worldBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-512.0f), AZ::Vector3(512.0f));

    // Use the global JobManager for terrain jobs (we could create our own dedicated terrain JobManager if needed).
    AZ::JobManagerBus::BroadcastResult(m_terrainJobManager, &AZ::JobManagerEvents::GetManager);
    AZ_Assert(m_terrainJobManager, "No global JobManager found.");
}

TerrainSystem::~TerrainSystem()
{
    AZ::TickBus::Handler::BusDisconnect();
    Terrain::TerrainSystemServiceRequestBus::Handler::BusDisconnect();

    Deactivate();
}

void TerrainSystem::Activate()
{
    AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(
        &AzFramework::Terrain::TerrainDataNotificationBus::Events::OnTerrainDataCreateBegin);

    m_dirtyRegion = AZ::Aabb::CreateNull();
    m_terrainHeightDirty = true;
    m_terrainSettingsDirty = true;
    m_terrainSurfacesDirty = true;
    m_requestedSettings.m_systemActive = true;

    {
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);
        m_registeredAreas.clear();
    }

    AzFramework::Terrain::TerrainDataRequestBus::Handler::BusConnect();

    // Register any terrain spawners that were already active before the terrain system activated.
    auto enumerationCallback = [&]([[maybe_unused]] Terrain::TerrainSpawnerRequests* terrainSpawner) -> bool
    {
        AZ::EntityId areaId = *(Terrain::TerrainSpawnerRequestBus::GetCurrentBusId());
        RegisterArea(areaId);

        // Keep Enumerating
        return true;
    };
    Terrain::TerrainSpawnerRequestBus::EnumerateHandlers(enumerationCallback);

    AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(
        &AzFramework::Terrain::TerrainDataNotificationBus::Events::OnTerrainDataCreateEnd);
}

void TerrainSystem::Deactivate()
{
    {
        // Cancel all active terrain jobs, and wait until they have completed.
        AZStd::unique_lock<AZStd::mutex> lock(m_activeTerrainJobContextMutex);
        for (auto activeTerrainJobContext : m_activeTerrainJobContexts)
        {
            activeTerrainJobContext->Cancel();
        }
        m_activeTerrainJobContextMutexConditionVariable.wait(lock, [this]{ return m_activeTerrainJobContexts.empty(); });
    }

    // Stop listening to the bus even before we signal DestroyBegin so that way any calls to the terrain system as a *result* of
    // calling DestroyBegin will fail to reach the terrain system.
    AzFramework::Terrain::TerrainDataRequestBus::Handler::BusDisconnect();

    AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(
        &AzFramework::Terrain::TerrainDataNotificationBus::Events::OnTerrainDataDestroyBegin);

    {
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);
        m_registeredAreas.clear();
    }

    m_dirtyRegion = AZ::Aabb::CreateNull();
    m_terrainHeightDirty = true;
    m_terrainSettingsDirty = true;
    m_terrainSurfacesDirty = true;
    m_requestedSettings.m_systemActive = false;

    AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(
        &AzFramework::Terrain::TerrainDataNotificationBus::Events::OnTerrainDataDestroyEnd);
}

void TerrainSystem::SetTerrainAabb(const AZ::Aabb& worldBounds)
{   
    m_requestedSettings.m_worldBounds = worldBounds;
    m_terrainSettingsDirty = true;
}

void TerrainSystem::SetTerrainHeightQueryResolution(float queryResolution)
{
    m_requestedSettings.m_heightQueryResolution = queryResolution;
    m_terrainSettingsDirty = true;
}

void TerrainSystem::SetTerrainSurfaceDataQueryResolution(float queryResolution)
{
    m_requestedSettings.m_surfaceDataQueryResolution = queryResolution;
    m_terrainSettingsDirty = true;
}

AZ::Aabb TerrainSystem::GetTerrainAabb() const
{
    return m_currentSettings.m_worldBounds;
}

float TerrainSystem::GetTerrainHeightQueryResolution() const
{
    return m_currentSettings.m_heightQueryResolution;
}

float TerrainSystem::GetTerrainSurfaceDataQueryResolution() const
{
    return m_currentSettings.m_surfaceDataQueryResolution;
}

void TerrainSystem::ClampPosition(float x, float y, AZ::Vector2& outPosition, AZ::Vector2& normalizedDelta) const
{
    // Given an input position, clamp the values to our terrain grid, where it will always go to the terrain grid point
    // at a lower value, whether positive or negative.  Ex: 3.3 -> 3, -3.3 -> -4
    // Also, return the normalized delta as a value of [0-1) describing what fraction of a grid point the value moved.

    // Scale the position by the query resolution, so that integer values represent exact steps on the grid,
    // and fractional values are the amount in-between each grid point, in the range [0-1).
    AZ::Vector2 normalizedPosition = AZ::Vector2(x, y) / m_currentSettings.m_heightQueryResolution;
    normalizedDelta = AZ::Vector2(
        normalizedPosition.GetX() - floor(normalizedPosition.GetX()), normalizedPosition.GetY() - floor(normalizedPosition.GetY()));

    // Remove the fractional part, then scale back down into world space.
    outPosition = (normalizedPosition - normalizedDelta) * m_currentSettings.m_heightQueryResolution;
}

bool TerrainSystem::InWorldBounds(float x, float y) const
{
    const float zTestValue = m_currentSettings.m_worldBounds.GetMin().GetZ();
    const AZ::Vector3 testValue{ x, y, zTestValue };
    if (m_currentSettings.m_worldBounds.Contains(testValue))
    {
        return true;
    }
    return false;
}

// Generate positions to be queried based on the sampler type.
void TerrainSystem::GenerateQueryPositions(const AZStd::span<const AZ::Vector3>& inPositions,
    AZStd::vector<AZ::Vector3>& outPositions,
    Sampler sampler) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    const float minHeight = m_currentSettings.m_worldBounds.GetMin().GetZ();
    for (auto& position : inPositions)
    {
        switch(sampler)
        {
        case AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR:
            {
                if (InWorldBounds(position.GetX(), position.GetY()))
                {
                    AZ::Vector2 normalizedDelta;
                    AZ::Vector2 pos0;
                    ClampPosition(position.GetX(), position.GetY(), pos0, normalizedDelta);
                    const AZ::Vector2 pos1(
                        pos0.GetX() + m_currentSettings.m_heightQueryResolution, pos0.GetY() + m_currentSettings.m_heightQueryResolution);
                    outPositions.emplace_back(AZ::Vector3(pos0.GetX(), pos0.GetY(), minHeight));
                    outPositions.emplace_back(AZ::Vector3(pos1.GetX(), pos0.GetY(), minHeight));
                    outPositions.emplace_back(AZ::Vector3(pos0.GetX(), pos1.GetY(), minHeight));
                    outPositions.emplace_back(AZ::Vector3(pos1.GetX(), pos1.GetY(), minHeight));
                }
                else
                {
                    // If the query position isn't within the world bounds, we'll place that position 4x into the query list
                    // instead of the normal bilinear positions, because we don't want to interpolate between partially inside and
                    // partially outside. We just want to give it a min height and "terrain doesn't exist".
                    outPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY(), minHeight));
                    outPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY(), minHeight));
                    outPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY(), minHeight));
                    outPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY(), minHeight));
                }
            }
            break;
        case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
            {
                AZ::Vector2 normalizedDelta;
                AZ::Vector2 clampedPosition;
                ClampPosition(position.GetX(), position.GetY(), clampedPosition, normalizedDelta);
                outPositions.emplace_back(AZ::Vector3(clampedPosition.GetX(), clampedPosition.GetY(), minHeight));
            }
            break;
        case AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT:
            [[fallthrough]];
        default:
            outPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY(), minHeight));
            break;
        }
    }
}

AZStd::vector<AZ::Vector3> TerrainSystem::GenerateInputPositionsFromRegion(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    Sampler sampler) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    AZStd::vector<AZ::Vector3> inPositions;
    const auto [numSamplesX, numSamplesY] = GetNumSamplesFromRegion(inRegion, stepSize, sampler);
    inPositions.reserve(numSamplesX * numSamplesY);

    AZ::Vector2 startPosition = AZ::Vector2(inRegion.GetMin().GetX(), inRegion.GetMin().GetY());

    if (sampler == Sampler::CLAMP)
    {
        // Adjust the start position to be on a clamped position.
        const int32_t firstSampleX = aznumeric_cast<int32_t>(AZStd::ceilf(inRegion.GetMin().GetX() / stepSize.GetX()));
        const int32_t firstSampleY = aznumeric_cast<int32_t>(AZStd::ceilf(inRegion.GetMin().GetY() / stepSize.GetY()));
        startPosition.SetX(firstSampleX * stepSize.GetX());
        startPosition.SetY(firstSampleY * stepSize.GetY());
    }

    for (size_t y = 0; y < numSamplesY; y++)
    {
        float fy = aznumeric_cast<float>(inRegion.GetMin().GetY() + (y * stepSize.GetY()));
        for (size_t x = 0; x < numSamplesX; x++)
        {
            float fx = aznumeric_cast<float>(inRegion.GetMin().GetX() + (x * stepSize.GetX()));
            inPositions.emplace_back(AZ::Vector3(fx, fy, 0.0f));
        }
    }

    return inPositions;
}

AZStd::vector<AZ::Vector3> TerrainSystem::GenerateInputPositionsFromListOfVector2(
    const AZStd::span<const AZ::Vector2> inPositionsVec2) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    AZStd::vector<AZ::Vector3> inPositions;
    inPositions.reserve(inPositionsVec2.size());

    for (auto& pos : inPositionsVec2)
    {
        inPositions.emplace_back(AZ::Vector3(pos.GetX(), pos.GetY(), 0.0f));
    }

    return inPositions;
}

void TerrainSystem::MakeBulkQueries(
    const AZStd::span<const AZ::Vector3> inPositions,
    AZStd::span<AZ::Vector3> outPositions,
    AZStd::span<bool> outTerrainExists,
    AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeights,
    BulkQueriesCallback queryCallback) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    AZ::Aabb bounds;
    AZ::EntityId prevAreaId = FindBestAreaEntityAtPosition(inPositions[0], bounds);
    
    // We use a sliding window here and update the window end for each
    // position that falls in the same area as the previous positions. This consumes lesser memory
    // than sorting the points into separate lists and handling putting them back together.
    // This may be sub optimal if the points are randomly distributed in the list as opposed
    // to points in the same area id being close to each other.
    size_t windowStart = 0;
    size_t windowEnd = 0;
    const size_t numPositions = inPositions.size();
    for(int i = 1; i < numPositions; i++)
    {
        AZ::EntityId areaId = FindBestAreaEntityAtPosition(inPositions[i], bounds);
        bool queryHeights = false;
        if (areaId == prevAreaId)
        {
            // Update window end to current position.
            // If it's the last position, submit the query.
            windowEnd = i;
            if (windowEnd == numPositions - 1)
            {
                queryHeights = true;
            }
        }
        else
        {
            queryHeights = true;
        }

        if (queryHeights)
        {
            // If the area id is a default entity id, it usually means the
            // position is outside world bounds.
            if (prevAreaId != AZ::EntityId())
            {
                size_t spanLength = (windowEnd - windowStart) + 1;
                queryCallback(AZStd::span<const AZ::Vector3>(inPositions.begin() + windowStart, spanLength),
                    AZStd::span<AZ::Vector3>(outPositions.begin() + windowStart, spanLength),
                    AZStd::span<bool>(outTerrainExists.begin() + windowStart, spanLength),
                    AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList>(outSurfaceWeights.begin() + windowStart, spanLength),
                    prevAreaId);
            }

            // Reset the window to start at the current position. Set the new area
            // id on which to run the next query.
            windowStart = windowEnd = i;
            prevAreaId = areaId;
        }   
    }
}

void TerrainSystem::GetHeightsSynchronous(const AZStd::span<const AZ::Vector3>& inPositions, Sampler sampler, 
    AZStd::span<float> heights, AZStd::span<bool> terrainExists) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    AZStd::vector<AZ::Vector3> outPositions;
    AZStd::vector<bool> outTerrainExists;

    // outPositions holds the iterators to results of the bulk queries.
    // In the case of the bilinear sampler, we'll be making 4 queries per
    // input position.
    size_t indexStepSize = (sampler == AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) ? 4 : 1;
    outPositions.reserve(inPositions.size() * indexStepSize);
    outTerrainExists.resize(inPositions.size() * indexStepSize);

    GenerateQueryPositions(inPositions, outPositions, sampler);

    auto callback = []([[maybe_unused]] const AZStd::span<const AZ::Vector3> inPositions,
                        AZStd::span<AZ::Vector3> outPositions,
                        AZStd::span<bool> outTerrainExists,
                        [[maybe_unused]] AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeights,
                        AZ::EntityId areaId)
                        {
                            AZ_Assert((inPositions.size() == outPositions.size() && inPositions.size() == outTerrainExists.size()),
                                "The sizes of the terrain exists list and in/out positions list should match.");
                            Terrain::TerrainAreaHeightRequestBus::Event(areaId, &Terrain::TerrainAreaHeightRequestBus::Events::GetHeights,
                                outPositions, outTerrainExists);
                        };

    // This will be unused for heights. It's fine if it's empty.
    AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeights;
    MakeBulkQueries(outPositions, outPositions, outTerrainExists, outSurfaceWeights, callback);

    // Compute/store the final result
    for (size_t i = 0, iteratorIndex = 0; i < inPositions.size(); i++, iteratorIndex += indexStepSize)
    {
        switch(sampler)
        {
        case AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR:
            {
                // We now need to compute the final height after all the bulk queries are done.
                AZ::Vector2 normalizedDelta;
                AZ::Vector2 clampedPosition;
                ClampPosition(inPositions[i].GetX(), inPositions[i].GetY(), clampedPosition, normalizedDelta);
                const float heightX0Y0 = outPositions[iteratorIndex].GetZ();
                const float heightX1Y0 = outPositions[iteratorIndex + 1].GetZ();
                const float heightX0Y1 = outPositions[iteratorIndex + 2].GetZ();
                const float heightX1Y1 = outPositions[iteratorIndex + 3].GetZ();
                const float heightXY0 = AZ::Lerp(heightX0Y0, heightX1Y0, normalizedDelta.GetX());
                const float heightXY1 = AZ::Lerp(heightX0Y1, heightX1Y1, normalizedDelta.GetX());
                heights[i] = AZ::Lerp(heightXY0, heightXY1, normalizedDelta.GetY());
                terrainExists[i] = outTerrainExists[iteratorIndex];
            }
            break;
        case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
            [[fallthrough]];
        case AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT:
            [[fallthrough]];
        default:
            // For clamp and exact, we just need to store the results of the bulk query.
            heights[i] = outPositions[iteratorIndex].GetZ();
            terrainExists[i] = outTerrainExists[iteratorIndex];
            break;
        }
    }
}

float TerrainSystem::GetHeightSynchronous(float x, float y, Sampler sampler, bool* terrainExistsPtr) const
{
    bool terrainExists = false;
    float height = m_currentSettings.m_worldBounds.GetMin().GetZ();

    if (!InWorldBounds(x, y))
    {
        if (terrainExistsPtr)
        {
            *terrainExistsPtr = terrainExists;
            return height;
        }
    }

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    switch (sampler)
    {
    // Get the value at the requested location, using the terrain grid to bilinear filter between sample grid points.
    case AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR:
        {
            // pos0 contains one corner of our grid square, pos1 contains the opposite corner, and normalizedDelta is the fractional
            // amount the position exists between those corners.
            // Ex: (3.3, 4.4) would have a pos0 of (3, 4), a pos1 of (4, 5), and a delta of (0.3, 0.4).
            AZ::Vector2 normalizedDelta;
            AZ::Vector2 pos0;
            ClampPosition(x, y, pos0, normalizedDelta);
            const AZ::Vector2 pos1 = pos0 + AZ::Vector2(m_currentSettings.m_heightQueryResolution);

            const float heightX0Y0 = GetTerrainAreaHeight(pos0.GetX(), pos0.GetY(), terrainExists);
            const float heightX1Y0 = GetTerrainAreaHeight(pos1.GetX(), pos0.GetY(), terrainExists);
            const float heightX0Y1 = GetTerrainAreaHeight(pos0.GetX(), pos1.GetY(), terrainExists);
            const float heightX1Y1 = GetTerrainAreaHeight(pos1.GetX(), pos1.GetY(), terrainExists);
            const float heightXY0 = AZ::Lerp(heightX0Y0, heightX1Y0, normalizedDelta.GetX());
            const float heightXY1 = AZ::Lerp(heightX0Y1, heightX1Y1, normalizedDelta.GetX());
            height = AZ::Lerp(heightXY0, heightXY1, normalizedDelta.GetY());
        }
        break;

    //! Clamp the input point to the terrain sample grid, then get the height at the given grid location.
    case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
        {
            AZ::Vector2 normalizedDelta;
            AZ::Vector2 clampedPosition;
            ClampPosition(x, y, clampedPosition, normalizedDelta);

            height = GetTerrainAreaHeight(clampedPosition.GetX(), clampedPosition.GetY(), terrainExists);
        }
        break;

    //! Directly get the value at the location, regardless of terrain sample grid density.
    case AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT:
        [[fallthrough]];
    default:
        height = GetTerrainAreaHeight(x, y, terrainExists);
        break;
    }

    // For now, always set terrainExists to true, as we don't have a way to author data for terrain holes yet.
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = terrainExists;
    }

    return AZ::GetClamp(
        height, m_currentSettings.m_worldBounds.GetMin().GetZ(), m_currentSettings.m_worldBounds.GetMax().GetZ());
}

float TerrainSystem::GetTerrainAreaHeight(float x, float y, bool& terrainExists) const
{
    const float worldMin = m_currentSettings.m_worldBounds.GetMin().GetZ();
    AZ::Vector3 inPosition(x, y, worldMin);
    float height = worldMin;
    terrainExists = false;

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    for (auto& [areaId, areaData] : m_registeredAreas)
    {
        const float areaMin = areaData.m_areaBounds.GetMin().GetZ();
        inPosition.SetZ(areaMin);
        if (areaData.m_areaBounds.Contains(inPosition))
        {
            AZ::Vector3 outPosition;
            Terrain::TerrainAreaHeightRequestBus::Event(
                areaId, &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition, terrainExists);
            height = outPosition.GetZ();
            if (!terrainExists)
            {
                // If the terrain height provider doesn't have any data, then check the area's "use ground plane" setting.
                // If it's set, then create a default ground plane by saying terrain exists at the minimum height for the area.
                // Otherwise, we'll set the height at the terrain world minimum and say it doesn't exist.
                terrainExists = areaData.m_useGroundPlane;
                height = areaData.m_useGroundPlane ? areaMin : worldMin;
            }
            break;
        }
    }

    return height;
}

float TerrainSystem::GetHeight(const AZ::Vector3& position, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetHeightSynchronous(position.GetX(), position.GetY(), sampler, terrainExistsPtr);
}

float TerrainSystem::GetHeightFromVector2(const AZ::Vector2& position, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetHeightSynchronous(position.GetX(), position.GetY(), sampler, terrainExistsPtr);
}

float TerrainSystem::GetHeightFromFloats(float x, float y, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetHeightSynchronous(x, y, sampler, terrainExistsPtr);
}

bool TerrainSystem::GetIsHole(const AZ::Vector3& position, Sampler sampler) const
{
    bool terrainExists = false;
    GetHeightSynchronous(position.GetX(), position.GetY(), sampler, &terrainExists);
    return !terrainExists;
}

bool TerrainSystem::GetIsHoleFromVector2(const AZ::Vector2& position, Sampler sampler) const
{
    bool terrainExists = false;
    GetHeightSynchronous(position.GetX(), position.GetY(), sampler, &terrainExists);
    return !terrainExists;
}

bool TerrainSystem::GetIsHoleFromFloats(float x, float y, Sampler sampler) const
{
    bool terrainExists = false;
    GetHeightSynchronous(x, y, sampler, &terrainExists);
    return !terrainExists;
}

void TerrainSystem::GetNormalsSynchronous(const AZStd::span<const AZ::Vector3>& inPositions, Sampler sampler, 
    AZStd::span<AZ::Vector3> normals, AZStd::span<bool> terrainExists) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    AZStd::vector<AZ::Vector3> directionVectors;
    directionVectors.reserve(inPositions.size() * 4);
    const AZ::Vector2 range(m_currentSettings.m_heightQueryResolution / 2.0f, m_currentSettings.m_heightQueryResolution / 2.0f);
    size_t indexStepSize = 4;
    for (auto& position : inPositions)
    {
        directionVectors.emplace_back(position.GetX(), position.GetY() - range.GetY(), 0.0f);
        directionVectors.emplace_back(position.GetX() - range.GetX(), position.GetY(), 0.0f);
        directionVectors.emplace_back(position.GetX() + range.GetX(), position.GetY(), 0.0f);
        directionVectors.emplace_back(position.GetX(), position.GetY() + range.GetY(), 0.0f);
    }

    AZStd::vector<float> heights(directionVectors.size());
    AZStd::vector<bool> exists(directionVectors.size());
    GetHeightsSynchronous(directionVectors, sampler, heights, exists);

    for (size_t i = 0, iteratorIndex = 0; i < inPositions.size(); i++, iteratorIndex += indexStepSize)
    {
        directionVectors[iteratorIndex].SetZ(heights[iteratorIndex]);
        directionVectors[iteratorIndex + 1].SetZ(heights[iteratorIndex + 1]);
        directionVectors[iteratorIndex + 2].SetZ(heights[iteratorIndex + 2]);
        directionVectors[iteratorIndex + 3].SetZ(heights[iteratorIndex + 3]);

        normals[i] = (directionVectors[iteratorIndex + 2] - directionVectors[iteratorIndex + 1]).
                         Cross(directionVectors[iteratorIndex + 3] - directionVectors[iteratorIndex]).GetNormalized();

        // This needs better logic for handling cases where some points exist and some don't, but for now we'll say that if
        // any of the four points exist, then the terrain exists.
        terrainExists[i] = exists[iteratorIndex] || exists[iteratorIndex + 1] || exists [iteratorIndex + 2] || exists[iteratorIndex + 3];
    }
}

AZ::Vector3 TerrainSystem::GetNormalSynchronous(float x, float y, Sampler sampler, bool* terrainExistsPtr) const
{
    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    bool terrainExists = false;

    AZ::Vector3 outNormal = AZ::Vector3::CreateAxisZ();

    if (!InWorldBounds(x, y))
    {
        if (terrainExistsPtr)
        {
            *terrainExistsPtr = terrainExists;
            return outNormal;
        }
    }
    float range = m_currentSettings.m_heightQueryResolution / 2.0f;
    const AZ::Vector2 left (x - range, y);
    const AZ::Vector2 right(x + range, y);
    const AZ::Vector2 up   (x, y - range);
    const AZ::Vector2 down (x, y + range);

    bool terrainExists1 = false;
    bool terrainExists2 = false;
    bool terrainExists3 = false;
    bool terrainExists4 = false;

    AZ::Vector3 v1(up.GetX(), up.GetY(), GetHeightSynchronous(up.GetX(), up.GetY(), sampler, &terrainExists1));
    AZ::Vector3 v2(left.GetX(), left.GetY(), GetHeightSynchronous(left.GetX(), left.GetY(), sampler, &terrainExists2));
    AZ::Vector3 v3(right.GetX(), right.GetY(), GetHeightSynchronous(right.GetX(), right.GetY(), sampler, &terrainExists3));
    AZ::Vector3 v4(down.GetX(), down.GetY(), GetHeightSynchronous(down.GetX(), down.GetY(), sampler, &terrainExists4));

    outNormal = (v3 - v2).Cross(v4 - v1).GetNormalized();

    if (terrainExistsPtr)
    {
        // This needs better logic for handling cases where some points exist and some don't, but for now we'll say that if
        // any of the four points exist, then the terrain exists.
        *terrainExistsPtr = terrainExists1 || terrainExists2 || terrainExists3 || terrainExists4;
    }

    return outNormal;
}

AZ::Vector3 TerrainSystem::GetNormal(const AZ::Vector3& position, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetNormalSynchronous(position.GetX(), position.GetY(), sampler, terrainExistsPtr);
}

AZ::Vector3 TerrainSystem::GetNormalFromVector2(const AZ::Vector2& position, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetNormalSynchronous(position.GetX(), position.GetY(), sampler, terrainExistsPtr);
}

AZ::Vector3 TerrainSystem::GetNormalFromFloats(float x, float y, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetNormalSynchronous(x, y, sampler, terrainExistsPtr);
}

AzFramework::SurfaceData::SurfaceTagWeight TerrainSystem::GetMaxSurfaceWeight(
    const AZ::Vector3& position, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetMaxSurfaceWeightFromFloats(position.GetX(), position.GetY(), sampler, terrainExistsPtr);
}

AzFramework::SurfaceData::SurfaceTagWeight TerrainSystem::GetMaxSurfaceWeightFromVector2(
    const AZ::Vector2& inPosition, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetMaxSurfaceWeightFromFloats(inPosition.GetX(), inPosition.GetY(), sampler, terrainExistsPtr);
}

AzFramework::SurfaceData::SurfaceTagWeight TerrainSystem::GetMaxSurfaceWeightFromFloats(
    const float x, const float y, Sampler sampler, bool* terrainExistsPtr) const
{
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    AzFramework::SurfaceData::SurfaceTagWeightList weightSet;

    if (!InWorldBounds(x, y))
    {
        if (terrainExistsPtr)
        {
            *terrainExistsPtr = false;
            return {};
        }
    }

    GetOrderedSurfaceWeights(x, y, sampler, weightSet, terrainExistsPtr);

    if (weightSet.empty())
    {
        return {};
    }

    return *weightSet.begin();
}

void TerrainSystem::GetSurfacePoint(
    const AZ::Vector3& inPosition,
    AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
    Sampler sampler,
    bool* terrainExistsPtr) const
{
    outSurfacePoint.m_position = inPosition;
    outSurfacePoint.m_position.SetZ(GetHeightSynchronous(inPosition.GetX(), inPosition.GetY(), sampler, terrainExistsPtr));
    outSurfacePoint.m_normal = GetNormalSynchronous(inPosition.GetX(), inPosition.GetY(), sampler, nullptr);
    GetSurfaceWeights(inPosition, outSurfacePoint.m_surfaceTags, sampler, nullptr);
}

void TerrainSystem::GetSurfacePointFromVector2(
    const AZ::Vector2& inPosition,
    AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
    Sampler sampler,
    bool* terrainExistsPtr) const
{
    GetSurfacePoint(AZ::Vector3(inPosition.GetX(), inPosition.GetY(), 0.0f), outSurfacePoint, sampler, terrainExistsPtr);
}

void TerrainSystem::GetSurfacePointFromFloats(
    float x,
    float y,
    AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
    Sampler sampler,
    bool* terrainExistsPtr) const
{
    GetSurfacePoint(AZ::Vector3(x, y, 0.0f), outSurfacePoint, sampler, terrainExistsPtr);
}

AzFramework::EntityContextId TerrainSystem::GetTerrainRaycastEntityContextId() const
{
    return m_terrainRaycastContext.GetEntityContextId();
}

AzFramework::RenderGeometry::RayResult TerrainSystem::GetClosestIntersection(
    const AzFramework::RenderGeometry::RayRequest& ray) const
{
    return m_terrainRaycastContext.RayIntersect(ray);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessHeightsFromListAsync(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromListAsync(AZStd::bind(&TerrainSystem::ProcessHeightsFromList, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        inPositions, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessNormalsFromListAsync(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromListAsync(AZStd::bind(&TerrainSystem::ProcessNormalsFromList, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        inPositions, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessSurfaceWeightsFromListAsync(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromListAsync(AZStd::bind(&TerrainSystem::ProcessSurfaceWeightsFromList, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        inPositions, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessSurfacePointsFromListAsync(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromListAsync(AZStd::bind(&TerrainSystem::ProcessSurfacePointsFromList, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        inPositions, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessHeightsFromListOfVector2Async(
    const AZStd::span<const AZ::Vector2>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromListAsync(AZStd::bind(&TerrainSystem::ProcessHeightsFromListOfVector2, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        inPositions, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessNormalsFromListOfVector2Async(
    const AZStd::span<const AZ::Vector2>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromListAsync(AZStd::bind(&TerrainSystem::ProcessNormalsFromListOfVector2, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        inPositions, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessSurfaceWeightsFromListOfVector2Async(
    const AZStd::span<const AZ::Vector2>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromListAsync(AZStd::bind(&TerrainSystem::ProcessSurfaceWeightsFromListOfVector2, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        inPositions, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessSurfacePointsFromListOfVector2Async(
    const AZStd::span<const AZ::Vector2>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromListAsync(AZStd::bind(&TerrainSystem::ProcessSurfacePointsFromListOfVector2, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        inPositions, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessHeightsFromRegionAsync(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromRegionAsync(AZStd::bind(&TerrainSystem::ProcessHeightsFromRegion, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
        inRegion, stepSize, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessNormalsFromRegionAsync(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromRegionAsync(AZStd::bind(&TerrainSystem::ProcessNormalsFromRegion, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
        inRegion, stepSize, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessSurfaceWeightsFromRegionAsync(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromRegionAsync(AZStd::bind(&TerrainSystem::ProcessSurfaceWeightsFromRegion, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
        inRegion, stepSize, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessSurfacePointsFromRegionAsync(
    const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler,
            AZStd::shared_ptr<ProcessAsyncParams> params) const
{
    return ProcessFromRegionAsync(AZStd::bind(&TerrainSystem::ProcessSurfacePointsFromRegion, this, AZStd::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
        inRegion, stepSize, perPositionCallback, sampler, params);
}

AZ::EntityId TerrainSystem::FindBestAreaEntityAtPosition(const AZ::Vector3& position, AZ::Aabb& bounds) const
{
    // Find the highest priority layer that encompasses this position
    // The areas are sorted into priority order: the first area that contains inPosition is the most suitable.
    for (const auto& [areaId, areaData] : m_registeredAreas)
    {
        if (SurfaceData::AabbContains2D(areaData.m_areaBounds, position))
        {
            bounds = areaData.m_areaBounds;
            return areaId;
        }
    }

    return AZ::EntityId();
}

void TerrainSystem::GetOrderedSurfaceWeightsFromList(
    const AZStd::span<const AZ::Vector3>& inPositions,
    [[maybe_unused]] Sampler sampler,
    AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList,
    AZStd::span<bool> terrainExists) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    if (terrainExists.size() == outSurfaceWeightsList.size())
    {
        AZStd::vector<float> heights(inPositions.size());
        GetHeightsSynchronous(inPositions, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, heights, terrainExists);
    }

    auto callback = [](const AZStd::span<const AZ::Vector3> inPositions,
                        [[maybe_unused]] AZStd::span<AZ::Vector3> outPositions,
                        [[maybe_unused]] AZStd::span<bool> outTerrainExists,
                        AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeights,
                        AZ::EntityId areaId)
                        {
                            AZ_Assert(inPositions.size() == outSurfaceWeights.size(),
                                "The sizes of the surface weights list and in/out positions list should match.");
                            Terrain::TerrainAreaSurfaceRequestBus::Event(areaId, &Terrain::TerrainAreaSurfaceRequestBus::Events::GetSurfaceWeightsFromList,
                                inPositions, outSurfaceWeights);

                            // Sort the surface weights on each output weight list in decreasing weight order.
                            for (auto& outSurfaceWeight : outSurfaceWeights)
                            {
                                AZStd::sort(
                                    outSurfaceWeight.begin(), outSurfaceWeight.end(),
                                    AzFramework::SurfaceData::SurfaceTagWeightComparator());
                            }
                        };
    
    // This will be unused for surface weights. It's fine if it's empty.
    AZStd::vector<AZ::Vector3> outPositions;
    MakeBulkQueries(inPositions, outPositions, terrainExists, outSurfaceWeightsList, callback);
}

void TerrainSystem::GetOrderedSurfaceWeights(
    const float x,
    const float y,
    [[maybe_unused]] Sampler sampler,
    AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
    bool* terrainExistsPtr) const
{
    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    AZ::Aabb bounds;
    AZ::EntityId bestAreaId = FindBestAreaEntityAtPosition(AZ::Vector3(x, y, 0.0f), bounds);

    if (terrainExistsPtr)
    {
        GetHeightFromFloats(x, y, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, terrainExistsPtr);
    }

    outSurfaceWeights.clear();

    if (!bestAreaId.IsValid())
    {
        return;
    }

    const AZ::Vector3 inPosition = AZ::Vector3(x, y, 0.0f);

    // Get all the surfaces with weights at the given point.
    Terrain::TerrainAreaSurfaceRequestBus::Event(
        bestAreaId, &Terrain::TerrainAreaSurfaceRequestBus::Events::GetSurfaceWeights, inPosition, outSurfaceWeights);

    AZStd::sort(outSurfaceWeights.begin(), outSurfaceWeights.end(), AzFramework::SurfaceData::SurfaceTagWeightComparator());
}

void TerrainSystem::GetSurfaceWeights(
    const AZ::Vector3& inPosition,
    AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
    Sampler sampler,
    bool* terrainExistsPtr) const
{
    GetOrderedSurfaceWeights(inPosition.GetX(), inPosition.GetY(), sampler, outSurfaceWeights, terrainExistsPtr);
}

void TerrainSystem::GetSurfaceWeightsFromVector2(
    const AZ::Vector2& inPosition,
    AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
    Sampler sampler,
    bool* terrainExistsPtr) const
{
    GetOrderedSurfaceWeights(inPosition.GetX(), inPosition.GetY(), sampler, outSurfaceWeights, terrainExistsPtr);
}

void TerrainSystem::GetSurfaceWeightsFromFloats(
    float x, float y,
    AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
    Sampler sampler,
    bool* terrainExistsPtr) const
{
    GetOrderedSurfaceWeights(x, y, sampler, outSurfaceWeights, terrainExistsPtr);
}

const char* TerrainSystem::GetMaxSurfaceName(
    [[maybe_unused]] const AZ::Vector3& position, [[maybe_unused]] Sampler sampler, [[maybe_unused]] bool* terrainExistsPtr) const
{
    // For now, always set terrainExists to true, as we don't have a way to author data for terrain holes yet.
    if (terrainExistsPtr)
    {
        *terrainExistsPtr = true;
    }

    return "";
}

void TerrainSystem::ProcessHeightsFromList(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<bool> terrainExists(inPositions.size());
    AZStd::vector<float> heights(inPositions.size());

    GetHeightsSynchronous(inPositions, sampler, heights, terrainExists);

    AzFramework::SurfaceData::SurfacePoint surfacePoint;
    for (size_t i = 0; i < inPositions.size(); i++)
    {
        surfacePoint.m_position = inPositions[i];
        surfacePoint.m_position.SetZ(heights[i]);
        perPositionCallback(surfacePoint, terrainExists[i]);
    }
}

void TerrainSystem::ProcessNormalsFromList(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<bool> terrainExists(inPositions.size());
    AZStd::vector<AZ::Vector3> normals(inPositions.size());

    GetNormalsSynchronous(inPositions, sampler, normals, terrainExists);

    AzFramework::SurfaceData::SurfacePoint surfacePoint;
    for (size_t i = 0; i < inPositions.size(); i++)
    {
        surfacePoint.m_position = inPositions[i];
        surfacePoint.m_normal = AZStd::move(normals[i]);
        perPositionCallback(surfacePoint, terrainExists[i]);
    }
}

void TerrainSystem::ProcessSurfaceWeightsFromList(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList(inPositions.size());
    AZStd::vector<bool> terrainExists(inPositions.size());

    GetOrderedSurfaceWeightsFromList(inPositions, sampler, outSurfaceWeightsList, terrainExists);

    AzFramework::SurfaceData::SurfacePoint surfacePoint;
    for (size_t i = 0; i < inPositions.size(); i++)
    {
        surfacePoint.m_position = inPositions[i];
        surfacePoint.m_surfaceTags = AZStd::move(outSurfaceWeightsList[i]);
        perPositionCallback(surfacePoint, terrainExists[i]);
    }
}

void TerrainSystem::ProcessSurfacePointsFromList(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<float> heights(inPositions.size());
    AZStd::vector<AZ::Vector3> normals(inPositions.size());
    AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList(inPositions.size());
    AZStd::vector<bool> terrainExists(inPositions.size());

    GetHeightsSynchronous(inPositions, sampler, heights, terrainExists);
    GetNormalsSynchronous(inPositions, sampler, normals, terrainExists);

    // We can skip the unnecessary call to GetHeights since we already
    // got the terrain exists flags in the earlier call to GetHeights
    AZStd::vector<bool> terrainExistsEmpty;
    GetOrderedSurfaceWeightsFromList(inPositions, sampler, outSurfaceWeightsList, terrainExistsEmpty);

    AzFramework::SurfaceData::SurfacePoint surfacePoint;
    for (size_t i = 0; i < inPositions.size(); i++)
    {
        surfacePoint.m_position.Set(inPositions[i].GetX(), inPositions[i].GetY(), heights[i]);
        surfacePoint.m_normal = AZStd::move(normals[i]);
        surfacePoint.m_surfaceTags = AZStd::move(outSurfaceWeightsList[i]);
        perPositionCallback(surfacePoint, terrainExists[i]);
    }
}

void TerrainSystem::ProcessHeightsFromListOfVector2(
    const AZStd::span<const AZ::Vector2>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<AZ::Vector3> inPositionsVec3 = GenerateInputPositionsFromListOfVector2(inPositions);

    ProcessHeightsFromList(inPositionsVec3, perPositionCallback, sampler);
}

void TerrainSystem::ProcessNormalsFromListOfVector2(
    const AZStd::span<const AZ::Vector2>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<AZ::Vector3> inPositionsVec3 = GenerateInputPositionsFromListOfVector2(inPositions);

    ProcessNormalsFromList(inPositionsVec3, perPositionCallback, sampler);
}

void TerrainSystem::ProcessSurfaceWeightsFromListOfVector2(
    const AZStd::span<const AZ::Vector2>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<AZ::Vector3> inPositionsVec3 = GenerateInputPositionsFromListOfVector2(inPositions);

    ProcessSurfaceWeightsFromList(inPositionsVec3, perPositionCallback, sampler);
}

void TerrainSystem::ProcessSurfacePointsFromListOfVector2(
    const AZStd::span<const AZ::Vector2>& inPositions,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<AZ::Vector3> inPositionsVec3 = GenerateInputPositionsFromListOfVector2(inPositions);

    ProcessSurfacePointsFromList(inPositionsVec3, perPositionCallback, sampler);
}

AZStd::pair<size_t, size_t> TerrainSystem::GetNumSamplesFromRegion(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    Sampler sampler) const
{
    size_t countX = 0;
    size_t countY = 0;
    
    switch (sampler)
    {
    case Sampler::CLAMP:
    {
        // Only consider points that line up with the query resolution
        const int32_t firstSampleX = aznumeric_cast<int32_t>(AZStd::ceilf(inRegion.GetMin().GetX() / stepSize.GetX()));
        const int32_t lastSampleX = aznumeric_cast<int32_t>(AZStd::floorf(inRegion.GetMax().GetX() / stepSize.GetX()));
        const int32_t firstSampleY = aznumeric_cast<int32_t>(AZStd::ceilf(inRegion.GetMin().GetY() / stepSize.GetY()));
        const int32_t lastSampleY = aznumeric_cast<int32_t>(AZStd::floorf(inRegion.GetMax().GetY() / stepSize.GetY()));
        countX = lastSampleX - firstSampleX + 1;
        countY = lastSampleY - firstSampleY + 1;
    }
    default:
        countX = aznumeric_cast<size_t>(AZStd::floorf(inRegion.GetExtents().GetX() / stepSize.GetX()));
        countY = aznumeric_cast<size_t>(AZStd::floorf(inRegion.GetExtents().GetY() / stepSize.GetY()));
    }
    
    return AZStd::make_pair(countX, countY);
}

void TerrainSystem::ProcessHeightsFromRegion(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler) const
{
    // Don't bother processing if we don't have a callback
    if (!perPositionCallback)
    {
        return;
    }

    const auto [numSamplesX, numSamplesY] = GetNumSamplesFromRegion(inRegion, stepSize, sampler);

    AZStd::vector<AZ::Vector3> inPositions = GenerateInputPositionsFromRegion(inRegion, stepSize, sampler);

    if (inPositions.empty())
    {
        return;
    }

    AZStd::vector<bool> terrainExists(inPositions.size());
    AZStd::vector<float> heights(inPositions.size());

    GetHeightsSynchronous(inPositions, sampler, heights, terrainExists);
    
    AzFramework::SurfaceData::SurfacePoint surfacePoint;
    for (size_t y = 0, i = 0; y < numSamplesY; y++)
    {
        for (size_t x = 0; x < numSamplesX; x++)
        {
            surfacePoint.m_position.Set(inPositions[i].GetX(), inPositions[i].GetY(), heights[i]);
            perPositionCallback(x, y, surfacePoint, terrainExists[i]);
            i++;
        }
    }
}

void TerrainSystem::ProcessNormalsFromRegion(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler) const
{
    // Don't bother processing if we don't have a callback
    if (!perPositionCallback)
    {
        return;
    }

    const auto [numSamplesX, numSamplesY] = GetNumSamplesFromRegion(inRegion, stepSize, sampler);

    AZStd::vector<AZ::Vector3> inPositions = GenerateInputPositionsFromRegion(inRegion, stepSize, sampler);

    if (inPositions.empty())
    {
        return;
    }

    AZStd::vector<bool> terrainExists(inPositions.size());
    AZStd::vector<AZ::Vector3> normals(inPositions.size());

    GetNormalsSynchronous(inPositions, sampler, normals, terrainExists);

    AzFramework::SurfaceData::SurfacePoint surfacePoint;
    for (size_t y = 0, i = 0; y < numSamplesY; y++)
    {
        for (size_t x = 0; x < numSamplesX; x++)
        {
            surfacePoint.m_position.Set(inPositions[i].GetX(), inPositions[i].GetY(), 0.0f);
            surfacePoint.m_normal = AZStd::move(normals[i]);
            perPositionCallback(x, y, surfacePoint, terrainExists[i]);
            i++;
        }
    }
}

void TerrainSystem::ProcessSurfaceWeightsFromRegion(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler) const
{
    // Don't bother processing if we don't have a callback
    if (!perPositionCallback)
    {
        return;
    }

    const auto [numSamplesX, numSamplesY] = GetNumSamplesFromRegion(inRegion, stepSize, sampler);

    AZStd::vector<AZ::Vector3> inPositions = GenerateInputPositionsFromRegion(inRegion, stepSize, sampler);

    if (inPositions.empty())
    {
        return;
    }

    AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList(inPositions.size());
    AZStd::vector<bool> terrainExists(inPositions.size());

    GetOrderedSurfaceWeightsFromList(inPositions, sampler, outSurfaceWeightsList, terrainExists);

    AzFramework::SurfaceData::SurfacePoint surfacePoint;
    for (size_t y = 0, i = 0; y < numSamplesY; y++)
    {
        for (size_t x = 0; x < numSamplesX; x++)
        {
            surfacePoint.m_position.Set(inPositions[i].GetX(), inPositions[i].GetY(), 0.0f);
            surfacePoint.m_surfaceTags = AZStd::move(outSurfaceWeightsList[i]);
            perPositionCallback(x, y, surfacePoint, terrainExists[i]);
            i++;
        }
    }
}

void TerrainSystem::ProcessSurfacePointsFromRegion(
    const AZ::Aabb& inRegion,
    const AZ::Vector2& stepSize,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler) const
{
    // Don't bother processing if we don't have a callback
    if (!perPositionCallback)
    {
        return;
    }

    const auto [numSamplesX, numSamplesY] = GetNumSamplesFromRegion(inRegion, stepSize, sampler);

    AZStd::vector<AZ::Vector3> inPositions = GenerateInputPositionsFromRegion(inRegion, stepSize, sampler);

    if (inPositions.empty())
    {
        return;
    }

    AZStd::vector<float> heights(inPositions.size());
    AZStd::vector<AZ::Vector3> normals(inPositions.size());
    AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList(inPositions.size());
    AZStd::vector<bool> terrainExists(inPositions.size());

    GetHeightsSynchronous(inPositions, sampler, heights, terrainExists);
    GetNormalsSynchronous(inPositions, sampler, normals, terrainExists);

    // We can skip the unnecessary call to GetHeights since we already
    // got the terrain exists flags in the earlier call to GetHeights
    AZStd::vector<bool> terrainExistsEmpty;
    GetOrderedSurfaceWeightsFromList(inPositions, sampler, outSurfaceWeightsList, terrainExistsEmpty);

    AzFramework::SurfaceData::SurfacePoint surfacePoint;
    for (size_t y = 0, i = 0; y < numSamplesY; y++)
    {
        for (size_t x = 0; x < numSamplesX; x++)
        {
            surfacePoint.m_position.Set(inPositions[i].GetX(), inPositions[i].GetY(), heights[i]);
            surfacePoint.m_normal = AZStd::move(normals[i]);
            surfacePoint.m_surfaceTags = AZStd::move(outSurfaceWeightsList[i]);
            perPositionCallback(x, y, surfacePoint, terrainExists[i]);
            i++;
        }
    }
}

void TerrainSystem::RegisterArea(AZ::EntityId areaId)
{
    AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);
    AZ::Aabb aabb = AZ::Aabb::CreateNull();
    LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

    // Cache off whether or not this layer spawner should have a default ground plane when no other terrain height data exists.
    bool useGroundPlane = false;
    Terrain::TerrainSpawnerRequestBus::EventResult(useGroundPlane, areaId, &Terrain::TerrainSpawnerRequestBus::Events::GetUseGroundPlane);

    m_registeredAreas[areaId] = { aabb, useGroundPlane };
    m_dirtyRegion.AddAabb(aabb);
    m_terrainHeightDirty = true;
    m_terrainSurfacesDirty = true;
}

void TerrainSystem::UnregisterArea(AZ::EntityId areaId)
{
    AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);

    // Remove the data for this entity from the registered areas.
    // Erase_if is used as erase would use the comparator to lookup the entity id in the map.
    // As the comparator will get the new layer/priority data for the entity, the id lookup will fail.
    AZStd::erase_if(
        m_registeredAreas,
        [areaId, this](const auto& item)
        {
            auto const& [entityId, areaData] = item;
            if (areaId == entityId)
            {
                m_dirtyRegion.AddAabb(areaData.m_areaBounds);
                m_terrainHeightDirty = true;
                m_terrainSurfacesDirty = true;
                return true;
            }
            return false;
        });
}

void TerrainSystem::RefreshArea(AZ::EntityId areaId, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask changeMask)
{
    using Terrain = AzFramework::Terrain::TerrainDataNotifications;

    AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);

    auto areaAabb = m_registeredAreas.find(areaId);

    AZ::Aabb oldAabb = (areaAabb != m_registeredAreas.end()) ? areaAabb->second.m_areaBounds : AZ::Aabb::CreateNull();
    AZ::Aabb newAabb = AZ::Aabb::CreateNull();
    LmbrCentral::ShapeComponentRequestsBus::EventResult(newAabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
    m_registeredAreas[areaId].m_areaBounds = newAabb;

    AZ::Aabb expandedAabb = oldAabb;
    expandedAabb.AddAabb(newAabb);

    m_dirtyRegion.AddAabb(expandedAabb);

    // Keep track of which types of data have changed so that we can send out the appropriate notifications later.

    m_terrainHeightDirty = m_terrainHeightDirty || ((changeMask & Terrain::HeightData) == Terrain::HeightData);

    m_terrainSurfacesDirty = m_terrainSurfacesDirty || ((changeMask & Terrain::SurfaceData) == Terrain::SurfaceData);
}

void TerrainSystem::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
{
    using Terrain = AzFramework::Terrain::TerrainDataNotifications;

    bool terrainSettingsChanged = false;

    if (m_terrainSettingsDirty)
    {
        terrainSettingsChanged = true;
        m_terrainSettingsDirty = false;

        // This needs to happen before the "system active" check below, because activating the system will cause the various
        // terrain layer areas to request the current world bounds.
        if (m_requestedSettings.m_worldBounds != m_currentSettings.m_worldBounds)
        {
            m_dirtyRegion = m_currentSettings.m_worldBounds;
            m_dirtyRegion.AddAabb(m_requestedSettings.m_worldBounds);
            m_terrainHeightDirty = true;
            m_terrainSurfacesDirty = true;
            m_currentSettings.m_worldBounds = m_requestedSettings.m_worldBounds;
        }

        if (m_requestedSettings.m_heightQueryResolution != m_currentSettings.m_heightQueryResolution)
        {
            m_dirtyRegion.AddAabb(m_requestedSettings.m_worldBounds);
            m_terrainHeightDirty = true;
        }

        if (m_requestedSettings.m_surfaceDataQueryResolution != m_currentSettings.m_surfaceDataQueryResolution)
        {
            m_dirtyRegion.AddAabb(m_requestedSettings.m_worldBounds);
            m_terrainSurfacesDirty = true;
        }

        m_currentSettings = m_requestedSettings;
    }

    if (terrainSettingsChanged || m_terrainHeightDirty || m_terrainSurfacesDirty)
    {
        // Block other threads from accessing the surface data bus while we are in GetValue (which may call into the SurfaceData bus).
        // This prevents lock inversion deadlocks between this calling Gradient->Surface and something else calling Surface->Gradient.
        auto& surfaceDataContext = SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext(false);
        typename SurfaceData::SurfaceDataSystemRequestBus::Context::DispatchLockGuard scopeLock(surfaceDataContext.m_contextMutex);

        Terrain::TerrainDataChangedMask changeMask = Terrain::TerrainDataChangedMask::None;

        if (terrainSettingsChanged)
        {
            changeMask = static_cast<Terrain::TerrainDataChangedMask>(changeMask | Terrain::TerrainDataChangedMask::Settings);
        }
        if (m_terrainHeightDirty)
        {
            changeMask = static_cast<Terrain::TerrainDataChangedMask>(changeMask | Terrain::TerrainDataChangedMask::HeightData);
        }

        if (m_terrainSurfacesDirty)
        {
            changeMask = static_cast<Terrain::TerrainDataChangedMask>(changeMask | Terrain::TerrainDataChangedMask::SurfaceData);
        }

        // Make sure to set these *before* calling OnTerrainDataChanged, since it's possible that subsystems reacting to that call will
        // cause the data to become dirty again.
        AZ::Aabb dirtyRegion = m_dirtyRegion;
        m_terrainHeightDirty = false;
        m_terrainSurfacesDirty = false;
        m_dirtyRegion = AZ::Aabb::CreateNull();

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(
            &AzFramework::Terrain::TerrainDataNotificationBus::Events::OnTerrainDataChanged, dirtyRegion,
            changeMask);
    }

}
