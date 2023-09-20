/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Math/SimdMath.h>
#include <Math/MathTest.h>

namespace UnitTest
{
    TEST(MATH_Vector4, TestConstructors)
    {
        AZ::Vector4 v1(0.0f);
        EXPECT_FLOAT_EQ(v1.GetX(), 0.0f);
        EXPECT_FLOAT_EQ(v1.GetY(), 0.0f);
        EXPECT_FLOAT_EQ(v1.GetZ(), 0.0f);
        EXPECT_FLOAT_EQ(v1.GetW(), 0.0f);
        AZ::Vector4 v2(5.0f);
        EXPECT_FLOAT_EQ(v2.GetX(), 5.0f);
        EXPECT_FLOAT_EQ(v2.GetY(), 5.0f);
        EXPECT_FLOAT_EQ(v2.GetZ(), 5.0f);
        EXPECT_FLOAT_EQ(v2.GetW(), 5.0f);
        AZ::Vector4 v3(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_FLOAT_EQ(v3.GetX(), 1.0f);
        EXPECT_FLOAT_EQ(v3.GetY(), 2.0f);
        EXPECT_FLOAT_EQ(v3.GetZ(), 3.0f);
        EXPECT_FLOAT_EQ(v3.GetW(), 4.0f);
        EXPECT_THAT(AZ::Vector4(AZ::Vector3(10.0f, 3.0f, 2.0f)), IsClose(AZ::Vector4(10.0f, 3.0f, 2.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector4(AZ::Vector3(10.0f, 3.0f, 2.0f), 5.0f), IsClose(AZ::Vector4(10.0f, 3.0f, 2.0f, 5.0f)));
        EXPECT_THAT(AZ::Vector4(AZ::Vector2(10.0f, 3.0f)), IsClose(AZ::Vector4(10.0f, 3.0f, 0.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector4(AZ::Vector2(10.0f, 3.0f), 5.0f), IsClose(AZ::Vector4(10.0f, 3.0f, 5.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector4(AZ::Vector2(10.0f, 3.0f), 5.0f, 6.0f), IsClose(AZ::Vector4(10.0f, 3.0f, 5.0f, 6.0f)));
    }

    TEST(MATH_Vector4, TestCreateFrom)
    {
        const float values[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
        AZ::Vector4 v4 = AZ::Vector4::CreateFromFloat4(values);
        EXPECT_THAT(v4, IsClose(AZ::Vector4(10.0f, 20.0f, 30.0f, 40.0f)));
        AZ::Vector4 v5 = AZ::Vector4::CreateFromVector3(AZ::Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(v5, IsClose(AZ::Vector4(2.0f, 3.0f, 4.0f, 1.0f)));
        AZ::Vector4 v6 = AZ::Vector4::CreateFromVector3AndFloat(AZ::Vector3(2.0f, 3.0f, 4.0f), 5.0f);
        EXPECT_THAT(v6, IsClose(AZ::Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
    }

    TEST(MATH_Vector4, TestCreate)
    {
        EXPECT_THAT(AZ::Vector4::CreateOne(), IsClose(AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector4::CreateZero(), IsClose(AZ::Vector4(0.0f)));
    }

    TEST(MATH_Vector4, TestCreateAxis)
    {
        EXPECT_THAT(AZ::Vector4::CreateAxisX(), IsClose(AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector4::CreateAxisY(), IsClose(AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector4::CreateAxisZ(), IsClose(AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector4::CreateAxisW(), IsClose(AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Vector4, TestCompareEqual)
    {
        AZ::Vector4 vA(-100.0f, 10.0f, -1.0f, 0.0f);
        AZ::Vector4 vB(35.0f, 10.0f, -5.0f, 0.0f);
        AZ::Vector4 vC(35.0f, 20.0f, -1.0f, 0.0f);

        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        AZ::Vector4 compareEqualAB = AZ::Vector4::CreateSelectCmpEqual(vA, vB, AZ::Vector4(1.0f), AZ::Vector4(0.0f));
        EXPECT_THAT(compareEqualAB, IsClose(AZ::Vector4(0.0f, 1.0f, 0.0f, 1.0f)));
        AZ::Vector4 compareEqualBC = AZ::Vector4::CreateSelectCmpEqual(vB, vC, AZ::Vector4(1.0f), AZ::Vector4(0.0f));
        EXPECT_THAT(compareEqualBC, IsClose(AZ::Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Vector4, TestCompareGreaterEqual)
    {
        AZ::Vector4 vA(-100.0f, 10.0f, -1.0f, 0.0f);
        AZ::Vector4 vB(35.0f, 10.0f, -5.0f, 0.0f);
        AZ::Vector4 vD(15.0f, 30.0f, 45.0f, 0.0f);

        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        AZ::Vector4 compareGreaterEqualAB = AZ::Vector4::CreateSelectCmpGreaterEqual(vA, vB, AZ::Vector4(1.0f), AZ::Vector4(0.0f));
        EXPECT_THAT(compareGreaterEqualAB, IsClose(AZ::Vector4(0.0f, 1.0f, 1.0f, 1.0f)));
        AZ::Vector4 compareGreaterEqualBD = AZ::Vector4::CreateSelectCmpGreaterEqual(vB, vD, AZ::Vector4(1.0f), AZ::Vector4(0.0f));
        EXPECT_THAT(compareGreaterEqualBD, IsClose(AZ::Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Vector4, TestCompareGreater)
    {
        AZ::Vector4 vA(-100.0f, 10.0f, -1.0f, 0.0f);
        AZ::Vector4 vB(35.0f, 10.0f, -5.0f, 0.0f);
        AZ::Vector4 vC(35.0f, 20.0f, -1.0f, 0.0f);

        // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        AZ::Vector4 compareGreaterAB = AZ::Vector4::CreateSelectCmpGreater(vA, vB, AZ::Vector4(1.0f), AZ::Vector4(0.0f));
        EXPECT_THAT(compareGreaterAB, IsClose(AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        AZ::Vector4 compareGreaterCA = AZ::Vector4::CreateSelectCmpGreater(vC, vA, AZ::Vector4(1.0f), AZ::Vector4(0.0f));
        EXPECT_THAT(compareGreaterCA, IsClose(AZ::Vector4(1.0f, 1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestGetSet)
    {
        AZ::Vector4 v1(2.0f, 3.0f, 4.0f, 5.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        v1.SetX(10.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(10.0f, 3.0f, 4.0f, 5.0f)));
        v1.SetY(11.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(10.0f, 11.0f, 4.0f, 5.0f)));
        v1.SetZ(12.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(10.0f, 11.0f, 12.0f, 5.0f)));
        v1.SetW(13.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(10.0f, 11.0f, 12.0f, 13.0f)));
        v1.Set(15.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(15.0f)));
        const float values[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
        v1.Set(values);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(10.0f, 20.0f, 30.0f, 40.0f)));
        v1.Set(AZ::Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(v1, IsClose(AZ::Vector4(2.0f, 3.0f, 4.0f, 1.0f)));
        v1.Set(AZ::Vector3(2.0f, 3.0f, 4.0f), 5.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
    }

    TEST(MATH_Vector4, TestIndexOperators)
    {
        AZ::Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(0), 1.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(1), 2.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(2), 3.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(3), 4.0f);
        EXPECT_FLOAT_EQ(v1(0), 1.0f);
        EXPECT_FLOAT_EQ(v1(1), 2.0f);
        EXPECT_FLOAT_EQ(v1(2), 3.0f);
        EXPECT_FLOAT_EQ(v1(3), 4.0f);
    }

    TEST(MATH_Vector4, TestGetElementSetElement)
    {
        AZ::Vector4 v1;
        v1.SetElement(0, 5.0f);
        v1.SetElement(1, 6.0f);
        v1.SetElement(2, 7.0f);
        v1.SetElement(3, 8.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(0), 5.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(1), 6.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(2), 7.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(3), 8.0f);
    }

    TEST(MATH_Vector4, TestEquality)
    {
        AZ::Vector4 v3(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(v3 == AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0));
        EXPECT_FALSE((v3 == AZ::Vector4(1.0f, 2.0f, 3.0f, 5.0f)));
        EXPECT_TRUE(v3 != AZ::Vector4(1.0f, 2.0f, 3.0f, 5.0f));
        EXPECT_FALSE((v3 != AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
    }

    TEST(MATH_Vector4, TestGetLength)
    {
        EXPECT_FLOAT_EQ(AZ::Vector4(0.0f, 3.0f, 4.0f, 0.0f).GetLengthSq(), 25.0f);
        EXPECT_FLOAT_EQ(AZ::Vector4(0.0f, 0.0f, 4.0f, -3.0f).GetLength(), 5.0f);
        EXPECT_FLOAT_EQ(AZ::Vector4(0.0f, 0.0f, 4.0f, -3.0f).GetLengthEstimate(), 5.0f);
    }

    TEST(MATH_Vector4, TestGetLengthReciprocal)
    {
        EXPECT_NEAR(AZ::Vector4(0.0f, 0.0f, 4.0f, -3.0f).GetLengthReciprocal(), 0.2f, 0.01f);
        EXPECT_NEAR(AZ::Vector4(0.0f, 0.0f, 4.0f, -3.0f).GetLengthReciprocalEstimate(), 0.2f, 0.01f);
    }

    TEST(MATH_Vector4, TestGetNormalized)
    {
        EXPECT_THAT(AZ::Vector4(3.0f, 0.0f, 4.0f, 0.0f).GetNormalized(), IsCloseTolerance(AZ::Vector4(3.0f / 5.0f, 0.0f, 4.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector4(3.0f, 0.0f, 4.0f, 0.0f).GetNormalizedEstimate(), IsCloseTolerance(AZ::Vector4(3.0f / 5.0f, 0.0f, 4.0f / 5.0f, 0.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector4, TestGetNormalizedSafe)
    {
        EXPECT_THAT(AZ::Vector4(3.0f, 0.0f, 4.0f, 0.0f).GetNormalizedSafe(), IsCloseTolerance(AZ::Vector4(3.0f / 5.0f, 0.0f, 4.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector4(3.0f, 0.0f, 4.0f, 0.0f).GetNormalizedSafeEstimate(), IsCloseTolerance(AZ::Vector4(3.0f / 5.0f, 0.0f, 4.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector4(0.0f).GetNormalizedSafe(), IsClose(AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector4(0.0f).GetNormalizedSafeEstimate(), IsClose(AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestNormalize)
    {
        AZ::Vector4 v1(4.0f, 3.0f, 0.0f, 0.0f);
        v1.Normalize();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(4.0f / 5.0f, 3.0f / 5.0f, 0.0f, 0.0f), Constants::SimdTolerance));
        v1.Set(4.0f, 3.0f, 0.0f, 0.0f);
        v1.NormalizeEstimate();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(4.0f / 5.0f, 3.0f / 5.0f, 0.0f, 0.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector4, TestNormalizeWithLength)
    {
        AZ::Vector4 v1(4.0f, 3.0f, 0.0f, 0.0f);
        float length = v1.NormalizeWithLength();
        ASSERT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(4.0f / 5.0f, 3.0f / 5.0f, 0.0f, 0.0f), Constants::SimdTolerance));
        v1.Set(4.0f, 3.0f, 0.0f, 0.0f);
        length = v1.NormalizeWithLengthEstimate();
        ASSERT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(4.0f / 5.0f, 3.0f / 5.0f, 0.0f, 0.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector4, TestNormalizeSafe)
    {
        AZ::Vector4 v1(0.0f, 3.0f, 4.0f, 0.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(0.0f, 3.0f / 5.0f, 4.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsClose(AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f)));
        v1.Set(0.0f, 3.0f, 4.0f, 0.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(0.0f, 3.0f / 5.0f, 4.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsClose(AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestNormalizeSafeWithLength)
    {
        AZ::Vector4 v1(0.0f, 3.0f, 4.0f, 0.0f);
        float length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(0.0f, 3.0f / 5.0f, 4.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f)));
        v1.Set(0.0f, 3.0f, 4.0f, 0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(0.0f, 3.0f / 5.0f, 4.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestIsNormalized)
    {
        EXPECT_TRUE(AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f).IsNormalized());
        EXPECT_TRUE(AZ::Vector4(0.7071f, 0.7071f, 0.0f, 0.0f).IsNormalized());
        EXPECT_FALSE(AZ::Vector4(1.0f, 1.0f, 0.0f, 0.0f).IsNormalized());
    }

    TEST(MATH_Vector4, TestSetLength)
    {
        AZ::Vector4 v1(3.0f, 4.0f, 0.0f, 0.0f);
        v1.SetLength(10.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(6.0f, 8.0f, 0.0f, 0.0f)));
        v1.Set(3.0f, 4.0f, 0.0f, 0.0f);
        v1.SetLengthEstimate(10.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector4(6.0f, 8.0f, 0.0f, 0.0f), Constants::SimdToleranceEstimateFuncs));
    }

    TEST(MATH_Vector4, TestDistance)
    {
        AZ::Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_FLOAT_EQ(v1.GetDistanceSq(AZ::Vector4(-2.0f, 6.0f, 3.0f, 4.0f)), 25.0f);
        EXPECT_FLOAT_EQ(v1.GetDistance(AZ::Vector4(-2.0f, 2.0f, -1.0f, 4.0f)), 5.0f);
        EXPECT_FLOAT_EQ(v1.GetDistanceEstimate(AZ::Vector4(-2.0f, 2.0f, -1.0f, 4.0f)), 5.0f);
    }

    TEST(MATH_Vector4, TestIsLessThan)
    {
        EXPECT_TRUE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(AZ::Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        EXPECT_FALSE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(AZ::Vector4(0.0f, 3.0f, 4.0f, 5.0f)));
        EXPECT_FALSE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(AZ::Vector4(1.0f, 2.0f, 4.0f, 5.0f)));
    }

    TEST(MATH_Vector4, TestIsLessEqualThan)
    {
        EXPECT_TRUE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(AZ::Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        EXPECT_FALSE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(AZ::Vector4(0.0f, 3.0f, 4.0f, 5.0f)));
        EXPECT_TRUE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(AZ::Vector4(2.0f, 2.0f, 4.0f, 5.0f)));
    }

    TEST(MATH_Vector4, TestIsGreaterThan)
    {
        EXPECT_TRUE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(AZ::Vector4(0.0f, 1.0f, 2.0f, 3.0f)));
        EXPECT_FALSE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(AZ::Vector4(0.0f, 3.0f, 2.0f, 3.0f)));
        EXPECT_FALSE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(AZ::Vector4(0.0f, 2.0f, 2.0f, 3.0f)));
    }

    TEST(MATH_Vector4, TestIsGreaterEqualThan)
    {
        EXPECT_TRUE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(AZ::Vector4(0.0f, 1.0f, 2.0f, 3.0f)));
        EXPECT_FALSE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(AZ::Vector4(0.0f, 3.0f, 2.0f, 3.0f)));
        EXPECT_TRUE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(AZ::Vector4(0.0f, 2.0f, 2.0f, 3.0f)));
    }

    TEST(MATH_Vector4, TestLerpSlerpNLerp)
    {
        EXPECT_THAT(AZ::Vector4(4.0f, 5.0f, 6.0f, 7.0f).Lerp(AZ::Vector4(5.0f, 10.0f, 2.0f, 1.0f), 0.5f), IsClose(AZ::Vector4(4.5f, 7.5f, 4.0f, 4.0f)));
        EXPECT_THAT(AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f).Slerp(AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f), 0.5f), IsClose(AZ::Vector4(0.7071f, 0.7071f, 0.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f).Nlerp(AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f), 0.5f), IsClose(AZ::Vector4(0.7071f, 0.7071f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestDot)
    {
        EXPECT_FLOAT_EQ(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).Dot(AZ::Vector4(-1.0f, 5.0f, 3.0f, 2.0f)), 26.0f);
        EXPECT_FLOAT_EQ(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).Dot3(AZ::Vector3(-1.0f, 5.0f, 3.0f)), 18.0f);
    }

    TEST(MATH_Vector4, TestIsClose)
    {
        EXPECT_TRUE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        EXPECT_FALSE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Vector4(1.0f, 2.0f, 3.0f, 5.0f)));
        EXPECT_TRUE(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.4f), 0.5f));
    }

    TEST(MATH_Vector4, TestHomogenize)
    {
        AZ::Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_THAT(v1.GetHomogenized(), IsClose(AZ::Vector3(0.25f, 0.5f, 0.75f)));
        v1.Homogenize();
        EXPECT_THAT(v1, IsClose(AZ::Vector4(0.25f, 0.5f, 0.75f, 1.0f)));
    }

    TEST(MATH_Vector4, TestMinMax)
    {
        EXPECT_THAT(AZ::Vector4(2.0f, 5.0f, 6.0f, 7.0f).GetMin(AZ::Vector4(1.0f, 6.0f, 5.0f, 4.0f)), IsClose(AZ::Vector4(1.0f, 5.0f, 5.0f, 4.0f)));
        EXPECT_THAT(AZ::Vector4(2.0f, 5.0f, 6.0f, 7.0f).GetMax(AZ::Vector4(1.0f, 6.0f, 5.0f, 4.0f)), IsClose(AZ::Vector4(2.0f, 6.0f, 6.0f, 7.0f)));
    }

    TEST(MATH_Vector4, TestClamp)
    {
        EXPECT_THAT(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f).GetClamp(AZ::Vector4(0.0f, -1.0f, 4.0f, 4.0f), AZ::Vector4(2.0f, 1.0f, 10.0f, 4.0f)), IsClose(AZ::Vector4(1.0f, 1.0f, 4.0f, 4.0f)));
    }

    TEST(MATH_Vector4, TestTrig)
    {
        EXPECT_THAT(AZ::Vector4(AZ::DegToRad( 78.0f), AZ::DegToRad(-150.0f), AZ::DegToRad( 190.0f), AZ::DegToRad( 78.0f)).GetAngleMod(), IsClose(AZ::Vector4(AZ::DegToRad(78.0f), AZ::DegToRad(-150.0f), AZ::DegToRad(-170.0f), AZ::DegToRad(78.0f))));
        EXPECT_THAT(AZ::Vector4(AZ::DegToRad(390.0f), AZ::DegToRad(-190.0f), AZ::DegToRad(-400.0f), AZ::DegToRad(390.0f)).GetAngleMod(), IsClose(AZ::Vector4(AZ::DegToRad(30.0f), AZ::DegToRad(170.0f), AZ::DegToRad(-40.0f), AZ::DegToRad(30.0f))));
        EXPECT_THAT(AZ::Vector4(AZ::DegToRad( 60.0f), AZ::DegToRad( 105.0f), AZ::DegToRad(-174.0f), AZ::DegToRad( 60.0f)).GetSin(), IsCloseTolerance(AZ::Vector4(0.866f, 0.966f, -0.105f, 0.866f), 0.005f));
        EXPECT_THAT(AZ::Vector4(AZ::DegToRad( 60.0f), AZ::DegToRad( 105.0f), AZ::DegToRad(-174.0f), AZ::DegToRad( 60.0f)).GetCos(), IsCloseTolerance(AZ::Vector4(0.5f, -0.259f, -0.995f, 0.5f), 0.005f));
        AZ::Vector4 sin, cos;
        AZ::Vector4 v1(AZ::DegToRad(60.0f), AZ::DegToRad(105.0f), AZ::DegToRad(-174.0f), AZ::DegToRad(60.0f));
        v1.GetSinCos(sin, cos);
        EXPECT_THAT(sin, IsCloseTolerance(AZ::Vector4(0.866f, 0.966f, -0.105f, 0.866f), 0.005f));
        EXPECT_THAT(cos, IsCloseTolerance(AZ::Vector4(0.5f, -0.259f, -0.995f, 0.5f), 0.005f));
    }

    TEST(MATH_Vector4, TestAbs)
    {
        EXPECT_THAT(AZ::Vector4(-1.0f, 2.0f, -5.0f, 1.0f).GetAbs(), IsClose(AZ::Vector4(1.0f, 2.0f, 5.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector4(1.0f, -2.0f, 5.0f, -1.0f).GetAbs(), IsClose(AZ::Vector4(1.0f, 2.0f, 5.0f, 1.0f)));
    }

    TEST(MATH_Vector4, TestReciprocal)
    {
        EXPECT_THAT(AZ::Vector4(2.0f, 4.0f, 5.0f, 10.0f).GetReciprocal(), IsClose(AZ::Vector4(0.5f, 0.25f, 0.2f, 0.1f)));
        EXPECT_THAT(AZ::Vector4(2.0f, 4.0f, 5.0f, 10.0f).GetReciprocalEstimate(), IsCloseTolerance(AZ::Vector4(0.5f, 0.25f, 0.2f, 0.1f), 1e-3f));
    }

    TEST(MATH_Vector4, TestNegate)
    {
        EXPECT_THAT((-AZ::Vector4(1.0f, 2.0f, -3.0f, -1.0f)), IsClose(AZ::Vector4(-1.0f, -2.0f, 3.0f, 1.0f)));
    }

    TEST(MATH_Vector4, TestAdd)
    {
        EXPECT_THAT((AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f) + AZ::Vector4(-1.0f, 4.0f, 5.0f, 2.0f)), IsClose(AZ::Vector4(0.0f, 6.0f, 8.0f, 6.0f)));

        AZ::Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += AZ::Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(6.0f, 5.0f, 2.0f, 6.0f)));
    }

    TEST(MATH_Vector4, TestSub)
    {
        EXPECT_THAT((AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f) - AZ::Vector4(-1.0f, 4.0f, 5.0f, 2.0f)), IsClose(AZ::Vector4(2.0f, -2.0f, -2.0f, 2.0f)));

        AZ::Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += AZ::Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        v1 -= AZ::Vector4(2.0f, -1.0f, 3.0f, 1.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector4(4.0f, 6.0f, -1.0f, 5.0f)));
    }

    TEST(MATH_Vector4, TestMultiply)
    {
        EXPECT_THAT((AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f) * AZ::Vector4(-1.0f, 4.0f, 5.0f, 2.0f)), IsClose(AZ::Vector4(-1.0f, 8.0f, 15.0f, 8.0f)));
        EXPECT_THAT((AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f), IsClose(AZ::Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT((2.0f * AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f)), IsClose(AZ::Vector4(2.0f, 4.0f, 6.0f, 8.0f)));

        AZ::Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += AZ::Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        v1 -= AZ::Vector4(2.0f, -1.0f, 3.0f, 1.0f);
        v1 *= 3.0f;
        EXPECT_THAT(v1, IsClose(AZ::Vector4(12.0f, 18.0f, -3.0f, 15.0f)));
    }

    TEST(MATH_Vector4, TestDivide)
    {
        EXPECT_THAT((AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f) / AZ::Vector4(-1.0f, 4.0f, 5.0f, 2.0f)), IsClose(AZ::Vector4(-1.0f, 0.5f, 3.0f / 5.0f, 2.0f)));
        EXPECT_THAT((AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f) / 2.0f), IsClose(AZ::Vector4(0.5f, 1.0f, 1.5f, 2.0f)));

        AZ::Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += AZ::Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        v1 -= AZ::Vector4(2.0f, -1.0f, 3.0f, 1.0f);
        v1 *= 3.0f;
        v1 /= 2.0f;
        EXPECT_THAT(v1, IsClose(AZ::Vector4(6.0f, 9.0f, -1.5f, 7.5f)));
    }

    struct Vector4AngleTestArgs
    {
        AZ::Vector4 current;
        AZ::Vector4 target;
        float angle;
    };

    using Vector4AngleTestFixture = ::testing::TestWithParam<Vector4AngleTestArgs>;

    TEST_P(Vector4AngleTestFixture, TestAngle)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.Angle(param.target), param.angle, Constants::SimdTolerance);
    }

    TEST_P(Vector4AngleTestFixture, TestAngleSafe)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleSafe(param.target), param.angle, Constants::SimdTolerance);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector4,
        Vector4AngleTestFixture,
        ::testing::Values(
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, AZ::Constants::HalfPi },
            Vector4AngleTestArgs{ AZ::Vector4{ 42.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 23.0f, 0.0f, 0.0f }, AZ::Constants::HalfPi },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ -1.0f, 0.0f, 0.0f, 0.0f }, AZ::Constants::Pi },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 1.0f, 1.0f, 0.0f, 0.0f }, AZ::Constants::QuarterPi },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, 0.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 1.0f, 0.0f, 0.0f }, AZ::Vector4{ -1.0f, -1.0f, 0.0f, 0.0f }, AZ::Constants::Pi }));

    using Vector4AngleDegTestFixture = ::testing::TestWithParam<Vector4AngleTestArgs>;

    TEST_P(Vector4AngleDegTestFixture, TestAngleDeg)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleDeg(param.target), param.angle, Constants::SimdToleranceAngleDeg);
    }

    TEST_P(Vector4AngleDegTestFixture, TestAngleDegSafe)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleSafeDeg(param.target), param.angle, Constants::SimdToleranceAngleDeg);
    }
    INSTANTIATE_TEST_CASE_P(
        MATH_Vector4,
        Vector4AngleDegTestFixture,
        ::testing::Values(
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, 90.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 42.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 23.0f, 0.0f, 0.0f }, 90.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ -1.0f, 0.0f, 0.0f, 0.0f }, 180.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 1.0f, 1.0f, 0.0f, 0.0f }, 45.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, 0.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 1.0f, 0.0f, 0.0f }, AZ::Vector4{ -1.0f, -1.0f, 0.0f, 0.0f }, 180.f }));

    using AngleSafeInvalidVector4AngleTestFixture = ::testing::TestWithParam<Vector4AngleTestArgs>;
    TEST_P(AngleSafeInvalidVector4AngleTestFixture, TestInvalidAngle)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafe(param.target), param.angle);
    }

    TEST_P(AngleSafeInvalidVector4AngleTestFixture, TestInvalidAngleDeg)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafeDeg(param.target), param.angle);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector4,
        AngleSafeInvalidVector4AngleTestFixture,
        ::testing::Values(
            Vector4AngleTestArgs{ AZ::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, 0.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 323432.0f, 0.0f, 0.0f }, 0.f },
            Vector4AngleTestArgs{ AZ::Vector4{ 323432.0f, 0.0f, 0.0f, 0.0f }, AZ::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.f }));
} // namespace UnitTest
