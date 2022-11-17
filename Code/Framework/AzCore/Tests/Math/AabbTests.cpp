/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/span.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Aabb, TestCreateNull)
    {
        Aabb aabb = Aabb::CreateNull();
        EXPECT_FALSE(aabb.IsValid());
        EXPECT_TRUE(aabb.GetMin().IsGreaterThan(aabb.GetMax()));
    }

    TEST(MATH_Aabb, TestCreateFromPoint)
    {
        Aabb aabb = Aabb::CreateFromPoint(Vector3(0.0f));
        EXPECT_TRUE(aabb.IsValid());
        EXPECT_TRUE(aabb.IsFinite());
        EXPECT_TRUE(aabb.GetMin().IsClose(aabb.GetMax()));
    }

    TEST(MATH_Aabb, TestCreateFromMinMax)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(1.0f));
        EXPECT_TRUE(aabb.IsValid());
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(0.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(1.0f)));
    }

    TEST(MATH_Aabb, TestCreateCenterHalfExtents)
    {
        Aabb aabb = Aabb::CreateCenterHalfExtents(Vector3(1.0f), Vector3(1.0f));
        EXPECT_TRUE(aabb.IsValid());
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(0.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(2.0f)));
        EXPECT_TRUE(aabb.GetCenter().IsClose(Vector3(1.0f)));
    }

    TEST(MATH_Aabb, TestCreateCenterRadius)
    {
        Aabb aabb = Aabb::CreateCenterRadius(Vector3(4.0f), 2.0f);
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(2.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(6.0f)));
        EXPECT_TRUE(aabb.GetExtents().IsClose(Vector3(4.0f)));
    }

    TEST(MATH_Aabb, TestCreatePoints)
    {
        const int numPoints = 3;
        Vector3 points[numPoints] =
        {
            Vector3(10.0f, 0.0f, -10.0f),
            Vector3(-10.0f, 10.0f, 0.0f),
            Vector3(0.0f, -10.0f, 10.0f)
        };
        Aabb aabb = Aabb::CreatePoints(points, numPoints);
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-10.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(10.0f)));
    }

    TEST(MATH_Aabb, TestCreatePointsSlice)
    {
        Vector3 points[] =
        { 
            Vector3(10.0f, 0.0f, -10.0f),
            Vector3(-10.0f, 10.0f, 0.0f),
            Vector3(0.0f, -10.0f, 10.0f)
        };
        Aabb aabb = Aabb::CreatePoints(points);
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-10.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(10.0f)));
    }

    TEST(MATH_Aabb, TestCreatePointsEmpty)
    {
        Aabb aabb = Aabb::CreatePoints(nullptr, 0);
        EXPECT_FALSE(aabb.IsValid());
    }

    TEST(MATH_Aabb, TestCreatePointsEmptySlice)
    {
        AZStd::span<Vector3> points = {};
        Aabb aabb = Aabb::CreatePoints(points);
        EXPECT_FALSE(aabb.IsValid());
    }

    TEST(MATH_Aabb, TestCreateFromObb)
    {
        const Vector3 position(1.0f, 1.0f, 1.0f);
        const Quaternion rotation = Quaternion::CreateRotationZ(Constants::QuarterPi);
        const Vector3 halfLengths = Vector3::CreateOne();
        const Obb obb = Obb::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        Aabb aabb = Aabb::CreateFromObb(obb);
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-0.414f, -0.414f, 0.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(2.414f, 2.414f, 2.0f)));
        EXPECT_TRUE(aabb.GetCenter().IsClose(Vector3(1.0f, 1.0f, 1.0f)));
    }

    TEST(MATH_Aabb, TestSetters)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(1.0f), Vector3(4.0f));

        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(4.0f)));

        aabb.SetMin(Vector3(0.0f));
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(0.0f)));
        EXPECT_TRUE(aabb.GetCenter().IsClose(Vector3(2.0f)));

        aabb.SetMax(Vector3(6.0f));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(6.0f)));
        EXPECT_TRUE(aabb.GetCenter().IsClose(Vector3(3.0f)));
    }

    TEST(MATH_Aabb, TestGetExtents)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(1.0f, 2.0f, 3.0f));

        EXPECT_TRUE(AZ::IsClose(aabb.GetXExtent(), 1.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetYExtent(), 2.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetZExtent(), 3.0f));
    }

    TEST(MATH_Aabb, TestSupport)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(1.0f));

        EXPECT_TRUE(aabb.GetSupport(Vector3( 0.5774,  0.5774,  0.5774)).IsClose(Vector3(0.0f, 0.0f, 0.0f)));
        EXPECT_TRUE(aabb.GetSupport(Vector3( 0.5774,  0.5774, -0.5774)).IsClose(Vector3(0.0f, 0.0f, 1.0f)));
        EXPECT_TRUE(aabb.GetSupport(Vector3( 0.5774, -0.5774,  0.5774)).IsClose(Vector3(0.0f, 1.0f, 0.0f)));
        EXPECT_TRUE(aabb.GetSupport(Vector3( 0.5774, -0.5774, -0.5774)).IsClose(Vector3(0.0f, 1.0f, 1.0f)));
        EXPECT_TRUE(aabb.GetSupport(Vector3(-0.5774,  0.5774,  0.5774)).IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_TRUE(aabb.GetSupport(Vector3(-0.5774,  0.5774, -0.5774)).IsClose(Vector3(1.0f, 0.0f, 1.0f)));
        EXPECT_TRUE(aabb.GetSupport(Vector3(-0.5774, -0.5774,  0.5774)).IsClose(Vector3(1.0f, 1.0f, 0.0f)));
        EXPECT_TRUE(aabb.GetSupport(Vector3(-0.5774, -0.5774, -0.5774)).IsClose(Vector3(1.0f, 1.0f, 1.0f)));
    }

    TEST(MATH_Aabb, TestGetAsSphere)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-2.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f));

        Vector3 center;
        float radius;
        aabb.GetAsSphere(center, radius);
        EXPECT_TRUE(center.IsClose(Vector3(0.0f)));
        EXPECT_TRUE(AZ::IsClose(radius, 2.0f));
    }

    TEST(MATH_Aabb, TestContains)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-2.0f), Vector3(2.0f));
        Aabb aabb2 = Aabb::CreateFromMinMax(Vector3(1.0f), Vector3(2.0f));

        //Contains(Vector3)
        EXPECT_TRUE(aabb.Contains(Vector3(1.0f)));

        //Contains(Aabb)
        EXPECT_TRUE(aabb.Contains(aabb2));
        EXPECT_FALSE(aabb2.Contains(aabb));
    }

    TEST(MATH_Aabb, TestOverlaps)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(2.0f));
        Aabb aabb2 = Aabb::CreateFromMinMax(Vector3(1.0f), Vector3(3.0f));

        EXPECT_TRUE(aabb.Overlaps(aabb2));
        aabb2.Set(Vector3(5.0f), Vector3(6.0f));
        EXPECT_FALSE(aabb.Overlaps(aabb2));
    }

    TEST(MATH_Aabb, TestDisjoint)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-2.0f), Vector3(2.0f));
        Aabb aabb2 = Aabb::CreateFromMinMax(Vector3(1.0f), Vector3(2.0f));
        Aabb aabb3 = Aabb::CreateFromMinMax(Vector3(3.0f), Vector3(4.0f));

        //Disjoint(Aabb)
        EXPECT_FALSE(aabb.Disjoint(aabb2));
        EXPECT_FALSE(aabb2.Disjoint(aabb));
        EXPECT_TRUE(aabb3.Disjoint(aabb));
        EXPECT_TRUE(aabb3.Disjoint(aabb2));
        EXPECT_TRUE(aabb.Disjoint(aabb3));
        EXPECT_TRUE(aabb2.Disjoint(aabb3));
    }

    TEST(MATH_Aabb, TestExpand)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(2.0f));

        aabb.Expand(Vector3(1.0f));
        EXPECT_TRUE(aabb.GetCenter().IsClose(Vector3(1.0f)));
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(3.0f)));
    }

    TEST(MATH_Aabb, TestGetExpanded)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f));

        aabb = aabb.GetExpanded(Vector3(9.0f));
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-10.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(10.0f)));
    }

    TEST(MATH_Aabb, TestAddPoint)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f));

        aabb.AddPoint(Vector3(1.0f));
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(1.0f)));
        aabb.AddPoint(Vector3(2.0f));
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(2.0f)));
    }

    TEST(MATH_Aabb, TestAddAabb)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(2.0f));
        Aabb aabb2 = Aabb::CreateFromMinMax(Vector3(-2.0f), Vector3(0.0f));

        aabb.AddAabb(aabb2);
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-2.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(2.0f)));
    }

    TEST(MATH_Aabb, TestGetDistance)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f));

        EXPECT_TRUE(AZ::IsClose(aabb.GetDistance(Vector3(2.0f, 0.0f, 0.0f)), 1.0f));
        // make sure a point inside the box returns zero, even if that point isn't the center.
        EXPECT_TRUE(AZ::IsClose(aabb.GetDistance(Vector3(0.5f, 0.0f, 0.0f)), 0.0f));
    }

    TEST(MATH_Aabb, TestGetDistanceSq)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f));

        EXPECT_TRUE(AZ::IsClose(aabb.GetDistanceSq(Vector3(0.0f, 3.0f, 0.0f)), 4.0f));
        // make sure a point inside the box returns zero, even if that point isn't the center.
        EXPECT_TRUE(AZ::IsClose(aabb.GetDistanceSq(Vector3(0.0f, 0.5f, 0.0f)), 0.0f));
    }

    TEST(MATH_Aabb, TestGetMaxDistance)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f));

        // The max distance for all of the following should be the square root of (4^2 + 3^2 + 2^2)
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(3.0f, 2.0f, 1.0f)), sqrtf(16.0f + 9.0f + 4.0f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(3.0f, 2.0f, -1.0f)), sqrtf(16.0f + 9.0f + 4.0f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(3.0f, -2.0f, 1.0f)), sqrtf(16.0f + 9.0f + 4.0f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(3.0f, -2.0f, -1.0f)), sqrtf(16.0f + 9.0f + 4.0f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(-3.0f, 2.0f, 1.0f)), sqrtf(16.0f + 9.0f + 4.0f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(-3.0f, 2.0f, -1.0f)), sqrtf(16.0f + 9.0f + 4.0f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(-3.0f, -2.0f, 1.0f)), sqrtf(16.0f + 9.0f + 4.0f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(-3.0f, -2.0f, -1.0f)), sqrtf(16.0f + 9.0f + 4.0f)));
        // make sure points inside the box return a correct max distance as well - sqrt of (1.5^2 + 1.5^2 + 1.5^2)
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(0.5f, 0.5f, 0.5f)), sqrtf(2.25f + 2.25f + 2.25f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(0.5f, 0.5f, -0.5f)), sqrtf(2.25f + 2.25f + 2.25f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(0.5f, -0.5f, 0.5f)), sqrtf(2.25f + 2.25f + 2.25f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(0.5f, -0.5f, -0.5f)), sqrtf(2.25f + 2.25f + 2.25f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(-0.5f, 0.5f, 0.5f)), sqrtf(2.25f + 2.25f + 2.25f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(-0.5f, 0.5f, -0.5f)), sqrtf(2.25f + 2.25f + 2.25f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(-0.5f, -0.5f, 0.5f)), sqrtf(2.25f + 2.25f + 2.25f)));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(-0.5f, -0.5f, -0.5f)), sqrtf(2.25f + 2.25f + 2.25f)));
        // make sure the center returns our minimal max distance (1^2 + 1^2 + 1^2)
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistance(Vector3(0.0f, 0.0f, 0.0f)), sqrtf(1.0f + 1.0f + 1.0f)));
    }

    TEST(MATH_Aabb, TestGetMaxDistanceSq)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f));

        // The max distance for all of the following should be (4^2 + 3^2 + 2^2)
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(3.0f, 2.0f, 1.0f)), 16.0f + 9.0f + 4.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(3.0f, 2.0f, -1.0f)), 16.0f + 9.0f + 4.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(3.0f, -2.0f, 1.0f)), 16.0f + 9.0f + 4.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(3.0f, -2.0f, -1.0f)), 16.0f + 9.0f + 4.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(-3.0f, 2.0f, 1.0f)), 16.0f + 9.0f + 4.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(-3.0f, 2.0f, -1.0f)), 16.0f + 9.0f + 4.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(-3.0f, -2.0f, 1.0f)), 16.0f + 9.0f + 4.0f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(-3.0f, -2.0f, -1.0f)), 16.0f + 9.0f + 4.0f));
        // make sure points inside the box return a correct max distance as well: (1.5^2 + 1.5^2 + 1.5^2)
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(0.5f, 0.5f, 0.5f)), 2.25f + 2.25f + 2.25f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(0.5f, 0.5f, -0.5f)), 2.25f + 2.25f + 2.25f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(0.5f, -0.5f, 0.5f)), 2.25f + 2.25f + 2.25f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(0.5f, -0.5f, -0.5f)), 2.25f + 2.25f + 2.25f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(-0.5f, 0.5f, 0.5f)), 2.25f + 2.25f + 2.25f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(-0.5f, 0.5f, -0.5f)), 2.25f + 2.25f + 2.25f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(-0.5f, -0.5f, 0.5f)), 2.25f + 2.25f + 2.25f));
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(-0.5f, -0.5f, -0.5f)), 2.25f + 2.25f + 2.25f));
        // make sure the center returns our minimal max distance (1^2 + 1^2 + 1^2)
        EXPECT_TRUE(AZ::IsClose(aabb.GetMaxDistanceSq(Vector3(0.0f, 0.0f, 0.0f)), 1.0f + 1.0f + 1.0f));
    }

    TEST(MATH_Aabb, TestGetClamped)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(2.0f));
        Aabb aabb2 = Aabb::CreateFromMinMax(Vector3(1.0f), Vector3(4.0f));

        aabb = aabb.GetClamped(aabb2);
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(2.0f)));
    }

    TEST(MATH_Aabb, TestClamp)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(2.0f));
        Aabb aabb2 = Aabb::CreateFromMinMax(Vector3(-2.0f), Vector3(1.0f));

        aabb.Clamp(aabb2);
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(0.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(1.0f)));
    }

    TEST(MATH_Aabb, TestSetNull)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(1.0f));

        aabb.SetNull();
        EXPECT_FALSE(aabb.IsValid());
    }

    TEST(MATH_Aabb, TestTranslate)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(-1.0f), Vector3(1.0f));

        aabb.Translate(Vector3(2.0f));
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(3.0f)));
    }

    TEST(MATH_Aabb, TestGetTranslated)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(1.0f), Vector3(3.0f));

        aabb = aabb.GetTranslated(Vector3(-2.0f));
        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(1.0f)));
    }

    TEST(MATH_Aabb, TestGetSurfaceArea)
    {
        Aabb aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(1.0f));

        EXPECT_NEAR(aabb.GetSurfaceArea(), 6.0f, 0.002f);
    }

    TEST(MATH_Aabb, TestIsClose)
    {
        const Aabb aabb1 = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(1.0f));
        const Aabb aabb2 = Aabb::CreateFromMinMax(Vector3(0.1f), Vector3(0.9f));
        const Aabb aabb3 = Aabb::CreateFromMinMax(Vector3(0.3f), Vector3(0.7f));
        EXPECT_TRUE(aabb1.IsClose(aabb2, 0.2f));
        EXPECT_FALSE(aabb1.IsClose(aabb3, 0.2f));
        EXPECT_TRUE(aabb2.IsClose(aabb3, 0.3f));
    }

    TEST(MATH_Aabb, TestIsFinite)
    {
        Aabb aabb = Aabb::CreateNull();

        const float infinity = std::numeric_limits<float>::infinity();
        const Vector3 infiniteV3 = Vector3(infinity);
        aabb.Set(Vector3(0), infiniteV3);
        // A bounding box is only invalid if the min is greater than the max.
        // A bounding box with an infinite min or max is valid as long as this is true.
        EXPECT_TRUE(aabb.IsValid());
        EXPECT_FALSE(aabb.IsFinite());
    }

    // Check if both aabb transform functions give the same result
    TEST(MATH_AabbTransform, CompareTest)
    {
        // create aabb
        Vector3 min(-100.0f, 50.0f, 0.0f);
        Vector3 max(120.0f, 300.0f, 50.0f);
        Aabb    aabb = Aabb::CreateFromMinMax(min, max);

        // make the transformation matrix
        Transform tm = Transform::CreateRotationX(1.0f);
        tm.SetTranslation(100.0f, 0.0f, -50.0f);

        // transform
        Obb     obb = aabb.GetTransformedObb(tm);
        Aabb    transAabb = aabb.GetTransformedAabb(tm);
        Aabb    transAabb2 = Aabb::CreateFromObb(obb);
        aabb.ApplyTransform(tm);

        // compare the transformations
        EXPECT_TRUE(transAabb.GetMin().IsClose(transAabb2.GetMin()));
        EXPECT_TRUE(transAabb.GetMax().IsClose(transAabb2.GetMax()));
        EXPECT_TRUE(aabb.GetMin().IsClose(transAabb.GetMin()));
        EXPECT_TRUE(aabb.GetMax().IsClose(transAabb.GetMax()));
    }

    TEST(MATH_AabbTransform, GetTransformedAabbCorrectResult)
    {
        // create aabb
        const Vector3 min(20.0f, 30.0f, 40.0f);
        const Vector3 max(50.0f, 80.0f, 60.0f);
        const Aabb aabb = Aabb::CreateFromMinMax(min, max);

        const Transform transform(Vector3(10.0f, 15.0f, 25.0f), Quaternion(0.08f, 0.44f, 0.16f, 0.88f), 0.5f);
        const Aabb transformedAabb = aabb.GetTransformedAabb(transform);

        EXPECT_THAT(transformedAabb.GetMin(), IsClose(Vector3(23.168f, 32.56f, 22.504f)));
        EXPECT_THAT(transformedAabb.GetMax(), IsClose(Vector3(44.872f, 61.24f, 46.776f)));
    }

    TEST(MATH_AabbTransform, GetTransformedObbMatrix3x4)
    {
        Vector3 min(-1.0f, -2.0f, -3.0f);
        Vector3 max(4.0f, 3.0f, 2.0f);
        Aabb aabb = Aabb::CreateFromMinMax(min, max);

        Quaternion rotation(0.46f, 0.26f, 0.58f, 0.62f);
        Vector3 translation(5.0f, 7.0f, 9.0f);

        Matrix3x4 matrix3x4 = Matrix3x4::CreateFromQuaternionAndTranslation(rotation, translation);

        matrix3x4.MultiplyByScale(Vector3(0.5f, 1.5f, 2.0f));

        Obb obb = aabb.GetTransformedObb(matrix3x4);

        EXPECT_THAT(obb.GetRotation(), IsClose(rotation));
        EXPECT_THAT(obb.GetHalfLengths(), IsClose(Vector3(1.25f, 3.75f, 5.0f)));
        EXPECT_THAT(obb.GetPosition(), IsClose(Vector3(3.928f, 7.9156f, 9.3708f)));
    }

    TEST(MATH_AabbTransform, GetTransformedAabbMatrix3x4)
    {
        Vector3 min(2.0f, 3.0f, 5.0f);
        Vector3 max(6.0f, 5.0f, 11.0f);
        Aabb aabb = Aabb::CreateFromMinMax(min, max);

        Quaternion rotation(0.34f, 0.46f, 0.58f, 0.58f);
        Vector3 translation(-3.0f, -4.0f, -5.0f);

        Matrix3x4 matrix3x4 = Matrix3x4::CreateFromQuaternionAndTranslation(rotation, translation);

        matrix3x4.MultiplyByScale(Vector3(1.2f, 0.8f, 2.0f));

        Aabb transformedAabb = aabb.GetTransformedAabb(matrix3x4);

        EXPECT_THAT(transformedAabb.GetMin(), IsClose(Vector3(4.1488f, -0.01216f, -0.31904f)));
        EXPECT_THAT(transformedAabb.GetMax(), IsClose(Vector3(16.3216f, 6.54272f, 5.98112f)));
    }

    TEST(MATH_AabbTransform, GetTransformedObbFitsInsideTransformedAabb)
    {
        Vector3 min(4.0f, 3.0f, 1.0f);
        Vector3 max(7.0f, 6.0f, 8.0f);
        Aabb aabb = Aabb::CreateFromMinMax(min, max);

        Quaternion rotation(0.40f, 0.40f, 0.64f, 0.52f);
        Vector3 translation(-2.0f, 4.0f, -3.0f);

        Matrix3x4 matrix3x4 = Matrix3x4::CreateFromQuaternionAndTranslation(rotation, translation);

        matrix3x4.MultiplyByScale(Vector3(2.2f, 0.6f, 1.4f));

        Aabb transformedAabb = aabb.GetTransformedAabb(matrix3x4);
        Obb transformedObb = aabb.GetTransformedObb(matrix3x4);
        Aabb aabbContainingTransformedObb = Aabb::CreateFromObb(transformedObb);

        EXPECT_THAT(transformedAabb.GetMin(), IsClose(aabbContainingTransformedObb.GetMin()));
        EXPECT_THAT(transformedAabb.GetMax(), IsClose(aabbContainingTransformedObb.GetMax()));
    }

    TEST(MATH_AabbTransform, MultiplyByScale)
    {
        Vector3 min(2.0f, 6.0f, 8.0f);
        Vector3 max(6.0f, 9.0f, 10.0f);
        Aabb aabb = Aabb::CreateFromMinMax(min, max);

        Vector3 scale(0.5f, 2.0f, 1.5f);
        aabb.MultiplyByScale(scale);

        EXPECT_THAT(aabb.GetMin(), IsClose(Vector3(1.0f, 12.0f, 12.0f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(Vector3(3.0f, 18.0f, 15.0f)));
    }
}
