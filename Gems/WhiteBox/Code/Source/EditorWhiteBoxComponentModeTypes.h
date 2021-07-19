/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Viewport/WhiteBoxManipulatorBounds.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AZ
{
    class Color;
}

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace WhiteBox
{
    //! Structure to hold edge bounds and handles for both 'user' and 'mesh' edges.
    struct UserMeshEdgeBounds
    {
        AZStd::vector<EdgeBoundWithHandle> m_user;
        AZStd::vector<EdgeBoundWithHandle> m_mesh;
    };

    //! Structure to hold white box mesh data to do ray-casts against.
    //! @note This structure is also used for edge rendering.
    struct GeometryIntersectionData
    {
        AZStd::vector<PolygonBoundWithHandle> m_polygonBounds;
        AZStd::vector<EdgeBoundWithHandle> m_edgeBounds;
        AZStd::vector<VertexBoundWithHandle> m_vertexBounds;
    };

    //! All edges ('user' and 'mesh') to render when in edge restore mode.
    struct EdgeRenderData
    {
        UserMeshEdgeBounds m_bounds;
    };

    //! Group data use for both intersection and rendering (data for edges
    //! is used both for rendering and intersection).
    struct IntersectionAndRenderData
    {
        //! The vertices/edges/polygons created from the white box source data to perform ray-casts against.
        GeometryIntersectionData m_whiteBoxIntersectionData;
        //! All edges we might want to draw for the mesh (including both 'user' and 'mesh' edges).
        EdgeRenderData m_whiteBoxEdgeRenderData;
    };

    //! Group intersection (hit) point of ray and distance from the viewport camera.
    struct Intersection
    {
        //! The intersection point is in the local space of the Entity the White Box Component is on.
        AZ::Vector3 m_localIntersectionPoint = AZ::Vector3::CreateZero();
        float m_closestDistance = std::numeric_limits<float>::max();
    };

    //! The closest edge returned after performing a ray intersection.
    struct EdgeIntersection
    {
        EdgeBoundWithHandle m_closestEdgeWithHandle;
        Intersection m_intersection;

        Api::EdgeHandle GetHandle() const;
    };

    inline Api::EdgeHandle EdgeIntersection::GetHandle() const
    {
        return m_closestEdgeWithHandle.m_handle;
    }

    //! The closest polygon returned after performing a ray intersection.
    struct PolygonIntersection
    {
        PolygonBoundWithHandle m_closestPolygonWithHandle; //!< Polygon and corresponding handle.
        Intersection m_intersection; //!< Intersection information (distance and position).
        Api::FaceHandle m_pickedFaceHandle; //!< The individual face that was picked.

        Api::PolygonHandle GetHandle() const;
    };

    inline Api::PolygonHandle PolygonIntersection::GetHandle() const
    {
        return m_closestPolygonWithHandle.m_handle;
    }

    //! The closest vertex returned after performing a ray intersection.
    struct VertexIntersection
    {
        VertexBoundWithHandle m_closestVertexWithHandle;
        Intersection m_intersection;

        Api::VertexHandle GetHandle() const;
    };

    inline Api::VertexHandle VertexIntersection::GetHandle() const
    {
        return m_closestVertexWithHandle.m_handle;
    }

    //! Utility function to draw all edge handles in edgeBoundsWithHandle.
    //! Note: Any edges in excludedEdgeHandles will be filtered out and not drawn.
    void DrawEdges(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color,
        const AZStd::vector<EdgeBoundWithHandle>& edgeBoundsWithHandle, const Api::EdgeHandles& excludedEdgeHandles);
} // namespace WhiteBox
