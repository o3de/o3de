/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
