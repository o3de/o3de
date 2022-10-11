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
    // Convenience function to clamp a value to the given grid resolution, rounding up.
    inline float ClampToGridRoundUp(float value, float gridResolution)
    {
        return ceil(value / gridResolution) * gridResolution;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convenience function to clamp a value to the given grid resolution, rounding down.
    inline float ClampToGridRoundDown(float value, float gridResolution)
    {
        return floor(value / gridResolution) * gridResolution;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convenience function to get the terrain height values at each corner of an AABB, triangulate them,
    // and then find the nearest intersection (if any) between the resulting triangles and the given ray.
    inline void TriangulateAndFindNearestIntersection(const TerrainSystem& terrainSystem,
                                                      const AZ::Aabb& aabb,
                                                      AZ::Intersect::SegmentTriangleHitTester& hitTester,
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Iterative function that divides an AABB encompasing terrain points into columns (or a voxel grid)
    // of size equal to the given grid resolution, steps along the ray visiting each voxel it intersects
    // in order from nearest to farthest, then obtains the terrain height values at each corner in order
    // to triangulate them and find the nearest intersection (if any) between the triangles and the ray.
    //
    // Visualization:
    // - X: Column intersection but no triangle hit found
    // - T: Column intersection with a triangle hit found
    // ________________________________________
    // |    |    |    |    |    |    |    |    |
    // |____|____|____|____|____|____|____|____|  Ray
    // |    |    |    |    |    |    |    |    |  /
    // |____|____|____|____|____|____|____|____| /
    // |    |    |    |    |    |    |    | X  |/
    // |____|____|____|____|____|____|____|____/
    // |    |    |    |    |    |    |    | X /|
    // |____|____|____|____|____|____|____|__/_|
    // |    |    |    |    |    |    |    | /X |
    // |____|____|____|____|____|____|____|/___|
    // |    |    |    |    |    |    | X  / X  |
    // |____|____|____|____|____|____|___/|____|
    // |    |    |    |    |    |    | T/ |    |
    // |____|____|____|____|____|____|____|____|
    // |    |    |    |    |    |    |    |    |
    // |____|____|____|____|____|____|____|____|
    inline void FindNearestIntersectionIterative(const TerrainSystem& terrainSystem,
                                                 const AZ::Vector2& terrainResolution,
                                                 const AZ::Aabb& terrainWorldBounds,
                                                 const AZ::Vector3& rayStart,
                                                 const AZ::Vector3& rayEnd,
                                                 AzFramework::RenderGeometry::RayResult& result)
    {
        // Find the nearest intersection (if any) between the ray and terrain world bounds.
        // Note that the ray might (and often will) start inside the terrain world bounds.
        AZ::Vector3 clippedRayStart = rayStart;
        AZ::Vector3 clippedRayEnd = rayEnd;
        float tClipStart, tClipEnd;
        bool rayIntersected = AZ::Intersect::ClipRayWithAabb(terrainWorldBounds, clippedRayStart, clippedRayEnd, tClipStart, tClipEnd);

        if (!rayIntersected)
        {
            // The ray does not intersect the terrain world bounds.
            return;
        }

        const AZ::Vector3 nonNormalizedDirection = (clippedRayEnd - clippedRayStart);

        // The terrain world can be visualized as a 2D grid of height values,
        // where the terrain resolution determines the size of each 2D grid square.
        //
        // Starting at the initial intersection between the ray and the grid,
        // we want to step along the ray in the XY direction and visit each voxel the ray
        // intersects in order from nearest to furthest until we find
        // an intersection with the terrain or the ray exits the grid.
        const AZ::Vector3& initialIntersection = clippedRayStart;
        const float initialIntersectionX = initialIntersection.GetX();
        const float initialIntersectionY = initialIntersection.GetY();
        const float gridResolutionX = terrainResolution.GetX();
        const float gridResolutionY = terrainResolution.GetY();
        float initialVoxelMinX = ClampToGridRoundDown(initialIntersectionX, gridResolutionX);
        float initialVoxelMinY = ClampToGridRoundDown(initialIntersectionY, gridResolutionY);
        float initialVoxelMaxX = ClampToGridRoundUp(initialIntersectionX, gridResolutionX);
        float initialVoxelMaxY = ClampToGridRoundUp(initialIntersectionY, gridResolutionY);

        // For each axis calculate the distance t we need to move along
        // the ray in order to fully traverse a voxel in that dimension.
        const float rayDirectionX = nonNormalizedDirection.GetX();
        const float rayDirectionY = nonNormalizedDirection.GetY();
        const float stepX = AZ::GetSign(rayDirectionX) * gridResolutionX;
        const float stepY = AZ::GetSign(rayDirectionY) * gridResolutionY;
        const float tDeltaX = rayDirectionX ?
                              stepX / rayDirectionX :
                              std::numeric_limits<float>::max();
        const float tDeltaY = rayDirectionY ?
                              stepY / rayDirectionY :
                              std::numeric_limits<float>::max();

        // For each axis, calculate the distance t we need to move along the ray
        // from the initial intersection point to the next voxel along that axis.
        const float offsetX = stepX < 0.0f ?
                              initialVoxelMinX - initialIntersectionX :
                              initialVoxelMaxX - initialIntersectionX;
        const float offsetY = stepY < 0.0f ?
                              initialVoxelMinY - initialIntersectionY :
                              initialVoxelMaxY - initialIntersectionY;
        float tMaxX = rayDirectionX ?
                      offsetX / rayDirectionX :
                      std::numeric_limits<float>::max();
        float tMaxY = rayDirectionY ?
                      offsetY / rayDirectionY :
                      std::numeric_limits<float>::max();

        // Calculate the min/max voxel grid value on each axis by expanding
        // the terrain world bounds so they align with the grid resolution.
        const float voxelGridMinX = ClampToGridRoundDown(terrainWorldBounds.GetMin().GetX(), gridResolutionX);
        const float voxelGridMinY = ClampToGridRoundDown(terrainWorldBounds.GetMin().GetY(), gridResolutionY);
        const float voxelGridMaxX = ClampToGridRoundUp(terrainWorldBounds.GetMax().GetX(), gridResolutionX);
        const float voxelGridMaxY = ClampToGridRoundUp(terrainWorldBounds.GetMax().GetY(), gridResolutionY);

        // Using the initial voxel values, construct an AABB representing the current voxel,
        // then grab references to AABBs min/max vectors so we can manipulate them directly.
        AZ::Aabb currentVoxel = AZ::Aabb::CreateFromMinMax({initialVoxelMinX, initialVoxelMinY, terrainWorldBounds.GetMin().GetZ()},
                                                           {initialVoxelMaxX, initialVoxelMaxY, terrainWorldBounds.GetMax().GetZ()});
        AZ::Vector3& currentVoxelMin = const_cast<AZ::Vector3&>(currentVoxel.GetMin());
        AZ::Vector3& currentVoxelMax = const_cast<AZ::Vector3&>(currentVoxel.GetMax());
        const AZ::Vector3 stepVecX(stepX, 0.0f, 0.0f);
        const AZ::Vector3 stepVecY(0.0f, stepY, 0.0f);

        // Now we can step along the ray and visit each voxel the ray
        // intersects in order from nearest to furthest until we find
        // an intersection with the terrain or the ray exits the grid.
        result = AzFramework::RenderGeometry::RayResult();
        AZ::Intersect::SegmentTriangleHitTester hitTester(clippedRayStart, clippedRayEnd);
        while (currentVoxel.GetMin().GetX() <= voxelGridMaxX &&
               currentVoxel.GetMax().GetX() >= voxelGridMinX &&
               currentVoxel.GetMin().GetY() <= voxelGridMaxY &&
               currentVoxel.GetMax().GetY() >= voxelGridMinY &&
               tMaxX <= 1.0f && tMaxY <= 1.0f)
        {
            TriangulateAndFindNearestIntersection(terrainSystem, currentVoxel, hitTester, result);
            if (result)
            {
                // Intersection found. Replace the triangle normal from the hit with a higher-quality normal calculated
                // by the terrain system.
                result.m_worldNormal =
                    terrainSystem.GetNormal(result.m_worldPosition, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT);
                break;
            }

            // Step to the next voxel.
            if (tMaxX < tMaxY)
            {
                currentVoxelMin += stepVecX;
                currentVoxelMax += stepVecX;
                tMaxX += tDeltaX;
            }
            else
            {
                currentVoxelMin += stepVecY;
                currentVoxelMax += stepVecY;
                tMaxY += tDeltaY;
            }
        }
    }
}

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

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::RenderGeometry::RayResult TerrainRaycastContext::RayIntersect(
    const AzFramework::RenderGeometry::RayRequest& ray)
{
    const AZ::Aabb terrainWorldBounds = m_terrainSystem.GetTerrainAabb();
    const float terrainResolution = m_terrainSystem.GetTerrainHeightQueryResolution();
    const AZ::Vector2 terrainResolution2d(terrainResolution);
    AzFramework::RenderGeometry::RayResult rayIntersectionResult;
    FindNearestIntersectionIterative(m_terrainSystem,
                                     terrainResolution2d,
                                     terrainWorldBounds,
                                     ray.m_startWorldPosition,
                                     ray.m_endWorldPosition,
                                     rayIntersectionResult);

    // If needed we could call m_terrainSystem.FindBestAreaEntityAtPosition in order to set
    // rayIntersectionResult.m_entityAndComponent, but I'm not sure whether that is correct.
    return rayIntersectionResult;
}
