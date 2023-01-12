/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRaycast/TerrainRaycastContext.h>
#include <TerrainSystem/TerrainSystem.h>

#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/std/sort.h>

using namespace Terrain;

namespace
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convenience function to get the terrain height values at each corner of an AABB, triangulate them,
    // and then find the nearest intersection (if any) between the resulting triangles and the given ray.
    static void TriangulateAndFindNearestIntersection(const TerrainSystem& terrainSystem,
                                                      const AZ::Aabb& aabb,
                                                      const AZ::Intersect::SegmentTriangleHitTester& hitTester,
                                                      AzFramework::RenderGeometry::RayResult& result)
    {
        // Obtain the height values at each corner of the AABB.
        const AZ::Vector3& aabbMin = aabb.GetMin();
        const AZ::Vector3& aabbMax = aabb.GetMax();
        AZ::Vector3 point0 = aabbMin;
        AZ::Vector3 point2 = aabbMax;
        AZ::Vector3 point1(point0.GetX(), point2.GetY(), 0.0f);
        AZ::Vector3 point3(point2.GetX(), point0.GetY(), 0.0f);
        point0.SetZ(terrainSystem.GetHeight(point0, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT));
        point1.SetZ(terrainSystem.GetHeight(point1, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT));
        point2.SetZ(terrainSystem.GetHeight(point2, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT));
        point3.SetZ(terrainSystem.GetHeight(point3, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT));

        // Finally, triangulate the four terrain points and check for a hit,
        // splitting using the top-left -> bottom-right diagonal so to match
        // the current behavior of the terrain physics and rendering systems.
        AZ::Vector3 bottomLeftHitNormal;
        float bottomLeftHitDistance;
        bool bottomLeftHit =
            hitTester.IntersectSegmentTriangleCCW(point0, point3, point1, bottomLeftHitNormal, bottomLeftHitDistance);

        AZ::Vector3 topRightHitNormal;
        float topRightHitDistance;
        bool topRightHit =
            hitTester.IntersectSegmentTriangleCCW(point2, point1, point3, topRightHitNormal, topRightHitDistance);

        if (bottomLeftHit && (!topRightHit || (bottomLeftHitDistance < topRightHitDistance)))
        {
            result.m_distance = bottomLeftHitDistance;
            result.m_worldNormal = bottomLeftHitNormal;
            result.m_worldPosition = hitTester.GetIntersectionPoint(result.m_distance);
        }
        else if (topRightHit)
        {
            result.m_distance = topRightHitDistance;
            result.m_worldNormal = topRightHitNormal;
            result.m_worldPosition = hitTester.GetIntersectionPoint(result.m_distance);
        }
    }

} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
TerrainRaycastContext::TerrainRaycastContext(TerrainSystem& terrainSystem)
    : m_terrainSystem(terrainSystem)
    , m_entityContextId(AzFramework::EntityContextId::CreateRandom())
{
    AzFramework::RenderGeometry::IntersectorBus::Handler::BusConnect(m_entityContextId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TerrainRaycastContext::~TerrainRaycastContext()
{
    AzFramework::RenderGeometry::IntersectorBus::Handler::BusDisconnect();
}

/*
   Iterative function that divides an AABB encompasing terrain points into grid squares based on
   the given grid resolution and steps along the ray visiting each voxel it intersects in order
   from nearest to farthest. In each square, it obtains the terrain height values at each corner and
   triangulates them to find the nearest intersection (if any) between the triangles and the ray.

   To step through the grid, we use an algorithm similar to Bresenham's line algorithm or a Digital
   Differential Analyzer. We can't use Bresenham's line algorithm itself because it will sometimes skip
   squares if the ray only passes through a tiny portion, and we need to use every square that it passes through.

   We start by clipping the ray itself to the terrain AABB so that we don't walk through any grid squares
   that cannot contain terrain. We then walk through the grid one square at a time, either moving horizontally
   or vertically to the next square based on the ray's slope, until we reach the end of the ray or we've found a hit.

   Visualization:
    - X: Grid square intersection but no triangle hit found
    - T: Grid square intersection with a triangle hit found
    ________________________________________
    |    |    |    |    |    |    |    |    |
    |____|____|____|____|____|____|____|____|  Ray
    |    |    |    |    |    |    |    |    |  /
    |____|____|____|____|____|____|____|____| /
    |    |    |    |    |    |    |    | X  |/
    |____|____|____|____|____|____|____|____/
    |    |    |    |    |    |    |    | X /|
    |____|____|____|____|____|____|____|__/_|
    |    |    |    |    |    |    |    | /X |
    |____|____|____|____|____|____|____|/___|
    |    |    |    |    |    |    | X  / X  |
    |____|____|____|____|____|____|___/|____|
    |    |    |    |    |    |    | T/ |    |
    |____|____|____|____|____|____|____|____|
    |    |    |    |    |    |    |    |    |
    |____|____|____|____|____|____|____|____|
*/
AzFramework::RenderGeometry::RayResult TerrainRaycastContext::RayIntersect(
    const AzFramework::RenderGeometry::RayRequest& ray)
{
    const AZ::Aabb terrainWorldBounds = m_terrainSystem.GetTerrainAabb();
    const AZ::Vector2 terrainResolution(m_terrainSystem.GetTerrainHeightQueryResolution());

    // Initialize the result to invalid at the start.
    AzFramework::RenderGeometry::RayResult rayIntersectionResult = AzFramework::RenderGeometry::RayResult();

    if (!terrainWorldBounds.IsValid())
    {
        // There is no terrain to intersect.
        return rayIntersectionResult;
    }

    // Start by clipping the ray to the terrain world bounds so that we can reduce our iteration over the ray to just
    // the subset that can potentially collide with the terrain.
    // We use a slightly expanded terrain world bounds for clipping the ray so that precision errors don't cause the ray
    // to get overly truncated and miss a collision that might occur right on the world boundary.
    AZ::Vector3 clippedRayStart = ray.m_startWorldPosition;
    AZ::Vector3 clippedRayEnd = ray.m_endWorldPosition;
    float tClipStart, tClipEnd;
    bool rayIntersected = AZ::Intersect::ClipRayWithAabb(
        terrainWorldBounds.GetExpanded(AZ::Vector3(0.01f)), clippedRayStart, clippedRayEnd, tClipStart, tClipEnd);

    if (!rayIntersected)
    {
        // The ray does not intersect the terrain world bounds.
        return rayIntersectionResult;
    }

    // Move our clipped line segment into Vector2s for more convenient use below.
    const AZ::Vector2 clippedStart(clippedRayStart);
    const AZ::Vector2 clippedEnd(clippedRayEnd);
    const AZ::Vector2 clippedLineSegment = clippedEnd - clippedStart;

    // Calculate the total number of terrain squares we'll need to visit to trace the ray segment.
    // We need to visit 1 at the start, 1 for each X square we need to move, and 1 for each Y square we need to move,
    // since we'll always move either horizontally or vertically one square at a time when traversing the ray segment.
    const AZ::Vector2 numSquaresToMove =
        ((clippedEnd / terrainResolution).GetFloor() - (clippedStart / terrainResolution).GetFloor()).GetAbs();
    const int32_t numTerrainSquares =
        1 + aznumeric_cast<int32_t>(numSquaresToMove.GetX()) + aznumeric_cast<int32_t>(numSquaresToMove.GetY());

    // This tells us how much t distance on the line to move to increment one terrain square in each direction.
    // Note that it could be infinity (due to a divide-by-0) if we're not moving in that direction.
    const AZ::Vector2 tDelta(terrainResolution / clippedLineSegment.GetAbs());

    // Get the min world space corner of the terrain grid square containing (x0, y0)
    const AZ::Vector2 clippedStartGridCorner = (clippedStart / terrainResolution).GetFloor() * terrainResolution;

    // tUntilNextBoundary stores how much further we currently need to move along t to get to the next terrain grid square boundary
    // in each direction.
    // We initialize with the fractional amount that we're starting in the square or max() if we're not moving in this
    // direction at all (when clippedLineSegment == 0)
    const AZ::Vector2 tFromMinCorner((clippedStart - clippedStartGridCorner) / clippedLineSegment.GetAbs());

    AZ::Vector2 tUntilNextBoundary = AZ::Vector2::CreateSelectCmpEqual(
        clippedLineSegment, AZ::Vector2::CreateZero(), AZ::Vector2(AZStd::numeric_limits<float>::max()), tFromMinCorner);

    // If we're moving in the positive direction in the square, then the amount till the next boundary is actually
    // the distance remaining to the max corner, not the distance in from the min corner, so flip our calculation.
    tUntilNextBoundary = AZ::Vector2::CreateSelectCmpGreater(clippedEnd, clippedStart, tDelta - tUntilNextBoundary, tUntilNextBoundary);

    // Initialize our segment/triangle hit tester with the ray that we're using. We use the full ray instead of the clipped one
    // to make sure we don't run into any precision issues caused from the clipping.
    AZ::Intersect::SegmentTriangleHitTester hitTester(ray.m_startWorldPosition, ray.m_endWorldPosition);

    // These will hold our current square coordinates in world space values as we loop through the squares, starting with the
    // grid square for (x0, y0). These values represent the minimum corner of each terrain square.
    AZ::Vector2 curGridCorner = clippedStartGridCorner;

    // This is how much we need to increment our x and y by to get to the next grid square along the line.
    // They will either be +/- terrainResolution or 0 if we're not moving in that direction.
    const AZ::Vector2 gridIncrement = terrainResolution *
        AZ::Vector2::CreateSelectCmpEqual(clippedLineSegment,
                                          AZ::Vector2::CreateZero(),
                                          AZ::Vector2::CreateZero(),
                                          AZ::Vector2(AZ::GetSign(clippedLineSegment.GetX()), AZ::GetSign(clippedLineSegment.GetY())));

    // Convenience vectors that we can use in the loop to just increment one direction.
    const AZ::Vector2 tDeltaX(tDelta.GetX(), 0.0f);
    const AZ::Vector2 tDeltaY(0.0f, tDelta.GetY());
    const AZ::Vector2 gridIncrementX(gridIncrement.GetX(), 0.0f);
    const AZ::Vector2 gridIncrementY(0.0f, gridIncrement.GetY());

    // Walk through each grid square in the terrain that intersects the XY coordinates of the line.
    // We'll check each square to see if the ray intersections actually intersect the terrain triangles in the square.
    for (int terrainSquare = 0; terrainSquare < numTerrainSquares; terrainSquare++)
    {
        // Create a bounding volume for this terrain square.
        AZ::Aabb currentVoxel = AZ::Aabb::CreateFromMinMax(
            AZ::Vector3(curGridCorner, terrainWorldBounds.GetMin().GetZ()),
            AZ::Vector3(curGridCorner + terrainResolution, terrainWorldBounds.GetMax().GetZ()));

        // Check for a hit against the terrain triangles in this square.
        // Note - this could be optimized to be 2x faster by adding some code to keep track of the terrain heights
        // from the previous square checked so that we only get the 2 new corners instead of all 4 every time.
        TriangulateAndFindNearestIntersection(m_terrainSystem, currentVoxel, hitTester, rayIntersectionResult);
        if (rayIntersectionResult)
        {
            // Intersection found. Replace the triangle normal from the hit with a higher-quality normal calculated
            // by the terrain system.
            rayIntersectionResult.m_worldNormal = m_terrainSystem.GetNormal(
                rayIntersectionResult.m_worldPosition, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT);

            // Return the distance in world space instead of in ray distance space.
            rayIntersectionResult.m_distance = rayIntersectionResult.m_worldPosition.GetDistance(ray.m_startWorldPosition);
            break;
        }

        // No hit yet, so move forward along the line (either horizontally or vertically) to the next terrain square.
        if (tUntilNextBoundary.GetY() < tUntilNextBoundary.GetX())
        {
            curGridCorner += gridIncrementY;
            tUntilNextBoundary += tDeltaY;
        }
        else
        {
            curGridCorner += gridIncrementX;
            tUntilNextBoundary += tDeltaX;
        }
    }

    // If needed we could call m_terrainSystem.FindBestAreaEntityAtPosition in order to set
    // rayIntersectionResult.m_entityAndComponent, but I'm not sure whether that is correct.
    return rayIntersectionResult;
}
