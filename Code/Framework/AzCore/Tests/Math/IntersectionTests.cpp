/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Intersection, ClosestSegmentSegment)
    {
        // line2 right and above line1 (no overlap, parallel)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 0.0f, 0.0f);
            Vector3 line2Start(7.0f, 0.0f, 4.0f);
            Vector3 line2End(10.0f, 0.0f, 4.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 5.0f);
            EXPECT_TRUE(line1Proportion == 1.0f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // line2 halfway over the top of the line1 (overlap, parallel)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 0.0f, 0.0f);
            Vector3 line2Start(2.0f, 0.0f, 3.0f);
            Vector3 line2End(6.0f, 0.0f, 3.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 3.0f);
            EXPECT_TRUE(line1Proportion == 0.5f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // line2 over the top of the line1 (inside, parallel)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(8.0f, 0.0f, 0.0f);
            Vector3 line2Start(2.0f, 0.0f, 3.0f);
            Vector3 line2End(6.0f, 0.0f, 3.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 3.0f);
            EXPECT_TRUE(line1Proportion == 0.25f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // line2 over the top of the line1 (overlap, skew (cross))
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(8.0f, 0.0f, 0.0f);
            Vector3 line2Start(4.0f, 4.0f, 4.0f);
            Vector3 line2End(4.0f, -4.0f, 4.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 4.0f);
            EXPECT_TRUE(line1Proportion == 0.5f);
            EXPECT_TRUE(line2Proportion == 0.5f);
        }

        // line2 flat diagonal to line1 (no overlap, skew)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 4.0f, 0.0f);
            Vector3 line2Start(10.0f, 0.0f, 0.0f);
            Vector3 line2End(6.0f, 4.0f, 0.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 2.0f);
            EXPECT_TRUE(line1Proportion == 1.0f);
            EXPECT_TRUE(line2Proportion == 1.0f);
        }

        // line2 perpendicular to line1 (skew, no overlap)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 0.0f, 0.0f);
            Vector3 line2Start(2.0f, 1.0f, 0.0f);
            Vector3 line2End(2.0f, 4.0f, 0.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 1.0f);
            EXPECT_TRUE(line1Proportion == 0.5f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // line 1 degenerates to point
        {
            Vector3 line1Start(4.0f, 2.0f, 0.0f);
            Vector3 line1End(4.0f, 2.0f, 0.0f);
            Vector3 line2Start(2.0f, 0.0f, 0.0f);
            Vector3 line2End(2.0f, 4.0f, 0.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 2.0f);
            EXPECT_TRUE(line1Proportion == 0.0f);
            EXPECT_TRUE(line2Proportion == 0.5f);
        }

        // line 2 degenerates to point
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 0.0f, 0.0f);
            Vector3 line2Start(2.0f, 1.0f, 0.0f);
            Vector3 line2End(2.0f, 1.0f, 0.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 1.0f);
            EXPECT_TRUE(line1Proportion == 0.5f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // both lines degenerate to points
        {
            Vector3 line1Start(5.0f, 5.0f, 5.0f);
            Vector3 line1End(5.0f, 5.0f, 5.0f);
            Vector3 line2Start(10.0f, 10.0f, 10.0f);
            Vector3 line2End(10.0f, 10.0f, 10.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            float line1Proportion;
            float line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            // (10, 10, 10) - (5, 5, 5) == (5, 5, 5)
            // |(5,5,5)| == sqrt(5*5+5*5+5*5) == sqrt(75)
            EXPECT_TRUE(pointDifference == sqrtf(75.0f));
            EXPECT_TRUE(line1Proportion == 0.0f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }
    }

    TEST(MATH_Intersection, ClosestPointSegment)
    {
        // point above center of line
        {
            Vector3 lineStart(0.0f, 0.0f, 0.0f);
            Vector3 lineEnd(4.0f, 0.0f, 0.0f);
            Vector3 point(2.0f, 2.0f, 0.0f);

            Vector3 lineClosestPoint;
            float lineProportion;
            Intersect::ClosestPointSegment(point, lineStart, lineEnd, lineProportion, lineClosestPoint);

            float pointDifference = (lineClosestPoint - point).GetLength();

            EXPECT_TRUE(pointDifference == 2.0f);
            EXPECT_TRUE(lineProportion == 0.5f);
        }

        // point same height behind line
        {
            Vector3 lineStart(0.0f, 0.0f, 0.0f);
            Vector3 lineEnd(0.0f, 4.0f, 0.0f);
            Vector3 point(0.0f, -2.0f, 0.0f);

            Vector3 lineClosestPoint;
            float lineProportion;
            Intersect::ClosestPointSegment(point, lineStart, lineEnd, lineProportion, lineClosestPoint);

            float pointDifference = (lineClosestPoint - point).GetLength();

            EXPECT_TRUE(pointDifference == 2.0f);
            EXPECT_TRUE(lineProportion == 0.0f);
        }

        // point passed end of line
        {
            Vector3 lineStart(0.0f, 0.0f, 0.0f);
            Vector3 lineEnd(0.0f, 0.0f, 10.0f);
            Vector3 point(0.0f, 0.0f, 15.0f);

            Vector3 lineClosestPoint;
            float lineProportion;
            Intersect::ClosestPointSegment(point, lineStart, lineEnd, lineProportion, lineClosestPoint);

            float pointDifference = (lineClosestPoint - point).GetLength();

            EXPECT_TRUE(pointDifference == 5.0f);
            EXPECT_TRUE(lineProportion == 1.0f);
        }

        // point above part way along line
        {
            Vector3 lineStart(0.0f, 0.0f, 0.0f);
            Vector3 lineEnd(4.0f, 4.0f, 0.0f);
            Vector3 point(3.0f, 3.0f, -1.0f);

            Vector3 lineClosestPoint;
            float lineProportion;
            Intersect::ClosestPointSegment(point, lineStart, lineEnd, lineProportion, lineClosestPoint);

            float pointDifference = (lineClosestPoint - point).GetLength();

            EXPECT_TRUE(pointDifference == 1.0f);
            EXPECT_TRUE(lineProportion == 0.75f);
        }
    }

    class MATH_IntersectRayCappedCylinderTest
        : public AllocatorsFixture
    {
    protected:

        void SetUp() override
        {
            m_cylinderEnd1 = Vector3(1.1f, 2.2f, 3.3f);
            m_cylinderDir = Vector3(1.0, 1.0f, 1.0f);
            m_cylinderDir.Normalize();
            m_radiusDir = Vector3(-1.0f, 1.0f, 0.0f);
            m_radiusDir.Normalize();
            m_cylinderHeight = 5.5f;
            m_cylinderRadius = 3.768f;
        }

        void TearDown() override
        {

        }

        Vector3 m_cylinderEnd1;
        Vector3 m_cylinderDir;
        Vector3 m_radiusDir;
        float m_cylinderHeight;
        float m_cylinderRadius;
    };

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOverlapCylinderAxis)
    {
        Vector3 rayOrigin = m_cylinderEnd1 - 0.5f * m_cylinderHeight * m_cylinderDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayParallelToAndInsideCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 - 0.5f * m_cylinderHeight * m_cylinderDir + 0.5f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayAlongCylinderSuface)
    {
        Vector3 rayOrigin = m_cylinderEnd1 - 0.5f * m_cylinderHeight * m_cylinderDir + m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayParallelAndRayOriginInsideCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.2f * m_cylinderHeight * m_cylinderDir + 0.5f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayParallelToButOutsideCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 - 0.5f * m_cylinderHeight * m_cylinderDir + 1.1f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOuside_RayDirParallelButPointingAwayCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 1.5f * m_cylinderHeight * m_cylinderDir + 0.5f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        rayDir.Normalize();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideEnds_BothIntersectionsInbetweenEnds)
    {
        Vector3 intersection2 = m_cylinderEnd1 + 0.8f * m_cylinderHeight * m_cylinderDir - m_cylinderRadius * m_radiusDir;
        Vector3 intersection1 = m_cylinderEnd1 + 0.2f * m_cylinderHeight * m_cylinderDir + m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = intersection2 - intersection1;
        Vector3 rayOrigin = intersection1 - 2.0f * rayDir;
        rayDir.Normalize();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideEnds_Intersection1OnEnd1_Intersection2InbetweenEnds)
    {
        Vector3 intersection2 = m_cylinderEnd1 + 0.6f * m_cylinderHeight * m_cylinderDir - m_cylinderRadius * m_radiusDir;
        Vector3 intersection1 = m_cylinderEnd1 + 0.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = intersection2 - intersection1;
        Vector3 rayOrigin = intersection1 - 2.0f * rayDir;
        rayDir.Normalize();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginInside_RayGoThroughCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.3f * m_cylinderHeight * m_cylinderDir - 0.4f * m_cylinderRadius * m_radiusDir;
        Vector3 intersection = m_cylinderEnd1 + 0.8f * m_cylinderHeight * m_cylinderDir + m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = (intersection - rayOrigin).GetNormalized();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirPointingAwayCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.4f * m_cylinderHeight * m_cylinderDir - 1.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = rayOrigin - m_cylinderEnd1;
        rayDir.Normalize();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirMissingCylinderEnds)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.4f * m_cylinderHeight * m_cylinderDir - 1.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = (m_cylinderEnd1 + m_cylinderHeight * m_cylinderDir - 1.2f * m_cylinderRadius * m_radiusDir) - rayOrigin;
        rayDir.Normalize();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirShootingOutEnd1)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.4f * m_cylinderHeight * m_cylinderDir - 1.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = (m_cylinderEnd1 - 0.2f * m_cylinderRadius * m_radiusDir) - rayOrigin;
        rayDir.Normalize();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirShootingOutEnd2)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.4f * m_cylinderHeight * m_cylinderDir - 1.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = (m_cylinderEnd1 + 0.67f * m_cylinderHeight * m_cylinderDir) - rayOrigin;
        rayDir.Normalize();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirShootingOutCylinder)
    {
        Vector3 perpDir(1.0f, 0, 0);
        float parallelDiff = 0.453f;
        float perpendicularDiff = 2.54f;
        Vector3 rayOrigin = (m_cylinderEnd1 + parallelDiff * m_cylinderHeight * m_cylinderDir) + perpendicularDiff * m_cylinderRadius * perpDir;
        Vector3 rayDir = (m_cylinderEnd1 + m_cylinderHeight * m_cylinderDir - 2.6f * m_cylinderRadius * perpDir) - rayOrigin;
        rayDir.Normalize();
        float t1 = AZ::Constants::FloatMax;
        float t2 = AZ::Constants::FloatMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    class MATH_IntersectRayConeTest
        : public AllocatorsFixture
    {
    protected:

        void SetUp() override
        {
            m_coneApex = Vector3(1.264f, 6.773f, 9.612f);
            m_coneDir = Vector3(-64.0f, -82.783f, -12.97f);
            m_radiusDir = Vector3(m_coneDir.GetY(), -m_coneDir.GetX(), 0.0f);

            m_coneDir.Normalize();
            m_coneHeight = 1.643f;
            m_coneRadius = m_coneHeight;

            m_radiusDir.Normalize();

            m_tangentDir = m_coneHeight * m_coneDir + m_coneRadius * m_radiusDir;
            m_tangentDir.Normalize();
        }

        void TearDown() override
        {
        }

        Vector3 m_coneApex;
        Vector3 m_coneDir;
        Vector3 m_radiusDir;
        Vector3 m_tangentDir; // direction of tangent line on the surface from the apex to the base
        float m_coneHeight;
        float m_coneRadius;
    };

    TEST_F(MATH_IntersectRayConeTest, RayOriginAtApex_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeBelowApex_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 0.5f * m_coneHeight * m_coneDir - m_coneRadius * m_radiusDir;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeBelowBase_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 1.5f * m_coneHeight * m_coneDir - m_coneRadius * m_radiusDir;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeAboveApex_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex - 1.5f * m_coneHeight * m_coneDir - m_coneRadius * m_radiusDir;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginInsideCone_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 0.5f * m_coneHeight * m_coneDir;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginInsideCone_RayDirParallelConeSurfaceOppositeDirection)
    {
        Vector3 rayOrigin = m_coneApex + 0.5f * m_coneHeight * m_coneDir;
        Vector3 rayDir = -m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideCone_RayDirThroughApex)
    {
        Vector3 rayOrigin = m_coneApex + 0.03f * m_coneDir + m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex - rayOrigin).GetNormalized();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginInsideMirrorCone_RayDirThroughApex)
    {
        Vector3 rayOrigin = m_coneApex - 0.5f * m_coneHeight * m_coneDir + 0.25f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex - rayOrigin).GetNormalized();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

#if AZ_TRAIT_DISABLE_FAILED_MATH_TESTS
    TEST_F(MATH_IntersectRayConeTest, DISABLED_RayOriginOutsideBase_RayDirThroughApex)
#else
    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideBase_RayDirThroughApex)
#endif // AZ_TRAIT_DISABLE_FAILED_MATH_TESTS
    {
        Vector3 rayOrigin = m_coneApex + 1.3f * m_coneHeight * m_coneDir + 0.3f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex - rayOrigin).GetNormalized();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideBase_RayDirThroughConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 1.3f * m_coneHeight * m_coneDir + 0.3f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex - 0.3f * m_coneRadius * m_radiusDir - rayOrigin).GetNormalized();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeApexSide_RayDirThroughConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 1.5f * m_coneHeight * m_coneDir + 0.7f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex + 0.7f * m_coneHeight * m_coneDir - 0.8f * m_coneRadius * m_radiusDir - rayOrigin).GetNormalized();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeApexSide_RayDirMissCone)
    {
        Vector3 rayOrigin = m_coneApex - 0.5f * m_coneHeight * m_coneDir + 0.7f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex + 1.1f * m_coneHeight * m_coneDir + 1.1f * m_coneRadius * m_radiusDir - rayOrigin).GetNormalized();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginInsideMirrorConeApexSide_RayDirMissCone)
    {
        Vector3 rayOrigin = m_coneApex - 0.7f * m_coneHeight * m_coneDir + 0.6f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex + 1.5f * m_coneHeight * m_coneDir + 1.5f * m_coneRadius * m_radiusDir - rayOrigin).GetNormalized();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    // replicates a scenario in the Editor using a cone and a pick ray which should have failed but passed
    // note: To replicate this, select an entity so the default translation manipulator appears, move very close to the
    // entity, hover the mouse over one of the manipulator linear manipulator arrows (cone part) and move the mouse away
    // notice the manipulator will remain highlighted as a successful intersection is still reported
    TEST(MATH_IntersectRayConeTestEditor, DISABLED_RayConeEditorScenarioTest)
    {
        auto rayOrigin = Vector3(0.0f, -0.808944702f, 0.0f);
        auto rayDir = Vector3(0.301363617f, 0.939044654f, 0.165454566f);
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(
            rayOrigin, rayDir, AZ::Vector3(0.0f, 0.0f, 0.161788940f), AZ::Vector3(0.0f, 0.0f, -1.0f), 0.0453009047, 0.0113252262, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    class MATH_IntersectRayQuadTest
        : public AllocatorsFixture
    {
    protected:

        void SetUp() override
        {
            m_vertexA = Vector3(1.04f, 2.46f, 5.26f);

            m_axisB = Vector3(1.0f, 2.0f, 3.0f);
            m_axisD = Vector3(3.0f, -2.0f, 1.0f);
            m_axisB.Normalize();
            m_axisD.Normalize();
            m_lengthAxisB = 4.56f;
            m_lengthAxisD = 7.19f;
            m_normal = m_axisB.Cross(m_axisD);
            m_normal.Normalize();

            m_vertexB = m_vertexA + m_lengthAxisB * m_axisB;
            m_vertexC = m_vertexA + m_lengthAxisB * m_axisB + m_lengthAxisD * m_axisD;
            m_vertexD = m_vertexA + m_lengthAxisD * m_axisD;
        }

        void TearDown() override
        {

        }

        Vector3 m_vertexA;
        Vector3 m_vertexB;
        Vector3 m_vertexC;
        Vector3 m_vertexD;

        // two axes defining the quad plane, originating from m_vertexA
        Vector3 m_axisB;
        Vector3 m_axisD;
        float m_lengthAxisB;
        float m_lengthAxisD;

        Vector3 m_normal;
    };

    TEST_F(MATH_IntersectRayQuadTest, RayShootingAway_CCW)
    {
        Vector3 rayOrigin = (m_vertexA + 0.23f * m_axisB + 0.75f * m_axisD) + 2.0f * m_normal;
        Vector3 rayDir = m_normal;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayShootingAway_CW)
    {
        Vector3 rayOrigin = (m_vertexA + 0.23f * m_axisB + 0.75f * m_axisD) + 2.0f * m_normal;
        Vector3 rayDir = m_normal;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexC, m_vertexB, m_vertexA, m_vertexD, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectTriangleABC_CCW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = ((0.5f * (0.5f * m_vertexA + 0.5f * m_vertexC)) + 0.5f * m_vertexB) - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectTriangleABC_CW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = ((0.5f * (0.5f * m_vertexA + 0.5f * m_vertexC)) + 0.5f * m_vertexB) - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexD, m_vertexC, m_vertexB, m_vertexA, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectTriangleACD_CCW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = ((0.5f * (0.5f * m_vertexA + 0.5f * m_vertexC)) + 0.5f * m_vertexD) - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectTriangleACD_CW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = ((0.5f * (0.5f * m_vertexA + 0.5f * m_vertexC)) + 0.5f * m_vertexD) - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexD, m_vertexC, m_vertexB, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectLineAC_CCW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = (0.5f * m_vertexA + 0.5f * m_vertexC) - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayShootOverAB_CCW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = m_vertexA + 1.7f * m_lengthAxisB * m_axisB - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayShootOverAC_CW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = m_vertexA + 1.3f * (m_vertexD - m_vertexA) - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexD, m_vertexC, m_vertexB, t);
        EXPECT_EQ(hit, 0);
    }

    class MATH_IntersectRayBoxTest
        : public AllocatorsFixture
    {
    protected:

        void SetUp() override
        {
            m_boxCenter = Vector3(1.234f, 2.345f, 9.824f);
            m_boxAxis1 = Vector3(1.0f, 2.0f, 3.0f);
            m_boxAxis2 = Vector3(-3.0f, 2.0f, -1.0f);
            m_boxAxis3 = m_boxAxis1.Cross(m_boxAxis2);
            m_boxAxis1.Normalize();
            m_boxAxis2.Normalize();
            m_boxAxis3.Normalize();
            m_boxHalfExtentAxis1 = 4.775f;
            m_boxHalfExtentAxis2 = 8.035f;
            m_boxHalfExtentAxis3 = 14.007f;
        }

        void TearDown() override
        {

        }

        Vector3 m_boxCenter;
        Vector3 m_boxAxis1;
        Vector3 m_boxAxis2;
        Vector3 m_boxAxis3;
        float m_boxHalfExtentAxis1;
        float m_boxHalfExtentAxis2;
        float m_boxHalfExtentAxis3;
    };

    TEST_F(MATH_IntersectRayBoxTest, RayOriginOutside_HitBoxAxis1Side)
    {
        Vector3 rayOrigin = m_boxCenter + 2.0f * m_boxHalfExtentAxis1 * m_boxAxis1 + 2.0f * m_boxHalfExtentAxis2 * m_boxAxis2;
        Vector3 rayDir = m_boxCenter + m_boxHalfExtentAxis1 * m_boxAxis1 - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayBox(rayOrigin, rayDir, m_boxCenter, m_boxAxis1, m_boxAxis2, m_boxAxis3,
            m_boxHalfExtentAxis1, m_boxHalfExtentAxis2, m_boxHalfExtentAxis3, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayBoxTest, RayOriginOutside_ShootAwayFromBox)
    {
        Vector3 rayOrigin = m_boxCenter + 2.0f * m_boxHalfExtentAxis3 * m_boxAxis3 + 2.0f * m_boxHalfExtentAxis2 * m_boxAxis2;
        Vector3 rayDir = rayOrigin - m_boxCenter + m_boxHalfExtentAxis3 * m_boxAxis3;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayBox(rayOrigin, rayDir, m_boxCenter, m_boxAxis1, m_boxAxis2, m_boxAxis3,
            m_boxHalfExtentAxis1, m_boxHalfExtentAxis2, m_boxHalfExtentAxis3, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayBoxTest, RayOriginOutside_RayParallelToAxis2MissBox)
    {
        Vector3 rayOrigin = m_boxCenter + 2.0f * m_boxHalfExtentAxis3 * m_boxAxis3 + 2.0f * m_boxHalfExtentAxis2 * m_boxAxis2;
        Vector3 rayDir = m_boxAxis2;
        float t = 0.0f;
        int hit = Intersect::IntersectRayBox(rayOrigin, rayDir, m_boxCenter, m_boxAxis1, m_boxAxis2, m_boxAxis3,
            m_boxHalfExtentAxis1, m_boxHalfExtentAxis2, m_boxHalfExtentAxis3, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayBoxTest, RayOriginInside_ShootToConner)
    {
        Vector3 rayOrigin = m_boxCenter + 0.5f * m_boxHalfExtentAxis1 * m_boxAxis1 +
            0.5f * m_boxHalfExtentAxis2 * m_boxAxis2 + 0.5f * m_boxHalfExtentAxis3 * m_boxAxis3;
        Vector3 rayDir = (m_boxCenter - m_boxHalfExtentAxis1 * m_boxAxis1 -
            m_boxHalfExtentAxis2 * m_boxAxis2 - m_boxHalfExtentAxis3 * m_boxAxis3) - rayOrigin;
        rayDir.Normalize();
        float t = 0.0f;
        int hit = Intersect::IntersectRayBox(rayOrigin, rayDir, m_boxCenter, m_boxAxis1, m_boxAxis2, m_boxAxis3,
            m_boxHalfExtentAxis1, m_boxHalfExtentAxis2, m_boxHalfExtentAxis3, t);
        EXPECT_EQ(hit, 1);
    }

    class MATH_IntersectRayPolyhedronTest
        : public AllocatorsFixture
    {
    protected:
        void SetUp() override
        {
            // base of cube
            m_vertices[0] = Vector3(0.0f, 0.0f, 0.0f);
            m_vertices[1] = Vector3(10.0f, 0.0f, 0.0f);
            m_vertices[2] = Vector3(10.0f, 10.0f, 0.0f);
            m_vertices[3] = Vector3(0.0f, 10.0f, 0.0f);

            // setup planes
            for (size_t i = 0; i < 4; ++i)
            {
                const Vector3 start = m_vertices[i];
                const Vector3 end = m_vertices[(i + 1) % 4];
                const Vector3 top = start + Vector3::CreateAxisZ();

                const Vector3 normal = (end - start).Cross(top - start).GetNormalizedSafe();

                m_planes[i] = Plane::CreateFromNormalAndPoint(normal, start);
            }

            const Vector3 normalTop =
                (m_vertices[2] - m_vertices[0]).Cross(m_vertices[0] - m_vertices[1]).GetNormalizedSafe();
            const Vector3 normalBottom = -normalTop;

            const float height = 10.0f;
            m_planes[4] = Plane::CreateFromNormalAndPoint(normalTop, m_vertices[0] + Vector3::CreateAxisZ(height));
            m_planes[5] = Plane::CreateFromNormalAndPoint(normalBottom, m_vertices[0]);
        }

        void TearDown() override
        {
        }

        Vector3 m_vertices[4];
        Plane m_planes[6];
    };

    TEST_F(MATH_IntersectRayPolyhedronTest, RayParallelHit)
    {
        const Vector3 src = Vector3(0.0f, -1.0f, 1.0f);
        const Vector3 dir = Vector3(0.0f, 1.0f, 0.0f);
        const Vector3 end = (src + dir * 100.0f) - src;

        float f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 1);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayAboveMiss)
    {
        const Vector3 src = Vector3(5.0f, 11.0f, 11.0f);
        const Vector3 dir = Vector3(0.0f, -1.0f, 0.0f);
        const Vector3 end = (src + dir * 100.0f) - src;

        float f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 0);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayDiagonalDownHit)
    {
        const Vector3 src = Vector3(5.0f, -1.0f, 11.0f);
        const Vector3 end = Vector3(5.0f, 11.0f, -11.0f);

        float f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 1);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayDiagonalAcrossHit)
    {
        const Vector3 src = Vector3(-5.0f, -5.0f, 5.0f);
        const Vector3 end = Vector3(15.0f, 15.0f, 5.0f);

        float f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 1);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayDiagonalAcrossMiss)
    {
        const Vector3 src = Vector3(-5.0f, -15.0f, 5.0f);
        const Vector3 end = Vector3(15.0f, 5.0f, 5.0f);

        float f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 0);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayStartInside)
    {
        const Vector3 src = Vector3(5.0f, 5.0f, 5.0f);
        const Vector3 end = Vector3(5.0f, 5.0f, 5.0f);

        float f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 0);
    }
}
