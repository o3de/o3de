/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    // Tests a point inside sphere
    TEST(INTERSECT_PointSphere, TestPointInside)
    {
        Vector3 sphereCenter(1.f, 2.f, 3.f);
        float sphereRadius = 5.f;
        Vector3 testPoint(2.f, 3.f, 4.f);

        AZ_TEST_ASSERT(Intersect::PointSphere(sphereCenter, sphereRadius * sphereRadius, testPoint));
    }

    // Tests a point outside sphere
    TEST(INTERSECT_PointSphere, TestPointOutside)
    {
        Vector3 sphereCenter(1.f, 2.f, 3.f);
        float sphereRadius = 5.f;
        Vector3 testPoint(10.f, 10.f, 10.f);

        AZ_TEST_ASSERT(Intersect::PointSphere(sphereCenter, sphereRadius * sphereRadius, testPoint) == false);
    }

    // Tests a point just inside the border of the sphere
    TEST(INTERSECT_PointSphere, TestPointInsideBorder)
    {
        Vector3 sphereCenter(1.f, 2.f, 3.f);
        float sphereRadius = 5.f;
        Vector3 testPoint(5.99f, 2.f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointSphere(sphereCenter, sphereRadius * sphereRadius, testPoint));
    }

    // Tests a point just outside the border of the sphere
    TEST(INTERSECT_PointSphere, TestPointOutsideBorder)
    {
        Vector3 sphereCenter(1.f, 2.f, 3.f);
        float sphereRadius = 5.f;
        Vector3 testPoint(6.01f, 2.f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointSphere(sphereCenter, sphereRadius * sphereRadius, testPoint) == false);
    }

    // Tests a point inside cylinder
    TEST(INTERSECT_PointCylinder, TestPointInside)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 5.f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point outside cylinder
    TEST(INTERSECT_PointCylinder, TestPointOutside)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 1.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = 2.f;
        Vector3 testPoint(9.f, 9.f, 9.f);

        AZ_TEST_ASSERT(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint) == false);
    }

    // Tests a point just inside border at the top of the cylinder
    TEST(INTERSECT_PointCylinder, TestPointInsideBorderTop)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 6.99f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just outside border at the top of the cylinder
    TEST(INTERSECT_PointCylinder, TestPointOutsideBorderTop)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 7.01f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint) == false);
    }

    // Tests a point just inside border at the bottom of the cylinder
    TEST(INTERSECT_PointCylinder, TestPointInsideBorderBottom)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 2.01f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just outside border at the bottom of the cylinder
    TEST(INTERSECT_PointCylinder, TestPointOutsideBorderBottom)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.f, 1.99f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint) == false);
    }

    // Tests a point just inside border at the side of the cylinder
    TEST(INTERSECT_PointCylinder, TestPointInsideBorderSide)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(1.99f, 2.f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint));
    }

    // Tests a point just outside border at the side of the cylinder
    TEST(INTERSECT_PointCylinder, TestPointOutsideBorderSide)
    {
        Vector3 cylinderBase(1.f, 2.f, 3.f);
        Vector3 cylinderAxis(0.f, 5.f, 0.f);
        float cylinderRadius = 1.f;
        float cylinderLength = cylinderAxis.GetLength();
        Vector3 testPoint(2.01f, 2.f, 3.f);

        AZ_TEST_ASSERT(Intersect::PointCylinder(cylinderBase, cylinderAxis, cylinderLength * cylinderLength, cylinderRadius * cylinderRadius, testPoint) == false);
    }

}
