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
        point0.SetZ(terrainSystem.GetHeight(point0, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT));
        point1.SetZ(terrainSystem.GetHeight(point1, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT));
        point2.SetZ(terrainSystem.GetHeight(point2, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT));
        point3.SetZ(terrainSystem.GetHeight(point3, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT));

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
                                point3,
                                point1,
                                bottomLeftIntersectionResult);

        AzFramework::RenderGeometry::RayResult topRightIntersectionResult;
        FindNearestIntersection(rayStart,
                                rayDirection,
                                point2,
                                point1,
                                point3,
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
                                                 const AZ::Vector3& rayDirection,
                                                 const AZ::Vector3& rayDirectionReciprocal,
                                                 AzFramework::RenderGeometry::RayResult& result)
    {
        // Find the nearest intersection (if any) between the ray and terrain world bounds.
        // Note that the ray might (and often will) start inside the terrain world bounds.
        FindNearestIntersection(terrainWorldBounds,
                                rayStart,
                                rayDirection,
                                rayDirectionReciprocal,
                                result);
        if (!result)
        {
            // The ray does not intersect the terrain world bounds.
            return;
        }

        // The terrain world can be visualized as a grid of columns,
        // where the terrain resolution determines the dimensions of
        // each column, or a voxel grid with one cell in z dimension.
        //
        // Starting at the voxel containing the initial intersection,
        // we want to step along the ray and visit each voxel the ray
        // intersects in order from nearest to furthest until we find
        // an intersection with the terrain or the ray exits the grid.
        const AZ::Vector3& initialIntersection = result.m_worldPosition;
        const float initialIntersectionX = initialIntersection.GetX();
        const float initialIntersectionY = initialIntersection.GetY();
        const float initialIntersectionZ = initialIntersection.GetZ();
        const float gridResolutionX = terrainResolution.GetX();
        const float gridResolutionY = terrainResolution.GetY();
        const float gridResolutionZ = terrainWorldBounds.GetMax().GetZ() - terrainWorldBounds.GetMin().GetZ();
        float initialVoxelMinX = ClampToGridRoundDown(initialIntersectionX, gridResolutionX);
        float initialVoxelMinY = ClampToGridRoundDown(initialIntersectionY, gridResolutionY);
        float initialVoxelMinZ = terrainWorldBounds.GetMin().GetZ();
        float initialVoxelMaxX = ClampToGridRoundUp(initialIntersectionX, gridResolutionX);
        float initialVoxelMaxY = ClampToGridRoundUp(initialIntersectionY, gridResolutionY);
        float initialVoxelMaxZ = terrainWorldBounds.GetMax().GetZ();

        // For each axis calculate the distance t we need to move along
        // the ray in order to fully traverse a voxel in that dimension.
        const float rayDirectionX = rayDirection.GetX();
        const float rayDirectionY = rayDirection.GetY();
        const float rayDirectionZ = rayDirection.GetZ();
        const float stepX = AZ::GetSign(rayDirectionX) * gridResolutionX;
        const float stepY = AZ::GetSign(rayDirectionY) * gridResolutionY;
        const float stepZ = AZ::GetSign(rayDirectionZ) * gridResolutionZ;
        const float tDeltaX = rayDirectionX ?
                              stepX / rayDirectionX :
                              std::numeric_limits<float>::max();
        const float tDeltaY = rayDirectionY ?
                              stepY / rayDirectionY :
                              std::numeric_limits<float>::max();
        const float tDeltaZ = rayDirectionZ ?
                              stepZ / rayDirectionZ :
                              std::numeric_limits<float>::max();

        // For each axis, calculate the distance t we need to move along the ray
        // from the initial intersection point to the next voxel along that axis.
        const float offsetX = stepX < 0.0f ?
                              initialVoxelMinX - initialIntersectionX :
                              initialVoxelMaxX - initialIntersectionX;
        const float offsetY = stepY < 0.0f ?
                              initialVoxelMinY - initialIntersectionY :
                              initialVoxelMaxY - initialIntersectionY;
        const float offsetZ = stepZ < 0.0f ?
                              initialVoxelMinZ - initialIntersectionZ :
                              initialVoxelMaxZ - initialIntersectionZ;
        float tMaxX = rayDirectionX ?
                      offsetX / rayDirectionX :
                      std::numeric_limits<float>::max();
        float tMaxY = rayDirectionY ?
                      offsetY / rayDirectionY :
                      std::numeric_limits<float>::max();
        float tMaxZ = rayDirectionZ ?
                      offsetZ / rayDirectionZ :
                      std::numeric_limits<float>::max();

        // Calculate the min/max voxel grid value on each axis by expanding
        // the terrain world bounds so they align with the grid resolution.
        const float voxelGridMinX = ClampToGridRoundDown(terrainWorldBounds.GetMin().GetX(), gridResolutionX);
        const float voxelGridMinY = ClampToGridRoundDown(terrainWorldBounds.GetMin().GetY(), gridResolutionY);
        const float voxelGridMinZ = terrainWorldBounds.GetMin().GetZ();
        const float voxelGridMaxX = ClampToGridRoundUp(terrainWorldBounds.GetMax().GetX(), gridResolutionX);
        const float voxelGridMaxY = ClampToGridRoundUp(terrainWorldBounds.GetMax().GetY(), gridResolutionY);
        const float voxelGridMaxZ = terrainWorldBounds.GetMax().GetZ();

        // Using the initial voxel values, construct an AABB representing the current voxel,
        // then grab references to AABBs min/max vectors so we can manipulate them directly.
        AZ::Aabb currentVoxel = AZ::Aabb::CreateFromMinMax({initialVoxelMinX, initialVoxelMinY, initialVoxelMinZ},
                                                           {initialVoxelMaxX, initialVoxelMaxY, initialVoxelMaxZ});
        AZ::Vector3& currentVoxelMin = const_cast<AZ::Vector3&>(currentVoxel.GetMin());
        AZ::Vector3& currentVoxelMax = const_cast<AZ::Vector3&>(currentVoxel.GetMax());
        const AZ::Vector3 stepVecX(stepX, 0.0f, 0.0f);
        const AZ::Vector3 stepVecY(0.0f, stepY, 0.0f);
        const AZ::Vector3 stepVecZ(0.0f, 0.0f, stepZ);

        // Now we can step along the ray and visit each voxel the ray
        // intersects in order from nearest to furthest until we find
        // an intersection with the terrain or the ray exits the grid.
        result = AzFramework::RenderGeometry::RayResult();
        while (currentVoxel.GetMin().GetX() <= voxelGridMaxX &&
               currentVoxel.GetMax().GetX() >= voxelGridMinX &&
               currentVoxel.GetMin().GetY() <= voxelGridMaxY &&
               currentVoxel.GetMax().GetY() >= voxelGridMinY &&
               currentVoxel.GetMin().GetZ() <= voxelGridMaxZ &&
               currentVoxel.GetMax().GetZ() >= voxelGridMinZ)
        {
            TriangulateAndFindNearestIntersection(terrainSystem,
                                                  currentVoxel,
                                                  rayStart,
                                                  rayDirection,
                                                  rayDirectionReciprocal,
                                                  result);
            if (result)
            {
                // Intersection found.
                break;
            }

            // Step to the next voxel.
            if (tMaxX < tMaxY && tMaxX < tMaxZ)
            {
                currentVoxelMin += stepVecX;
                currentVoxelMax += stepVecX;
                tMaxX += tDeltaX;
            }
            else if (tMaxY < tMaxZ)
            {
                currentVoxelMin += stepVecY;
                currentVoxelMax += stepVecY;
                tMaxY += tDeltaY;
            }
            else
            {
                currentVoxelMin += stepVecZ;
                currentVoxelMax += stepVecZ;
                tMaxZ += tDeltaZ;
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
    const AZ::Vector2 terrainResolution = m_terrainSystem.GetTerrainHeightQueryResolution();
    const AZ::Vector3 rayDirection = ray.m_endWorldPosition - ray.m_startWorldPosition;
    const AZ::Vector3 rayDirectionReciprocal = rayDirection.GetReciprocal();
    AzFramework::RenderGeometry::RayResult rayIntersectionResult;
    FindNearestIntersectionIterative(m_terrainSystem,
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
