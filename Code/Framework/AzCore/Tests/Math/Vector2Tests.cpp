/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/SimdMath.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Math/MathTest.h>

namespace UnitTest
{
    TEST(MATH_Vector2, TestConstructors)
    {
        AZ::Vector2 vA(0.0f);
        EXPECT_FLOAT_EQ(vA.GetX(), 0.0f);
        EXPECT_FLOAT_EQ(vA.GetY(), 0.0f);
        AZ::Vector2 vB(5.0f);
        EXPECT_FLOAT_EQ(vB.GetX(), 5.0f);
        EXPECT_FLOAT_EQ(vB.GetY(), 5.0f);
        AZ::Vector2 vC(1.0f, 2.0f);
        EXPECT_FLOAT_EQ(vC.GetX(), 1.0f);
        EXPECT_FLOAT_EQ(vC.GetY(), 2.0f);
        EXPECT_THAT(AZ::Vector2(AZ::Vector4(1.0f, 3.0f, 2.0f, 4.0f)), IsClose(AZ::Vector2(1.0f, 3.0f)));
        EXPECT_THAT(AZ::Vector2(AZ::Vector3(1.0f, 3.0f, 2.0f)), IsClose(AZ::Vector2(1.0f, 3.0f)));
    }

    TEST(MATH_Vector2, TestCreate)
    {
        float values[2] = { 10.0f, 20.0f };

        EXPECT_THAT(AZ::Vector2::CreateOne(), IsClose(AZ::Vector2(1.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector2::CreateZero(), IsClose(AZ::Vector2(0.0f, 0.0f)));

        EXPECT_THAT(AZ::Vector2::CreateFromFloat2(values), IsClose(AZ::Vector2(10.0f, 20.0f)));
        EXPECT_THAT(AZ::Vector2::CreateAxisX(), IsClose(AZ::Vector2(1.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector2::CreateAxisY(), IsClose(AZ::Vector2(0.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestCreateFromAngle)
    {
        EXPECT_THAT(AZ::Vector2::CreateFromAngle(), IsClose(AZ::Vector2(0.0f, 1.0f)));
        EXPECT_THAT(AZ::Vector2::CreateFromAngle(0.78539816339f), IsClose(AZ::Vector2(0.7071067811f, 0.7071067811f)));
        EXPECT_THAT(AZ::Vector2::CreateFromAngle(4.0f), IsClose(AZ::Vector2(-0.7568024953f, -0.6536436208f)));
        EXPECT_THAT(AZ::Vector2::CreateFromAngle(-1.0f), IsClose(AZ::Vector2(-0.8414709848f, 0.5403023058f)));
    }

    TEST(MATH_Vector2, TestCompareEqual)
    {
        AZ::Vector2 vA(-100.0f, 10.0f);
        AZ::Vector2 vB(35.0f, 10.0f);
        AZ::Vector2 vC(35.0f, 20.0f);

        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        AZ::Vector2 compareEqualAB = AZ::Vector2::CreateSelectCmpEqual(vA, vB, AZ::Vector2(1.0f), AZ::Vector2(0.0f));
        EXPECT_THAT(compareEqualAB, IsClose(AZ::Vector2(0.0f, 1.0f)));
        AZ::Vector2 compareEqualBC = AZ::Vector2::CreateSelectCmpEqual(vB, vC, AZ::Vector2(1.0f), AZ::Vector2(0.0f));
        EXPECT_THAT(compareEqualBC, IsClose(AZ::Vector2(1.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestCompareGreaterEqual)
    {
        AZ::Vector2 vA(-100.0f, 10.0f);
        AZ::Vector2 vB(35.0f, 10.0f);
        AZ::Vector2 vD(15.0f, 30.0f);

        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        AZ::Vector2 compareGreaterEqualAB = AZ::Vector2::CreateSelectCmpGreaterEqual(vA, vB, AZ::Vector2(1.0f), AZ::Vector2(0.0f));
        EXPECT_THAT(compareGreaterEqualAB, IsClose(AZ::Vector2(0.0f, 1.0f)));
        AZ::Vector2 compareGreaterEqualBD = AZ::Vector2::CreateSelectCmpGreaterEqual(vB, vD, AZ::Vector2(1.0f), AZ::Vector2(0.0f));
        EXPECT_THAT(compareGreaterEqualBD, IsClose(AZ::Vector2(1.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestCompareGreater)
    {
        AZ::Vector2 vA(-100.0f, 10.0f);
        AZ::Vector2 vB(35.0f, 10.0f);
        AZ::Vector2 vC(35.0f, 20.0f);

        // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        AZ::Vector2 compareGreaterAB = AZ::Vector2::CreateSelectCmpGreater(vA, vB, AZ::Vector2(1.0f), AZ::Vector2(0.0f));
        EXPECT_THAT(compareGreaterAB, IsClose(AZ::Vector2(0.0f, 0.0f)));
        AZ::Vector2 compareGreaterCA = AZ::Vector2::CreateSelectCmpGreater(vC, vA, AZ::Vector2(1.0f), AZ::Vector2(0.0f));
        EXPECT_THAT(compareGreaterCA, IsClose(AZ::Vector2(1.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestStoreFloat)
    {
        float values[2] = { 0.0f, 0.0f };

        AZ::Vector2 vA(1.0f, 2.0f);
        vA.StoreToFloat2(values);
        EXPECT_EQ(values[0], 1.0f);
        EXPECT_EQ(values[1], 2.0f);
    }

    TEST(MATH_Vector2, TestGetSet)
    {
        AZ::Vector2 vA(2.0f, 3.0f);

        EXPECT_THAT(vA, IsClose(AZ::Vector2(2.0f, 3.0f)));
        EXPECT_FLOAT_EQ(vA.GetX(), 2.0f);
        EXPECT_FLOAT_EQ(vA.GetY(), 3.0f);

        vA.SetX(10.0f);
        EXPECT_THAT(vA, IsClose(AZ::Vector2(10.0f, 3.0f)));

        vA.SetY(11.0f);
        EXPECT_THAT(vA, IsClose(AZ::Vector2(10.0f, 11.0f)));

        vA.Set(15.0f);
        EXPECT_THAT(vA, IsClose(AZ::Vector2(15.0f)));

        vA.SetElement(0, 20.0f);
        EXPECT_FLOAT_EQ(vA.GetX(), 20.0f);
        EXPECT_FLOAT_EQ(vA.GetY(), 15.0f);
        EXPECT_FLOAT_EQ(vA.GetElement(0), 20.0f);
        EXPECT_FLOAT_EQ(vA.GetElement(1), 15.0f);
        EXPECT_FLOAT_EQ(vA(0), 20.0f);
        EXPECT_FLOAT_EQ(vA(1), 15.0f);

        vA.SetElement(1, 21.0f);
        EXPECT_FLOAT_EQ(vA.GetX(), 20.0f);
        EXPECT_FLOAT_EQ(vA.GetY(), 21.0f);
        EXPECT_FLOAT_EQ(vA.GetElement(0), 20.0f);
        EXPECT_FLOAT_EQ(vA.GetElement(1), 21.0f);
        EXPECT_FLOAT_EQ(vA(0), 20.0f);
        EXPECT_FLOAT_EQ(vA(1), 21.0f);
    }

    TEST(MATH_Vector2, TestGetLength)
    {
        EXPECT_FLOAT_EQ(AZ::Vector2(3.0f, 4.0f).GetLengthSq(), 25.0f);
        EXPECT_FLOAT_EQ(AZ::Vector2(4.0f, -3.0f).GetLength(), 5.0f);
        EXPECT_FLOAT_EQ(AZ::Vector2(4.0f, -3.0f).GetLengthEstimate(), 5.0f);
    }

    TEST(MATH_Vector2, TestGetLengthReciprocal)
    {
        EXPECT_NEAR(AZ::Vector2(4.0f, -3.0f).GetLengthReciprocal(), 0.2f, 0.01f);
        EXPECT_NEAR(AZ::Vector2(4.0f, -3.0f).GetLengthReciprocalEstimate(), 0.2f, 0.01f);
    }

    TEST(MATH_Vector2, TestGetNormalized)
    {
        EXPECT_THAT(AZ::Vector2(3.0f, 4.0f).GetNormalized(), IsCloseTolerance(AZ::Vector2(3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector2(3.0f, 4.0f).GetNormalizedEstimate(), IsCloseTolerance(AZ::Vector2(3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector2, TestGetNormalizedSafe)
    {
        EXPECT_THAT(AZ::Vector2(3.0f, 4.0f).GetNormalizedSafe(), IsCloseTolerance(AZ::Vector2(3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector2(3.0f, 4.0f).GetNormalizedSafeEstimate(), IsCloseTolerance(AZ::Vector2(3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        EXPECT_THAT(AZ::Vector2(0.0f).GetNormalizedSafe(), IsClose(AZ::Vector2(0.0f, 0.0f)));
        EXPECT_THAT(AZ::Vector2(0.0f).GetNormalizedSafeEstimate(), IsClose(AZ::Vector2(0.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestNormalize)
    {
        AZ::Vector2 v1(4.0f, 3.0f);
        v1.Normalize();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(4.0f / 5.0f, 3.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(4.0f, 3.0f);
        v1.NormalizeEstimate();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(4.0f / 5.0f, 3.0f / 5.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector2, TestNormalizeWithLength)
    {
        AZ::Vector2 v1(4.0f, 3.0f);
        float length = v1.NormalizeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(4.0f / 5.0f, 3.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(4.0f, 3.0f);
        length = v1.NormalizeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(4.0f / 5.0f, 3.0f / 5.0f), Constants::SimdTolerance));
    }

    TEST(MATH_Vector2, TestNormalizeSafe)
    {
        AZ::Vector2 v1(3.0f, 4.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsClose(AZ::Vector2(0.0f, 0.0f)));
        v1.Set(3.0f, 4.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsClose(AZ::Vector2(0.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestNormalizeSafeWithLength)
    {
        AZ::Vector2 v1(3.0f, 4.0f);
        float length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector2(0.0f, 0.0f)));
        v1.Set(3.0f, 4.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(3.0f / 5.0f, 4.0f / 5.0f), Constants::SimdTolerance));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector2(0.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestIsNormalized)
    {
        EXPECT_TRUE(AZ::Vector2(1.0f, 0.0f).IsNormalized());
        EXPECT_TRUE(AZ::Vector2(0.7071f, 0.7071f).IsNormalized());
        EXPECT_FALSE(AZ::Vector2(1.0f, 1.0f).IsNormalized());
    }

    TEST(MATH_Vector2, TestSetLength)
    {
        AZ::Vector2 v1(3.0f, 4.0f);
        v1.SetLength(10.0f);
        EXPECT_THAT(v1, IsClose(AZ::Vector2(6.0f, 8.0f)));
        v1.Set(3.0f, 4.0f);
        v1.SetLengthEstimate(10.0f);
        EXPECT_THAT(v1, IsCloseTolerance(AZ::Vector2(6.0f, 8.0f), Constants::SimdToleranceEstimateFuncs));
    }

    TEST(MATH_Vector2, TestDistance)
    {
        AZ::Vector2 vA(1.0f, 2.0f);
        EXPECT_FLOAT_EQ(vA.GetDistanceSq(AZ::Vector2(-2.0f, 6.0f)), 25.0f);
        EXPECT_FLOAT_EQ(vA.GetDistance(AZ::Vector2(5.0f, -1.0f)), 5.0f);
        EXPECT_FLOAT_EQ(vA.GetDistanceEstimate(AZ::Vector2(5.0f, -1.0f)), 5.0f);
    }

    TEST(MATH_Vector2, TestLerpSlerpNLerp)
    {
        EXPECT_THAT(AZ::Vector2(4.0f, 5.0f).Lerp(AZ::Vector2(5.0f, 10.0f), 0.5f), IsClose(AZ::Vector2(4.5f, 7.5f)));
        EXPECT_THAT(AZ::Vector2(1.0f, 0.0f).Slerp(AZ::Vector2(0.0f, 1.0f), 0.5f), IsClose(AZ::Vector2(0.7071f, 0.7071f)));
        EXPECT_THAT(AZ::Vector2(1.0f, 0.0f).Nlerp(AZ::Vector2(0.0f, 1.0f), 0.5f), IsClose(AZ::Vector2(0.7071f, 0.7071f)));
    }

    TEST(MATH_Vector2, TestGetPerpendicular)
    {
        EXPECT_THAT(AZ::Vector2(1.0f, 2.0f).GetPerpendicular(), IsClose(AZ::Vector2(-2.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestIsClose)
    {
        EXPECT_TRUE(AZ::Vector2(1.0f, 2.0f).IsClose(AZ::Vector2(1.0f, 2.0f)));
        EXPECT_FALSE(AZ::Vector2(1.0f, 2.0f).IsClose(AZ::Vector2(1.0f, 3.0f)));

        // Verify tolerance works
        EXPECT_TRUE(AZ::Vector2(1.0f, 2.0f).IsClose(AZ::Vector2(1.0f, 2.4f), 0.5f));
    }

    TEST(MATH_Vector2, TestIsZero)
    {
        EXPECT_TRUE(AZ::Vector2(0.0f).IsZero());
        EXPECT_FALSE(AZ::Vector2(1.0f).IsZero());

        // Verify tolerance works
        EXPECT_TRUE(AZ::Vector2(0.05f).IsZero(0.1f));
    }

    TEST(MATH_Vector2, TestEqualityInequality)
    {
        AZ::Vector2 vA(1.0f, 2.0f);
        EXPECT_TRUE(vA == AZ::Vector2(1.0f, 2.0f));
        EXPECT_FALSE((vA == AZ::Vector2(5.0f, 6.0f)));
        EXPECT_TRUE(vA != AZ::Vector2(7.0f, 8.0f));
        EXPECT_FALSE((vA != AZ::Vector2(1.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsLessThan)
    {
        EXPECT_TRUE(AZ::Vector2(1.0f, 2.0f).IsLessThan(AZ::Vector2(2.0f, 3.0f)));
        EXPECT_FALSE(AZ::Vector2(1.0f, 2.0f).IsLessThan(AZ::Vector2(0.0f, 3.0f)));
        EXPECT_FALSE(AZ::Vector2(1.0f, 2.0f).IsLessThan(AZ::Vector2(2.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsLessEqualThan)
    {
        EXPECT_TRUE(AZ::Vector2(1.0f, 2.0f).IsLessEqualThan(AZ::Vector2(2.0f, 3.0f)));
        EXPECT_FALSE(AZ::Vector2(1.0f, 2.0f).IsLessEqualThan(AZ::Vector2(0.0f, 3.0f)));
        EXPECT_TRUE(AZ::Vector2(1.0f, 2.0f).IsLessEqualThan(AZ::Vector2(2.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsGreaterThan)
    {
        EXPECT_TRUE(AZ::Vector2(1.0f, 2.0f).IsGreaterThan(AZ::Vector2(0.0f, 1.0f)));
        EXPECT_FALSE(AZ::Vector2(1.0f, 2.0f).IsGreaterThan(AZ::Vector2(0.0f, 3.0f)));
        EXPECT_FALSE(AZ::Vector2(1.0f, 2.0f).IsGreaterThan(AZ::Vector2(0.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsGreaterEqualThan)
    {
        EXPECT_TRUE(AZ::Vector2(1.0f, 2.0f).IsGreaterEqualThan(AZ::Vector2(0.0f, 1.0f)));
        EXPECT_FALSE(AZ::Vector2(1.0f, 2.0f).IsGreaterEqualThan(AZ::Vector2(0.0f, 3.0f)));
        EXPECT_TRUE(AZ::Vector2(1.0f, 2.0f).IsGreaterEqualThan(AZ::Vector2(0.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestMinMax)
    {
        EXPECT_THAT(AZ::Vector2(2.0f, 3.0f).GetMin(AZ::Vector2(1.0f, 4.0f)), IsClose(AZ::Vector2(1.0f, 3.0f)));
        EXPECT_THAT(AZ::Vector2(2.0f, 3.0f).GetMax(AZ::Vector2(1.0f, 4.0f)), IsClose(AZ::Vector2(2.0f, 4.0f)));
    }

    TEST(MATH_Vector2, TestClamp)
    {
        EXPECT_THAT(AZ::Vector2(1.0f, 2.0f).GetClamp(AZ::Vector2(5.0f, -10.0f), AZ::Vector2(10.0f, -5.0f)), IsClose(AZ::Vector2(5.0f, -5.0f)));
        EXPECT_THAT(AZ::Vector2(1.0f, 2.0f).GetClamp(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(10.0f, 10.0f)), IsClose(AZ::Vector2(1.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestSelect)
    {
        AZ::Vector2 vA(1.0f);
        AZ::Vector2 vB(2.0f);
        EXPECT_THAT(vA.GetSelect(AZ::Vector2(0.0f, 1.0f), vB), IsClose(AZ::Vector2(1.0f, 2.0f)));
        vA.Select(AZ::Vector2(1.0f, 0.0f), vB);
        EXPECT_THAT(vA, IsClose(AZ::Vector2(2.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestAbs)
    {
        EXPECT_THAT(AZ::Vector2(-1.0f, 2.0f).GetAbs(), IsClose(AZ::Vector2(1.0f, 2.0f)));
        EXPECT_THAT(AZ::Vector2(3.0f, -4.0f).GetAbs(), IsClose(AZ::Vector2(3.0f, 4.0f)));
    }

    TEST(MATH_Vector2, TestReciprocal)
    {
        EXPECT_THAT(AZ::Vector2(2.0f, 4.0f).GetReciprocal(), IsClose(AZ::Vector2(0.5f, 0.25f)));
        EXPECT_THAT(AZ::Vector2(2.0f, 4.0f).GetReciprocalEstimate(), IsClose(AZ::Vector2(0.5f, 0.25f)));
    }

    TEST(MATH_Vector2, TestAdd)
    {
        AZ::Vector2 vA(1.0f, 2.0f);
        vA += AZ::Vector2(3.0f, 4.0f);
        EXPECT_THAT(vA, IsClose(AZ::Vector2(4.0f, 6.0f)));
        EXPECT_THAT((AZ::Vector2(1.0f, 2.0f) + AZ::Vector2(2.0f, -1.0f)), IsClose(AZ::Vector2(3.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestSub)
    {
        AZ::Vector2 vA(10.0f, 11.0f);
        vA -= AZ::Vector2(2.0f, 4.0f);
        EXPECT_THAT(vA, IsClose(AZ::Vector2(8.0f, 7.0f)));
        EXPECT_THAT((AZ::Vector2(1.0f, 2.0f) - AZ::Vector2(2.0f, -1.0f)), IsClose(AZ::Vector2(-1.0f, 3.0f)));
    }

    TEST(MATH_Vector2, TestMul)
    {
        AZ::Vector2 vA(2.0f, 4.0f);
        vA *= AZ::Vector2(3.0f, 6.0f);
        EXPECT_THAT(vA, IsClose(AZ::Vector2(6.0f, 24.0f)));

        vA.Set(2.0f, 3.0f);
        vA *= 5.0f;
        EXPECT_THAT(vA, IsClose(AZ::Vector2(10.0f, 15.0f)));

        EXPECT_THAT((AZ::Vector2(3.0f, 2.0f) * AZ::Vector2(2.0f, -4.0f)), IsClose(AZ::Vector2(6.0f, -8.0f)));
        EXPECT_THAT((AZ::Vector2(3.0f, 2.0f) * 2.0f), IsClose(AZ::Vector2(6.0f, 4.0f)));
    }

    TEST(MATH_Vector2, TestDiv)
    {
        AZ::Vector2 vA(15.0f, 20.0f);
        vA /= AZ::Vector2(3.0f, 2.0f);
        EXPECT_THAT(vA, IsClose(AZ::Vector2(5.0f, 10.0f)));

        vA.Set(20.0f, 30.0f);
        vA /= 10.0f;
        EXPECT_THAT(vA, IsClose(AZ::Vector2(2.0f, 3.0f)));

        EXPECT_THAT((AZ::Vector2(30.0f, 20.0f) / AZ::Vector2(10.0f, -4.0f)), IsClose(AZ::Vector2(3.0f, -5.0f)));
        EXPECT_THAT((AZ::Vector2(30.0f, 20.0f) / 10.0f), IsClose(AZ::Vector2(3.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestNegate)
    {
        EXPECT_THAT((-AZ::Vector2(1.0f, -2.0f)), IsClose(AZ::Vector2(-1.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestDot)
    {
        EXPECT_FLOAT_EQ(AZ::Vector2(1.0f, 2.0f).Dot(AZ::Vector2(-1.0f, 5.0f)), 9.0f);
    }

    TEST(MATH_Vector2, TestMadd)
    {
        EXPECT_THAT(AZ::Vector2(1.0f, 2.0f).GetMadd(AZ::Vector2(2.0f, 6.0f), AZ::Vector2(3.0f, 4.0f)), IsClose(AZ::Vector2(5.0f, 16.0f)));
        AZ::Vector2 vA(1.0f, 2.0f);
        vA.Madd(AZ::Vector2(2.0f, 6.0f), AZ::Vector2(3.0f, 4.0f));
        EXPECT_THAT(vA, IsClose(AZ::Vector2(5.0f, 16.0f)));
    }

    TEST(MATH_Vector2, TestProject)
    {
        AZ::Vector2 vA(0.5f);
        vA.Project(AZ::Vector2(0.0f, 2.0f));
        EXPECT_THAT(vA, IsClose(AZ::Vector2(0.0f, 0.5f)));
        vA.Set(0.5f);
        vA.ProjectOnNormal(AZ::Vector2(0.0f, 1.0f));
        EXPECT_THAT(vA, IsClose(AZ::Vector2(0.0f, 0.5f)));
        vA.Set(2.0f, 4.0f);
        EXPECT_THAT(vA.GetProjected(AZ::Vector2(1.0f, 1.0f)), IsClose(AZ::Vector2(3.0f, 3.0f)));
        EXPECT_THAT(vA.GetProjectedOnNormal(AZ::Vector2(1.0f, 0.0f)), IsClose(AZ::Vector2(2.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestTrig)
    {
        EXPECT_THAT(AZ::Vector2(AZ::DegToRad(78.0f), AZ::DegToRad(-150.0f)).GetAngleMod(), IsClose(AZ::Vector2(AZ::DegToRad(78.0f), AZ::DegToRad(-150.0f))));
        EXPECT_THAT(AZ::Vector2(AZ::DegToRad(390.0f), AZ::DegToRad(-190.0f)).GetAngleMod(), IsClose(AZ::Vector2(AZ::DegToRad(30.0f), AZ::DegToRad(170.0f))));
        EXPECT_THAT(AZ::Vector2(AZ::DegToRad(60.0f), AZ::DegToRad(105.0f)).GetSin(), IsCloseTolerance(AZ::Vector2(0.866f, 0.966f), 0.005f));
        EXPECT_THAT(AZ::Vector2(AZ::DegToRad(60.0f), AZ::DegToRad(105.0f)).GetCos(), IsCloseTolerance(AZ::Vector2(0.5f, -0.259f), 0.005f));
        AZ::Vector2 sin, cos;
        AZ::Vector2 v1(AZ::DegToRad(60.0f), AZ::DegToRad(105.0f));
        v1.GetSinCos(sin, cos);
        EXPECT_THAT(sin, IsCloseTolerance(AZ::Vector2(0.866f, 0.966f), 0.005f));
        EXPECT_THAT(cos, IsCloseTolerance(AZ::Vector2(0.5f, -0.259f), 0.005f));
    }

    TEST(MATH_Vector2, TestIsFinite)
    {
        EXPECT_TRUE(AZ::Vector2(1.0f, 1.0f).IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(AZ::Vector2(infinity, infinity).IsFinite());
    }
    struct Vector2AngleTestArgs
    {
        AZ::Vector2 current;
        AZ::Vector2 target;
        float angle;
    };

    using Vector2AngleTestFixture = ::testing::TestWithParam<Vector2AngleTestArgs>;

    TEST_P(Vector2AngleTestFixture, TestAngle)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.Angle(param.target), param.angle, Constants::SimdTolerance);
    }

    TEST_P(Vector2AngleTestFixture, TestAngleSafe)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleSafe(param.target), param.angle, Constants::SimdTolerance);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector2,
        Vector2AngleTestFixture,
        ::testing::Values(
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ 0.0f, 1.0f }, AZ::Constants::HalfPi },
            Vector2AngleTestArgs{ AZ::Vector2{ 42.0f, 0.0f }, AZ::Vector2{ 0.0f, 23.0f }, AZ::Constants::HalfPi },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ -1.0f, 0.0f }, AZ::Constants::Pi },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ 1.0f, 1.0f }, AZ::Constants::QuarterPi },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ 1.0f, 0.0f }, 0.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 1.0f }, AZ::Vector2{ -1.0f, -1.0f }, AZ::Constants::Pi }));

    using Vector2AngleDegTestFixture = ::testing::TestWithParam<Vector2AngleTestArgs>;

    TEST_P(Vector2AngleDegTestFixture, TestAngleDeg)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleDeg(param.target), param.angle, Constants::SimdToleranceAngleDeg);
    }

    TEST_P(Vector2AngleDegTestFixture, TestAngleDegSafe)
    {
        auto& param = GetParam();
        EXPECT_NEAR(param.current.AngleSafeDeg(param.target), param.angle, Constants::SimdToleranceAngleDeg);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector2,
        Vector2AngleDegTestFixture,
        ::testing::Values(
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ 0.0f, 1.0f }, 90.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 42.0f, 0.0f }, AZ::Vector2{ 0.0f, 23.0f }, 90.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ -1.0f, 0.0f }, 180.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ 1.0f, 1.0f }, 45.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ 1.0f, 0.0f }, 0.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 1.0f }, AZ::Vector2{ -1.0f, -1.0f }, 180.f }));

    using AngleSafeInvalidVector2AngleTestFixture = ::testing::TestWithParam<Vector2AngleTestArgs>;

    TEST_P(AngleSafeInvalidVector2AngleTestFixture, TestInvalidAngle)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafe(param.target), param.angle);
    }

    TEST_P(AngleSafeInvalidVector2AngleTestFixture, TestInvalidAngleDeg)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafeDeg(param.target), param.angle);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector2,
        AngleSafeInvalidVector2AngleTestFixture,
        ::testing::Values(
            Vector2AngleTestArgs{ AZ::Vector2{ 0.0f, 0.0f }, AZ::Vector2{ 0.0f, 1.0f }, 0.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 0.0f, 0.0f }, AZ::Vector2{ 0.0f, 0.0f }, 0.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 1.0f, 0.0f }, AZ::Vector2{ 0.0f, 0.0f }, 0.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 0.0f, 0.0f }, AZ::Vector2{ 0.0f, 323432.0f }, 0.f },
            Vector2AngleTestArgs{ AZ::Vector2{ 323432.0f, 0.0f }, AZ::Vector2{ 0.0f, 0.0f }, 0.f }));
} // namespace UnitTest
