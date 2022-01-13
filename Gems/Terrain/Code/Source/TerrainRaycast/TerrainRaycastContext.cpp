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
    // Convenience function to clamp a value to the given grid resolution, rounding default.
    inline float ClampToGridRoundDefault(float value, float gridResolution)
    {
        return round(value / gridResolution) * gridResolution;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convenience function to find the T value of the nearest intersection between an AABB and a ray.
    inline float FindNearestIntersectionT(const AZ::Aabb& aabb,
                                          const AZ::Vector3& rayStart,
                                          const AZ::Vector3& rayDirectionReciprocal)
    {
        float intersectionT;
        float intersectionEndT;
        const int intersectionResult = AZ::Intersect::IntersectRayAABB2(rayStart,
                                                                        rayDirectionReciprocal,
                                                                        aabb,
                                                                        intersectionT,
                                                                        intersectionEndT);
        return (intersectionResult != AZ::Intersect::ISECT_RAY_AABB_NONE) ? intersectionT : FLT_MAX;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convenience function to find the nearest intersection (if any) between an AABB and a ray.
    inline void FindNearestIntersection(const AZ::Aabb& aabb,
                                        const AZ::Vector3& rayStart,
                                        const AZ::Vector3& rayDirection,
                                        const AZ::Vector3& rayDirectionReciprocal,
                                        AzFramework::RenderGeometry::RayResult& result)
    {
        float intersectionT;
        float intersectionEndT;
        AZ::Vector3 intersectionNormal;
        const int intersectionResult = AZ::Intersect::IntersectRayAABB(rayStart,
                                                                       rayDirection,
                                                                       rayDirectionReciprocal,
                                                                       aabb,
                                                                       intersectionT,
                                                                       intersectionEndT,
                                                                       intersectionNormal);
        if (intersectionResult != AZ::Intersect::ISECT_RAY_AABB_NONE)
        {
            result.m_worldPosition = rayStart + (rayDirection * intersectionT);
            result.m_worldNormal = intersectionNormal;
            result.m_distance = rayDirection.GetLength() * intersectionT;
        }
        else
        {
            result.m_distance = FLT_MAX;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convenience function to find the nearest intersection (if any) between a triangle and a ray.
    // This is an implementation of the Möller–Trumbore intersection algorithm. I first attempted to use
    // the existing AZ::Intersect::IntersectSegmentTriangleCCW, which appears to use the same algorithm,
    // but it takes a line segment as opposed to a ray and was not returning the expected results. Once
    // I've written some tests I can go back and try it again to figure out what is different, but this
    // is all likely to get replaced with an optimized SIMD version anyway so this should be ok for now.
    inline void FindNearestIntersection(const AZ::Vector3& vertexA,
                                        const AZ::Vector3& vertexB,
                                        const AZ::Vector3& vertexC,
                                        const AZ::Vector3& rayStart,
                                        const AZ::Vector3& rayDirection,
                                        AzFramework::RenderGeometry::RayResult& result)
    {
        const AZ::Vector3 edgeAB = vertexB - vertexA;
        const AZ::Vector3 edgeAC = vertexC - vertexA;
        const AZ::Vector3 pVec = rayDirection.Cross(edgeAC);
        const float det = edgeAB.Dot(pVec);
        if (AZ::IsClose(det, 0.0f))
        {
             // The ray is parallel to the triangle.
            return;
        }

        const float detInv = 1.0f / det;
        const AZ::Vector3 tVec = rayStart - vertexA;
        const float u = detInv * tVec.Dot(pVec);
        if (u < 0.0f || u > 1.0f)
        {
            // No intersection.
            return;
        }

        const AZ::Vector3 qVec = tVec.Cross(edgeAB);
        const float v = detInv * rayDirection.Dot(qVec);
        if (v < 0.0 || u + v > 1.0)
        {
            // No intersection.
            return;
        }

        const float t = detInv * edgeAC.Dot(qVec);
        if (t > FLT_EPSILON)
        {
            result.m_worldPosition = rayStart + (rayDirection * t);
            result.m_worldNormal = edgeAB.Cross(edgeAC);
            result.m_distance = rayDirection.GetLength() * t;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convenience function to get the terrain height values at each corner of an AABB, triangulate them,
    // and then find the nearest intersection (if any) between the resulting triangles and the given ray.
    inline void TriangulateAndFindNearestIntersection(const TerrainSystem& terrainSystem,
                                                      const AZ::Aabb& aabb,
                                                      const AZ::Vector3& rayStart,
                                                      const AZ::Vector3& rayDirection,
                                                      const AZ::Vector3& rayDirectionReciprocal,
                                                      AzFramework::RenderGeometry::RayResult& result)
    {
        // Obtain the height values at each corner of the AABB.
        const AZ::Vector3& aabbMin = aabb.GetMin();
        const AZ::Vector3& aabbMax = aabb.GetMax();
        AZ::Vector3 point0 = aabbMin;
        AZ::Vector3 point2 = aabbMax;
        AZ::Vector3 point1(point0.GetX(), point2.GetY(), 0.0f);
        AZ::Vector3 point3(point2.GetX(), point0.GetY(), 0.0f);
        point0.SetZ(terrainSystem.GetHeight(point0, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP));
        point1.SetZ(terrainSystem.GetHeight(point1, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP));
        point2.SetZ(terrainSystem.GetHeight(point2, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP));
        point3.SetZ(terrainSystem.GetHeight(point2, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP));

        // Construct a smaller AABB that tightly encloses the four terrain points.
        const float refinedMinZ = AZStd::GetMin(AZStd::GetMin(AZStd::GetMin(point0.GetZ(), point1.GetZ()), point2.GetZ()), point3.GetZ());
        const float refinedMaxZ = AZStd::GetMax(AZStd::GetMax(AZStd::GetMax(point0.GetZ(), point1.GetZ()), point2.GetZ()), point3.GetZ());
        const AZ::Vector3 refinedMin(aabbMin.GetX(), aabbMin.GetY(), refinedMinZ);
        const AZ::Vector3 refinedMax(aabbMax.GetX(), aabbMax.GetY(), refinedMaxZ);
        const AZ::Aabb refinedAABB = AZ::Aabb::CreateFromMinMax(refinedMin, refinedMax);

        // Check for a hit against the refined AABB.
        float intersectionT;
        float intersectionEndT;
        const int intersectionResult = AZ::Intersect::IntersectRayAABB2(rayStart,
                                                                        rayDirectionReciprocal,
                                                                        refinedAABB,
                                                                        intersectionT,
                                                                        intersectionEndT);
        if (intersectionResult == AZ::Intersect::ISECT_RAY_AABB_NONE)
        {
            return;
        }

        // Finally, triangulate the four terrain points and check for a hit,
        // splitting using the top-left -> bottom-right diagonal so to match
        // the current behavior of the terrain physics and rendering systems.
        AzFramework::RenderGeometry::RayResult bottomLeftIntersectionResult;
        FindNearestIntersection(rayStart,
                                rayDirection,
                                point0,
                                point1,
                                point3,
                                bottomLeftIntersectionResult);

        AzFramework::RenderGeometry::RayResult topRightIntersectionResult;
        FindNearestIntersection(rayStart,
                                rayDirection,
                                point2,
                                point3,
                                point1,
                                topRightIntersectionResult);

        if (bottomLeftIntersectionResult)
        {
            result = !topRightIntersectionResult || bottomLeftIntersectionResult.m_distance < topRightIntersectionResult.m_distance ?
                     bottomLeftIntersectionResult :
                     topRightIntersectionResult;
        }
        else if (topRightIntersectionResult)
        {
            result = topRightIntersectionResult;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Recursive function that sub-divides an AABB encompasing terrain points, stopping when the size of
    // the smallest sub-divided AABBs matches the given grid resolution, then obtains the terrain height
    // values at each corner in order to triangulate them and then find the nearest intersection (if any)
    // between the resulting triangles and the given ray. The AABB is only sub-divided about the x/y axes
    // and we don't calculate any height values until we get down to an AABB that matches the resolution.
    //
    // Visualization:
    // - O: AABB miss
    // - X: AABB hit and recursed but no trianlge hit found
    // - T: AABB hit and recursed with a trianlge hit found
    // - ?: AABB hit but not recursed because closer triangle hit already found
    // ________________________________________
    // |                   |         |         |
    // |                   |    O    |    O    |  Ray
    // |                   |         |         |  /
    // |         O         |_________|_________| /
    // |                   |         | O  | X  |/
    // |                   |    O    |____|____/
    // |                   |         | O  | X /|
    // |___________________|_________|____|__/_|
    // |                   |         | O  | /T |
    // |                   |    O    |____|____|
    // |                   |         | ?  | ?  |
    // |         O         |_________|____|____|
    // |                   |         |         |
    // |                   |    ?    |    ?    |
    // |                   |         |         |
    // |__________________ |_________|_________|
    inline void FindNearestIntersectionRecursive(const TerrainSystem& terrainSystem,
                                                 const AZ::Vector2& gridResolution,
                                                 const AZ::Aabb& bounds,
                                                 const AZ::Vector3& rayStart,
                                                 const AZ::Vector3& rayDirection,
                                                 const AZ::Vector3& rayDirectionReciprocal,
                                                 AzFramework::RenderGeometry::RayResult& result)
    {
        // First check whether the extents of the AABB match the grid resolution,
        // meaning we have sub-divided the global terrain bounds as far as we can.
        const AZ::Vector2 extentsXY(bounds.GetExtents());
        if (extentsXY.IsClose(gridResolution))
        {
            TriangulateAndFindNearestIntersection(terrainSystem,
                                                  bounds,
                                                  rayStart,
                                                  rayDirection,
                                                  rayDirectionReciprocal,
                                                  result);
            return;
        }

        // Calculate all values necessary to sub-divide the AABB into four quadrants
        // about the x / y axes, and so the quadrants align with the grid resolution,
        // ensuring to expand outwards (if necessary) in order to capture each point.
        const AZ::Vector3& min = bounds.GetMin();
        const AZ::Vector3& max = bounds.GetMax();
        const AZ::Vector3& center = bounds.GetCenter();
        const float gridResolutionX = gridResolution.GetX();
        const float gridResolutionY = gridResolution.GetY();
        const float minX = ClampToGridRoundDown(min.GetX(), gridResolutionX);
        const float minY = ClampToGridRoundDown(min.GetY(), gridResolutionY);
        const float minZ = min.GetZ();
        const float maxX = ClampToGridRoundUp(max.GetX(), gridResolutionX);
        const float maxY = ClampToGridRoundUp(max.GetY(), gridResolutionY);
        const float maxZ = max.GetZ();
        const float centerX = ClampToGridRoundDefault(center.GetX(), gridResolutionX);
        const float centerY = ClampToGridRoundDefault(center.GetY(), gridResolutionY);

        // Create an array containing each sub-divided quadrant paried with a
        // float where we will store the T value of the nearest intersection.
        AZStd::array<AZStd::pair<AZ::Aabb, float>, 4> quadrantNearestIntersectionTPairs;
        quadrantNearestIntersectionTPairs[0] = AZStd::make_pair<AZ::Aabb, float>(AZ::Aabb::CreateFromMinMax({minX, minY, minZ}, {centerX, centerY, maxZ}), FLT_MAX);
        quadrantNearestIntersectionTPairs[1] = AZStd::make_pair<AZ::Aabb, float>(AZ::Aabb::CreateFromMinMax({minX, centerY, minZ}, {centerX, maxY, maxZ}), FLT_MAX);
        quadrantNearestIntersectionTPairs[2] = AZStd::make_pair<AZ::Aabb, float>(AZ::Aabb::CreateFromMinMax({centerX, minY, minZ}, {maxX, centerY, maxZ}), FLT_MAX);
        quadrantNearestIntersectionTPairs[3] = AZStd::make_pair<AZ::Aabb, float>(AZ::Aabb::CreateFromMinMax({centerX, centerY, minZ}, {maxX, maxY, maxZ}), FLT_MAX);

        // For each quadrant, get the T value of the nearest intersection.
        for (auto& quadrantNearestIntersectionTPair : quadrantNearestIntersectionTPairs)
        {
            quadrantNearestIntersectionTPair.second = FindNearestIntersectionT(quadrantNearestIntersectionTPair.first,
                                                                               rayStart,
                                                                               rayDirectionReciprocal);
        }

        // Sort the quadrants by the T value of their nearest intersection.
        AZStd::sort(quadrantNearestIntersectionTPairs.begin(),
                    quadrantNearestIntersectionTPairs.end(),
                    [](const AZStd::pair<AZ::Aabb, float>& left, AZStd::pair<AZ::Aabb, float>& right) -> bool
                    { 
                        return left.second < right.second; 
                    });

        // For each sorted quadrant...
        for (const auto& quadrantNearestIntersectionTPair : quadrantNearestIntersectionTPairs)
        {
            // ...check if their was any intersection with the ray.
            if (quadrantNearestIntersectionTPair.second == FLT_MAX)
            {
                // No intersection was found.
                result.m_distance = FLT_MAX;
                return;
            }

            // Recurse into this function to further sub-divide the AABB,
            // until the extents of the bounds match the grid resolution.
            FindNearestIntersectionRecursive(terrainSystem,
                                             gridResolution,
                                             quadrantNearestIntersectionTPair.first,
                                             rayStart,
                                             rayDirection,
                                             rayDirectionReciprocal,
                                             result);

            // Return as soon as we find a result,
            // which is guaranteed to be the closest intersection.
            if (result)
            {
                return;
            }
        }

        // No intersection was found.
        result.m_distance = FLT_MAX;
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
    const AZ::Vector2 terrainResolution = m_terrainSystem.GetTerrainHeightQueryResolution();
    const AZ::Vector3 rayDirection = ray.m_endWorldPosition - ray.m_startWorldPosition;
    const AZ::Vector3 rayDirectionReciprocal = rayDirection.GetReciprocal();
    AzFramework::RenderGeometry::RayResult rayIntersectionResult;
    FindNearestIntersectionRecursive(m_terrainSystem,
                                     terrainResolution,
                                     terrainWorldBounds,
                                     ray.m_startWorldPosition,
                                     rayDirection,
                                     rayDirectionReciprocal,
                                     rayIntersectionResult);

    // If needed we could call m_terrainSystem.FindBestAreaEntityAtPosition in order to set
    // rayIntersectionResult.m_entityAndComponent, but I'm not sure whether that is correct.
    return rayIntersectionResult;
}
