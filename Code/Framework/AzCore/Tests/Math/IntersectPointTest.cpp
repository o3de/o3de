/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

using namespace AZ;

namespace UnitTest::IntersectTest
{
    // Tests a point inside sphere
    TEST(MATH_IntersectPointSphere, TestPointInside)
    {
        Vector3 sphereCenter(1.f, 2.f, 3.f);
        float sphereRadius = 5.f;
        Vector3 testPoint(2.f, 3.f, 4.f);

        EXPECT_TRUE(Intersect::PointSphere(sphereCenter, sphereRadius * sphereRadius, testPoint));
    }

    // Tests a point outside sphere
    TEST(MATH_IntersectPointSphere, TestPointOutside)
    {
        Vector3 sphereCenter(1.f, 2.f, 3.f);
        float sphereRadius = 5.f;
        Vector3 testPoint(10.f, 10.f, 10.f);

        EXPECT_FALSE(Intersect::PointSphere(sphereCenter, sphereRadius * sphereRadius, testPoint));
    }

    // Tests a point just inside the border of the sphere
    TEST(MATH_IntersectPointSphere, TestPointInsideBorder)
    {
        Vector3 sphereCenter(1.f, 2.f, 3.f);
        float sphereRadius = 5.f;
        Vector3 testPoint(5.99f, 2.f, 3.f);

        EXPECT_TRUE(Intersect::PointSphere(sphereCenter, sphereRadius * sphereRadius, testPoint));
    }

    // Tests a point just outside the border of the sphere
    TEST(MATH_IntersectPointSphere, TestPointOutsideBorder)
    {
        Vector3 sphereCenter(1.f, 2.f, 3.f);
        float sphereRadius = 5.f;
        Vector3 testPoint(6.01f, 2.f, 3.f);

        EXPECT_FALSE(Intersect::PointSphere(sphereCenter, sphereRadius * sphereRadius, testPoint));
    }

    // Tests a point inside cylinder
    TEST(MATH_IntersectPointCylinder, TestPointInside)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 5.f, 3.f);

        EXPECT_TRUE(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point outside cylinder
    TEST(MATH_IntersectPointCylinder, TestPointOutside)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 1.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = 2.f;
        Vector3 testPoint(9.f, 9.f, 9.f);

        EXPECT_FALSE(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just inside border at the top of the cylinder
    TEST(MATH_IntersectPointCylinder, TestPointInsideBorderTop)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 6.99f, 3.f);

        EXPECT_TRUE(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just outside border at the top of the cylinder
    TEST(MATH_IntersectPointCylinder, TestPointOutsideBorderTop)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 7.01f, 3.f);

        EXPECT_FALSE(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just inside border at the bottom of the cylinder
    TEST(MATH_IntersectPointCylinder, TestPointInsideBorderBottom)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 2.01f, 3.f);

        EXPECT_TRUE(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just outside border at the bottom of the cylinder
    TEST(MATH_IntersectPointCylinder, TestPointOutsideBorderBottom)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 1.99f, 3.f);

        EXPECT_FALSE(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just inside border at the side of the cylinder
    TEST(MATH_IntersectPointCylinder, TestPointInsideBorderSide)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.99f, 2.f, 3.f);

        EXPECT_TRUE(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just outside border at the side of the cylinder
    TEST(MATH_IntersectPointCylinder, TestPointOutsideBorderSide)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(2.01f, 2.f, 3.f);

        EXPECT_FALSE(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }


    TEST(MATH_IntersectBarycentric, TestBarycentric)
    {
        EXPECT_THAT(AZ::Intersect::Barycentric(AZ::Vector3(1.0f,0.0f,0.0f), AZ::Vector3(0.0f,1,0.0f), AZ::Vector3(0.0f,0.0f,1), AZ::Vector3(1,0.0f,0.0f)),
            IsClose(AZ::Vector3(1.0f,0.0f,0.0f)));
        EXPECT_THAT(AZ::Intersect::Barycentric(AZ::Vector3(1.0f,0.0f,0.0f), AZ::Vector3(0.0f,1.0f,0.0f), AZ::Vector3(0.0f,0.0f,1.0f), AZ::Vector3(0.0f,1.0f,0.0f)),
            IsClose(AZ::Vector3(0,1,0)));
        EXPECT_THAT(AZ::Intersect::Barycentric(AZ::Vector3(1.0f,0.0f,0.0f), AZ::Vector3(0.0f,1.0f,0.0f), AZ::Vector3(0.0f,0.0f,1.0f), AZ::Vector3(0.0f,0.0f,1.0f)),
            IsClose(AZ::Vector3(0,0,1)));
        EXPECT_THAT(AZ::Intersect::Barycentric(AZ::Vector3(1.0f,0.0f,0.0f), AZ::Vector3(0.0f,1.0f,0.0f), AZ::Vector3(0.0f,0.0f,1.0f), AZ::Vector3(0.3f,0.3f,0.3f)),
            IsClose(AZ::Vector3(0.333333f,0.333333f,0.333333f)));
    }

}
