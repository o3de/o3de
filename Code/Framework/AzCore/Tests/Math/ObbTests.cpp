/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest::ObbTests
{
    const Vector3 position(1.0f, 2.0f, 3.0f);
    const Quaternion rotation = Quaternion::CreateRotationZ(Constants::QuarterPi);
    const Vector3 halfLengths(0.5f);

    TEST(MATH_Obb, TestCreateFromPositionRotationAndHalfLengths)
    {
        const Obb obb = Obb::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        EXPECT_THAT(obb.GetPosition(), IsClose(position));
        EXPECT_THAT(obb.GetRotation(), IsClose(rotation));
        EXPECT_THAT(obb.GetHalfLengths(), IsClose(halfLengths));
    }

    TEST(MATH_Obb, TestCreateFromAabb)
    {
        const Vector3 min(-100.0f, 50.0f, 0.0f);
        const Vector3 max(120.0f, 300.0f, 50.0f);
        const Aabb aabb = Aabb::CreateFromMinMax(min, max);
        const Obb obb = Obb::CreateFromAabb(aabb);
        EXPECT_THAT(obb.GetPosition(), IsClose(Vector3(10.0f, 175.0f, 25.0f)));
        EXPECT_THAT(obb.GetAxisX(), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(obb.GetAxisY(), IsClose(Vector3(0.0f, 1.0f, 0.0f)));
        EXPECT_THAT(obb.GetAxisZ(), IsClose(Vector3(0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Obb, TestTransform)
    {
        Obb obb = Obb::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        Transform transform = Transform::CreateRotationY(DegToRad(90.0f));
        obb = transform * obb;
        EXPECT_THAT(obb.GetPosition(), IsClose(Vector3(3.0f, 2.0f, -1.0f)));
        EXPECT_THAT(obb.GetAxisX(), IsClose(Vector3(0.0f, 0.707f, -0.707f)));
        EXPECT_THAT(obb.GetAxisY(), IsClose(Vector3(0.0f, 0.707f, 0.707f)));
        EXPECT_THAT(obb.GetAxisZ(), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Obb, TestScaleTransform)
    {
        Obb obb = Obb::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        float scale = 3.0f;
        Transform transform = Transform::CreateUniformScale(scale);
        obb = transform * obb;
        EXPECT_THAT(obb.GetPosition(), IsClose(Vector3(3.0f, 6.0f, 9.0f)));
        EXPECT_THAT(obb.GetHalfLengths(), IsClose(Vector3(1.5f, 1.5f, 1.5f)));
    }

    TEST(MATH_Obb, TestSetPosition)
    {
        Obb obb;
        obb.SetPosition(position);
        EXPECT_THAT(obb.GetPosition(), IsClose(position));
    }

    TEST(MATH_Obb, TestSetHalfLengthX)
    {
        Obb obb;
        obb.SetHalfLengthX(2.0f);
        EXPECT_NEAR(obb.GetHalfLengthX(), 2.0f, AZ::Constants::Tolerance);
    }

    TEST(MATH_Obb, TestSetHalfLengthY)
    {
        Obb obb;
        obb.SetHalfLengthY(3.0f);
        EXPECT_NEAR(obb.GetHalfLengthY(), 3.0f, AZ::Constants::Tolerance);
    }

    TEST(MATH_Obb, TestSetHalfLengthZ)
    {
        Obb obb;
        obb.SetHalfLengthZ(4.0f);
        EXPECT_NEAR(obb.GetHalfLengthZ(), 4.0f, AZ::Constants::Tolerance);
    }

    TEST(MATH_Obb, TestSetHalfLengthIndex)
    {
        Obb obb;
        obb.SetHalfLength(2, 5.0f);
        EXPECT_NEAR(obb.GetHalfLength(2), 5.0f, AZ::Constants::Tolerance);
    }

    TEST(MATH_Obb, TestIsFinite)
    {
        Obb obb = Obb::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        EXPECT_TRUE(obb.IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        const Vector3 infiniteV3 = Vector3(infinity);
        // Test to make sure that setting properties of the bounding box
        // properly mark it as infinite, and when reset it becomes finite again.
        obb.SetPosition(infiniteV3);
        EXPECT_FALSE(obb.IsFinite());
        obb.SetPosition(position);
        EXPECT_TRUE(obb.IsFinite());
        obb.SetHalfLengthY(infinity);
        EXPECT_FALSE(obb.IsFinite());
    }

    TEST(MATH_Obb, Contains)
    {
        const Vector3 testPosition(1.0f, 2.0f, 3.0f);
        const Quaternion testRotation = Quaternion::CreateRotationZ(DegToRad(30.0f));
        const Vector3 testHalfLengths(2.0f, 1.0f, 2.5f);
        const Obb obb = Obb::CreateFromPositionRotationAndHalfLengths(testPosition, testRotation, testHalfLengths);
        // test some pairs of points which should be just either side of the Obb boundary
        EXPECT_TRUE(obb.Contains(Vector3(1.35f, 3.35f, 3.5f)));
        EXPECT_FALSE(obb.Contains(Vector3(1.35f, 3.4f, 3.5f)));
        EXPECT_TRUE(obb.Contains(Vector3(-1.1f, 1.7f, 2.0f)));
        EXPECT_FALSE(obb.Contains(Vector3(-1.2f, 1.7f, 2.0f)));
        EXPECT_TRUE(obb.Contains(Vector3(1.7f, 1.8f, 5.45f)));
        EXPECT_FALSE(obb.Contains(Vector3(1.7f, 1.8f, 5.55f)));
    }

    TEST(MATH_Obb, GetDistance)
    {
        const Vector3 testPosition(5.0f, 3.0f, 2.0f);
        const Quaternion testRotation = Quaternion::CreateRotationX(DegToRad(60.0f));
        const Vector3 testHalfLengths(0.5f, 2.0f, 1.5f);
        const Obb obb = Obb::CreateFromPositionRotationAndHalfLengths(testPosition, testRotation, testHalfLengths);
        EXPECT_NEAR(obb.GetDistance(Vector3(5.3f, 3.2f, 1.8f)), 0.0f, 1e-3f);
        EXPECT_NEAR(obb.GetDistance(Vector3(5.1f, 1.1f, 3.7f)), 0.9955f, 1e-3f);
        EXPECT_NEAR(obb.GetDistance(Vector3(4.7f, 4.5f, 4.2f)), 0.6553f, 1e-3f);
        EXPECT_NEAR(obb.GetDistance(Vector3(3.9f, 3.6f, -3.0f)), 2.6059f, 1e-3f);
    }

    TEST(MATH_Obb, GetDistanceSq)
    {
        const Vector3 testPosition(1.0f, 4.0f, 3.0f);
        const Quaternion testRotation = Quaternion::CreateRotationY(DegToRad(45.0f));
        const Vector3 testHalfLengths(1.5f, 3.0f, 1.0f);
        const Obb obb = Obb::CreateFromPositionRotationAndHalfLengths(testPosition, testRotation, testHalfLengths);
        EXPECT_NEAR(obb.GetDistanceSq(Vector3(1.1f, 4.3f, 2.7f)), 0.0f, 1e-3f);
        EXPECT_NEAR(obb.GetDistanceSq(Vector3(-0.7f, 3.5f, 2.0f)), 0.8266f, 1e-3f);
        EXPECT_NEAR(obb.GetDistanceSq(Vector3(2.4f, 0.5f, 1.5f)), 0.5532f, 1e-3f);
        EXPECT_NEAR(obb.GetDistanceSq(Vector3(1.1f, 7.3f, 5.8f)), 1.3612f, 1e-3f);
    }
} // namespace UnitTest::ObbTests
