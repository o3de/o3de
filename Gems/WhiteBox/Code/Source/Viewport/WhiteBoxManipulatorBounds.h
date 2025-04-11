/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Picking/BoundInterface.h>
#include <AzToolsFramework/Picking/ContextBoundAPI.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Represents all triangles composing a polygon that can have intersection
    //! queries performed against it.
    //! @note Each triangle must be defined in CCW order.
    struct PolygonBound
    {
        AZStd::vector<AZ::Vector3> m_triangles;
    };

    //! Provides a mapping between a polygon handle and the bound it represents.
    //! @note This is a cache to save computing the polygon bound each time from the polygon handle.
    struct PolygonBoundWithHandle
    {
        PolygonBound m_bound;
        Api::PolygonHandle m_handle;
    };

    //! Represents the beginning and end of an edge that can
    //! have intersection queries performed against it.
    struct EdgeBound
    {
        AZ::Vector3 m_start;
        AZ::Vector3 m_end;
        float m_radius;
    };

    //! Provides a mapping between an edge handle and the bound it represents.
    //! @note This is a cache to save computing the edge bound each time from the edge handle.
    struct EdgeBoundWithHandle
    {
        EdgeBound m_bound;
        Api::EdgeHandle m_handle;
    };

    //! Represents a vertex that can have intersection
    //! queries performed against it.
    struct VertexBound
    {
        AZ::Vector3 m_center;
        float m_radius;
    };

    //! Provides a mapping between a vertex handle and the bound it represents.
    //! @note This is a cache to save computing the vertex bound each time from the vertex handle.
    struct VertexBoundWithHandle
    {
        VertexBound m_bound;
        Api::VertexHandle m_handle;
    };

    //! Perform a ray intersection against a polygon, returning true or false if the intersection was successful.
    //! @param rayIntersectionDistance The distance from the origin to where the intersection occurs.
    //! @note The ray length is internally bounded (1000m) - this call is intended for editor functionality
    //! and anything greater than that distance away can usually safely be ignored.
    bool IntersectRayPolygon(
        const PolygonBound& polygonBound, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        float& rayIntersectionDistance, int64_t& intersectedTriangleIndex);

    //! Perform a ray intersection against an edge, returning true if the ray intersects the
    //! edge within a tolerance defined by edgeScreenWidth, or false if it does not intersect.
    //! @param rayIntersectionDistance The distance from the origin to where the intersection occurs.
    //! @param edgeScreenWidth The width of the edge in screen space (adjusted based on the
    //! view distance from the camera).
    //! @note The ray length is internally bounded (1000m) - this call is intended for editor functionality
    //! and anything greater than that distance away can usually safely be ignored.
    bool IntersectRayEdge(
        const EdgeBound& edgeBound, float edgeScreenWidth, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, float& rayIntersectionDistance);

    //! Perform a ray intersection against a vertex, returning true if the ray intersects the
    //! vertex within a tolerance defined by vertexScreenRadius, or false if it does not intersect.
    //! @param vertexScreenRadius The radius of the vertex in screen space (adjusted based on the
    //! view distance from the camera).
    //! @param rayIntersectionDistance The distance from the origin to where the intersection occurs.
    bool IntersectRayVertex(
        const VertexBound& vertexBound, float vertexScreenRadius, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, float& rayIntersectionDistance);

    //! Performs intersection for a manipulator using a polygon bound.
    class ManipulatorBoundPolygon : public AzToolsFramework::Picking::BoundShapeInterface
    {
    public:
        AZ_RTTI(
            ManipulatorBoundPolygon, "{C662AE0A-B299-485F-8BF0-C2DFBB019B80}",
            AzToolsFramework::Picking::BoundShapeInterface);

        AZ_CLASS_ALLOCATOR_DECL

        explicit ManipulatorBoundPolygon(AzToolsFramework::Picking::RegisteredBoundId boundId)
            : AzToolsFramework::Picking::BoundShapeInterface(boundId)
        {
        }

        bool IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const override;
        void SetShapeData(const AzToolsFramework::Picking::BoundRequestShapeBase& shapeData) override;

        PolygonBound m_polygonBound;
    };

    //! Implementation of BoundShapeInterfaces to create a concrete polygon bound.
    class BoundShapePolygon : public AzToolsFramework::Picking::BoundRequestShapeBase
    {
    public:
        AZ_RTTI(
            BoundShapePolygon, "{2FC93606-9E3A-47C8-A2DA-7C21ECA2190A}",
            AzToolsFramework::Picking::BoundRequestShapeBase)

        AZ_CLASS_ALLOCATOR_DECL

        AZStd::shared_ptr<AzToolsFramework::Picking::BoundShapeInterface> MakeShapeInterface(
            AzToolsFramework::Picking::RegisteredBoundId id) const override;

        AZStd::vector<AZ::Vector3> m_triangles;
    };

    //! Performs intersection for a manipulator using an edge bound.
    class ManipulatorBoundEdge : public AzToolsFramework::Picking::BoundShapeInterface
    {
    public:
        AZ_RTTI(
            ManipulatorBoundEdge, "{9CFB51B7-1631-42F4-AE92-613651A1D2F4}",
            AzToolsFramework::Picking::BoundShapeInterface)

        AZ_CLASS_ALLOCATOR_DECL

        explicit ManipulatorBoundEdge(AzToolsFramework::Picking::RegisteredBoundId boundId)
            : AzToolsFramework::Picking::BoundShapeInterface(boundId)
        {
        }

        bool IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const override;
        void SetShapeData(const AzToolsFramework::Picking::BoundRequestShapeBase& shapeData) override;

        EdgeBound m_edgeBound;
    };

    //! Implementation of BoundShapeInterfaces to create a concrete edge bound.
    class BoundShapeEdge : public AzToolsFramework::Picking::BoundRequestShapeBase
    {
    public:
        AZ_RTTI(
            BoundShapeEdge, "{7DE957A8-383D-4699-A3A1-795E345ED818}", AzToolsFramework::Picking::BoundRequestShapeBase)

        AZ_CLASS_ALLOCATOR_DECL

        AZStd::shared_ptr<AzToolsFramework::Picking::BoundShapeInterface> MakeShapeInterface(
            AzToolsFramework::Picking::RegisteredBoundId id) const override;

        AZ::Vector3 m_start;
        AZ::Vector3 m_end;
        float m_radius;
    };
} // namespace WhiteBox
