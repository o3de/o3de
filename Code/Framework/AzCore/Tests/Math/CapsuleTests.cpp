/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Capsule.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

namespace UnitTest
{
    TEST(MATH_Capsule, Constructor)
    {
        const AZ::Vector3 firstHemisphereCenter(1.0f, 2.0f, 3.0f);
        const AZ::Vector3 secondHemisphereCenter(3.0f, 5.0f, 7.0f);
        const float radius = 2.0f;
        AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        EXPECT_THAT(capsule.GetFirstHemisphereCenter(), IsClose(firstHemisphereCenter));
        EXPECT_THAT(capsule.GetSecondHemisphereCenter(), IsClose(secondHemisphereCenter));
        EXPECT_NEAR(capsule.GetRadius(), radius, AZ::Constants::Tolerance);
    }

    TEST(MATH_Capsule, GetCenter)
    {
        const AZ::Vector3 firstHemisphereCenter(2.0f, 4.0f, 1.0f);
        const AZ::Vector3 secondHemisphereCenter(6.0f, 6.0f, -3.0f);
        const float radius = 1.0f;
        AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        EXPECT_THAT(capsule.GetCenter(), IsClose(AZ::Vector3(4.0f, 5.0f, -1.0f)));
    }

    TEST(MATH_Capsule, GetCylinderHeight)
    {
        const AZ::Vector3 firstHemisphereCenter(1.0f, -1.0f, 4.0f);
        const AZ::Vector3 secondHemisphereCenter(3.0f, 2.0f, -2.0f);
        const float radius = 1.0f;
        AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        EXPECT_NEAR(capsule.GetCylinderHeight(), 7.0f, AZ::Constants::Tolerance);
    }

    TEST(MATH_Capsule, GetTotalHeight)
    {
        const AZ::Vector3 firstHemisphereCenter(-1.0f, -2.0f, 4.0f);
        const AZ::Vector3 secondHemisphereCenter(-3.0f, 4.0f, -5.0f);
        const float radius = 2.0f;
        AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        EXPECT_NEAR(capsule.GetTotalHeight(), 15.0f, AZ::Constants::Tolerance);
    }

    TEST(MATH_Capsule, IsCloseIdenticalCapsules)
    {
        const AZ::Vector3 firstHemisphereCenter(2.0f, -1.0f, 2.0f);
        const AZ::Vector3 secondHemisphereCenter(-1.0f, -4.0f, 3.0f);
        const float radius = 2.0f;
        AZ::Capsule capsuleA(firstHemisphereCenter, secondHemisphereCenter, radius);
        AZ::Capsule capsuleB(firstHemisphereCenter, secondHemisphereCenter, radius);
        EXPECT_TRUE(capsuleA.IsClose(capsuleB));
    }

    TEST(MATH_Capsule, IsCloseReversedEnds)
    {
        const AZ::Vector3 firstHemisphereCenter(2.0f, -1.0f, 2.0f);
        const AZ::Vector3 secondHemisphereCenter(-1.0f, -4.0f, 3.0f);
        const float radius = 2.0f;
        AZ::Capsule capsuleA(firstHemisphereCenter, secondHemisphereCenter, radius);
        AZ::Capsule capsuleB(secondHemisphereCenter, firstHemisphereCenter, radius);
        EXPECT_TRUE(capsuleA.IsClose(capsuleB));
    }

    TEST(MATH_Capsule, IsCloseDifferentRadius)
    {
        const AZ::Vector3 firstHemisphereCenter(2.0f, -1.0f, 2.0f);
        const AZ::Vector3 secondHemisphereCenter(-1.0f, -4.0f, 3.0f);
        const float radiusA = 2.0f;
        const float radiusB = 1.5f;
        AZ::Capsule capsuleA(firstHemisphereCenter, secondHemisphereCenter, radiusA);
        AZ::Capsule capsuleB(firstHemisphereCenter, secondHemisphereCenter, radiusB);
        EXPECT_FALSE(capsuleA.IsClose(capsuleB));
    }

    TEST(MATH_Capsule, IsCloseDifferentCenter)
    {
        const AZ::Vector3 firstHemisphereCenterA(2.0f, -1.0f, 2.0f);
        const AZ::Vector3 firstHemisphereCenterB(3.0f, -1.0f, 2.0f);
        const AZ::Vector3 secondHemisphereCenter(-1.0f, -4.0f, 3.0f);
        const float radius = 2.0f;
        AZ::Capsule capsuleA(firstHemisphereCenterA, secondHemisphereCenter, radius);
        AZ::Capsule capsuleB(firstHemisphereCenterB, secondHemisphereCenter, radius);
        EXPECT_FALSE(capsuleA.IsClose(capsuleB));
    }

    TEST(MATH_Capsule, IsCloseAllDifferent)
    {
        const AZ::Vector3 firstHemisphereCenterA(2.0f, -1.0f, 2.0f);
        const AZ::Vector3 firstHemisphereCenterB(3.0f, -1.0f, 2.0f);
        const AZ::Vector3 secondHemisphereCenterA(-1.0f, -4.0f, 3.0f);
        const AZ::Vector3 secondHemisphereCenterB(-2.0f, -4.0f, 3.0f);
        const float radiusA = 1.0f;
        const float radiusB = 3.0f;
        AZ::Capsule capsuleA(firstHemisphereCenterA, secondHemisphereCenterA, radiusA);
        AZ::Capsule capsuleB(firstHemisphereCenterB, secondHemisphereCenterB, radiusB);
        EXPECT_FALSE(capsuleA.IsClose(capsuleB));
    }

    TEST(MATH_Capsule, LineSegmentConstructor)
    {
        const AZ::Vector3 start(1.0f, 2.0f, 3.0f);
        const AZ::Vector3 end(2.0f, 3.0f, 4.0f);
        const AZ::LineSegment lineSegment(start, end);
        const float radius = 2.0f;
        const AZ::Capsule capsuleFromLineSegment(lineSegment, radius);
        const AZ::Capsule capsule(start, end, radius);
        EXPECT_THAT(capsuleFromLineSegment, IsClose(capsule));
    }
} // namespace UnitTest
