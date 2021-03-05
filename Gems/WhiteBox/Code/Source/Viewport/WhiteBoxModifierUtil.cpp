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

#include "WhiteBox_precompiled.h"

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
