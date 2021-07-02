/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace WhiteBox
{
    //! Enumerate the current append state for new vertices.
    enum class AppendStage
    {
        None, //!< No vertices currently being appended.
        Initiated, //!< An append action has started.
        Complete, //!< An append action has finished.
    };

    //! The type of intersection detected when interacting
    //! with a white box mesh in the viewport.
    enum class GeometryIntersection
    {
        Edge,
        Polygon,
        Vertex,

        None
    };

    struct EdgeIntersection;
    struct PolygonIntersection;
    struct VertexIntersection;

    //! Return the closest intersection out of the three different kinds
    //! (vertex, edge or polygon).
    GeometryIntersection FindClosestGeometryIntersection(
        const AZStd::optional<EdgeIntersection>& edgeIntersection,
        const AZStd::optional<PolygonIntersection>& polygonIntersection,
        const AZStd::optional<VertexIntersection>& vertexIntersection);
} // namespace WhiteBox
