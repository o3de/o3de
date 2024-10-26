/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxMathUtil.h"
#include "WhiteBoxManipulatorBounds.h"

#include <AzCore/Math/IntersectSegment.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(ManipulatorBoundPolygon, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ManipulatorBoundEdge, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BoundShapePolygon, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BoundShapeEdge, AZ::SystemAllocator)

    bool IntersectRayVertex(
        const VertexBound& vertexBound, const float vertexScreenRadius, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        float distance;
        if (AZ::Intersect::IntersectRaySphere(
                rayOrigin, rayDirection, vertexBound.m_center, vertexScreenRadius, distance))
        {
            rayIntersectionDistance = distance;
            return true;
        }

        return false;
    }

    bool IntersectRayPolygon(
        const PolygonBound& polygonBound, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        float& rayIntersectionDistance, int64_t& intersectedTriangleIndex)
    {
        AZ_Assert(polygonBound.m_triangles.size() % 3 == 0, "Invalid number of points to represent triangles");

        const float rayLength = 1000.0f;
        AZ::Intersect::SegmentTriangleHitTester hitTester(rayOrigin, rayOrigin + rayDirection * rayLength);

        for (size_t triangleIndex = 0; triangleIndex < polygonBound.m_triangles.size(); triangleIndex += 3)
        {
            AZ::Vector3 p0 = polygonBound.m_triangles[triangleIndex];
            AZ::Vector3 p1 = polygonBound.m_triangles[triangleIndex + 1];
            AZ::Vector3 p2 = polygonBound.m_triangles[triangleIndex + 2];

            float time;
            AZ::Vector3 normal;
            const bool intersected = hitTester.IntersectSegmentTriangleCCW(p0, p1, p2, normal, time);

            if (intersected)
            {
                rayIntersectionDistance = time * rayLength;
                intersectedTriangleIndex = triangleIndex / 3;
                return true;
            }
        }

        return false;
    }

    bool ManipulatorBoundPolygon::IntersectRay(
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
    {
        int64_t intersectedTriangleIndex = 0;
        return IntersectRayPolygon(
            m_polygonBound, rayOrigin, rayDirection, rayIntersectionDistance, intersectedTriangleIndex);
    }

    void ManipulatorBoundPolygon::SetShapeData(const AzToolsFramework::Picking::BoundRequestShapeBase& shapeData)
    {
        if (const auto* polygonData = azrtti_cast<const BoundShapePolygon*>(&shapeData))
        {
            m_polygonBound.m_triangles = polygonData->m_triangles;
        }
    }

    AZStd::shared_ptr<AzToolsFramework::Picking::BoundShapeInterface> BoundShapePolygon::MakeShapeInterface(
        AzToolsFramework::Picking::RegisteredBoundId id) const
    {
        AZStd::shared_ptr<AzToolsFramework::Picking::BoundShapeInterface> quad =
            AZStd::make_shared<ManipulatorBoundPolygon>(id);
        quad->SetShapeData(*this);
        return quad;
    }

    bool IntersectRayEdge(
        const EdgeBound& edgeBound, const float edgeScreenWidth, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        const float rayLength = 1000.0f; // this is arbitrary to turn it into a ray
        const AZ::Vector3 sa = rayOrigin;
        const AZ::Vector3 sb = rayOrigin + rayDirection * rayLength;
        const AZ::Vector3 p = edgeBound.m_start;
        const AZ::Vector3 q = edgeBound.m_end;
        const float r = edgeScreenWidth;

        float t = 0.0f;
        if (IntersectSegmentCylinder(sa, sb, p, q, r, t) != 0)
        {
            // intersected, t is normalized distance along the ray
            rayIntersectionDistance = t * rayLength;
            return true;
        }

        return false;
    }

    bool ManipulatorBoundEdge::IntersectRay(
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
    {
        return IntersectRayEdge(m_edgeBound, m_edgeBound.m_radius, rayOrigin, rayDirection, rayIntersectionDistance);
    }

    void ManipulatorBoundEdge::SetShapeData(const AzToolsFramework::Picking::BoundRequestShapeBase& shapeData)
    {
        if (const auto* edgeData = azrtti_cast<const BoundShapeEdge*>(&shapeData))
        {
            m_edgeBound = {edgeData->m_start, edgeData->m_end, edgeData->m_radius};
        }
    }

    AZStd::shared_ptr<AzToolsFramework::Picking::BoundShapeInterface> BoundShapeEdge::MakeShapeInterface(
        AzToolsFramework::Picking::RegisteredBoundId id) const
    {
        AZStd::shared_ptr<ManipulatorBoundEdge> edge = AZStd::make_shared<ManipulatorBoundEdge>(id);
        edge->SetShapeData(*this);
        return edge;
    }
} // namespace WhiteBox
