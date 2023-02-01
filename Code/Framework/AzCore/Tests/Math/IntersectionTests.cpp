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
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Geometry3DUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <random>
#include <Tests/Math/IntersectionTestHelpers.h>

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

            EXPECT_NEAR(pointDifference, 5.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 1.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 0.0f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 3.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 0.5f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 0.0f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 3.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 0.25f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 0.0f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 4.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 0.5f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 0.5f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 2.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 1.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 1.0f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 1.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 0.5f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 0.0f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 2.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 0.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 0.5f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 1.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 0.5f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 0.0f, AZ::Constants::Tolerance);
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
            EXPECT_NEAR(pointDifference, sqrtf(75.0f), AZ::Constants::Tolerance);
            EXPECT_NEAR(line1Proportion, 0.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(line2Proportion, 0.0f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 2.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(lineProportion, 0.5f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 2.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(lineProportion, 0.0f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 5.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(lineProportion, 1.0f, AZ::Constants::Tolerance);
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

            EXPECT_NEAR(pointDifference, 1.0f, AZ::Constants::Tolerance);
            EXPECT_NEAR(lineProportion, 0.75f, AZ::Constants::Tolerance);
        }
    }

    class MATH_IntersectSegmentTriangleTest : public LeakDetectionFixture
    {
        public:

        void ClearResultsOnMiss(bool didHit, AZ::Vector3& outNormal, float& outDistance)
        {
            if (!didHit)
            {
                outDistance = 0.0f;
                outNormal = AZ::Vector3::CreateZero();
            }
        }

    };

    TEST_F(MATH_IntersectSegmentTriangleTest, SegmentThroughSphereCenterHasCorrectIntersectionCount)
    {
        // This verifies the general correctness of the segment/triangle intersections by generating a mesh sphere with ~20k triangles
        // and a large quantity of segments that start at random locations outside the sphere and pass directly through the center of
        // the sphere to end at the same location on the opposite side of the sphere.
        // Each segment should have exactly one triangle collision (the sphere entry point) for one-sided tests, and two triangle
        // collisions (the sphere entry and exit points) for two-sided tests.
        // Segment/triangle algorithms that aren't "watertight" (such as Arensen and Moller Trumbore) will fail this test
        // when run with a high enough quantity of generated segments.

        // Generate a ~20k triangle sphere of unit size centered at the origin.
        constexpr uint8_t SubdivisionDepth = 5;
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        // Create random number generators in the positive 2-50 range so that we're outside the sphere bounds.
        constexpr unsigned int Seed = 1;
        std::mt19937_64 rng(Seed);
        std::uniform_real_distribution<float> unif(2.0f, 50.0f);

        // When getting the random number, return numbers in the [-50, -2] or [2, 50] ranges so that we cover both negative and positive
        // positional values, but they're all outside the sphere bounds.
        auto GetRandomPosition = [&rng, &unif]() -> float
        {
            float num = unif(rng);
            return (rand() & 0x01) ? num : (num * -1.0f);
        };

        // This is enough iterations to generate a failing result on both the Arenberg and the Moller Trumbore algorithms.
        // (Specific triangles causing their failures have been captured and tested in a separate regression unit test)
        constexpr int NumTests = 2500;

        // We'll keep track of the specific segment/triangle information for any hit so that it's easier to add false negatives
        // and false positives to the regression unit test.
        AZStd::vector<float> hitDistanceCCW;
        AZStd::vector<float> hitDistance;
        AZStd::vector<AZ::Vector3> hitNormalCCW;
        AZStd::vector<AZ::Vector3> hitNormal;
        AZStd::vector<AZ::Vector3> vertsForFalseResultCCW;
        AZStd::vector<AZ::Vector3> vertsForFalseResult;

        for (int test = 0; test < NumTests; test++)
        {
            // Generate a random ray segment that crosses through the center of the sphere and out the other side.
            AZ::Vector3 rayStart(GetRandomPosition(), GetRandomPosition(), GetRandomPosition());
            AZ::Vector3 rayEnd = -rayStart;

            AZ::Intersect::SegmentTriangleHitTester hitTester(rayStart, rayEnd);

            float outDistance = 0.0f;
            AZ::Vector3 outNormal = AZ::Vector3::CreateZero();

            hitDistanceCCW.clear();
            hitDistance.clear();
            hitNormalCCW.clear();
            hitNormal.clear();
            vertsForFalseResultCCW.clear();
            vertsForFalseResult.clear();

            // Run through all the triangles and look for hits. For each segment that we test, there should be exactly
            // one hit for one-sided tests (sphere entry), and two hits for two-sided tests (sphere entry and exit).
            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                // One-sided triangle hit test
                if (hitTester.IntersectSegmentTriangleCCW(
                        sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2], outNormal, outDistance))
                {
                    hitDistanceCCW.emplace_back(outDistance);
                    hitNormalCCW.emplace_back(outNormal);
                    vertsForFalseResultCCW.emplace_back(sphereGeometry[triangle]);
                    vertsForFalseResultCCW.emplace_back(sphereGeometry[triangle + 1]);
                    vertsForFalseResultCCW.emplace_back(sphereGeometry[triangle + 2]);
                }

                // Two-sided triangle hit test
                if (hitTester.IntersectSegmentTriangle(
                        sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2], outNormal, outDistance))
                {
                    hitDistance.emplace_back(outDistance);
                    hitNormal.emplace_back(outNormal);
                    vertsForFalseResult.emplace_back(sphereGeometry[triangle]);
                    vertsForFalseResult.emplace_back(sphereGeometry[triangle + 1]);
                    vertsForFalseResult.emplace_back(sphereGeometry[triangle + 2]);
                }
            }

            // Check results of one-sided test
            if (hitDistanceCCW.size() < 1)
            {
                EXPECT_EQ(hitDistanceCCW.size(), 1)
                    << "One-sided test: False negative, no triangles hit by segment (" << rayStart.GetX() << ", " << rayStart.GetY()
                    << ", " << rayStart.GetZ() << ") - (" << rayEnd.GetX() << ", " << rayEnd.GetY() << ", " << rayEnd.GetZ() << ")\n";
            }
            else if (hitDistanceCCW.size() > 1)
            {
                EXPECT_EQ(hitDistanceCCW.size(), 1)
                    << "One-sided test: False positive, too many triangles hit by segment (" << rayStart.GetX() << ", " << rayStart.GetY()
                    << ", " << rayStart.GetZ() << ") - (" << rayEnd.GetX() << ", " << rayEnd.GetY() << ", " << rayEnd.GetZ() << ")\n";
            }

            // Check results of two-sided test
            if (hitDistance.size() < 2)
            {
                EXPECT_EQ(hitDistance.size(), 2)
                    << "Two-sided test: False negative, too few triangles hit by segment (" << rayStart.GetX() << ", " << rayStart.GetY()
                    << ", " << rayStart.GetZ() << ") - (" << rayEnd.GetX() << ", " << rayEnd.GetY() << ", " << rayEnd.GetZ() << ")\n";
            }
            else if (hitDistance.size() > 2)
            {
                EXPECT_EQ(hitDistance.size(), 2)
                    << "Two-sided test: False positive, too many triangles hit by segment (" << rayStart.GetX() << ", " << rayStart.GetY()
                    << ", " << rayStart.GetZ() << ") - (" << rayEnd.GetX() << ", " << rayEnd.GetY() << ", " << rayEnd.GetZ() << ")\n";
            }
        }
    }

    TEST_F(MATH_IntersectSegmentTriangleTest, CompareSegmentTriangleIntersectionMethods)
    {
        // This verifies "relative correctness" of the segment/triangle intersection code by comparing the results against
        // two other segment/triangle intersection methods and verifying that all three produce the same answers. The test uses
        // randomly-generated (but deterministic) segments and triangles, so the vast majority will miss, and some will hit. The
        // test tracks counts of hits and misses just to verify that we've gotten at least one hit and one miss.
        // 
        // Note that this test only works for smaller quantities of test iterations. If it is run too many times, some differences
        // will show up because the algorithms we're comparing against have non-zero failure rates of false positives and false
        // negatives when the segments intersect edges and vertices. The current seed and iteration count have been chosen so that
        // all three produce identical correct results, but have previously been run at > 10M iterations. The few failures that showed up
        // across those iterations have been cross-checked for accuracy and then added to specific-case regression unit test below.

        constexpr unsigned int Seed = 1;
        std::mt19937_64 rng(Seed);
        std::uniform_real_distribution<float> unif(-1000.0f, 1000.0f);

        // Track the number of hits, misses, and differences we have across the iterations.
        // We'll use the hit / miss counts to verify that we've successfully tested at least one of each type of case.
        int numDifferentHits = 0;
        int numHits = 0;
        int numMisses = 0;

        // We're comparing 3 different algorithms against each other.
        bool didHit[3];
        float outDistance[3];
        AZ::Vector3 outNormal[3];

        constexpr int NumTests = 10000;

        for (int test = 0; test < NumTests; test++)
        {
            AZ::Vector3 rayStart(unif(rng), unif(rng), unif(rng));
            AZ::Vector3 rayEnd(unif(rng), unif(rng), unif(rng));
            AZ::Vector3 triVerts0(unif(rng), unif(rng), unif(rng));
            AZ::Vector3 triVerts1(unif(rng), unif(rng), unif(rng));
            AZ::Vector3 triVerts2(unif(rng), unif(rng), unif(rng));

            // Test our current watertight algorithm implementation
            didHit[0] = AZ::Intersect::IntersectSegmentTriangle(
                rayStart, rayEnd, triVerts0, triVerts1, triVerts2, outNormal[0], outDistance[0]);
            ClearResultsOnMiss(didHit[0], outNormal[0], outDistance[0]);

            // Test using a Moller Trumbore algorithm implementation
            didHit[1] = IntersectTest::MollerTrumboreIntersectSegmentTriangle(
                rayStart, rayEnd, triVerts0, triVerts1, triVerts2, outNormal[1], outDistance[1]);
            ClearResultsOnMiss(didHit[1], outNormal[1], outDistance[1]);

            // Test using an Arenberg algorithm implementation
            didHit[2] = IntersectTest::ArenbergIntersectSegmentTriangle(
                rayStart, rayEnd, triVerts0, triVerts1, triVerts2, outNormal[2], outDistance[2]);
            ClearResultsOnMiss(didHit[2], outNormal[2], outDistance[2]);

            if ((didHit[0] != didHit[1]) || (didHit[0] != didHit[2]))
            {
                // At least one of our three implementations produced different results.
                // If this triggers, the specific segment and triangle should get examined further.
                // This likely happened because of different behaviors in the algorithms on edge or vertex collisions.
                numDifferentHits++;
                EXPECT_EQ(didHit[0], didHit[1]);
                EXPECT_EQ(didHit[0], didHit[2]);
            }
            else
            {
                // If all three algorithms agree on a hit, verify that the results of the hit match across all three.
                if (didHit[0] && didHit[1] && didHit[2])
                {
                    // These case should never fail. A failure means that all the algorithms agree there was a hit, but
                    // somehow disagree on the location of the hit or the normal of the triangle that was hit.
                    if (!AZ::IsClose(outDistance[0], outDistance[1]) || !AZ::IsClose(outDistance[0], outDistance[2]))
                    {
                        numDifferentHits++;
                        EXPECT_NEAR(outDistance[0], outDistance[1], 0.00001f);
                        EXPECT_NEAR(outDistance[0], outDistance[2], 0.00001f);
                    }
                    else if (!outNormal[0].IsClose(outNormal[1]) || !outNormal[0].IsClose(outNormal[2]))
                    {
                        numDifferentHits++;
                        EXPECT_THAT(outNormal[0], IsClose(outNormal[1]));
                        EXPECT_THAT(outNormal[0], IsClose(outNormal[2]));
                    }
                }
            }

            if (didHit[0])
            {
                numHits++;
            }
            else
            {
                numMisses++;
            }
        }

        // Verify that all three algorithms produced the same results across all our iterations.
        EXPECT_EQ(numDifferentHits, 0);

        // Verify that we tested at least one test case each of a hit and a miss.
        EXPECT_GT(numHits, 0);
        EXPECT_GT(numMisses, 0);
    }

     struct RayTriangleTest
    {
        AZ::Vector3 m_rayStart;
        AZ::Vector3 m_rayEnd;
        AZ::Vector3 m_triVerts[3];
        bool m_shouldHit;
        float m_hitDistance;
        AZ::Vector3 m_hitNormal;
    };

    static const RayTriangleTest RayTriangleTestParams[] =
    {
        // This failure came from a vegetation system raycast to a triangle in a sphere model.
        {
            { 0.0, 0.16, 1.01948643 }, { 0.0, 0.16, -0.0194873810 }, // segment
            { { 0.0294727981, 0.18608, 0.9605 }, { 0.0, 0.18842, 0.9605 }, { 0.0, 0.15215, 0.9741 } }, // triangle
            true, 0.0465169623, { 0.0278645027, 0.350958735, 0.935976326 }  // expected results
        },

        // These failures were generated from purely random segment/triangle combinations that were cross-checked against
        // multiple algorithms in the "CompareSegmentTriangleIntersectionMethods" unit test.
        {
            { 962.503540, -788.401978, -320.390747 }, { -838.229004, 555.508301, 748.038086 }, // segment
            { { -353.655212, 579.209229, -827.134216 }, { 358.185913, 346.845093, 729.328979 }, { 222.018555, -358.014160, -4.521179 } }, // triangle
            true, 0.399751157, { 0.899101138, 0.220195562, -0.378326714 } // expected results
        },
        {
            { 444.587769, 667.560425, -30.933167 }, { -701.140015, -835.056152, -521.363892 }, // segment
            { { -711.895020, -679.238892, -727.331543 }, { -269.888733, 985.126099, 592.807617 }, { 752.411377, 504.572876, 772.386597 } }, // triangle
            true, 0.714566, { 0.376415, 0.512304, -0.771917 } // expected results
        },
        {
            { -441.883545, -278.100342, 903.960938 }, { -475.418335, 334.792114, 420.302734 }, // segment
            { { 717.479004, -116.081299, -610.493530 }, { 603.540039, -305.713684, -86.320618 }, { -466.514709, 160.187012, 558.720825 } }, // triangle
            false, 0.0, { 0.0, 0.0, 0.0 } // expected results
        },

        // These failures were generated from combinations of random segments and a generated icosphere in the
        // "SegmentThroughSphereCenterHasCorrectIntersectionCount" unit test
        {
            {42.6055908, 31.7755775, -13.6140671}, {-42.6055908, -31.7755775, 13.6140671}, // segment
            { { 0.797190964, 0.557785630, -0.231001750 }, { 0.775978208, 0.576951861, -0.254920423 }, { 0.778470039, 0.588076711, -0.219431564 } }, // triangle
            true, 0.490887880, { 0.784085453, 0.574396968, -0.235112920 } // expected results
        },
        {
            {42.6055908, 31.7755775, -13.6140671}, {-42.6055908, -31.7755775, 13.6140671}, // segment
            { { 0.756726921, 0.606817961, -0.243179753 }, { 0.778470039, 0.588076711, -0.219431564 }, { 0.775978208, 0.576951861, -0.254920423 } }, // triangle
            false, 0.0, {0.0, 0.0, 0.0} // expected results
        },
        {
            { -46.8041611, -7.23778582, 18.3756104 }, { 46.8041611, 7.23778582, -18.3756104 }, // segment
            {{ -0.935130417, -0.122800261, 0.332341939 },{ -0.921611011, -0.143026009, 0.360800087 }, { -0.910672069, -0.122609399, 0.394516677 } }, // triangle
            true, 0.490157783, {-0.925243855, -0.105879672, 0.364298195} // expected results
        },
        {
            { 20.5424690, -33.8884621, 26.2357140 }, { -20.5424690, 33.8884621, -26.2357140 }, // segment
            { { 0.448259473, -0.725299001, 0.522498608 }, { 0.437129468, -0.707290292, 0.555570245 }, { 0.415217489, -0.732416809, 0.539592505 } }, // triangle
            true, 0.489480674, { 0.434347391, -0.721596122, 0.539111614 } // expected results
        },
        {
            { 20.5424690, -33.8884621, 26.2357140 }, { -20.5424690, 33.8884621, -26.2357140 }, // segment
            { { 0.403728217, -0.713824689, 0.572239339 }, { 0.415217489, -0.732416809, 0.539592505 }, { 0.437129468, -0.707290292, 0.555570245 } }, // triangle
            false, 0.0, {0.0, 0.0, 0.0} // expected results
        },

        // These failures previously came from the vegetation system and the Editor using FLT_MAX as segment start or end values.
        {
            { 5.0f, -15.0f, 1.0f }, { 5.0f, -15.0f, -FLT_MAX }, // segment
            { { 0.0f, -10.0f, 0.0f }, { 0.0f, -20.0f, 0.0f }, { 10.0f, -10.0f, 0.0f } }, // triangle
            true, 0.0f, {0.0f, 0.0f, 1.0f} // expected results
        },
        {
            { 5.0f, -15.0f, FLT_MAX }, { 5.0f, -15.0f, -1.0f }, // segment
            { { 0.0f, -10.0f, 0.0f }, { 0.0f, -20.0f, 0.0f }, { 10.0f, -10.0f, 0.0f } }, // triangle
            true, 1.0f, {0.0f, 0.0f, 1.0f} // expected results
        },
    };
    
    using RayTriangleTests = ::testing::TestWithParam<RayTriangleTest>;

    TEST_P(RayTriangleTests, RegressionTestForSpecificSegmentsAndTriangles)
    {
        // This unit test is set up to validate regressions of erroneous segment/triangle intersection test results.
        // All the failures that have been found so far have been false negatives or false positives that occur on triangle
        // edges or vertices.

        const RayTriangleTest test = GetParam();

        AZ::Vector3 outNormal = AZ::Vector3::CreateZero();
        float outDistance = 0.0f;
        bool result = AZ::Intersect::IntersectSegmentTriangleCCW(
            test.m_rayStart, test.m_rayEnd, test.m_triVerts[0], test.m_triVerts[1], test.m_triVerts[2], outNormal, outDistance);

        EXPECT_EQ(test.m_shouldHit, result);
        if (test.m_shouldHit && result)
        {
            EXPECT_NEAR(test.m_hitDistance, outDistance, 0.0001f);
            EXPECT_THAT(test.m_hitNormal, IsClose(outNormal));
        }
    }

    INSTANTIATE_TEST_CASE_P(MATH_IntersectSegmentTriangleTest, RayTriangleTests, ::testing::ValuesIn(RayTriangleTestParams));

    class MATH_IntersectRayCappedCylinderTest
        : public LeakDetectionFixture
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
        : public LeakDetectionFixture
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

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideBase_RayDirThroughApex)
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
        : public LeakDetectionFixture
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

    TEST(MATH_Intersection, RaySmallQuad)
    {
        Vector3 rayOrigin = AZ::Vector3::CreateAxisX(-10.0f);
        Vector3 rayDir = AZ::Vector3::CreateAxisX();
        Vector3 vertexA(0.0f, -0.001f, 0.001f);
        Vector3 vertexB(0.0f, 0.001f, 0.001f);
        Vector3 vertexC(0.0f, 0.001f, -0.001f);
        Vector3 vertexD(0.0f, -0.001f, -0.001f);
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, vertexA, vertexB, vertexC, vertexD, t);
        EXPECT_EQ(hit, 1);
    }

    class MATH_IntersectRayBoxTest
        : public LeakDetectionFixture
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
        : public LeakDetectionFixture
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

    class MATH_ClipRayWithAabbTest : public LeakDetectionFixture
    {
    };

    TEST_F(MATH_ClipRayWithAabbTest, RayDoesNotGetClippedWhenMissingAabb)
    {
        // Test a ray that points straight down and completely misses a box.

        const Aabb aabb = Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 10.0f);

        Vector3 rayStart(50.0f, 50.0f, 20.0f);
        Vector3 rayEnd(50.0f, 50.0f, -20.0f);

        float tClipStart = AZStd::numeric_limits<float>::max();
        float tClipEnd = AZStd::numeric_limits<float>::max();

        bool clipped = Intersect::ClipRayWithAabb(aabb, rayStart, rayEnd, tClipStart, tClipEnd);

        // We expect the ray not to get clipped. There are no guarantees on the validity of any of the output values
        // (rayStart, rayEnd, tClipStart, tClipEnd), so we won't validate them here.
        EXPECT_FALSE(clipped);
    }

    TEST_F(MATH_ClipRayWithAabbTest, RayIsClippedWithAabb)
    {
        // Test a ray that starts 10 above a box, points straight down, and continues to 10 below the box.
        // The z values should get clipped to the box.

        const Aabb aabb = Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 10.0f);

        Vector3 rayStart(5.0f, 5.0f, 20.0f);
        Vector3 rayEnd(5.0f, 5.0f, -20.0f);

        float tClipStart = AZStd::numeric_limits<float>::max();
        float tClipEnd = AZStd::numeric_limits<float>::max();

        bool clipped = Intersect::ClipRayWithAabb(aabb, rayStart, rayEnd, tClipStart, tClipEnd);

        // We expect the ray to get clipped.
        EXPECT_TRUE(clipped);
        EXPECT_THAT(rayStart, IsClose(AZ::Vector3(5.0f, 5.0f, 10.0f)));
        EXPECT_THAT(rayEnd, IsClose(AZ::Vector3(5.0f, 5.0f, -10.0f)));
        EXPECT_NEAR(tClipStart, 0.25f, 0.001f);
        EXPECT_NEAR(tClipEnd, 0.75f, 0.001f);
    }

    TEST_F(MATH_ClipRayWithAabbTest, RayStartingInsideAabbGetsClipped)
    {
        // Test a ray that starts inside a box, points straight down, and continues to 10 below the box.
        // The z values should get clipped to the box.

        const Aabb aabb = Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 10.0f);

        Vector3 rayStart(5.0f, 5.0f, 0.0f);
        Vector3 rayEnd(5.0f, 5.0f, -20.0f);

        float tClipStart = AZStd::numeric_limits<float>::max();
        float tClipEnd = AZStd::numeric_limits<float>::max();

        bool clipped = Intersect::ClipRayWithAabb(aabb, rayStart, rayEnd, tClipStart, tClipEnd);

        // We expect the ray to get clipped.
        EXPECT_TRUE(clipped);
        EXPECT_THAT(rayStart, IsClose(AZ::Vector3(5.0f, 5.0f, 0.0f)));
        EXPECT_THAT(rayEnd, IsClose(AZ::Vector3(5.0f, 5.0f, -10.0f)));
        EXPECT_NEAR(tClipStart, 0.0f, 0.001f);
        EXPECT_NEAR(tClipEnd, 0.5f, 0.001f);
    }

    TEST_F(MATH_ClipRayWithAabbTest, RayEndingInsideAabbGetsClipped)
    {
        // Test a ray that starts 10 above a box, points straight down, and ends inside the box.
        // The z values should get clipped to the box.

        const Aabb aabb = Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 10.0f);

        Vector3 rayStart(5.0f, 5.0f, 20.0f);
        Vector3 rayEnd(5.0f, 5.0f, 0.0f);

        float tClipStart = AZStd::numeric_limits<float>::max();
        float tClipEnd = AZStd::numeric_limits<float>::max();

        bool clipped = Intersect::ClipRayWithAabb(aabb, rayStart, rayEnd, tClipStart, tClipEnd);

        // We expect the ray to get clipped.
        EXPECT_TRUE(clipped);
        EXPECT_THAT(rayStart, IsClose(AZ::Vector3(5.0f, 5.0f, 10.0f)));
        EXPECT_THAT(rayEnd, IsClose(AZ::Vector3(5.0f, 5.0f, 0.0f)));
        EXPECT_NEAR(tClipStart, 0.5f, 0.001f);
        EXPECT_NEAR(tClipEnd, 1.0f, 0.001f);
    }
} // namespace UnitTest
