/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Geometry2DUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>

namespace UnitTest
{
    using namespace AZ;
    using Geometry2DUtilsFixture = ::UnitTest::LeakDetectionFixture;

    struct Signed2DTriangleAreaTestData
    {
        const float tolerance = 1e-3f;

        const Vector2 a = Vector2(0.1f, 0.2f);
        const Vector2 b = Vector2(0.3f, 0.5f);
        const Vector2 c = Vector2(-0.2f, 0.3f);
        const Vector2 d = Vector2(-0.2f, 0.7f);
        const Vector2 e = Vector2(-0.2f, 1.0f);
    };

    TEST_F(Geometry2DUtilsFixture, Signed2DTriangleArea_CounterclockwiseTrianglesReturnCorrectPositiveArea)
    {
        Signed2DTriangleAreaTestData data;
        EXPECT_NEAR(Geometry2DUtils::Signed2DTriangleArea(data.a, data.b, data.c), 0.055f, data.tolerance);
        EXPECT_NEAR(Geometry2DUtils::Signed2DTriangleArea(data.b, data.d, data.c), 0.1f, data.tolerance);
        EXPECT_NEAR(Geometry2DUtils::Signed2DTriangleArea(data.a, data.d, data.c), 0.06f, data.tolerance);
        EXPECT_NEAR(Geometry2DUtils::Signed2DTriangleArea(data.a, data.b, data.d), 0.095f, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, Signed2DTriangleArea_ClockwiseTriangleReturnsCorrectNegativeArea)
    {
        Signed2DTriangleAreaTestData data;
        EXPECT_NEAR(Geometry2DUtils::Signed2DTriangleArea(data.a, data.c, data.b), -0.055f, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, Signed2DTriangleArea_TriangleWithTwoIdenticalPointsReturnsZeroArea)
    {
        Signed2DTriangleAreaTestData data;
        EXPECT_NEAR(Geometry2DUtils::Signed2DTriangleArea(data.a, data.b, data.b), 0.0f, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, Signed2DTriangleArea_TriangleWithThreeCollinearPointsReturnsZeroArea)
    {
        Signed2DTriangleAreaTestData data;
        EXPECT_NEAR(Geometry2DUtils::Signed2DTriangleArea(data.c, data.d, data.e), 0.0f, data.tolerance);
    }

    struct ShortestDistanceSqPointSegmentTestData
    {
        float tolerance = 1e-6f;

        const Vector2 segmentStart = Vector2(-0.4f, -0.1f);
        const Vector2 segmentEnd = Vector2(0.6f, 0.3f);
        const AZ::Vector2 point1 = AZ::Vector2(-0.8f, 0.2f);
        const AZ::Vector2 point2 = AZ::Vector2(0.72f, 0.25f);

        const float expectedDistanceSqFromOrigin = (0.06f * 0.06f) / (1.0f * 1.0f + 0.4f * 0.4f);
        const float expectedDistanceSqFromPoint1 = 0.5f * 0.5f;
        const float expectedDistanceSqFromPoint2 = 0.13f * 0.13f;
    };

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqPointSegment_PointOnLineReturnsZero)
    {
        ShortestDistanceSqPointSegmentTestData data;
        const Vector2 point(0.1f, 0.1f);
        const float distanceSq = Geometry2DUtils::ShortestDistanceSqPointSegment(point, data.segmentStart, data.segmentEnd);
        EXPECT_NEAR(distanceSq, 0.0f, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqPointSegment_PointWhichProjectsInsideSegmentReturnsOrthogonalDistanceSq)
    {
        ShortestDistanceSqPointSegmentTestData data;
        const Vector2 point = Vector2::CreateZero();
        const float distanceSq = Geometry2DUtils::ShortestDistanceSqPointSegment(point, data.segmentStart, data.segmentEnd);
        EXPECT_NEAR(distanceSq, data.expectedDistanceSqFromOrigin, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqPointSegment_ReversingSegmentStartAndEndDoesNotAffectResult)
    {
        ShortestDistanceSqPointSegmentTestData data;
        const Vector2 point = Vector2::CreateZero();
        const float distanceSq = Geometry2DUtils::ShortestDistanceSqPointSegment(point, data.segmentEnd, data.segmentStart);
        EXPECT_NEAR(distanceSq, data.expectedDistanceSqFromOrigin, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqPointSegment_PointsWhichProjectOutsideSegmentReturnCorrectDistanceSq)
    {
        ShortestDistanceSqPointSegmentTestData data;
        // this point should be closest to segmentStart
        const float distanceSq1 = Geometry2DUtils::ShortestDistanceSqPointSegment(data.point1, data.segmentStart, data.segmentEnd);
        EXPECT_NEAR(distanceSq1, data.expectedDistanceSqFromPoint1, data.tolerance);

        // this point should be closest to segmentEnd
        const float distanceSq2 = Geometry2DUtils::ShortestDistanceSqPointSegment(data.point2, data.segmentStart, data.segmentEnd);
        EXPECT_NEAR(distanceSq2, data.expectedDistanceSqFromPoint2, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqPointSegment_ExtremelyShortLineSegmentsReturnCorrectDistanceSq)
    {
        ShortestDistanceSqPointSegmentTestData data;

        // extremely short segment
        const float distanceSq1 = Geometry2DUtils::ShortestDistanceSqPointSegment(data.point1, data.segmentStart,
            data.segmentStart + AZ::Vector2(1e-5f, 1e-5f));
        EXPECT_NEAR(distanceSq1, data.expectedDistanceSqFromPoint1, data.tolerance);

        // zero length segment
        const float distanceSq2 = Geometry2DUtils::ShortestDistanceSqPointSegment(data.point1, data.segmentStart, data.segmentStart);
        EXPECT_NEAR(distanceSq2, data.expectedDistanceSqFromPoint1, data.tolerance);
    }

    struct ShortestDistanceSqSegmentSegmentTestData
    {
        const float tolerance = 1e-6f;

        const Vector2 segment1Start = Vector2(0.2f, 0.4f);
        const Vector2 segment1End = Vector2(0.44f, 0.47f);
        const Vector2 segment2Start = Vector2(0.3f, 0.3f);
        const Vector2 segment2End = Vector2(0.4f, 0.54f);
        const Vector2 segment3Start = Vector2(0.25f, 0.18f);
        const Vector2 segment3End = Vector2(0.35f, 0.42f);
        const Vector2 segment4Start = Vector2(0.15f, -0.06f);
        const Vector2 segment4End = segment3Start;
        const Vector2 segment5Start = Vector2(0.2f, 0.3f);
        const Vector2 segment5End = Vector2(0.44f, 0.37f);

        // segments 2 and 4 are collinear but do not overlap, so the shortest distance should be from the end
        // of segment 4 to the start of segment 2.
        const float expectedDistanceSqSegment2Segment4 = (0.05f * 0.05f + 0.12f * 0.12f);

        // segments 1 and 5 are parallel and their projections overlap, so the shortest distance should be
        // in the directional orthogonal to both segments
        const float expectedDistanceSqSegment1Segment5 = (0.1f * 0.1f) * (0.24f * 0.24f) / (0.25f * 0.25f);

        // the shortest distance between segment 1 and segment 3 should be between the end of segment 3 and
        // an internal point of segment 1
        const float expectedDistanceSegment1Segment3 = (0.24f / 0.25f) * (0.4f + 0.07f * 0.15f / 0.24f - 0.42f);
        const float expectedDistanceSqSegment1Segment3 = expectedDistanceSegment1Segment3 * expectedDistanceSegment1Segment3;
    };

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqSegmentSegment_IntersectingSegmentsReturnZero)
    {
        ShortestDistanceSqSegmentSegmentTestData data;
        const float distanceSq = Geometry2DUtils::ShortestDistanceSqSegmentSegment(
            data.segment1Start, data.segment1End, data.segment2Start, data.segment2End);
        EXPECT_NEAR(distanceSq, 0.0f, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqSegmentSegment_OverlappingCollinearSegmentsReturnZero)
    {
        ShortestDistanceSqSegmentSegmentTestData data;
        const float distanceSq = Geometry2DUtils::ShortestDistanceSqSegmentSegment(
            data.segment2Start, data.segment2End, data.segment3Start, data.segment3End);
        EXPECT_NEAR(distanceSq, 0.0f, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqSegmentSegment_NonOverlappingCollinearSegmentsReturnCorrectDistanceSq)
    {
        ShortestDistanceSqSegmentSegmentTestData data;
        const float distanceSq = Geometry2DUtils::ShortestDistanceSqSegmentSegment(
            data.segment2Start, data.segment2End, data.segment4Start, data.segment4End);
        EXPECT_NEAR(distanceSq, data.expectedDistanceSqSegment2Segment4, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqSegmentSegment_ParallelSegmentsWithOverlappingProjectionsReturnCorrectDistanceSq)
    {
        ShortestDistanceSqSegmentSegmentTestData data;
        const float distanceSq = Geometry2DUtils::ShortestDistanceSqSegmentSegment(
            data.segment1Start, data.segment1End, data.segment5Start, data.segment5End);
        EXPECT_NEAR(distanceSq, data.expectedDistanceSqSegment1Segment5, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqSegmentSegment_EndOfOneSegmentClosestToInternalPointOfOtherSegmentReturnsCorrectDistanceSq)
    {
        ShortestDistanceSqSegmentSegmentTestData data;
        const float distanceSq = Geometry2DUtils::ShortestDistanceSqSegmentSegment(
            data.segment1Start, data.segment1End, data.segment3Start, data.segment3End);
        EXPECT_NEAR(distanceSq, data.expectedDistanceSqSegment1Segment3, data.tolerance);
    }

    TEST_F(Geometry2DUtilsFixture, ShortestDistanceSqSegmentSegment_ReversingOrderOfSegmentsDoesNotAffectResult)
    {
        ShortestDistanceSqSegmentSegmentTestData data;

        // reverse the order of the segments
        const float distanceSq1 = Geometry2DUtils::ShortestDistanceSqSegmentSegment(
            data.segment3Start, data.segment3End, data.segment1Start, data.segment1End);
        EXPECT_NEAR(distanceSq1, data.expectedDistanceSqSegment1Segment3, data.tolerance);

        // reverse the start and end of one of the segments
        const float distanceSq2 = Geometry2DUtils::ShortestDistanceSqSegmentSegment(
            data.segment3End, data.segment3Start, data.segment1Start, data.segment1End);
        EXPECT_NEAR(distanceSq2, data.expectedDistanceSqSegment1Segment3, data.tolerance);
    }

    struct PolygonTestData
    {
        const AZStd::vector<Vector2> polygonHShape = {
            Vector2(0.0f, 0.0f),
            Vector2(0.0f, 3.0f),
            Vector2(1.0f, 3.0f),
            Vector2(1.0f, 2.0f),
            Vector2(2.0f, 2.0f),
            Vector2(2.0f, 3.0f),
            Vector2(3.0f, 3.0f),
            Vector2(3.0f, 0.0f),
            Vector2(2.0f, 0.0f),
            Vector2(2.0f, 1.0f),
            Vector2(1.0f, 1.0f),
            Vector2(1.0f, 0.0f)
        };

        const AZStd::vector<Vector2> pentagon = {
            Vector2(0.0f, 0.0f),
            Vector2(1.0f, 2.0f),
            Vector2(-1.0f, 3.0f),
            Vector2(-3.0f, 2.0f),
            Vector2(-2.0f, 0.0f)
        };

        const AZStd::vector<Vector2> pentagram = {
            Vector2(0.0f, 0.0f),
            Vector2(-1.0f, 3.0f),
            Vector2(-2.0f, 0.0f),
            Vector2(1.0f, 2.0f),
            Vector2(-3.0f, 2.0f)
        };

        // a shape with 2 saw teeth, with the notch extremely close to the long edge
        const AZStd::vector<Vector2> sawShape = {
            Vector2(0.0f, 0.0f),
            Vector2(-1.0f, 2.0f),
            Vector2(-2.0f, 1e-6f),
            Vector2(-3.0f, 2.0f),
            Vector2(-4.0f, 0.0f)
        };

        // two diamonds connected at the corners, with two extremely close vertices where the diamonds meet
        const AZStd::vector<Vector2> doubleDiamond = {
            Vector2(0.0f, 0.0f),
            Vector2(-1.0f, 1.0f),
            Vector2(-2.0f, 1e-6f),
            Vector2(-3.0f, 1.0f),
            Vector2(-4.0f, 0.0f),
            Vector2(-3.0f, -1.0f),
            Vector2(-2.0f, -1e-6f),
            Vector2(-1.0f, -1.0f)
        };
    };

    TEST_F(Geometry2DUtilsFixture, IsSimplePolygon_HShapedPolygonReturnsTrue)
    {
        PolygonTestData data;
        EXPECT_TRUE(Geometry2DUtils::IsSimplePolygon(data.polygonHShape));

        // reversing the winding order should not affect the result here
        AZStd::vector<Vector2> pointsReversed(data.polygonHShape.size());
        AZStd::reverse_copy(data.polygonHShape.begin(), data.polygonHShape.end(), pointsReversed.begin());
        EXPECT_TRUE(Geometry2DUtils::IsSimplePolygon(pointsReversed));
    }

    TEST_F(Geometry2DUtilsFixture, IsSimplePolygon_PentagonReturnsTrue)
    {
        PolygonTestData data;
        EXPECT_TRUE(Geometry2DUtils::IsSimplePolygon(data.pentagon));
    }

    TEST_F(Geometry2DUtilsFixture, IsSimplePolygon_PentagramReturnsFalse)
    {
        PolygonTestData data;
        EXPECT_FALSE(Geometry2DUtils::IsSimplePolygon(data.pentagram));
    }

    TEST_F(Geometry2DUtilsFixture, IsSimplePolygon_AlmostIntersectingPolygonReturnsFalse)
    {
        PolygonTestData data;
        EXPECT_FALSE(Geometry2DUtils::IsSimplePolygon(data.sawShape));
    }

    TEST_F(Geometry2DUtilsFixture, IsSimplePolygon_PolygonWithTwoExtremelyCloseVerticesReturnsFalse)
    {
        PolygonTestData data;
        EXPECT_FALSE(Geometry2DUtils::IsSimplePolygon(data.doubleDiamond));
    }

    TEST_F(Geometry2DUtilsFixture, IsConvex_HShapedPolygonReturnsFalse)
    {
        PolygonTestData data;
        EXPECT_FALSE(Geometry2DUtils::IsConvex(data.polygonHShape));
    }

    TEST_F(Geometry2DUtilsFixture, IsConvex_PentagonReturnsTrue)
    {
        PolygonTestData data;
        EXPECT_TRUE(Geometry2DUtils::IsConvex(data.pentagon));
    }

    TEST_F(Geometry2DUtilsFixture, IsConvex_PentagramReturnsTrue)
    {
        PolygonTestData data;
        EXPECT_TRUE(Geometry2DUtils::IsConvex(data.pentagram));
    }

    TEST_F(Geometry2DUtilsFixture, IsConvex_PolygonWithNotchReturnsFalse)
    {
        PolygonTestData data;
        EXPECT_FALSE(Geometry2DUtils::IsConvex(data.sawShape));
    }
} // namespace UnitTest
