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
#include <TerrainProfiler.h>

using namespace Terrain;

AZ_DEFINE_BUDGET(Terrain);

bool TerrainLayerPriorityComparator::operator()(const AZ::EntityId& layer1id, const AZ::EntityId& layer2id) const
{
    // Comparator for insertion/key lookup.
    // Sorts into layer/priority order, highest priority first.
    int32_t priority1 = 0;
    uint32_t layer1 = 0;
    Terrain::TerrainSpawnerRequestBus::Event(layer1id, &Terrain::TerrainSpawnerRequestBus::Events::GetPriority, layer1, priority1);

    int32_t priority2 = 0;
    uint32_t layer2 = 0;
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
    m_currentSettings.m_heightRange = AzFramework::Terrain::FloatRange::CreateNull();

    m_requestedSettings = m_currentSettings;
    m_requestedSettings.m_heightRange = { -512.0f, 512.0f };

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
    m_terrainDirtyMask = AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::All;
    m_requestedSettings.m_systemActive = true;
    m_cachedAreaBounds = AZ::Aabb::CreateNull();

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
    m_terrainDirtyMask = AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::All;
    m_requestedSettings.m_systemActive = false;

    AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(
        &AzFramework::Terrain::TerrainDataNotificationBus::Events::OnTerrainDataDestroyEnd);
}

void TerrainSystem::SetTerrainHeightBounds(const AzFramework::Terrain::FloatRange& heightRange)
{   
    m_requestedSettings.m_heightRange = heightRange;
    m_terrainDirtyMask |= AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::Settings;
}

bool TerrainSystem::TerrainAreaExistsInBounds(const AZ::Aabb& bounds) const
{
    for (const auto& area : m_registeredAreas)
    {
        if (area.second.m_areaBounds.Overlaps(bounds))
        {
            return true;
        }
    }
    return false;
}

void TerrainSystem::SetTerrainHeightQueryResolution(float queryResolution)
{
    m_requestedSettings.m_heightQueryResolution = queryResolution;
    m_terrainDirtyMask |= AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::Settings;
}

void TerrainSystem::SetTerrainSurfaceDataQueryResolution(float queryResolution)
{
    m_requestedSettings.m_surfaceDataQueryResolution = queryResolution;
    m_terrainDirtyMask |= AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::Settings;
}

AZ::Aabb TerrainSystem::GetTerrainAabb() const
{
    return ClampZBoundsToHeightBounds(m_cachedAreaBounds);
}

AzFramework::Terrain::FloatRange TerrainSystem::GetTerrainHeightBounds() const
{
    return m_currentSettings.m_heightRange;
}

float TerrainSystem::GetTerrainHeightQueryResolution() const
{
    return m_currentSettings.m_heightQueryResolution;
}

float TerrainSystem::GetTerrainSurfaceDataQueryResolution() const
{
    return m_currentSettings.m_surfaceDataQueryResolution;
}

void TerrainSystem::ClampPosition(float x, float y, float queryResolution, AZ::Vector2& outPosition, AZ::Vector2& normalizedDelta)
{
    // Given an input position, clamp the values to our terrain grid, where it will always go to the terrain grid point
    // at a lower value, whether positive or negative.  Ex: 3.3 -> 3, -3.3 -> -4
    // Also, return the normalized delta as a value of [0-1) describing what fraction of a grid point the value moved.

    // Scale the position by the query resolution, so that integer values represent exact steps on the grid,
    // and fractional values are the amount in-between each grid point, in the range [0-1).
    AZ::Vector2 normalizedPosition = AZ::Vector2(x, y) / queryResolution;
    normalizedDelta = AZ::Vector2(
        normalizedPosition.GetX() - floor(normalizedPosition.GetX()), normalizedPosition.GetY() - floor(normalizedPosition.GetY()));

    // Remove the fractional part, then scale back down into world space.
    outPosition = (normalizedPosition - normalizedDelta) * queryResolution;
}

void TerrainSystem::RoundPosition(float x, float y, float queryResolution, AZ::Vector2& outPosition)
{
    // Given an input position, clamp the values to our terrain grid, where it will always go to the nearest terrain grid point
    // whether positive or negative.  Ex: 3.3 -> 3, 3.6 -> 4, -3.3 -> -3, -3.6 -> -4

    // Scale the position by the query resolution, so that integer values represent exact steps on the grid,
    // and fractional values are the amount in-between each grid point, in the range [0-1).
    AZ::Vector2 normalizedPosition = AZ::Vector2(x, y) / queryResolution;

    // Round the fractional part, then scale back down into world space.
    // Note that we use "floor(pos + 0.5f)" instead of round() because round() will round to the nearest even integer (banker's rounding)
    // on the 0.5 points instead of to the nearest integer biased away from 0 (symmetric arithmetic rounding), which is what we want.
    // "floor(pos + 0.5f)" will round 1.5 -> 2, 2.5 -> 3, -1.5 -> -2, -2.5 -> -3, etc. 
    // (i.e. don't use: outPosition = normalizedPosition.GetRound() * queryResolution;)
    outPosition = (normalizedPosition + AZ::Vector2(0.5f)).GetFloor() * queryResolution;
}

void TerrainSystem::InterpolateHeights(const AZStd::array<float,4>& heights, const AZStd::array<bool,4>& exists,
    float lerpX, float lerpY, float& outHeight, bool& outExists)
{
    // When interpolating between 4 height points, we also need to take the existence of the 4 points into account.
    // The logic below uses a precomputed lookup table to determine how to interpolate the points in each combination of existence.
    // The final "terrain exists" flag gets computed based on the existence of the corner that's closest to the interpolated point.

    uint8_t indexLookup = (exists[3] << 3) | (exists[2] << 2) | (exists[1] << 1) | (exists[0] << 0);

    constexpr uint8_t heightIndices[16][4] =
    {
                        // x0y0 x1y0 x0y1 x1y1  output
        { 0, 0, 0, 0 }, // F    F    F    F     x0y0
        { 0, 0, 0, 0 }, // T    F    F    F     x0y0
        { 1, 1, 1, 1 }, // F    T    F    F     x1y0
        { 0, 1, 0, 1 }, // T    T    F    F     lerp(x0y0, x1y0)
        { 2, 2, 2, 2 }, // F    F    T    F     x0y1
        { 0, 0, 2, 2 }, // T    F    T    F     lerp(x0y0, x0y1)
        { 1, 1, 2, 2 }, // F    T    T    F     lerp(x1y0, x0y1)
        { 0, 1, 2, 2 }, // T    T    T    F     lerp(lerp(x0y0, x1y0), x0y1)

        { 3, 3, 3, 3 }, // F    F    F    T     x1y1
        { 0, 0, 3, 3 }, // T    F    F    T     lerp(x0y0, x1y1)
        { 1, 1, 3, 3 }, // F    T    F    T     lerp(x1y0, x1y1)
        { 0, 1, 3, 3 }, // T    T    F    T     lerp(lerp(x0y0, x1y0), x1y1)
        { 2, 3, 2, 3 }, // F    F    T    T     lerp(x0y1, x1y1)
        { 0, 0, 2, 3 }, // T    F    T    T     lerp(x0y0, lerp(x0y1, x1y1))
        { 1, 1, 2, 2 }, // F    T    T    T     lerp(x1y0, lerp(x0y1, x1y1))
        { 0, 1, 2, 3 }, // T    T    T    T     lerp(lerp(x0y0, x1y0), lerp(x0y1, x1y1))
    };

    const float heightX0Y0 = heights[heightIndices[indexLookup][0]];
    const float heightX1Y0 = heights[heightIndices[indexLookup][1]];
    const float heightX0Y1 = heights[heightIndices[indexLookup][2]];
    const float heightX1Y1 = heights[heightIndices[indexLookup][3]];

    const float heightXY0 = AZ::Lerp(heightX0Y0, heightX1Y0, lerpX);
    const float heightXY1 = AZ::Lerp(heightX0Y1, heightX1Y1, lerpX);
    outHeight = AZ::Lerp(heightXY0, heightXY1, lerpY);

    // "Terrain exists" is set based on the existance of the nearest vertex to the point,
    // which is determined by which 1/4 of the quad the point falls in. We can determine that based on
    // which side of 0.5 our lerp X and Y values land on.
    uint8_t existsIndex = ((lerpY >= 0.5f) << 1) | (lerpX >= 0.5f);
    outExists = exists[existsIndex];
}

void TerrainSystem::RecalculateCachedBounds()
{
    m_cachedAreaBounds = AZ::Aabb::CreateNull();
    for (const auto& [entityid, area] : m_registeredAreas)
    {
        m_cachedAreaBounds.AddAabb(area.m_areaBounds);
    }
}

AZ::Aabb TerrainSystem::ClampZBoundsToHeightBounds(const AZ::Aabb& aabb) const
{
    if (!aabb.IsValid())
    {
        return aabb; // Don't try to clamp invalid aabbs
    }
    AZ::Vector3 min = aabb.GetMin();
    AZ::Vector3 max = aabb.GetMax();
    min.SetZ(AZ::GetClamp<float>(min.GetZ(), m_currentSettings.m_heightRange.m_min, m_currentSettings.m_heightRange.m_max));
    max.SetZ(AZ::GetClamp<float>(max.GetZ(), m_currentSettings.m_heightRange.m_min, m_currentSettings.m_heightRange.m_max));
    return AZ::Aabb::CreateFromMinMax(min, max);
}

// Generate positions to be queried based on the sampler type.
void TerrainSystem::GenerateQueryPositions(const AZStd::span<const AZ::Vector3>& inPositions,
    AZStd::vector<AZ::Vector3>& outPositions, float queryResolution,
    Sampler sampler) const
{
    TERRAIN_PROFILE_FUNCTION_VERBOSE

    const float minHeight = m_currentSettings.m_heightRange.m_min;
    for (auto& position : inPositions)
    {
        switch(sampler)
        {
        case AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR:
            {
                AZ::Vector2 normalizedDelta;
                AZ::Vector2 pos0;
                ClampPosition(position.GetX(), position.GetY(), queryResolution, pos0, normalizedDelta);
                const AZ::Vector2 pos1(pos0.GetX() + queryResolution, pos0.GetY() + queryResolution);
                outPositions.emplace_back(AZ::Vector3(pos0.GetX(), pos0.GetY(), minHeight));
                outPositions.emplace_back(AZ::Vector3(pos1.GetX(), pos0.GetY(), minHeight));
                outPositions.emplace_back(AZ::Vector3(pos0.GetX(), pos1.GetY(), minHeight));
                outPositions.emplace_back(AZ::Vector3(pos1.GetX(), pos1.GetY(), minHeight));
            }
            break;
        case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
            {
                AZ::Vector2 clampedPosition;
                RoundPosition(position.GetX(), position.GetY(), queryResolution, clampedPosition);
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
    const AzFramework::Terrain::TerrainQueryRegion& queryRegion) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    AZStd::vector<AZ::Vector3> inPositions;
    inPositions.reserve(queryRegion.m_numPointsX * queryRegion.m_numPointsY);

    AZ::Vector2 startPosition(queryRegion.m_startPoint);

    for (size_t y = 0; y < queryRegion.m_numPointsY; y++)
    {
        float fy = aznumeric_cast<float>(queryRegion.m_startPoint.GetY() + (y * queryRegion.m_stepSize.GetY()));
        for (size_t x = 0; x < queryRegion.m_numPointsX; x++)
        {
            float fx = aznumeric_cast<float>(queryRegion.m_startPoint.GetX() + (x * queryRegion.m_stepSize.GetX()));
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
    TERRAIN_PROFILE_FUNCTION_VERBOSE

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    AZ::Aabb bounds;
    
    // We use a sliding window here and update the window end for each
    // position that falls in the same area as the previous positions. This consumes lesser memory
    // than sorting the points into separate lists and handling putting them back together.
    // This may be sub optimal if the points are randomly distributed in the list as opposed
    // to points in the same area id being close to each other.
    size_t windowStart = 0;
    AZ::EntityId windowAreaId = FindBestAreaEntityAtPosition(inPositions[0], bounds);
    const size_t inPositionSize = inPositions.size();

    for (size_t windowEnd = 0; windowEnd < inPositionSize; windowEnd++)
    {
        size_t nextWindowEnd = windowEnd + 1;
        AZ::EntityId areaId = (nextWindowEnd < inPositionSize)
            ? FindBestAreaEntityAtPosition(inPositions[nextWindowEnd], bounds)
            : AZ::EntityId();

        if (areaId != windowAreaId)
        {
            // If the area id is a default entity id, it usually means the
            // position is outside world bounds.
            if (windowAreaId != AZ::EntityId())
            {
                size_t spanLength = (windowEnd - windowStart) + 1;
                queryCallback(
                    AZStd::span<const AZ::Vector3>(inPositions.begin() + windowStart, spanLength),
                    AZStd::span<AZ::Vector3>(outPositions.begin() + windowStart, spanLength),
                    AZStd::span<bool>(outTerrainExists.begin() + windowStart, spanLength),
                    AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList>(outSurfaceWeights.begin() + windowStart, spanLength),
                    windowAreaId);
            }

            // Reset the window to start at the current position. Set the new area
            // id on which to run the next query.
            windowStart = nextWindowEnd;
            windowAreaId = areaId;
        }
    }
}

void TerrainSystem::GetHeightsSynchronous(const AZStd::span<const AZ::Vector3>& inPositions, Sampler sampler, 
    AZStd::span<float> heights, AZStd::span<bool> terrainExists) const
{
    TERRAIN_PROFILE_FUNCTION_VERBOSE

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    AZStd::vector<AZ::Vector3> outPositions;
    AZStd::vector<bool> outTerrainExists;

    // outPositions holds the iterators to results of the bulk queries.
    // In the case of the bilinear sampler, we'll be making 4 queries per
    // input position.
    size_t indexStepSize = (sampler == AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) ? 4 : 1;
    outPositions.reserve(inPositions.size() * indexStepSize);
    outTerrainExists.resize(inPositions.size() * indexStepSize);

    const float queryResolution = m_currentSettings.m_heightQueryResolution;

    GenerateQueryPositions(inPositions, outPositions, queryResolution, sampler);

    auto callback = [this]([[maybe_unused]] const AZStd::span<const AZ::Vector3> inPositions,
                        AZStd::span<AZ::Vector3> outPositions,
                        AZStd::span<bool> outTerrainExists,
                        [[maybe_unused]] AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeights,
                        AZ::EntityId areaId)
                        {
                            AZ_Assert((inPositions.size() == outPositions.size() && inPositions.size() == outTerrainExists.size()),
                                "The sizes of the terrain exists list and in/out positions list should match.");
                            Terrain::TerrainAreaHeightRequestBus::Event(areaId, &Terrain::TerrainAreaHeightRequestBus::Events::GetHeights,
                                outPositions, outTerrainExists);

                            // If the area has "use ground plane" checked, make sure any points that fall in the area that didn't
                            // return data are filled in with the area's minimum height.
                            const auto& area = m_registeredAreas.find(areaId);
                            if ((area != m_registeredAreas.end()) && area->second.m_useGroundPlane)
                            {
                                const float areaMin = AZStd::clamp(
                                    area->second.m_areaBounds.GetMin().GetZ(),
                                    m_currentSettings.m_heightRange.m_min,
                                    m_currentSettings.m_heightRange.m_max);

                                for (size_t index = 0; index < outPositions.size(); index++)
                                {
                                    if (!outTerrainExists[index])
                                    {
                                        outTerrainExists[index] = true;
                                        outPositions[index].SetZ(areaMin);
                                    }
                                }
                            }
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
                ClampPosition(inPositions[i].GetX(), inPositions[i].GetY(), queryResolution, clampedPosition, normalizedDelta);
                AZStd::array<float,4> queriedHeights = { outPositions[iteratorIndex].GetZ(),
                                     outPositions[iteratorIndex + 1].GetZ(),
                                     outPositions[iteratorIndex + 2].GetZ(),
                                     outPositions[iteratorIndex + 3].GetZ() };
                AZStd::array<bool, 4> queriedExistsFlags = { outTerrainExists[iteratorIndex],
                    outTerrainExists[iteratorIndex + 1],
                    outTerrainExists[iteratorIndex + 2],
                    outTerrainExists[iteratorIndex + 3]};

                InterpolateHeights(queriedHeights, queriedExistsFlags,
                    normalizedDelta.GetX(), normalizedDelta.GetY(), heights[i], terrainExists[i]);
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

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    float height = m_currentSettings.m_heightRange.m_min;
    const float queryResolution = m_currentSettings.m_heightQueryResolution;

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
            ClampPosition(x, y, queryResolution, pos0, normalizedDelta);
            const AZ::Vector2 pos1 = pos0 + AZ::Vector2(queryResolution);

            AZStd::array<bool,4> exists = { false, false, false, false };
            const AZStd::array<float, 4> queriedHeights = { GetTerrainAreaHeight(pos0.GetX(), pos0.GetY(), exists[0]),
                                                            GetTerrainAreaHeight(pos1.GetX(), pos0.GetY(), exists[1]),
                                                            GetTerrainAreaHeight(pos0.GetX(), pos1.GetY(), exists[2]),
                                                            GetTerrainAreaHeight(pos1.GetX(), pos1.GetY(), exists[3]) };

            InterpolateHeights(queriedHeights, exists, normalizedDelta.GetX(), normalizedDelta.GetY(), height, terrainExists);
        }
        break;

    //! Clamp the input point to the terrain sample grid, then get the height at the given grid location.
    case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
        {
            AZ::Vector2 clampedPosition;
            RoundPosition(x, y, queryResolution, clampedPosition);

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
        height, m_currentSettings.m_heightRange.m_min, m_currentSettings.m_heightRange.m_max);
}

float TerrainSystem::GetTerrainAreaHeight(float x, float y, bool& terrainExists) const
{
    const float worldMin = m_currentSettings.m_heightRange.m_min;
    AZ::Vector3 inPosition(x, y, worldMin);
    float height = worldMin;
    terrainExists = false;

    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    for (auto& [areaId, areaData] : m_registeredAreas)
    {
        const float areaMin = areaData.m_areaBounds.GetMin().GetZ();
        inPosition.SetZ(areaMin);
        if (SurfaceData::AabbContains2DMaxExclusive(areaData.m_areaBounds, inPosition))
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
    TERRAIN_PROFILE_FUNCTION_VERBOSE
    // We use different algorithms for calculating the normals depending on the input sampler type,
    // with no real shared logic, so they've been split out into separate methods.
    switch (sampler)
    {
    case Sampler::EXACT:
        // This will return the normal of the requested point using the underlying height data at a much higher frequency than the
        // query resolution, which means the normal can have significant fluctuations across each terrain grid square.
        GetNormalsSynchronousExact(inPositions, normals, terrainExists);
        break;
    case Sampler::CLAMP:
        // This will treat each terrain grid square as two triangles, and return the normal for the triangle that contains the requested
        // point. There is no interpolation, so each terrain grid square will exactly have two possible normals that can get returned
        // for any queried point within the square.
        GetNormalsSynchronousClamp(inPositions, normals, terrainExists);
        break;
    case Sampler::BILINEAR:
        // This will smoothly interpolate the normals from grid point to grid point across the entire terrain grid.
        GetNormalsSynchronousBilinear(inPositions, normals, terrainExists);
        break;
    default:
        AZ_Assert(false, "Unknown sampler type");
        break;
    }
}

void TerrainSystem::GetNormalsSynchronousExact(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AZStd::span<AZ::Vector3> normals,
    AZStd::span<bool> terrainExists) const
{
    // When querying for normals with an EXACT sampler, we get the normal by querying 4 points around the requested position
    // in an extremely small + pattern, then get the cross product of the two lines of the plus sign.

    // Because we're querying for exact heights from the underlying data, independent of the query resolution, we want to make
    // the + as small as possible so that we can correctly capture high-frequency changes in the data.
    // We've arbitrarily chosen the smaller of 1/32 of a meter or 1/32 of our query resolution as the size of each leg of the + sign.
    // It's possible that we may want to expose this as a tuning variable at some point.
    const float exactRange = AZStd::min(1.0f / 32.0f, m_currentSettings.m_heightQueryResolution / 32.0f);

    // The number of points that we're querying for each normal.
    const size_t queryCount = 4;

    // The full set of positions to query to be able to calculate all the normals.
    AZStd::vector<AZ::Vector3> queryPositions;
    queryPositions.reserve(inPositions.size() * queryCount);

    for (const auto& position : inPositions)
    {
        // For each input position, query the four outer points of the + sign.
        queryPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY() - exactRange, 0.0f)); // down
        queryPositions.emplace_back(AZ::Vector3(position.GetX() - exactRange, position.GetY(), 0.0f)); // left
        queryPositions.emplace_back(AZ::Vector3(position.GetX() + exactRange, position.GetY(), 0.0f)); // right
        queryPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY() + exactRange, 0.0f)); // up
    }

    // These constants are the relative index for each of the four positions that we pushed for each input above.
    constexpr size_t down = 0;
    constexpr size_t left = 1;
    constexpr size_t right = 2;
    constexpr size_t up = 3;

    AZStd::vector<float> heights(queryPositions.size());
    AZStd::vector<bool> exists(queryPositions.size());

    // We want to query the underlying heights with an EXACT sampler as well, so that we get to the input data that might be
    // at a much higher frequency than the height query resolution.
    GetHeightsSynchronous(queryPositions, Sampler::EXACT, heights, exists);

    for (size_t inPosIndex = 0, queryPositionIndex = 0; inPosIndex < inPositions.size(); inPosIndex++, queryPositionIndex += queryCount)
    {
        terrainExists[inPosIndex] = true;

        for (size_t querySubindex = 0; querySubindex < queryCount; querySubindex++)
        {
            // Combine the output heights with our query positions.
            queryPositions[queryPositionIndex + querySubindex].SetZ(heights[queryPositionIndex + querySubindex]);

            // We're querying at a much higher frequency than our query resolution, so we'll simply say that the terrain
            // exists only if all 4 points of the + sign exist.
            terrainExists[inPosIndex] = terrainExists[inPosIndex] && exists[queryPositionIndex + querySubindex];
        }

        if (terrainExists[inPosIndex])
        {
            // We have 4 vertices that make a + sign, cross the two lines to get the normal at the center.
            // (right - left) x (up - down)
            normals[inPosIndex] = (queryPositions[queryPositionIndex + right] - queryPositions[queryPositionIndex + left])
                                .Cross(queryPositions[queryPositionIndex + up] - queryPositions[queryPositionIndex + down])
                                .GetNormalized();
        }
        else
        {
            // If at least one of the 4 points of the + sign didn't exist, just give it a Z-up normal.
            normals[inPosIndex] = AZ::Vector3::CreateAxisZ();
        }
    }
}

void TerrainSystem::GetNormalsSynchronousClamp(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AZStd::span<AZ::Vector3> normals,
    AZStd::span<bool> terrainExists) const
{
    // When querying for normals with a CLAMP sampler, we divide each terrain grid square into two triangles, and return the normal
    // for the triangle that the requested position falls on.
    // Right now, the terrain system will always split each terrain grid square like this:  |\|
    // Eventually, the terrain system might get more complicated per-square logic, at which point the logic below would need
    // to change to account for the per-square triangle split direction.

    const float queryResolution = m_currentSettings.m_heightQueryResolution;

    // The number of points we're querying for each normal
    const size_t queryCount = 3;

    // The full set of positions to query to be able to calculate all the normals.
    AZStd::vector<AZ::Vector3> queryPositions;
    queryPositions.reserve(inPositions.size() * queryCount);

    for (const auto& position : inPositions)
    {
        // For each position, determine where in the square the point falls and get the bottom left corner of the square.
        AZ::Vector2 normalizedDelta;
        AZ::Vector2 bottomLeft;
        ClampPosition(position.GetX(), position.GetY(), queryResolution, bottomLeft, normalizedDelta);

        // Calculate the four corners of our grid square:
        //  2 *-* 3
        //    |\|
        //  0 *-* 1

        AZStd::array<AZ::Vector3, 4> corners =
        {
            AZ::Vector3(bottomLeft.GetX()                  , bottomLeft.GetY(), 0.0f),
            AZ::Vector3(bottomLeft.GetX() + queryResolution, bottomLeft.GetY(), 0.0f),
            AZ::Vector3(bottomLeft.GetX()                  , bottomLeft.GetY() + queryResolution, 0.0f),
            AZ::Vector3(bottomLeft.GetX() + queryResolution, bottomLeft.GetY() + queryResolution, 0.0f)
        };

        // Our grid squares are squares so the diagonal is a 45 degree angle. We can determine which triangle the query point
        // falls in just by checking if (x + y) < 1. This arbitrarily assigns points on the diagonal to the upper triangle.
        // We could use "<=" if we wanted them to go to the upper triangle.
        bool bottomTriangle = (normalizedDelta.GetX() + normalizedDelta.GetY()) < 1.0f;

        // We'll query for the 3 vertices of whichever triangle of the square the query point falls in.
        // We'll order them in counter-clockwise order and follow a mirrored pattern between the two, so that we can calculate the
        // normal from the results for either triangle by using (second - first) x (third - first).
        if (bottomTriangle)
        {
            queryPositions.emplace_back(corners[0]);
            queryPositions.emplace_back(corners[1]);
            queryPositions.emplace_back(corners[2]);
        }
        else
        {
            queryPositions.emplace_back(corners[3]);
            queryPositions.emplace_back(corners[2]);
            queryPositions.emplace_back(corners[1]);
        }
    }

    AZStd::vector<float> heights(queryPositions.size());
    AZStd::vector<bool> exists(queryPositions.size());

    // Since our query points are grid-aligned, we can use EXACT queries.
    GetHeightsSynchronous(queryPositions, Sampler::EXACT, heights, exists);

    for (size_t inPosIndex = 0, queryPositionIndex = 0; inPosIndex < inPositions.size(); inPosIndex++, queryPositionIndex += queryCount)
    {
        // We'll set "exists" to true *only* if all three positions for calculating the normal exists.
        terrainExists[inPosIndex] = exists[queryPositionIndex] && exists[queryPositionIndex + 1] && exists[queryPositionIndex + 2];

        // Only calculate the normal if all the queried points exist.
        if (terrainExists[inPosIndex])
        {
            // Combine the output heights with our query positions.
            for (size_t querySubindex = 0; querySubindex < queryCount; querySubindex++)
            {
                queryPositions[queryPositionIndex + querySubindex].SetZ(heights[queryPositionIndex + querySubindex]);
            }

            // We have 3 vertices for a triangle, get the normal of the triangle.
            normals[inPosIndex] = (queryPositions[queryPositionIndex + 1] - queryPositions[queryPositionIndex])
                        .Cross(queryPositions[queryPositionIndex + 2] - queryPositions[queryPositionIndex])
                        .GetNormalized();
        }
        else
        {
            normals[inPosIndex] = AZ::Vector3::CreateAxisZ();
        }
    }
}

void TerrainSystem::GetNormalsSynchronousBilinear(
    const AZStd::span<const AZ::Vector3>& inPositions,
    AZStd::span<AZ::Vector3> normals, AZStd::span<bool> terrainExists) const
{
    // When querying for normals with a BILINEAR sampler, we calculate the normals at each corner of the terrain grid square that
    // the query point fall in then interpolate between those normals to get the final result.

    const float queryResolution = m_currentSettings.m_heightQueryResolution;
    const float twiceQueryResolution = m_currentSettings.m_heightQueryResolution * 2.0f;

    // We'll need a total of 12 unique positions queried to calculate the 4 normals that we'll be interpolating between.
    // (We need 16 non-unique positions, but we can reuse the results for the middle 4 positions)
    const size_t queryCount = 12;

    // The full set of positions to query to be able to calculate all the normals.
    AZStd::vector<AZ::Vector3> queryPositions;
    queryPositions.reserve(inPositions.size() * queryCount);

    for (const auto& position : inPositions)
    {
        // We'll query our 12 points in the following order, where the x represents the location of the query point.
        //            10     11
        //              *---*
        //         6   7|   |8   9
        //          *---*---*---*
        //          |   | x |   |
        //          *---*---*---*
        //         2   3|   |4   5
        //              *---*
        //             0     1

        // For each position, determine where in the square the point falls and get the bottom left corner of the center square
        // (corner 3).
        AZ::Vector2 normalizedDelta;
        AZ::Vector2 pos3;
        ClampPosition(position.GetX(), position.GetY(), queryResolution, pos3, normalizedDelta);

        // Corners 0-1
        queryPositions.emplace_back(pos3.GetX()                         , pos3.GetY() - queryResolution, 0.0f);
        queryPositions.emplace_back(pos3.GetX() + queryResolution       , pos3.GetY() - queryResolution, 0.0f);
        // Corners 2-5
        queryPositions.emplace_back(pos3.GetX() - queryResolution       , pos3.GetY(), 0.0f);
        queryPositions.emplace_back(pos3.GetX()                         , pos3.GetY(), 0.0f);
        queryPositions.emplace_back(pos3.GetX() + queryResolution       , pos3.GetY(), 0.0f);
        queryPositions.emplace_back(pos3.GetX() + twiceQueryResolution  , pos3.GetY(), 0.0f);
        // Corners 6-9
        queryPositions.emplace_back(pos3.GetX() - queryResolution       , pos3.GetY() + queryResolution, 0.0f);
        queryPositions.emplace_back(pos3.GetX()                         , pos3.GetY() + queryResolution, 0.0f);
        queryPositions.emplace_back(pos3.GetX() + queryResolution       , pos3.GetY() + queryResolution, 0.0f);
        queryPositions.emplace_back(pos3.GetX() + twiceQueryResolution  , pos3.GetY() + queryResolution, 0.0f);
        // Corners 10-11
        queryPositions.emplace_back(pos3.GetX()                         , pos3.GetY() + twiceQueryResolution, 0.0f);
        queryPositions.emplace_back(pos3.GetX() + queryResolution       , pos3.GetY() + twiceQueryResolution, 0.0f);
    }

    AZStd::vector<float> heights(queryPositions.size());
    AZStd::vector<bool> exists(queryPositions.size());

    // Since our query points are grid-aligned, we can use EXACT queries.
    GetHeightsSynchronous(queryPositions, Sampler::EXACT, heights, exists);

    for (size_t inPosIndex = 0, queryPositionIndex = 0; inPosIndex < inPositions.size(); inPosIndex++, queryPositionIndex += queryCount)
    {
        // Combine the output heights with our query positions.
        for (size_t querySubindex = 0; querySubindex < queryCount; querySubindex++)
        {
            queryPositions[queryPositionIndex + querySubindex].SetZ(heights[queryPositionIndex + querySubindex]);
        }

        // We calculate the normal by taking the cross product of the tips of a + shape around the point that we want the normal for.
        auto CalculateNormal = [&exists, &queryPositions, &queryPositionIndex]
            (uint8_t left, uint8_t center, uint8_t right, uint8_t up, uint8_t down) -> AZ::Vector3
        {
            const size_t centerQueryIdx = queryPositionIndex + center;
            const size_t upQueryIdx = queryPositionIndex + up;
            const size_t downQueryIdx = queryPositionIndex + down;
            const size_t leftQueryIdx = queryPositionIndex + left;
            const size_t rightQueryIdx = queryPositionIndex + right;

            // Only calculate the normal if the center point and all the points around it exist. Otherwise, just return a Z-up vector.
            if (exists[upQueryIdx] && exists[downQueryIdx] && exists[leftQueryIdx] && exists[centerQueryIdx] && exists[rightQueryIdx])
            {
                // Each normal is (right - left) x (up - down)
                return (queryPositions[rightQueryIdx] - queryPositions[leftQueryIdx])
                        .Cross(queryPositions[upQueryIdx] - queryPositions[downQueryIdx])
                        .GetNormalized();
            }
            else
            {
                return AZ::Vector3::CreateAxisZ();
            }
        };

        // Calculate the normals of the four corners of the square that our query point falls in by taking the cross product
        // of + shapes:
        //            10     11                normal0      normal1      normal2      normal3
        //              *---*                     7            8           10           11
        //         6   7|   |8   9                *            *            *            *    
        //          *---*---*---*             2   |3  4    3   |4  5    6   |7  8    7   |8  9
        //          |   | x |   |             *---*---*    *---*---*    *---*---*    *---*---*
        //          *---*---*---*                 |            |            |            |    
        //         2   3|   |4   5                *            *            *            *    
        //              *---*                     0            1            3            4
        //             0     1
        const AZ::Vector3 normal0 = CalculateNormal(2, 3, 4, 7, 0);
        const AZ::Vector3 normal1 = CalculateNormal(3, 4, 5, 8, 1);
        const AZ::Vector3 normal2 = CalculateNormal(6, 7, 8, 10, 3);
        const AZ::Vector3 normal3 = CalculateNormal(7, 8, 9, 11, 4);

        // For each position, determine where in the square the point falls and get the bottom left corner of the center square
        // (corner 3).
        AZ::Vector2 normalizedDelta;
        AZ::Vector2 pos3;
        ClampPosition(inPositions[inPosIndex].GetX(), inPositions[inPosIndex].GetY(), queryResolution, pos3, normalizedDelta);

        // Then finally, interpolate between the 4 normals.
        const float lerpX = normalizedDelta.GetX();
        const float lerpY = normalizedDelta.GetY();
        const float invLerpX = 1.0f - lerpX;
        const float invLerpY = 1.0f - lerpY;

        AZ::Vector3 combinedNormal =
            (normal0 * (invLerpX * invLerpY)) +
            (normal1 * (lerpX * invLerpY)) +
            (normal2 * (invLerpX * lerpY)) +
            (normal3 * (lerpX * lerpY));

        normals[inPosIndex] = combinedNormal.GetNormalized();

        // Use the "terrain exists" result from the nearest corner as the result we'll return.
        if ((lerpX < 0.5f) && (lerpY < 0.5f))
        {
            terrainExists[inPosIndex] = exists[queryPositionIndex + 3];
        }
        else if ((lerpX >= 0.5f) && (lerpY < 0.5f))
        {
            terrainExists[inPosIndex] = exists[queryPositionIndex + 4];
        }
        else if ((lerpX < 0.5f) && (lerpY >= 0.5f))
        {
            terrainExists[inPosIndex] = exists[queryPositionIndex + 7];
        }
        else
        {
            terrainExists[inPosIndex] = exists[queryPositionIndex + 8];
        }
    }
}

AZ::Vector3 TerrainSystem::GetNormalSynchronous(const AZ::Vector3& position, Sampler sampler, bool* terrainExistsPtr) const
{
    AZ::Vector3 normal;
    bool exists;

    GetNormalsSynchronous(
        AZStd::span<const AZ::Vector3>(&position, 1), sampler, AZStd::span<AZ::Vector3>(&normal, 1), AZStd::span<bool>(&exists, 1));

    if (terrainExistsPtr)
    {
        *terrainExistsPtr = exists;
    }
    return normal;
}

AZ::Vector3 TerrainSystem::GetNormal(const AZ::Vector3& position, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetNormalSynchronous(position, sampler, terrainExistsPtr);
}

AZ::Vector3 TerrainSystem::GetNormalFromVector2(const AZ::Vector2& position, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetNormalSynchronous(AZ::Vector3(position), sampler, terrainExistsPtr);
}

AZ::Vector3 TerrainSystem::GetNormalFromFloats(float x, float y, Sampler sampler, bool* terrainExistsPtr) const
{
    return GetNormalSynchronous(AZ::Vector3(x, y, 0.0f), sampler, terrainExistsPtr);
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
    // Query normals before heights because the height query produces better results for the terrainExists flag for a given point,
    // so we want to prefer keeping the results from the height query if we end up querying both.
    // (Ideally at some point they will produce identical results)
    outSurfacePoint.m_normal = GetNormalSynchronous(inPosition, sampler, terrainExistsPtr);
    outSurfacePoint.m_position = inPosition;
    outSurfacePoint.m_position.SetZ(GetHeightSynchronous(inPosition.GetX(), inPosition.GetY(), sampler, terrainExistsPtr));
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

AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> TerrainSystem::QueryListAsync(
    const AZStd::span<const AZ::Vector3>& inPositions,
    TerrainDataMask requestedData,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params) const
{
    return ProcessFromListAsync(inPositions, requestedData, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> TerrainSystem::QueryListOfVector2Async(
    const AZStd::span<const AZ::Vector2>& inPositions,
    TerrainDataMask requestedData,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params) const
{
    return ProcessFromListAsync(inPositions, requestedData, perPositionCallback, sampler, params);
}

AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> TerrainSystem::QueryRegionAsync(
    const AzFramework::Terrain::TerrainQueryRegion& queryRegion,
    TerrainDataMask requestedData,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler,
    AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    auto numSamplesX = queryRegion.m_numPointsX;
    auto numSamplesY = queryRegion.m_numPointsY;
    const int64_t numPositionsToProcess = numSamplesX * numSamplesY;

    if (numPositionsToProcess == 0)
    {
        AZ_Warning("TerrainSystem", false, "No positions to process.");
        return nullptr;
    }

    // Determine the maximum number of jobs, and the minimum number of positions that should be processed per job.
    const int32_t numJobsMax = CalculateMaxJobs(params);
    const int32_t minPositionsPerJob = params && (params->m_minPositionsPerJob > 0)
        ? params->m_minPositionsPerJob
        : AzFramework::Terrain::QueryAsyncParams::MinPositionsPerJobDefault;

    // Calculate the best subdivision of the region along both the X and Y axes to use as close to the maximum number of jobs
    // as possible while also keeping all the regions effectively the same size.
    int32_t xJobs, yJobs;
    SubdivideRegionForJobs(
        aznumeric_cast<int32_t>(numSamplesX), aznumeric_cast<int32_t>(numSamplesY), numJobsMax, minPositionsPerJob, xJobs, yJobs);

    // The number of jobs returned might be less than the total requested maximum number of jobs, so recalculate it here
    const int32_t numJobs = xJobs * yJobs;

    // Get the number of samples in each direction that we'll use for each query. We calculate this as a fractional value
    // so that we can keep each query pretty evenly balanced, with just +/- 1 count variation on each axis.
    const float xSamplesPerQuery = aznumeric_cast<float>(numSamplesX) / xJobs;
    const float ySamplesPerQuery = aznumeric_cast<float>(numSamplesY) / yJobs;

    // Make sure our subdivisions are producing at least minPositionsPerJob unless the *total* requested point count is
    // less than minPositionsPerJob.
    AZ_Assert(
        ((numSamplesX * numSamplesY) < minPositionsPerJob) ||
        (aznumeric_cast<int32_t>(xSamplesPerQuery) * aznumeric_cast<int32_t>(ySamplesPerQuery)) >= minPositionsPerJob,
        "Too few positions per job: %d vs %d", aznumeric_cast<int32_t>(xSamplesPerQuery) * aznumeric_cast<int32_t>(ySamplesPerQuery),
        minPositionsPerJob);

    // Create a terrain job context and split the work across multiple jobs.
    AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext =
        AZStd::make_shared<AzFramework::Terrain::TerrainJobContext>(*m_terrainJobManager, numJobs);
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_activeTerrainJobContextMutex);
        m_activeTerrainJobContexts.push_back(jobContext);
    }

    [[maybe_unused]] int32_t jobsStarted = 0;

    for (int32_t yJob = 0; yJob < yJobs; yJob++)
    {
        // Use the fractional samples per query to calculate the start and end of the region, but then convert it
        // back to integers so that our regions are always in exact multiples of the number of samples to process.
        // This is important because we want the XY values for each point that we're processing to exactly align with
        // 'start + N * (step size)', or else we'll start to process point locations that weren't actually what was requested.
        const int32_t y0 = aznumeric_cast<int32_t>(AZStd::lround(yJob * ySamplesPerQuery));
        const int32_t y1 = aznumeric_cast<int32_t>(AZStd::lround((yJob + 1) * ySamplesPerQuery));
        const float inRegionMinY = queryRegion.m_startPoint.GetY() + (y0 * queryRegion.m_stepSize.GetY());
        const int32_t numPointsY = AZStd::min(y1 - y0, aznumeric_cast<int32_t>(numSamplesY) - y0);


        for (int32_t xJob = 0; xJob < xJobs; xJob++)
        {
            // Same as above, calculate the start and end of the region, then convert back to integers and create the
            // region based on 'start + n * (step size)'.
            const int32_t x0 = aznumeric_cast<int32_t>(AZStd::lround(xJob * xSamplesPerQuery));
            const int32_t x1 = aznumeric_cast<int32_t>(AZStd::lround((xJob + 1) * xSamplesPerQuery));
            const float inRegionMinX = queryRegion.m_startPoint.GetX() + (x0 * queryRegion.m_stepSize.GetX());
            const int32_t numPointsX = AZStd::min(x1 - x0, aznumeric_cast<int32_t>(numSamplesX) - x0);

            // Define the job function using the sub region of positions to process.
            AzFramework::Terrain::TerrainQueryRegion subQueryRegion(
                AZ::Vector3(inRegionMinX, inRegionMinY, queryRegion.m_startPoint.GetZ()), numPointsX, numPointsY, queryRegion.m_stepSize);

            auto jobFunction = [this, subQueryRegion, x0, y0, requestedData, perPositionCallback, sampler, jobContext, params]()
            {
                // Process the sub region of positions, unless the associated job context has been cancelled.
                if (!jobContext->IsCancelled())
                {
                    QueryRegionInternal(subQueryRegion, x0, y0, requestedData, perPositionCallback, sampler);
                }

                // Decrement the number of completions remaining, invoke the completion callback if this happens
                // to be the final job completed, and remove this TerrainJobContext from the list of active ones.
                const bool wasLastJobCompleted = jobContext->OnJobCompleted();
                if (wasLastJobCompleted)
                {
                    if (params && params->m_completionCallback)
                    {
                        params->m_completionCallback(jobContext);
                    }

                    {
                        AZStd::unique_lock<AZStd::mutex> lock(m_activeTerrainJobContextMutex);
                        m_activeTerrainJobContexts.erase(
                            AZStd::find(m_activeTerrainJobContexts.begin(), m_activeTerrainJobContexts.end(), jobContext));
                        m_activeTerrainJobContextMutexConditionVariable.notify_one();
                    }
                }
            };

            // Create the job and start it immediately.
            AZ::Job* processJob = AZ::CreateJobFunction(jobFunction, true, jobContext.get());
            processJob->Start();
            jobsStarted++;
        }
    }

    // Validate this just to ensure that the fractional math for handling points didn't cause any rounding errors anywhere.
    AZ_Assert(jobsStarted == numJobs, "Wrong number of jobs created: %d vs %d", jobsStarted, numJobs);

    return jobContext;
}

AZ::EntityId TerrainSystem::FindBestAreaEntityAtPosition(const AZ::Vector3& position, AZ::Aabb& bounds) const
{
    // Find the highest priority layer that encompasses this position
    // The areas are sorted into priority order: the first area that contains inPosition is the most suitable.
    for (const auto& [areaId, areaData] : m_registeredAreas)
    {
        // We use min-inclusive-max-exclusive so that two spawners with a shared edge will have a single owner for that edge.
        if (SurfaceData::AabbContains2DMaxExclusive(areaData.m_areaBounds, position))
        {
            bounds = areaData.m_areaBounds;
            return areaId;
        }
    }

    return AZ::EntityId();
}

void TerrainSystem::GetOrderedSurfaceWeightsFromList(
    const AZStd::span<const AZ::Vector3>& inPositions,
    Sampler sampler,
    AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList,
    AZStd::span<bool> terrainExists) const
{
    TERRAIN_PROFILE_FUNCTION_VERBOSE

    if (terrainExists.size() == outSurfaceWeightsList.size())
    {
        AZStd::vector<float> heights(inPositions.size());
        GetHeightsSynchronous(inPositions, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, heights, terrainExists);
    }

    // queryPositions contains the modified positions based on our sampler type. For surface queries, we don't currently perform bilinear
    // interpolation of any results, so our query position size will always match our input size.
    AZStd::vector<AZ::Vector3> queryPositions;
    queryPositions.reserve(inPositions.size());
    const float queryResolution = m_currentSettings.m_surfaceDataQueryResolution;
    Sampler querySampler = (sampler == Sampler::EXACT) ? Sampler::EXACT : Sampler::CLAMP;
    GenerateQueryPositions(inPositions, queryPositions, queryResolution, querySampler);

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
    MakeBulkQueries(queryPositions, outPositions, terrainExists, outSurfaceWeightsList, callback);
}

void TerrainSystem::GetOrderedSurfaceWeights(
    const float x,
    const float y,
    Sampler sampler,
    AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
    bool* terrainExistsPtr) const
{
    AZStd::shared_lock<AZStd::shared_mutex> lock(m_areaMutex);

    if (terrainExistsPtr)
    {
        GetHeightFromFloats(x, y, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, terrainExistsPtr);
    }

    outSurfaceWeights.clear();

    const float queryResolution = m_currentSettings.m_surfaceDataQueryResolution;

    AZ::Vector3 inPosition;

    switch (sampler)
    {
    // Both bilinear and clamp samplers will clamp the input position to the surface data query grid and get the surface data there.
    // At some point we might want to consider interpolation of surface weights for the bilinear case, but it's unclear if that's
    // actually a desired outcome.
    case AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR:
        [[fallthrough]];
    case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
        {
            AZ::Vector2 clampedPosition;
            RoundPosition(x, y, queryResolution, clampedPosition);
            inPosition = AZ::Vector3(clampedPosition);
        }
        break;
    //! Directly get the value at the location, regardless of terrain sample grid density.
    case AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT:
        [[fallthrough]];
    default:
        inPosition = AZ::Vector3(x, y, 0.0f);
        break;
    }

    AZ::Aabb bounds;
    AZ::EntityId bestAreaId = FindBestAreaEntityAtPosition(inPosition, bounds);

    if (!bestAreaId.IsValid())
    {
        return;
    }

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

void TerrainSystem::QueryList(
    const AZStd::span<const AZ::Vector3>& inPositions,
    TerrainDataMask requestedData,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    TERRAIN_PROFILE_FUNCTION_VERBOSE

    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<bool> terrainExists(inPositions.size());
    AZStd::vector<float> heights;
    AZStd::vector<AZ::Vector3> normals;
    AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> surfaceWeights;

    // Query normals before heights because the height query produces better results for the terrainExists flag for a given point,
    // so we want to prefer keeping the results from the height query if we end up querying both.
    // (Ideally at some point they will produce identical results)
    if (requestedData & TerrainDataMask::Normals)
    {
        normals.resize(inPositions.size());
        GetNormalsSynchronous(inPositions, sampler, normals, terrainExists);
    }
    if (requestedData & TerrainDataMask::Heights)
    {
        heights.resize(inPositions.size());
        GetHeightsSynchronous(inPositions, sampler, heights, terrainExists);
    }
    if (requestedData & TerrainDataMask::SurfaceData)
    {
        // We can potentially skip an extra call to GetHeights if we already
        // got the terrain exists flags in the earlier call to GetHeights
        AZStd::vector<bool> terrainExistsEmpty;

        surfaceWeights.resize(inPositions.size());
        GetOrderedSurfaceWeightsFromList(inPositions, sampler, surfaceWeights,
            (requestedData & TerrainDataMask::Heights) ? terrainExistsEmpty : terrainExists);
    }

    {
        TERRAIN_PROFILE_SCOPE_VERBOSE("QueryList-PerPositionCallbacks");

        AzFramework::SurfaceData::SurfacePoint surfacePoint;
        for (size_t i = 0; i < inPositions.size(); i++)
        {
            surfacePoint.m_position = inPositions[i];
            if (requestedData & TerrainDataMask::Heights)
            {
                surfacePoint.m_position.SetZ(heights[i]);
            }
            if (requestedData & TerrainDataMask::Normals)
            {
                surfacePoint.m_normal = AZStd::move(normals[i]);
            }
            if (requestedData & TerrainDataMask::SurfaceData)
            {
                surfacePoint.m_surfaceTags = AZStd::move(surfaceWeights[i]);
            }
            perPositionCallback(surfacePoint, terrainExists[i]);
        }
    }
}

void TerrainSystem::QueryListOfVector2(
    const AZStd::span<const AZ::Vector2>& inPositions,
    TerrainDataMask requestedData,
    AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
    Sampler sampler) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<AZ::Vector3> inPositionsVec3 = GenerateInputPositionsFromListOfVector2(inPositions);

    QueryList(inPositionsVec3, requestedData, perPositionCallback, sampler);
}

//! Given a set of async parameters, calculate the max number of jobs that we can use for the async call.
int32_t TerrainSystem::CalculateMaxJobs(AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params) const
{
    // Determine the maximum number of jobs available to split the work across for async calls.
    const int32_t numWorkerThreads = m_terrainJobManager->GetNumWorkerThreads();
    const int32_t numJobsDesired = params ? params->m_desiredNumberOfJobs : AzFramework::Terrain::QueryAsyncParams::NumJobsDefault;
    const int32_t numJobsMax = (numJobsDesired > 0) ? AZStd::min(numWorkerThreads, numJobsDesired) : numWorkerThreads;

    return numJobsMax;
}

void TerrainSystem::SubdivideRegionForJobs(
    int32_t numSamplesX, int32_t numSamplesY, int32_t maxNumJobs, int32_t minPointsPerJob, int32_t& subdivisionsX, int32_t& subdivisionsY)
{
    // This method will try to determine the best way to distribute the number of X and Y samples in our region across as many jobs
    // as possible to meet the following constraints:
    //   subdivisionsX * subdivisionsY <= maxNumJobs
    //   (numSamplesX / subdivisionsX) * (numSamplesY / subdivisionsY) >= minPointsPerJob
    // Basically, the goal is to use the maximum number of jobs, as long as we're processing at least the minimum points per job.
    // We also try to keep the subdivisions of X as low as possible because it's generally more efficient to process
    // consecutive X values than consecutive Y values.

    // Start by initializing to a single job that processes the entire region.
    subdivisionsX = 1;
    subdivisionsY = 1;
    int32_t bestJobUsage = 1;

    // If the entire region is less than the minimum points, just return.
    if ((numSamplesX * numSamplesY) < minPointsPerJob)
    {
        return;
    }

    // Clamp the maximum number of jobs to whichever is smaller - the maximum number of jobs that have minPointsPerJob, or the
    // requested maxNumJobs. We can't have a solution that violates either constraint.
    int32_t clampedMaxNumJobs = AZStd::clamp(aznumeric_cast<int32_t>((numSamplesX * numSamplesY) / minPointsPerJob), 1, maxNumJobs);

    // MaxNumJobs will generally be a small value, so we can just brute-force the problem and try every solution to see what will
    // produce the most optimal results. We stop early if we find a solution that uses the maximum number of jobs.
    // We loop on X subdivisions first so that we bias towards solutions with a lower number of X subdivisions.
    for (int32_t xChoice = 1; xChoice <= clampedMaxNumJobs; xChoice++)
    {
        // For a given number of X subdivisions, find the maximum number of Y subdivisions that produces at least the
        // minimum number of points per job.
        int32_t xSamplesPerSubdivision = numSamplesX / xChoice;

        // This is how many rows of X we need to produce minPointsPerJob.
        int32_t minXRowsNeeded = aznumeric_cast<int32_t>(AZStd::ceil(aznumeric_cast<float>(minPointsPerJob) / xSamplesPerSubdivision));

        // Get the maximum number of subdivisions for Y that will produce minPointsPerJob (numSamplesY / minXRowsNeeded), but
        // also clamp it by the maximum number of jobs that we're allowed to produce (maxNumJobs / xChoice).
        int32_t yChoice = AZStd::min(numSamplesY / minXRowsNeeded, clampedMaxNumJobs / xChoice);

        // The maximum number of subdivisions in Y will decrease with increasing X subdivisions. If we've reached the point
        // where even the entire Y range (i.e. yChoice == 1) isn't sufficient, we can stop checking, we won't find any more solutions. 
        if (yChoice == 0)
        {
            break;
        }

        // If this combination is better than a previous solution, save it as our new best solution.
        int32_t jobUsage = xChoice * yChoice;
        if (jobUsage > bestJobUsage)
        {
            subdivisionsX = xChoice;
            subdivisionsY = yChoice;
            bestJobUsage = jobUsage;

            // If we've found an optimal solution, early-out.
            if (jobUsage == clampedMaxNumJobs)
            {
                break;
            }
        }
    }

    // Verify that our subdivision strategy has stayed within the max jobs constraint.
    AZ_Assert(
        (subdivisionsX * subdivisionsY) <= maxNumJobs, "The region was subdivided into too many jobs: %d x %d vs %d max", subdivisionsX,
        subdivisionsY, maxNumJobs);
}

bool TerrainSystem::ContainedAabbTouchesEdge(const AZ::Aabb& outerAabb, const AZ::Aabb& innerAabb)
{
    return outerAabb.Contains(innerAabb) &&
        (
            outerAabb.GetMin().GetX() == innerAabb.GetMin().GetX() ||
            outerAabb.GetMin().GetY() == innerAabb.GetMin().GetY() ||
            outerAabb.GetMin().GetZ() == innerAabb.GetMin().GetZ() ||
            outerAabb.GetMax().GetX() == innerAabb.GetMax().GetX() ||
            outerAabb.GetMax().GetY() == innerAabb.GetMax().GetY() ||
            outerAabb.GetMax().GetZ() == innerAabb.GetMax().GetZ()
        );
}

void TerrainSystem::QueryRegion(
    const AzFramework::Terrain::TerrainQueryRegion& queryRegion,
    TerrainDataMask requestedData,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler) const
{
    QueryRegionInternal(queryRegion, 0, 0, requestedData, perPositionCallback, sampler);
}

void TerrainSystem::QueryRegionInternal(
    const AzFramework::Terrain::TerrainQueryRegion& queryRegion,
    size_t xIndexOffset, size_t yIndexOffset,
    TerrainDataMask requestedData,
    AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
    Sampler sampler) const
{
    AZ_PROFILE_FUNCTION(Terrain);

    // Don't bother processing if we don't have a callback
    if (!perPositionCallback)
    {
        return;
    }

    AZStd::vector<AZ::Vector3> inPositions = GenerateInputPositionsFromRegion(queryRegion);

    if (inPositions.empty())
    {
        return;
    }

    AZStd::vector<bool> terrainExists(inPositions.size());
    AZStd::vector<float> heights;
    AZStd::vector<AZ::Vector3> normals;
    AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> surfaceWeights;

    // Query normals before heights because the height query produces better results for the terrainExists flag for a given point,
    // so we want to prefer keeping the results from the height query if we end up querying both.
    // (Ideally at some point they will produce identical results)
    if (requestedData & TerrainDataMask::Normals)
    {
        normals.resize(inPositions.size());
        {
            AZ_PROFILE_SCOPE(Terrain, "GetNormalsSynchronous");
            GetNormalsSynchronous(inPositions, sampler, normals, terrainExists);
        }
    }
    if (requestedData & TerrainDataMask::Heights)
    {
        heights.resize(inPositions.size());
        GetHeightsSynchronous(inPositions, sampler, heights, terrainExists);
    }
    if (requestedData & TerrainDataMask::SurfaceData)
    {
        // We can potentially skip an extra call to GetHeights if we already
        // got the terrain exists flags in the earlier call to GetHeights
        AZStd::vector<bool> terrainExistsEmpty;

        surfaceWeights.resize(inPositions.size());
        GetOrderedSurfaceWeightsFromList(
            inPositions, sampler, surfaceWeights, (requestedData & TerrainDataMask::Heights) ? terrainExistsEmpty : terrainExists);
    }

    {
        AZ_PROFILE_SCOPE(Terrain, "QueryRegionInternal-PerPositionCallbacks");

        AzFramework::SurfaceData::SurfacePoint surfacePoint;
        for (size_t y = 0, i = 0; y < queryRegion.m_numPointsY; y++)
        {
            for (size_t x = 0; x < queryRegion.m_numPointsX; x++)
            {
                surfacePoint.m_position = inPositions[i];
                if (requestedData & TerrainDataMask::Heights)
                {
                    surfacePoint.m_position.SetZ(heights[i]);
                }
                if (requestedData & TerrainDataMask::Normals)
                {
                    surfacePoint.m_normal = AZStd::move(normals[i]);
                }
                if (requestedData & TerrainDataMask::SurfaceData)
                {
                    surfacePoint.m_surfaceTags = AZStd::move(surfaceWeights[i]);
                }
                perPositionCallback(x + xIndexOffset, y + yIndexOffset, surfacePoint, terrainExists[i]);
                i++;
            }
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
    m_terrainDirtyMask |= AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::HeightData |
        AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::SurfaceData;
    m_cachedAreaBounds.AddAabb(aabb);
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
                m_terrainDirtyMask |= AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::HeightData |
                    AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::SurfaceData;

                if (ContainedAabbTouchesEdge(m_cachedAreaBounds, areaData.m_areaBounds))
                {
                    RecalculateCachedBounds();
                }

                return true;
            }
            return false;
        });
}

void TerrainSystem::RefreshArea(AZ::EntityId areaId, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask changeMask)
{
    AZStd::unique_lock<AZStd::shared_mutex> lock(m_areaMutex);

    auto areaAabb = m_registeredAreas.find(areaId);

    AZ::Aabb oldAabb = (areaAabb != m_registeredAreas.end()) ? areaAabb->second.m_areaBounds : AZ::Aabb::CreateNull();
    AZ::Aabb newAabb = AZ::Aabb::CreateNull();
    LmbrCentral::ShapeComponentRequestsBus::EventResult(newAabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
    m_registeredAreas[areaId].m_areaBounds = newAabb;

    AZ::Aabb expandedAabb = oldAabb;
    expandedAabb.AddAabb(newAabb);

    RefreshRegion(expandedAabb, changeMask);

    // Check to see which axis the aabbs changed in
    bool xDiff = oldAabb.GetMin().GetX() != newAabb.GetMin().GetX() || oldAabb.GetMax().GetX() != newAabb.GetMax().GetX();
    bool yDiff = oldAabb.GetMin().GetY() != newAabb.GetMin().GetY() || oldAabb.GetMax().GetY() != newAabb.GetMax().GetY();
    bool zDiff = oldAabb.GetMin().GetZ() != newAabb.GetMin().GetZ() || oldAabb.GetMax().GetZ() != newAabb.GetMax().GetZ();

    if ((xDiff && (m_cachedAreaBounds.GetMin().GetX() == oldAabb.GetMin().GetX() || m_cachedAreaBounds.GetMax().GetX() == oldAabb.GetMax().GetX())) ||
        (yDiff && (m_cachedAreaBounds.GetMin().GetY() == oldAabb.GetMin().GetY() || m_cachedAreaBounds.GetMax().GetY() == oldAabb.GetMax().GetY())) ||
        (zDiff && (m_cachedAreaBounds.GetMin().GetZ() == oldAabb.GetMin().GetZ() || m_cachedAreaBounds.GetMax().GetZ() == oldAabb.GetMax().GetZ())))
    {
        // Old aabb is on the edge of the bounds in at least one axis, and moved on that axis, so it will require a full refresh
        RecalculateCachedBounds();
    }
    else if(!m_cachedAreaBounds.Contains(newAabb))
    {
        // Old Aabb was inside the bounds and new aabb is outside the bounds, so just add it.
        m_cachedAreaBounds.AddAabb(newAabb);
    }

}

void TerrainSystem::RefreshRegion(
    const AZ::Aabb& dirtyRegion, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask changeMask)
{
    m_dirtyRegion.AddAabb(dirtyRegion);

    // Keep track of which types of data have changed so that we can send out the appropriate notifications later.
    m_terrainDirtyMask |= changeMask;
}

void TerrainSystem::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
{
    using Terrain = AzFramework::Terrain::TerrainDataNotifications;

    bool terrainSettingsChanged = false;

    if ((m_terrainDirtyMask & AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::Settings) ==
        AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::Settings)
    {
        terrainSettingsChanged = true;
        m_terrainDirtyMask &= ~AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::Settings;
        if (m_currentSettings.m_heightRange.IsValid())
        {
            m_dirtyRegion = ClampZBoundsToHeightBounds(m_cachedAreaBounds);
        }

        // This needs to happen before the "system active" check below, because activating the system will cause the various
        // terrain layer areas to request the current world bounds.
        if (m_requestedSettings.m_heightRange != m_requestedSettings.m_heightRange)
        {
            m_terrainDirtyMask |= AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::HeightData |
                AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::SurfaceData;
            m_currentSettings.m_heightRange = m_requestedSettings.m_heightRange;

            // Add the cached area bounds clamped to the new range, so both the old and new range are included.
            m_dirtyRegion.AddAabb(ClampZBoundsToHeightBounds(m_cachedAreaBounds));
        }

        if (m_requestedSettings.m_heightQueryResolution != m_currentSettings.m_heightQueryResolution)
        {
            m_terrainDirtyMask |= AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::HeightData;
        }

        if (m_requestedSettings.m_surfaceDataQueryResolution != m_currentSettings.m_surfaceDataQueryResolution)
        {
            m_terrainDirtyMask |= AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::SurfaceData;
        }

        m_currentSettings = m_requestedSettings;
    }

    if (terrainSettingsChanged || (m_terrainDirtyMask != AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::None))
    {
        Terrain::TerrainDataChangedMask changeMask = m_terrainDirtyMask;

        if (terrainSettingsChanged)
        {
            changeMask |= Terrain::TerrainDataChangedMask::Settings;
        }

        // Make sure to set these *before* calling OnTerrainDataChanged, since it's possible that subsystems reacting to that call will
        // cause the data to become dirty again.
        AZ::Aabb dirtyRegion = ClampZBoundsToHeightBounds(m_dirtyRegion);
        m_terrainDirtyMask = AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::None;
        m_dirtyRegion = AZ::Aabb::CreateNull();

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(
            &AzFramework::Terrain::TerrainDataNotificationBus::Events::OnTerrainDataChanged, dirtyRegion,
            changeMask);
    }

}
