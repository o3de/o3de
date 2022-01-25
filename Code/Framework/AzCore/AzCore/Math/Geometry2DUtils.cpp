/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Geometry2DUtils.h>

namespace AZ::Geometry2DUtils
{
    float ShortestDistanceSqPointSegment(const Vector2& point, const Vector2& segmentStart, const Vector2& segmentEnd,
        float epsilon)
    {
        const AZ::Vector2 segmentVector = segmentEnd - segmentStart;

        // check if the line degenerates to a point
        const float segmentLengthSq = segmentVector.GetLengthSq();
        if (segmentLengthSq < epsilon * epsilon)
        {
            return (point - segmentStart).GetLengthSq();
        }

        // if the point projects on to the line segment then the shortest distance is the perpendicular
        const float projection = (point - segmentStart).Dot(segmentVector);
        if (projection >= 0.0f && projection <= segmentLengthSq)
        {
            const Vector2 perpendicular = (point - segmentStart - projection / segmentLengthSq * segmentVector);
            return perpendicular.GetLengthSq();
        }

        // otherwise the point must be closest to one of the end points of the segment
        return GetMin(
            (point - segmentStart).GetLengthSq(),
            (point - segmentEnd).GetLengthSq());
    }

    float Signed2DTriangleArea(const Vector2& a, const Vector2& b, const Vector2& c)
    {
        return 0.5f * ((a.GetX() - c.GetX()) * (b.GetY() - c.GetY()) - (a.GetY() - c.GetY()) * (b.GetX() - c.GetX()));
    }

    float ShortestDistanceSqSegmentSegment(
        const Vector2& segment1Start, const Vector2& segment1End,
        const Vector2& segment2Start, const Vector2& segment2End)
    {
        // if the segments cross, then the distance is zero

        // if the two ends of segment 2 are on different sides of segment 1, then these two triangles will have
        // different winding orders (see Real-Time Collision Detection, Christer Ericson, ISBN 978-1558607323,
        // Chapter 5.1.9.1)
        const float area1 = Signed2DTriangleArea(segment1Start, segment1End, segment2End);
        const float area2 = Signed2DTriangleArea(segment1Start, segment1End, segment2Start);
        if (area1 * area2 < 0.0f)
        {
            // similarly we can check if the two ends of segment 1 are on different sides of segment 2
            const float area3 = Signed2DTriangleArea(segment2Start, segment2End, segment1Start);
            const float area4 = area3 + area2 - area1;
            if (area3 * area4 < 0.0f)
            {
                return 0.0f;
            }
        }

        // otherwise the shortest distance must be between one of the segment end points and the other segment
        return GetMin(
            GetMin(
                ShortestDistanceSqPointSegment(segment1Start, segment2Start, segment2End),
                ShortestDistanceSqPointSegment(segment1End, segment2Start, segment2End)),
            GetMin(
                ShortestDistanceSqPointSegment(segment2Start, segment1Start, segment1End),
                ShortestDistanceSqPointSegment(segment2End, segment1Start, segment1End))
        );
    }

    bool IsSimplePolygon(const AZStd::vector<AZ::Vector2>& vertices, float epsilon)
    {
        // note that this implementation is quadratic in the number of vertices
        // if it becomes a bottleneck, there are approaches which are O(n log n), e.g. the Bentley-Ottmann algorithm

        const size_t vertexCount = vertices.size();

        if (vertexCount < 3)
        {
            return false;
        }

        if (vertexCount == 3)
        {
            return true;
        }

        const float epsilonSq = epsilon * epsilon;

        for (size_t i = 0; i < vertexCount; ++i)
        {
            // make it easy to nicely wrap indices
            const size_t safeIndex = i + vertexCount;

            const size_t endIndex = (safeIndex - 1) % vertexCount;
            const size_t beginIndex = (safeIndex + 2) % vertexCount;

            for (size_t j = beginIndex; j != endIndex; j = (j + 1) % vertexCount)
            {
                const float distSq = ShortestDistanceSqSegmentSegment(
                    vertices[i],
                    vertices[(i + 1) % vertexCount],
                    vertices[j],
                    vertices[(j + 1) % vertexCount]
                );

                if (distSq < epsilonSq)
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool IsConvex(const AZStd::vector<AZ::Vector2>& vertices)
    {
        const size_t vertexCount = vertices.size();

        if (vertexCount < 3)
        {
            return false;
        }

        for (size_t i = 0; i < vertexCount; ++i)
        {
            if (Signed2DTriangleArea(vertices[i], vertices[(i + 1) % vertexCount], vertices[(i + 2) % vertexCount]) < 0.0f)
            {
                return false;
            }
        }

        return true;
    }
} // namespace AZ::Geometry2DUtils
