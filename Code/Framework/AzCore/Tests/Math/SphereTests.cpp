/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Sphere.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

namespace UnitTest
{
    TEST(MATH_Sphere, TestCreateUnitSphere)
    {
        AZ::Sphere unitSphere = AZ::Sphere::CreateUnitSphere();
        EXPECT_THAT(unitSphere.GetCenter(), IsClose(AZ::Vector3::CreateZero()));
        EXPECT_NEAR(unitSphere.GetRadius(), 1.f, 0.0001f);
    }

    TEST(MATH_Sphere, TestCreateFromAabb)
    {
        AZ::Aabb testBox = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(1.0f));
        AZ::Sphere testSphere = AZ::Sphere::CreateFromAabb(testBox);
        EXPECT_THAT(testSphere.GetCenter(), IsClose(AZ::Vector3::CreateZero()));
        EXPECT_NEAR(testSphere.GetRadius(), 1.f, 0.0001f);
    }

    TEST(MATH_Sphere, TestConstructFromVec3AndRadius)
    {
        AZ::Sphere sphere1(AZ::Vector3(10.f, 10.f, 10.f), 15.f);
        EXPECT_THAT(sphere1.GetCenter(), IsClose(AZ::Vector3(10.f, 10.f, 10.f)));
        EXPECT_NEAR(sphere1.GetRadius(), 15.f, 0.0001f);
    }

    TEST(MATH_Sphere, TestSet)
    {
        AZ::Sphere sphere1(AZ::Vector3(10.f, 10.f, 10.f), 15.f);
        AZ::Sphere sphere2(AZ::Vector3(12.f, 12.f, 12.f), 13.f);
        EXPECT_NE(sphere2, sphere1);

        sphere1.Set(sphere2);
        EXPECT_EQ(sphere2, sphere1);
    }

    TEST(MATH_Sphere, TestSetCenterAndRadius)
    {
        AZ::Sphere sphere2(AZ::Vector3(12.f, 12.f, 12.f), 13.f);
        AZ::Sphere sphere3(AZ::Vector3(10.f, 10.f, 10.f), 15.f);
        sphere3.SetCenter(AZ::Vector3(12.f, 12.f, 12.f));
        sphere3.SetRadius(13.f);
        EXPECT_EQ(sphere2, sphere3);
    }

    TEST(MATH_Sphere, TestAssignment)
    {
        AZ::Sphere unitSphere = AZ::Sphere::CreateUnitSphere();
        AZ::Sphere sphere2(AZ::Vector3(12.f, 12.f, 12.f), 13.f);
        sphere2 = unitSphere;
        EXPECT_EQ(sphere2, unitSphere);
    }

    TEST(MATH_Sphere, TestCopyConstructor)
    {
        AZ::Sphere unitSphere = AZ::Sphere::CreateUnitSphere();
        AZ::Sphere sphere2(unitSphere);
        EXPECT_EQ(sphere2, unitSphere);
    }
} // namespace UnitTest
