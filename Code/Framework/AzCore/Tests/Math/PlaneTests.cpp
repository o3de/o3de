/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <Math/MathTest.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Plane, TestCreateFromNormalAndDistance)
    {
        Plane pl = Plane::CreateFromNormalAndDistance(Vector3(1.0f, 0.0f, 0.0f), -100.0f);
        EXPECT_NEAR(pl.GetDistance(), -100.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetX(), 1.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetY(), 0.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetZ(), 0.0f, 2e-3f);
    }

    TEST(MATH_Plane, TestCreateFromNormalAndPoint)
    {
        Plane pl = Plane::CreateFromNormalAndPoint(Vector3(0.0f, 1.0f, 0.0f), Vector3(10, 10, 10));
        EXPECT_NEAR(pl.GetDistance(), -10.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetX(), 0.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetY(), 1.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetZ(), 0.0f, 2e-3f);
    }

    TEST(MATH_Plane, TestCreateFromCoefficients)
    {
        Plane pl = Plane::CreateFromCoefficients(0.0f, -1.0f, 0.0f, -5.0f);
        EXPECT_NEAR(pl.GetDistance(), -5.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetX(), 0.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetY(), -1.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetZ(), 0.0f, 2e-3f);
    }

    TEST(MATH_Plane, TestSet)
    {
        Plane pl;
        AZ_MATH_TEST_START_TRACE_SUPPRESSION;
        pl.Set(12.0f, 13.0f, 14.0f, 15.0f);
        AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_NEAR(pl.GetDistance(), 15.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetX(), 12.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetY(), 13.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetZ(), 14.0f, 2e-3f);
    }

    TEST(MATH_Plane, TestSetVector3)
    {
        Plane pl;
        AZ_MATH_TEST_START_TRACE_SUPPRESSION;
        pl.Set(Vector3(22.0f, 23.0f, 24.0f), 25.0f);
        AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_NEAR(pl.GetDistance(), 25.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetX(), 22.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetY(), 23.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetZ(), 24.0f, 2e-3f);
    }

    TEST(MATH_Plane, TestSetVector4)
    {
        Plane pl;
        pl.Set(Vector4(32.0f, 33.0f, 34.0f, 35.0f));
        EXPECT_NEAR(pl.GetDistance(), 35.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetX(), 32.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetY(), 33.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetZ(), 34.0f, 2e-3f);
    }

    TEST(MATH_Plane, TestSetNormalAndDistance)
    {
        Plane pl;
        pl.SetNormal(Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_NEAR(pl.GetNormal().GetX(), 0.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetY(), 0.0f, 2e-3f);
        EXPECT_NEAR(pl.GetNormal().GetZ(), 1.0f, 2e-3f);

        pl.SetDistance(55.0f);
        EXPECT_NEAR(pl.GetDistance(), 55.0f, 2e-3f);

        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetW(), 55.0f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetX(), 0.0f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetY(), 0.0f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetZ(), 1.0f, 2e-3f);
    }

    TEST(MATH_Plane, TestGetTransform)
    {
        Plane pl;
        pl.Set(Vector3(0.0f, 0.0f, 1.0f), 55.0f);

        Transform tm = Transform::CreateRotationY(DegToRad(90.0f));
        pl = pl.GetTransform(tm);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetW(), 55.0f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetX(), 1.0f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetY(), 0.0f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetZ(), 0.0f, 2e-3f);

        float dist = pl.GetPointDist(Vector3(10.0f, 0.0f, 0.0f));
        EXPECT_NEAR(dist, 65.0f, 2e-3f);  // 55 + 10
    }

    TEST(MATH_Plane, TestApplyTransform)
    {
        Plane pl;
        pl.Set(Vector3(0.0f, 0.0f, 1.0f), 55.0f);

        Transform tm1 = Transform::CreateRotationY(DegToRad(90.0f));
        Transform tm2 = Transform::CreateRotationZ(DegToRad(45.0f));
        pl.ApplyTransform(tm1);
        pl.ApplyTransform(tm2);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetW(), 55.0f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetX(), 0.707f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetY(), 0.707f, 2e-3f);
        EXPECT_NEAR(pl.GetPlaneEquationCoefficients().GetZ(), 0.0f, 2e-3f);
    }

    TEST(MATH_Plane, TestGetProjected)
    {
        Plane pl;
        pl.Set(Vector3(0.0f, 0.0f, 1.0f), 55.0f);
        Vector3 v1 = pl.GetProjected(Vector3(10.0f, 15.0f, 20.0f));
        EXPECT_THAT(v1, IsCloseTolerance(Vector3(10.0f, 15.0f, 0.0f), 1e-6f));
    }

    TEST(MATH_Plane, TestCastRay)
    {
        Vector3 hitPoint;

        Plane pl;
        pl.Set(0.0f, 0.0f, 1.0f, 10.0f);
        bool hit = pl.CastRay(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), hitPoint);
        EXPECT_TRUE(hit);
        EXPECT_THAT(hitPoint, IsClose(Vector3(0.0f, 0.0f, -10.0f)));

        pl.Set(0.0f, 1.0f, 0.0f, 10.0f);
        float time;
        hit = pl.CastRay(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), time);
        EXPECT_TRUE(hit);
        EXPECT_NEAR(time, 10.999f, 2e-3f);

        pl.Set(1.0f, 0.0f, 0.0f, 5.0f);
        hit = pl.CastRay(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), time);
        EXPECT_FALSE(hit);
    }

    TEST(MATH_Plane, TestIntersectSegment)
    {
        Vector3 hitPoint;
        Plane pl;
        pl.Set(1.0f, 0.0f, 0.0f, 0.0f);
        bool hit = pl.IntersectSegment(Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), hitPoint);
        EXPECT_TRUE(hit);
        EXPECT_THAT(hitPoint, IsClose(Vector3(0.0f, 0.0f, 0.0f)));

        float time;
        pl.Set(0.0f, 1.0f, 0.0f, 0.0f);
        hit = pl.IntersectSegment(Vector3(0.0f, -10.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), time);
        EXPECT_TRUE(hit);
        EXPECT_NEAR(time, 0.5f, 2e-3f);

        pl.Set(0.0f, 1.0f, 0.0f, 20.0f);
        hit = pl.IntersectSegment(Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), time);
        EXPECT_FALSE(hit);
    }

    TEST(MATH_Plane, TestIsFinite)
    {
        Plane pl;
        pl.Set(1.0f, 0.0f, 0.0f, 0.0f);
        EXPECT_TRUE(pl.IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        AZ_MATH_TEST_START_TRACE_SUPPRESSION;
        pl.Set(infinity, infinity, infinity, infinity);
        AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(pl.IsFinite());
    }

    TEST(MATH_Plane, CreateFromVectorCoefficients_IsEquivalentToCreateFromCoefficients)
    {
        AZ_MATH_TEST_START_TRACE_SUPPRESSION;
        Plane planeFromCoefficients = Plane::CreateFromCoefficients(1.0, 2.0, 3.0, 4.0);
        AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(1);
        Vector4 coefficients(1.0, 2.0, 3.0, 4.0);
        Plane planeFromVectorCoefficients = Plane::CreateFromVectorCoefficients(coefficients);


        EXPECT_EQ(planeFromVectorCoefficients, planeFromCoefficients);
    }
}
