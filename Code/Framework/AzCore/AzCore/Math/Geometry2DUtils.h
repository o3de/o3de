/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Geometry2DUtils
    {
        //! Computes the square of the shortest distance between a point and a line segment.
        float ShortestDistanceSqPointSegment(const Vector2& point, const Vector2& segmentStart, const Vector2& segmentEnd,
            float epsilon = 1e-4f);

        //! Computes a signed area of the triangle with vertices a, b, c.
        //! A positive result indicates a counterclockwise winding direction, negative indicates clockwise.
        //! See Real-Time Collision Detection, Christer Ericson, ISBN 978-1558607323, Chapter 5.1.9.1.
        float Signed2DTriangleArea(const Vector2& a, const Vector2& b, const Vector2& c);

        //! Computes the square of the shortest distance between two line segments.
        float ShortestDistanceSqSegmentSegment(
            const Vector2& segment1Start, const Vector2& segment1End,
            const Vector2& segment2Start, const Vector2& segment2End);

        //! Determines if a list of vertices constitute a simple polygon (none of the edges are self intersecting).
        //! Note that some definitions of simple polygon require that edges to not meet at 180 degrees, but that is not
        //! enforced here.
        //! @param vertices The vertices of the polygon, which may be in either winding order.
        //! @param epsilon If any non-consecutive edges of the polygon are closer than this value, it is considered self-intersecting.
        //! @return True if the vertices form a simple polygon.
        //! See https://en.wikipedia.org/wiki/Simple_polygon.
        bool IsSimplePolygon(const AZStd::vector<AZ::Vector2>& vertices, float epsilon = 1e-3f);

        //! Determines if every angle in a series of vertices considered in counterclockwise winding order is convex.
        //! Note that this function does not test for self-intersection, so a pentagram for example would be considered
        //! convex.  IsSimplePolygon may be used to separately test for self-intersection.
        //! @param vertices Vertices, assumed to be in counterclockwise order.
        //! @return True if all the angles are convex (but the edges may be self-intersecting).
        bool IsConvex(const AZStd::vector<AZ::Vector2>& vertices);
    } // namespace Geometry2DUtils
} // namespace AZ
