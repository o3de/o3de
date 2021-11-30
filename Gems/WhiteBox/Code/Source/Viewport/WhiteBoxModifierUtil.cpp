/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentModeTypes.h"
#include "WhiteBoxModifierUtil.h"

#include <AzCore/std/sort.h>

namespace WhiteBox
{
    template<typename Intersection>
    static float IntersectionDistance(const AZStd::optional<Intersection>& intersection)
    {
        return intersection.has_value() ? intersection->m_intersection.m_closestDistance
                                        : std::numeric_limits<float>::max();
    }

    GeometryIntersection FindClosestGeometryIntersection(
        const AZStd::optional<EdgeIntersection>& edgeIntersection,
        const AZStd::optional<PolygonIntersection>& polygonIntersection,
        const AZStd::optional<VertexIntersection>& vertexIntersection)
    {
        if (!edgeIntersection.has_value() && !polygonIntersection.has_value() && !vertexIntersection.has_value())
        {
            return GeometryIntersection::None;
        }

        // simple wrapper type to help sort values
        struct IntersectionType
        {
            float m_distance;
            GeometryIntersection m_type;
        };

        AZStd::array<IntersectionType, 3> intersections{
            IntersectionType{IntersectionDistance(edgeIntersection), GeometryIntersection::Edge},
            IntersectionType{IntersectionDistance(polygonIntersection), GeometryIntersection::Polygon},
            IntersectionType{IntersectionDistance(vertexIntersection), GeometryIntersection::Vertex}};

        AZStd::sort(
            AZStd::begin(intersections), AZStd::end(intersections),
            [](const IntersectionType& lhs, const IntersectionType& rhs)
            {
                return lhs.m_distance < rhs.m_distance;
            });

        return intersections.front().m_type;
    }
} // namespace WhiteBox
