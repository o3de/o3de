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
    TEST(MATH_Vector3, TestConstructors)
    {
        AZ::Vector3 v1(0.0f);
        EXPECT_FLOAT_EQ(v1.GetX(), 0.0f);
        EXPECT_FLOAT_EQ(v1.GetY(), 0.0f);
        EXPECT_FLOAT_EQ(v1.GetZ(), 0.0f);
        AZ::Vector3 v2(5.0f);
        EXPECT_FLOAT_EQ(v2.GetX(), 5.0f);
        EXPECT_FLOAT_EQ(v2.GetY(), 5.0f);
        EXPECT_FLOAT_EQ(v2.GetZ(), 5.0f);
        AZ::Vector3 v3(1.0f, 2.0f, 3.0f);
        EXPECT_FLOAT_EQ(v3.GetX(), 1.0f);
        EXPECT_FLOAT_EQ(v3.GetY(), 2.0f);
        EXPECT_FLOAT_EQ(v3.GetZ(), 3.0f);
        
        EXPECT_THAT(AZ::Vector3(AZ::Vector3(1.0f, 3.0f,2.0f)), IsClose(AZ::Vector3(1.0f, 3.0f,2.0f)));
        EXPECT_THAT(AZ::Vector3(AZ::Vector2(1.0f, 3.0f)), IsClose(AZ::Vector3(1.0f, 3.0f,0.0f)));
        EXPECT_THAT(AZ::Vector3(AZ::Vector2(1.0f, 3.0f), 6.0f), IsClose(AZ::Vector3(1.0f, 3.0f,6.0f)));
        EXPECT_THAT(AZ::Vector3(AZ::Vector4(1.0f, 3.0f, 5.0f, 2.0f)), IsClose(AZ::Vector3(1.0f, 3.0f, 5.0f)));
        EXPECT_THAT(AZ::Vector3(AZ::Simd::Vec3::LoadImmediate(1.0f, 2.0f, 4.0f)), IsClose(AZ::Vector3(1.0f, 2.0f, 4.0f)));

    }

    TEST(MATH_Vector3, TestCreateFunctions)
    {
        EXPECT_THAT(AZ::Vector3::CreateOne(), IsClose(AZ::Vector3(1.0f, 1.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector3::CreateZero(), IsClose(AZ::Vector3(0.0f)));
        float values[3] = { 10.0f, 20.0f, 30.0f };
        EXPECT_THAT(AZ::Vector3::CreateFromFloat3(values), IsClose(AZ::Vector3(10.0f, 20.0f, 30.0f)));
        EXPECT_THAT(AZ::Vector3::CreateAxisX(), IsClose(AZ::Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector3::CreateAxisY(), IsClose(AZ::Vector3(0.0f, 1.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector3::CreateAxisZ(), IsClose(AZ::Vector3(0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Vector3, TestCompareEqual)
    {
        AZ::Vector3 vA(-100.0f, 10.0f, -1.0f);
        AZ::Vector3 vB(35.0f, 10.0f, -5.0f);
        AZ::Vector3 vC(35.0f, 20.0f, -1.0f);

        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        AZ::Vector3 compareEqualAB = AZ::Vector3::CreateSelectCmpEqual(vA, vB, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(compareEqualAB, IsClose(AZ::Vector3(0.0f, 1.0f, 0.0f)));
        AZ::Vector3 compareEqualBC = AZ::Vector3::CreateSelectCmpEqual(vB, vC, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(compareEqualBC, IsClose(AZ::Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestCompareGreaterEqual)
    {
        AZ::Vector3 vA(-100.0f, 10.0f, -1.0f);
        AZ::Vector3 vB(35.0f, 10.0f, -5.0f);
        AZ::Vector3 vD(15.0f, 30.0f, 45.0f);

        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        AZ::Vector3 compareGreaterEqualAB = AZ::Vector3::CreateSelectCmpGreaterEqual(vA, vB, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(compareGreaterEqualAB, IsClose(AZ::Vector3(0.0f, 1.0f, 1.0f)));
        AZ::Vector3 compareGreaterEqualBD = AZ::Vector3::CreateSelectCmpGreaterEqual(vB, vD, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(compareGreaterEqualBD, IsClose(AZ::Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestCompareGreater)
    {
        AZ::Vector3 vA(-100.0f, 10.0f, -1.0f);
        AZ::Vector3 vB(35.0f, 10.0f, -5.0f);
        AZ::Vector3 vC(35.0f, 20.0f, -1.0f);

        // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        AZ::Vector3 compareGreaterAB = AZ::Vector3::CreateSelectCmpGreater(vA, vB, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(compareGreaterAB, IsClose(AZ::Vector3(0.0f, 0.0f, 1.0f)));
        AZ::Vector3 compareGreaterCA = AZ::Vector3::CreateSelectCmpGreater(vC, vA, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(compareGreaterCA, IsClose(AZ::Vector3(1.0f, 1.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestStoreFloat)
    {
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        float values[3];

        v1.StoreToFloat3(values);
        EXPECT_FLOAT_EQ(values[0], 1.0f);
        EXPECT_FLOAT_EQ(values[1], 2.0f);
        EXPECT_FLOAT_EQ(values[2], 3.0f);

        float values4[4];
        v1.StoreToFloat4(values4);
        EXPECT_FLOAT_EQ(values4[0], 1.0f);
        EXPECT_FLOAT_EQ(values4[1], 2.0f);
        EXPECT_FLOAT_EQ(values4[2], 3.0f);
        // The 4th element value cannot be guaranteed, it generally sets garbage.
    }

    TEST(MATH_Vector3, TestGetSet)
    {
        AZ::Vector3 v1(2.0f, 3.0f, 4.0f);
        const float values[3] = { 1.0f, 2.0f, 3.0f };

        EXPECT_THAT(v1, IsClose(AZ::Vector3(2.0f, 3.0f, 4.0f)));
        v1.SetX(10.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(10.0f, 3.0f, 4.0f)));
        v1.SetY(11.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(10.0f, 11.0f, 4.0f)));
        v1.SetZ(12.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(10.0f, 11.0f, 12.0f)));
        v1.Set(15.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(15.0f)));
        v1.Set(values);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(1.0f, 2.0f, 3.0f)));
    }

    TEST(MATH_Vector3, TestIndexElement)
    {
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        EXPECT_FLOAT_EQ(v1(0), 1.0f);
        EXPECT_FLOAT_EQ(v1(1), 2.0f);
        EXPECT_FLOAT_EQ(v1(2), 3.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(0), 1.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(1), 2.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(2), 3.0f);
        v1.SetElement(0, 5.0f);
        v1.SetElement(1, 6.0f);
        v1.SetElement(2, 7.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(0), 5.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(1), 6.0f);
        EXPECT_FLOAT_EQ(v1.GetElement(2), 7.0f);
    }

    TEST(MATH_Vector3, TestGetLength)
    {
        EXPECT_NEAR(AZ::Vector3(3.0f, 4.0f, 0.0f).GetLengthSq(), 25.0f, .01f);
        EXPECT_NEAR(AZ::Vector3(0.0f, 4.0f, -3.0f).GetLength(), 5.0f, .01f);
        EXPECT_NEAR(AZ::Vector3(0.0f, 4.0f, -3.0f).GetLengthEstimate(), 5.0f, .01f);
    }

    TEST(MATH_Vector3, TestGetLengthReciprocal)
    {
        EXPECT_NEAR(AZ::Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocal(), 0.2f, .01f);
        EXPECT_NEAR(AZ::Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocalEstimate(), 0.2f, .01f);
    }

    TEST(MATH_Vector3, TestGetNormalized)
    {
        EXPECT_THAT(AZ::Vector3(3.0f, 0.0f, 4.0f).GetNormalized(), IsCloseTolerance(AZ::Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector3(3.0f, 0.0f, 4.0f).GetNormalizedEstimate(), IsCloseTolerance(AZ::Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector3, TestGetNormalizedSafe)
    {
        EXPECT_THAT(AZ::Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafe(), IsCloseTolerance(AZ::Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafeEstimate(), IsCloseTolerance(AZ::Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector3(0.0f).GetNormalizedSafe(), AZ::Vector3(0.0f, 0.0f, 0.0f));
        EXPECT_THAT(AZ::Vector3(0.0f).GetNormalizedSafeEstimate(), AZ::Vector3(0.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector3, TestNormalize)
    {
        AZ::Vector3 v1(4.0f, 3.0f, 0.0f);
        v1.Normalize();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        v1.Set(4.0f, 3.0f, 0.0f);
        v1.NormalizeEstimate();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector3, TestNormalizeWithLength)
    {
        AZ::Vector3 v1(4.0f, 3.0f, 0.0f);
        float length = v1.NormalizeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f), Constants::SimdTolerance));
        v1.Set(4.0f, 3.0f, 0.0f);
        length = v1.NormalizeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector3, TestNormalizeSafe)
    {
        AZ::Vector3 v1(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f)));
        v1.Set(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestNormalizeSafeWithLength)
    {
        AZ::Vector3 v1(0.0f, 3.0f, 4.0f);
        float length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f)));
        v1.Set(0.0f, 3.0f, 4.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestIsNormalized)
    {
        EXPECT_TRUE(AZ::Vector3(1.0f, 0.0f, 0.0f).IsNormalized());
        EXPECT_TRUE(AZ::Vector3(0.7071f, 0.7071f, 0.0f).IsNormalized());
        EXPECT_FALSE(AZ::Vector3(1.0f, 1.0f, 0.0f).IsNormalized());
    }

    TEST(MATH_Vector3, TestSetLength)
    {
        AZ::Vector3 v1(3.0f, 4.0f, 0.0f);
        v1.SetLength(10.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(6.0f, 8.0f, 0.0f)));
        v1.Set(3.0f, 4.0f, 0.0f);
        v1.SetLengthEstimate(10.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector3(6.0f, 8.0f, 0.0f), Constants::SimdToleranceEstimateFuncs));
    }

    TEST(MATH_Vector3, TestDistance)
    {
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        EXPECT_FLOAT_EQ(v1.GetDistanceSq(AZ::Vector3(-2.0f, 6.0f, 3.0f)), 25.0f);
        EXPECT_FLOAT_EQ(v1.GetDistance(AZ::Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);
        EXPECT_FLOAT_EQ(v1.GetDistanceEstimate(AZ::Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);
    }

    TEST(MATH_Vector3, TestLerpSlerpNLerp)
    {
        EXPECT_THAT(AZ::Vector3(4.0f, 5.0f, 6.0f).Lerp(AZ::Vector3(5.0f, 10.0f, 2.0f), 0.5f), IsClose(AZ::Vector3(4.5f, 7.5f, 4.0f)));
        EXPECT_THAT(AZ::Vector3(1.0f, 0.0f, 0.0f).Slerp(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.5f), IsClose(AZ::Vector3(0.7071f, 0.7071f, 0.0f)));
        EXPECT_THAT(AZ::Vector3(1.0f, 0.0f, 0.0f).Nlerp(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.5f), IsClose(AZ::Vector3(0.7071f, 0.7071f, 0.0f)));
    }

    TEST(MATH_Vector3, TestDotProduct)
    {
        EXPECT_FLOAT_EQ(AZ::Vector3(1.0f, 2.0f, 3.0f).Dot(AZ::Vector3(-1.0f, 5.0f, 3.0f)), 18.0f);
    }

    TEST(MATH_Vector3, TestCrossProduct)
    {
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).Cross(AZ::Vector3(3.0f, 1.0f, -1.0f)), IsClose(AZ::Vector3(-5.0f, 10.0f, -5.0f)));
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).CrossXAxis(), IsClose(AZ::Vector3(0.0f, 3.0f, -2.0f)));
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).CrossYAxis(), IsClose(AZ::Vector3(-3.0f, 0.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).CrossZAxis(), IsClose(AZ::Vector3(2.0f, -1.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).XAxisCross(), IsClose(AZ::Vector3(0.0f, -3.0f, 2.0f)));
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).YAxisCross(), IsClose(AZ::Vector3(3.0f, 0.0f, -1.0f)));
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).ZAxisCross(), IsClose(AZ::Vector3(-2.0f, 1.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestIsClose)
    {
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsClose(AZ::Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_FALSE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsClose(AZ::Vector3(1.0f, 2.0f, 4.0f)));
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsClose(AZ::Vector3(1.0f, 2.0f, 3.4f), 0.5f));
    }

    TEST(MATH_Vector3, TestEquality)
    {
        AZ::Vector3 v3(1.0f, 2.0f, 3.0f);
        EXPECT_TRUE(v3 == AZ::Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_FALSE((v3 == AZ::Vector3(1.0f, 2.0f, 4.0f)));
        EXPECT_TRUE(v3 != AZ::Vector3(1.0f, 2.0f, 5.0f));
        EXPECT_FALSE((v3 != AZ::Vector3(1.0f, 2.0f, 3.0f)));
    }

    TEST(MATH_Vector3, TestIsLessThan)
    {
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsLessThan(AZ::Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_FALSE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsLessThan(AZ::Vector3(0.0f, 3.0f, 4.0f)));
        EXPECT_FALSE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsLessThan(AZ::Vector3(2.0f, 2.0f, 4.0f)));
    }

    TEST(MATH_Vector3, TestIsLessEqualThan)
    {
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(AZ::Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(AZ::Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_FALSE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(AZ::Vector3(0.0f, 3.0f, 4.0f)));
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(AZ::Vector3(2.0f, 2.0f, 4.0f)));
    }

    TEST(MATH_Vector3, TestIsGreaterThan)
    {
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(AZ::Vector3(0.0f, 1.0f, 2.0f)));
        EXPECT_FALSE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(AZ::Vector3(0.0f, 3.0f, 2.0f)));
        EXPECT_FALSE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(AZ::Vector3(0.0f, 2.0f, 2.0f)));
    }

    TEST(MATH_Vector3, TestIsGreaterEqualThan)
    {
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(AZ::Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(AZ::Vector3(0.0f, 1.0f, 2.0f)));
        EXPECT_FALSE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(AZ::Vector3(0.0f, 3.0f, 2.0f)));
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(AZ::Vector3(0.0f, 2.0f, 2.0f)));
    }

    TEST(MATH_Vector3, TestMinMax)
    {
        EXPECT_THAT(AZ::Vector3(2.0f, 5.0f, 6.0f).GetMin(AZ::Vector3(1.0f, 6.0f, 5.0f)), IsClose(AZ::Vector3(1.0f, 5.0f, 5.0f)));
        EXPECT_THAT(AZ::Vector3(2.0f, 5.0f, 6.0f).GetMax(AZ::Vector3(1.0f, 6.0f, 5.0f)), IsClose(AZ::Vector3(2.0f, 6.0f, 6.0f)));
    }

    TEST(MATH_Vector3, TestClamp)
    {
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).GetClamp(AZ::Vector3(0.0f, -1.0f, 4.0f), AZ::Vector3(2.0f, 1.0f, 10.0f)), IsClose(AZ::Vector3(1.0f, 1.0f, 4.0f)));
    }

    TEST(MATH_Vector3, TestTrig)
    {
        EXPECT_THAT(AZ::Vector3(AZ::DegToRad(78.0f), AZ::DegToRad(-150.0f), AZ::DegToRad(190.0f)).GetAngleMod(), IsClose(AZ::Vector3(AZ::DegToRad(78.0f), AZ::DegToRad(-150.0f), AZ::DegToRad(-170.0f))));
        EXPECT_THAT(AZ::Vector3(AZ::DegToRad(390.0f), AZ::DegToRad(-190.0f), AZ::DegToRad(-400.0f)).GetAngleMod(), IsClose(AZ::Vector3(AZ::DegToRad(30.0f), AZ::DegToRad(170.0f), AZ::DegToRad(-40.0f))));
        EXPECT_THAT(AZ::Vector3(AZ::DegToRad(60.0f), AZ::DegToRad(105.0f), AZ::DegToRad(-174.0f)).GetSin(), IsCloseTolerance(AZ::Vector3(0.866f, 0.966f, -0.105f), 0.005f));
        EXPECT_THAT(AZ::Vector3(AZ::DegToRad(60.0f), AZ::DegToRad(105.0f), AZ::DegToRad(-174.0f)).GetCos(), IsCloseTolerance(AZ::Vector3(0.5f, -0.259f, -0.995f), 0.005f));
        AZ::Vector3 sin, cos;
        AZ::Vector3 v1(AZ::DegToRad(60.0f), AZ::DegToRad(105.0f), AZ::DegToRad(-174.0f));
        v1.GetSinCos(sin, cos);
        EXPECT_THAT(sin, IsCloseTolerance(AZ::Vector3(0.866f, 0.966f, -0.105f), 0.005f));
        EXPECT_THAT(cos, IsCloseTolerance(AZ::Vector3(0.5f, -0.259f, -0.995f), 0.005f));
    }

    TEST(MATH_Vector3, TestAbs)
    {
        EXPECT_THAT(AZ::Vector3(-1.0f, 2.0f, -5.0f).GetAbs(), AZ::Vector3(1.0f, 2.0f, 5.0f));
    }

    TEST(MATH_Vector3, TestReciprocal)
    {
        EXPECT_THAT(AZ::Vector3(2.0f, 4.0f, 5.0f).GetReciprocal(), IsClose(AZ::Vector3(0.5f, 0.25f, 0.2f)));
        EXPECT_THAT(AZ::Vector3(2.0f, 4.0f, 5.0f).GetReciprocalEstimate(), IsCloseTolerance(AZ::Vector3(0.5f, 0.25f, 0.2f), 1e-3f));
    }

    TEST(MATH_Vector3, TestNegate)
    {
        EXPECT_THAT((-AZ::Vector3(1.0f, 2.0f, -3.0f)), IsClose(AZ::Vector3(-1.0f, -2.0f, 3.0f)));
    }

    TEST(MATH_Vector3, TestAdd)
    {
        EXPECT_THAT((AZ::Vector3(1.0f, 2.0f, 3.0f) + AZ::Vector3(-1.0f, 4.0f, 5.0f)) , IsClose(AZ::Vector3(0.0f, 6.0f, 8.0f)));
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += AZ::Vector3(5.0f, 3.0f, -1.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(6.0f, 5.0f, 2.0f)));
    }

    TEST(MATH_Vector3, TestSub)
    {
        EXPECT_THAT((AZ::Vector3(1.0f, 2.0f, 3.0f) - AZ::Vector3(-1.0f, 4.0f, 5.0f)), IsClose(AZ::Vector3(2.0f, -2.0f, -2.0f)));
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += AZ::Vector3(5.0f, 3.0f, -1.0f);
        v1 -= AZ::Vector3(2.0f, -1.0f, 3.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector3(4.0f, 6.0f, -1.0f)));
    }

    TEST(MATH_Vector3, TestMul)
    {
        EXPECT_THAT((AZ::Vector3(1.0f, 2.0f, 3.0f) * AZ::Vector3(-1.0f, 4.0f, 5.0f)), IsClose(AZ::Vector3(-1.0f, 8.0f, 15.0f)));
        EXPECT_THAT((AZ::Vector3(1.0f, 2.0f, 3.0f) * 2.0f), IsClose(AZ::Vector3(2.0f, 4.0f, 6.0f)));
        EXPECT_THAT((2.0f * AZ::Vector3(1.0f, 2.0f, 3.0f)), IsClose(AZ::Vector3(2.0f, 4.0f, 6.0f)));
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += AZ::Vector3(5.0f, 3.0f, -1.0f);
        v1 -= AZ::Vector3(2.0f, -1.0f, 3.0f);
        v1 *= 3.0f;
        EXPECT_THAT(v1, IsClose(AZ::Vector3(12.0f, 18.0f, -3.0f)));
    }

    TEST(MATH_Vector3, TestDiv)
    {
        EXPECT_THAT((AZ::Vector3(1.0f, 2.0f, 3.0f) / AZ::Vector3(-1.0f, 4.0f, 5.0f)), IsClose(AZ::Vector3(-1.0f, 0.5f, 3.0f / 5.0f)));
        EXPECT_THAT((AZ::Vector3(1.0f, 2.0f, 3.0f) / 2.0f), IsClose(AZ::Vector3(0.5f, 1.0f, 1.5f)));
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += AZ::Vector3(5.0f, 3.0f, -1.0f);
        v1 -= AZ::Vector3(2.0f, -1.0f, 3.0f);
        v1 *= 3.0f;
        v1 /= 2.0f;
        EXPECT_THAT(v1, IsClose(AZ::Vector3(6.0f, 9.0f, -1.5f)));
    }

    TEST(MATH_Vector3, TestBuildTangentBasis)
    {
        AZ::Vector3 v1 = AZ::Vector3(1.0f, 1.0f, 2.0f).GetNormalized();
        AZ::Vector3 v2, v3;

        v1.BuildTangentBasis(v2, v3);
        EXPECT_TRUE(v2.IsNormalized());
        EXPECT_TRUE(v3.IsNormalized());
        EXPECT_LT(fabsf(v2.Dot(v1)), 0.001f);
        EXPECT_LT(fabsf(v3.Dot(v1)), 0.001f);
        EXPECT_LT(fabsf(v2.Dot(v3)), 0.001f);
    }

    TEST(MATH_Vector3, TestMadd)
    {
        EXPECT_THAT(AZ::Vector3(1.0f, 2.0f, 3.0f).GetMadd(AZ::Vector3(2.0f, 1.0f, 4.0f), AZ::Vector3(1.0f, 2.0f, 4.0f)), IsClose(AZ::Vector3(3.0f, 4.0f, 16.0f)));
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        v1.Madd(AZ::Vector3(2.0f, 1.0f, 4.0f), AZ::Vector3(1.0f, 2.0f, 4.0f));
        EXPECT_THAT(v1, IsClose(AZ::Vector3(3.0f, 4.0f, 16.0f)));
    }

    TEST(MATH_Vector3, TestIsPerpendicular)
    {
        EXPECT_TRUE(AZ::Vector3(1.0f, 2.0f, 0.0f).IsPerpendicular(AZ::Vector3(0.0f, 0.0f, 1.0f)));
        EXPECT_FALSE(AZ::Vector3(1.0f, 2.0f, 0.0f).IsPerpendicular(AZ::Vector3(0.0f, 1.0f, 1.0f)));
    }

    TEST(MATH_Vector3, TestGetOrthogonalVector)
    {
        AZ::Vector3 v1(1.0f, 2.0f, 3.0f);
        AZ::Vector3 v2 = v1.GetOrthogonalVector();
        EXPECT_TRUE(v1.IsPerpendicular(v2));
        v1 = AZ::Vector3::CreateAxisX();
        v2 = v1.GetOrthogonalVector();
        EXPECT_TRUE(v1.IsPerpendicular(v2));
    }

    TEST(MATH_Vector3, TestProject)
    {
        AZ::Vector3 v1(0.5f, 0.5f, 0.5f);
        v1.Project(AZ::Vector3(0.0f, 2.0f, 1.0f));
        EXPECT_THAT(v1, IsClose(AZ::Vector3(0.0f, 0.6f, 0.3f)));
        v1.Set(0.5f, 0.5f, 0.5f);
        v1.ProjectOnNormal(AZ::Vector3(0.0f, 1.0f, 0.0f));
        EXPECT_THAT(v1, IsClose(AZ::Vector3(0.0f, 0.5f, 0.0f)));
        v1.Set(1.0f, 2.0f, 3.0f);
        EXPECT_THAT(v1.GetProjected(AZ::Vector3(1.0f, 1.0f, 1.0f)), IsClose(AZ::Vector3(2.0f, 2.0f, 2.0f)));
        EXPECT_THAT(v1.GetProjectedOnNormal(AZ::Vector3(1.0f, 0.0f, 0.0f)), IsClose(AZ::Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestIsFinite)
    {
        //IsFinite
        EXPECT_TRUE(AZ::Vector3(1.0f, 1.0f, 1.0f).IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(AZ::Vector3(infinity, infinity, infinity).IsFinite());
    }

    struct Vector3AngleTestArgs
    {
        AZ::Vector3 current;
        AZ::Vector3 target;
        float angle;
    };

    using Vector3AngleTestFixture = ::testing::TestWithParam<Vector3AngleTestArgs>;

    TEST_P(Vector3AngleTestFixture, TestAngle)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.Angle(param.target), param.angle, Constants::SimdTolerance);
    }

    TEST_P(Vector3AngleTestFixture, TestAngleSafe)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleSafe(param.target), param.angle, Constants::SimdTolerance);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector3,
        Vector3AngleTestFixture,
        ::testing::Values(
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 1.0f, 0.0f }, AZ::Constants::HalfPi },
            Vector3AngleTestArgs{ AZ::Vector3{ 42.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 23.0f, 0.0f }, AZ::Constants::HalfPi },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ -1.0f, 0.0f, 0.0f }, AZ::Constants::Pi },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ 1.0f, 1.0f, 0.0f }, AZ::Constants::QuarterPi },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ 1.0f, 0.0f, 0.0f }, 0.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 1.0f, 0.0f }, AZ::Vector3{ -1.0f, -1.0f, 0.0f }, AZ::Constants::Pi }));

    using Vector3AngleDegTestFixture = ::testing::TestWithParam<Vector3AngleTestArgs>;

    TEST_P(Vector3AngleDegTestFixture, TestAngleDeg)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleDeg(param.target), param.angle, Constants::SimdToleranceAngleDeg);
    }

    TEST_P(Vector3AngleDegTestFixture, TestAngleDegSafe)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleSafeDeg(param.target), param.angle, Constants::SimdToleranceAngleDeg);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector3,
        Vector3AngleDegTestFixture,
        ::testing::Values(
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 1.0f, 0.0f }, 90.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 42.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 23.0f, 0.0f }, 90.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ -1.0f, 0.0f, 0.0f }, 180.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ 1.0f, 1.0f, 0.0f }, 45.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ 1.0f, 0.0f, 0.0f }, 0.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 1.0f, 0.0f }, AZ::Vector3{ -1.0f, -1.0f, 0.0f }, 180.f }));

    using AngleSafeInvalidVector3AngleTestFixture = ::testing::TestWithParam<Vector3AngleTestArgs>;

    TEST_P(AngleSafeInvalidVector3AngleTestFixture, TestInvalidAngle)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafe(param.target), param.angle);
    }

    TEST_P(AngleSafeInvalidVector3AngleTestFixture, TestInvalidAngleDeg)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafeDeg(param.target), param.angle);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector3,
        AngleSafeInvalidVector3AngleTestFixture,
        ::testing::Values(
            Vector3AngleTestArgs{ AZ::Vector3{ 0.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 1.0f, 0.0f }, 0.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 0.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 0.0f, 0.0f }, 0.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 1.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 0.0f, 0.0f }, 0.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 0.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 323432.0f, 0.0f }, 0.f },
            Vector3AngleTestArgs{ AZ::Vector3{ 323432.0f, 0.0f, 0.0f }, AZ::Vector3{ 0.0f, 0.0f, 0.0f }, 0.f }));

    TEST(MATH_Vector3, CompareTest)
    {
        AZ::Vector3 vA(-100.0f, 10.0f, 10.0f);
        AZ::Vector3 vB(35.0f, -11.0f, 10.0f);

        // compare equal
        AZ::Vector3 rEq = AZ::Vector3::CreateSelectCmpEqual(vA, vB, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(rEq, IsClose(AZ::Vector3(0.0f, 0.0f, 1.0f)));

        // compare greater equal
        AZ::Vector3 rGr = AZ::Vector3::CreateSelectCmpGreaterEqual(vA, vB, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(rGr, IsClose(AZ::Vector3(0.0f, 1.0f, 1.0f)));

        // compare greater
        AZ::Vector3 rGrEq = AZ::Vector3::CreateSelectCmpGreater(vA, vB, AZ::Vector3(1.0f), AZ::Vector3(0.0f));
        EXPECT_THAT(rGrEq, IsClose(AZ::Vector3(0.0f, 1.0f, 0.0f)));
    }
}
