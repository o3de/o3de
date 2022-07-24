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

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Vector3, TestConstructors)
    {
        Vector3 v1(0.0f);
        EXPECT_FLOAT_EQ(v1.GetX(), 0.0f);
        EXPECT_FLOAT_EQ(v1.GetY(), 0.0f);
        EXPECT_FLOAT_EQ(v1.GetZ(), 0.0f);
        Vector3 v2(5.0f);
        EXPECT_FLOAT_EQ(v2.GetX(), 5.0f);
        EXPECT_FLOAT_EQ(v2.GetY(), 5.0f);
        EXPECT_FLOAT_EQ(v2.GetZ(), 5.0f);
        Vector3 v3(1.0f, 2.0f, 3.0f);
        EXPECT_FLOAT_EQ(v3.GetX(), 1.0f);
        EXPECT_FLOAT_EQ(v3.GetY(), 2.0f);
        EXPECT_FLOAT_EQ(v3.GetZ(), 3.0f);
        
        EXPECT_THAT(Vector3(Vector3(1.0f, 3.0f,2.0f)), IsClose(Vector3(1.0f, 3.0f,2.0f)));
        EXPECT_THAT(Vector3(Vector2(1.0f, 3.0f)), IsClose(Vector3(1.0f, 3.0f,0.0f)));
        EXPECT_THAT(Vector3(Vector2(1.0f, 3.0f), 6.0f), IsClose(Vector3(1.0f, 3.0f,6.0f)));
        EXPECT_THAT(Vector3(Vector4(1.0f, 3.0f, 5.0f, 2.0f)), IsClose(Vector3(1.0f, 3.0f, 5.0f)));
        EXPECT_THAT(Vector3(Simd::Vec3::LoadImmediate(1.0f, 2.0f, 4.0f)), IsClose(Vector3(1.0f, 2.0f, 4.0f)));

    }

    TEST(MATH_Vector3, TestCreateFunctions)
    {
        EXPECT_THAT(Vector3::CreateOne(), IsClose(Vector3(1.0f, 1.0f, 1.0f)));
        EXPECT_THAT(Vector3::CreateZero(), IsClose(Vector3(0.0f)));
        float values[3] = { 10.0f, 20.0f, 30.0f };
        EXPECT_THAT(Vector3::CreateFromFloat3(values), IsClose(Vector3(10.0f, 20.0f, 30.0f)));
        EXPECT_THAT(Vector3::CreateAxisX(), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(Vector3::CreateAxisY(), IsClose(Vector3(0.0f, 1.0f, 0.0f)));
        EXPECT_THAT(Vector3::CreateAxisZ(), IsClose(Vector3(0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Vector3, TestCompareEqual)
    {
        Vector3 vA(-100.0f, 10.0f, -1.0f);
        Vector3 vB(35.0f, 10.0f, -5.0f);
        Vector3 vC(35.0f, 20.0f, -1.0f);

        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        Vector3 compareEqualAB = Vector3::CreateSelectCmpEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        EXPECT_THAT(compareEqualAB, IsClose(Vector3(0.0f, 1.0f, 0.0f)));
        Vector3 compareEqualBC = Vector3::CreateSelectCmpEqual(vB, vC, Vector3(1.0f), Vector3(0.0f));
        EXPECT_THAT(compareEqualBC, IsClose(Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestCompareGreaterEqual)
    {
        Vector3 vA(-100.0f, 10.0f, -1.0f);
        Vector3 vB(35.0f, 10.0f, -5.0f);
        Vector3 vD(15.0f, 30.0f, 45.0f);

        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        Vector3 compareGreaterEqualAB = Vector3::CreateSelectCmpGreaterEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        EXPECT_THAT(compareGreaterEqualAB, IsClose(Vector3(0.0f, 1.0f, 1.0f)));
        Vector3 compareGreaterEqualBD = Vector3::CreateSelectCmpGreaterEqual(vB, vD, Vector3(1.0f), Vector3(0.0f));
        EXPECT_THAT(compareGreaterEqualBD, IsClose(Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestCompareGreater)
    {
        Vector3 vA(-100.0f, 10.0f, -1.0f);
        Vector3 vB(35.0f, 10.0f, -5.0f);
        Vector3 vC(35.0f, 20.0f, -1.0f);

        // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        Vector3 compareGreaterAB = Vector3::CreateSelectCmpGreater(vA, vB, Vector3(1.0f), Vector3(0.0f));
        EXPECT_THAT(compareGreaterAB, IsClose(Vector3(0.0f, 0.0f, 1.0f)));
        Vector3 compareGreaterCA = Vector3::CreateSelectCmpGreater(vC, vA, Vector3(1.0f), Vector3(0.0f));
        EXPECT_THAT(compareGreaterCA, IsClose(Vector3(1.0f, 1.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestStoreFloat)
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
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
        EXPECT_FLOAT_EQ(values4[3], 0.0f);
    }

    TEST(MATH_Vector3, TestGetSet)
    {
        Vector3 v1(2.0f, 3.0f, 4.0f);
        const float values[3] = { 1.0f, 2.0f, 3.0f };

        EXPECT_THAT(v1, IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        v1.SetX(10.0f);
        EXPECT_THAT(v1, IsClose(Vector3(10.0f, 3.0f, 4.0f)));
        v1.SetY(11.0f);
        EXPECT_THAT(v1, IsClose(Vector3(10.0f, 11.0f, 4.0f)));
        v1.SetZ(12.0f);
        EXPECT_THAT(v1, IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        v1.Set(15.0f);
        EXPECT_THAT(v1, IsClose(Vector3(15.0f)));
        v1.Set(values);
        EXPECT_THAT(v1, IsClose(Vector3(1.0f, 2.0f, 3.0f)));
    }

    TEST(MATH_Vector3, TestIndexElement)
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
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
        EXPECT_NEAR(Vector3(3.0f, 4.0f, 0.0f).GetLengthSq(), 25.0f, .01f);
        EXPECT_NEAR(Vector3(0.0f, 4.0f, -3.0f).GetLength(), 5.0f, .01f);
        EXPECT_NEAR(Vector3(0.0f, 4.0f, -3.0f).GetLengthEstimate(), 5.0f, .01f);
    }

    TEST(MATH_Vector3, TestGetLengthReciprocal)
    {
        EXPECT_NEAR(Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocal(), 0.2f, .01f);
        EXPECT_NEAR(Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocalEstimate(), 0.2f, .01f);
    }

    TEST(MATH_Vector3, TestGetNormalized)
    {
        EXPECT_THAT(Vector3(3.0f, 0.0f, 4.0f).GetNormalized(), IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        EXPECT_THAT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedEstimate(), IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
    }

    TEST(MATH_Vector3, TestGetNormalizedSafe)
    {
        EXPECT_THAT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafe(), IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        EXPECT_THAT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafeEstimate(), IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        EXPECT_THAT(Vector3(0.0f).GetNormalizedSafe(), Vector3(0.0f, 0.0f, 0.0f));
        EXPECT_THAT(Vector3(0.0f).GetNormalizedSafeEstimate(), Vector3(0.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector3, TestNormalize)
    {
        Vector3 v1(4.0f, 3.0f, 0.0f);
        v1.Normalize();
        EXPECT_THAT(v1, IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f);
        v1.NormalizeEstimate();
        EXPECT_THAT(v1, IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestNormalizeWithLength)
    {
        Vector3 v1(4.0f, 3.0f, 0.0f);
        float length = v1.NormalizeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f);
        length = v1.NormalizeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestNormalizeSafe)
    {
        Vector3 v1(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
        v1.Set(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestNormalizeSafeWithLength)
    {
        Vector3 v1(0.0f, 3.0f, 4.0f);
        float length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
        v1.Set(0.0f, 3.0f, 4.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestIsNormalized)
    {
        EXPECT_TRUE(Vector3(1.0f, 0.0f, 0.0f).IsNormalized());
        EXPECT_TRUE(Vector3(0.7071f, 0.7071f, 0.0f).IsNormalized());
        EXPECT_FALSE(Vector3(1.0f, 1.0f, 0.0f).IsNormalized());
    }

    TEST(MATH_Vector3, TestSetLength)
    {
        Vector3 v1(3.0f, 4.0f, 0.0f);
        v1.SetLength(10.0f);
        EXPECT_THAT(v1, IsClose(Vector3(6.0f, 8.0f, 0.0f)));
        v1.Set(3.0f, 4.0f, 0.0f);
        v1.SetLengthEstimate(10.0f);
        EXPECT_THAT(v1, IsCloseTolerance(Vector3(6.0f, 8.0f, 0.0f), 1e-3f));
    }

    TEST(MATH_Vector3, TestDistance)
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        EXPECT_FLOAT_EQ(v1.GetDistanceSq(Vector3(-2.0f, 6.0f, 3.0f)), 25.0f);
        EXPECT_FLOAT_EQ(v1.GetDistance(Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);
        EXPECT_FLOAT_EQ(v1.GetDistanceEstimate(Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);
    }

    TEST(MATH_Vector3, TestLerpSlerpNLerp)
    {
        EXPECT_THAT(Vector3(4.0f, 5.0f, 6.0f).Lerp(Vector3(5.0f, 10.0f, 2.0f), 0.5f), IsClose(Vector3(4.5f, 7.5f, 4.0f)));
        EXPECT_THAT(Vector3(1.0f, 0.0f, 0.0f).Slerp(Vector3(0.0f, 1.0f, 0.0f), 0.5f), IsClose(Vector3(0.7071f, 0.7071f, 0.0f)));
        EXPECT_THAT(Vector3(1.0f, 0.0f, 0.0f).Nlerp(Vector3(0.0f, 1.0f, 0.0f), 0.5f), IsClose(Vector3(0.7071f, 0.7071f, 0.0f)));
    }

    TEST(MATH_Vector3, TestDotProduct)
    {
        EXPECT_FLOAT_EQ(Vector3(1.0f, 2.0f, 3.0f).Dot(Vector3(-1.0f, 5.0f, 3.0f)), 18.0f);
    }

    TEST(MATH_Vector3, TestCrossProduct)
    {
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).Cross(Vector3(3.0f, 1.0f, -1.0f)), IsClose(Vector3(-5.0f, 10.0f, -5.0f)));
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).CrossXAxis(), IsClose(Vector3(0.0f, 3.0f, -2.0f)));
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).CrossYAxis(), IsClose(Vector3(-3.0f, 0.0f, 1.0f)));
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).CrossZAxis(), IsClose(Vector3(2.0f, -1.0f, 0.0f)));
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).XAxisCross(), IsClose(Vector3(0.0f, -3.0f, 2.0f)));
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).YAxisCross(), IsClose(Vector3(3.0f, 0.0f, -1.0f)));
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).ZAxisCross(), IsClose(Vector3(-2.0f, 1.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestIsClose)
    {
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_FALSE(Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 4.0f)));
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 3.4f), 0.5f));
    }

    TEST(MATH_Vector3, TestEquality)
    {
        Vector3 v3(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(v3 == Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(!(v3 == Vector3(1.0f, 2.0f, 4.0f)));
        AZ_TEST_ASSERT(v3 != Vector3(1.0f, 2.0f, 5.0f));
        AZ_TEST_ASSERT(!(v3 != Vector3(1.0f, 2.0f, 3.0f)));
    }

    TEST(MATH_Vector3, TestIsLessThan)
    {
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_FALSE(Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(0.0f, 3.0f, 4.0f)));
        EXPECT_FALSE(Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(2.0f, 2.0f, 4.0f)));
    }

    TEST(MATH_Vector3, TestIsLessEqualThan)
    {
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_FALSE(Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(0.0f, 3.0f, 4.0f)));
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(2.0f, 2.0f, 4.0f)));
    }

    TEST(MATH_Vector3, TestIsGreaterThan)
    {
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 1.0f, 2.0f)));
        EXPECT_FALSE(Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 3.0f, 2.0f)));
        EXPECT_FALSE(Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 2.0f, 2.0f)));
    }

    TEST(MATH_Vector3, TestIsGreaterEqualThan)
    {
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 1.0f, 2.0f)));
        EXPECT_FALSE(Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 3.0f, 2.0f)));
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 2.0f, 2.0f)));
    }

    TEST(MATH_Vector3, TestMinMax)
    {
        EXPECT_THAT(Vector3(2.0f, 5.0f, 6.0f).GetMin(Vector3(1.0f, 6.0f, 5.0f)), IsClose(Vector3(1.0f, 5.0f, 5.0f)));
        EXPECT_THAT(Vector3(2.0f, 5.0f, 6.0f).GetMax(Vector3(1.0f, 6.0f, 5.0f)), IsClose(Vector3(2.0f, 6.0f, 6.0f)));
    }

    TEST(MATH_Vector3, TestClamp)
    {
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).GetClamp(Vector3(0.0f, -1.0f, 4.0f), Vector3(2.0f, 1.0f, 10.0f)), IsClose(Vector3(1.0f, 1.0f, 4.0f)));
    }

    TEST(MATH_Vector3, TestTrig)
    {
        EXPECT_THAT(Vector3(DegToRad(78.0f), DegToRad(-150.0f), DegToRad(190.0f)).GetAngleMod(), IsClose(Vector3(DegToRad(78.0f), DegToRad(-150.0f), DegToRad(-170.0f))));
        EXPECT_THAT(Vector3(DegToRad(390.0f), DegToRad(-190.0f), DegToRad(-400.0f)).GetAngleMod(), IsClose(Vector3(DegToRad(30.0f), DegToRad(170.0f), DegToRad(-40.0f))));
        EXPECT_THAT(Vector3(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f)).GetSin(), IsCloseTolerance(Vector3(0.866f, 0.966f, -0.105f), 0.005f));
        EXPECT_THAT(Vector3(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f)).GetCos(), IsCloseTolerance(Vector3(0.5f, -0.259f, -0.995f), 0.005f));
        Vector3 sin, cos;
        Vector3 v1(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f));
        v1.GetSinCos(sin, cos);
        EXPECT_THAT(sin, IsCloseTolerance(Vector3(0.866f, 0.966f, -0.105f), 0.005f));
        EXPECT_THAT(cos, IsCloseTolerance(Vector3(0.5f, -0.259f, -0.995f), 0.005f));
    }

    TEST(MATH_Vector3, TestAbs)
    {
        EXPECT_THAT(Vector3(-1.0f, 2.0f, -5.0f).GetAbs(), Vector3(1.0f, 2.0f, 5.0f));
    }

    TEST(MATH_Vector3, TestReciprocal)
    {
        EXPECT_THAT(Vector3(2.0f, 4.0f, 5.0f).GetReciprocal(), IsClose(Vector3(0.5f, 0.25f, 0.2f)));
        EXPECT_THAT(Vector3(2.0f, 4.0f, 5.0f).GetReciprocalEstimate(), IsCloseTolerance(Vector3(0.5f, 0.25f, 0.2f), 1e-3f));
    }

    TEST(MATH_Vector3, TestNegate)
    {
        EXPECT_THAT((-Vector3(1.0f, 2.0f, -3.0f)), IsClose(Vector3(-1.0f, -2.0f, 3.0f)));
    }

    TEST(MATH_Vector3, TestAdd)
    {
        EXPECT_THAT((Vector3(1.0f, 2.0f, 3.0f) + Vector3(-1.0f, 4.0f, 5.0f)) , IsClose(Vector3(0.0f, 6.0f, 8.0f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        EXPECT_THAT(v1, IsClose(Vector3(6.0f, 5.0f, 2.0f)));
    }

    TEST(MATH_Vector3, TestSub)
    {
        EXPECT_THAT((Vector3(1.0f, 2.0f, 3.0f) - Vector3(-1.0f, 4.0f, 5.0f)), IsClose(Vector3(2.0f, -2.0f, -2.0f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        v1 -= Vector3(2.0f, -1.0f, 3.0f);
        EXPECT_THAT(v1, IsClose(Vector3(4.0f, 6.0f, -1.0f)));
    }

    TEST(MATH_Vector3, TestMul)
    {
        EXPECT_THAT((Vector3(1.0f, 2.0f, 3.0f) * Vector3(-1.0f, 4.0f, 5.0f)), IsClose(Vector3(-1.0f, 8.0f, 15.0f)));
        EXPECT_THAT((Vector3(1.0f, 2.0f, 3.0f) * 2.0f), IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        EXPECT_THAT((2.0f * Vector3(1.0f, 2.0f, 3.0f)), IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        v1 -= Vector3(2.0f, -1.0f, 3.0f);
        v1 *= 3.0f;
        EXPECT_THAT(v1, IsClose(Vector3(12.0f, 18.0f, -3.0f)));
    }

    TEST(MATH_Vector3, TestDiv)
    {
        EXPECT_THAT((Vector3(1.0f, 2.0f, 3.0f) / Vector3(-1.0f, 4.0f, 5.0f)), IsClose(Vector3(-1.0f, 0.5f, 3.0f / 5.0f)));
        EXPECT_THAT((Vector3(1.0f, 2.0f, 3.0f) / 2.0f), IsClose(Vector3(0.5f, 1.0f, 1.5f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        v1 -= Vector3(2.0f, -1.0f, 3.0f);
        v1 *= 3.0f;
        v1 /= 2.0f;
        EXPECT_THAT(v1, IsClose(Vector3(6.0f, 9.0f, -1.5f)));
    }

    TEST(MATH_Vector3, TestBuildTangentBasis)
    {
        Vector3 v1 = Vector3(1.0f, 1.0f, 2.0f).GetNormalized();
        Vector3 v2, v3;

        v1.BuildTangentBasis(v2, v3);
        EXPECT_TRUE(v2.IsNormalized());
        EXPECT_TRUE(v3.IsNormalized());
        EXPECT_TRUE(fabsf(v2.Dot(v1)) < 0.001f);
        EXPECT_TRUE(fabsf(v3.Dot(v1)) < 0.001f);
        EXPECT_TRUE(fabsf(v2.Dot(v3)) < 0.001f);
    }

    TEST(MATH_Vector3, TestMadd)
    {
        EXPECT_THAT(Vector3(1.0f, 2.0f, 3.0f).GetMadd(Vector3(2.0f, 1.0f, 4.0f), Vector3(1.0f, 2.0f, 4.0f)), IsClose(Vector3(3.0f, 4.0f, 16.0f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1.Madd(Vector3(2.0f, 1.0f, 4.0f), Vector3(1.0f, 2.0f, 4.0f));
        EXPECT_THAT(v1, IsClose(Vector3(3.0f, 4.0f, 16.0f)));
    }

    TEST(MATH_Vector3, TestIsPerpendicular)
    {
        EXPECT_TRUE(Vector3(1.0f, 2.0f, 0.0f).IsPerpendicular(Vector3(0.0f, 0.0f, 1.0f)));
        EXPECT_FALSE(Vector3(1.0f, 2.0f, 0.0f).IsPerpendicular(Vector3(0.0f, 1.0f, 1.0f)));
    }

    TEST(MATH_Vector3, TestGetOrthogonalVector)
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2 = v1.GetOrthogonalVector();
        EXPECT_TRUE(v1.IsPerpendicular(v2));
        v1 = Vector3::CreateAxisX();
        v2 = v1.GetOrthogonalVector();
        EXPECT_TRUE(v1.IsPerpendicular(v2));
    }

    TEST(MATH_Vector3, TestProject)
    {
        Vector3 v1(0.5f, 0.5f, 0.5f);
        v1.Project(Vector3(0.0f, 2.0f, 1.0f));
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 0.6f, 0.3f)));
        v1.Set(0.5f, 0.5f, 0.5f);
        v1.ProjectOnNormal(Vector3(0.0f, 1.0f, 0.0f));
        EXPECT_THAT(v1, IsClose(Vector3(0.0f, 0.5f, 0.0f)));
        v1.Set(1.0f, 2.0f, 3.0f);
        EXPECT_THAT(v1.GetProjected(Vector3(1.0f, 1.0f, 1.0f)), IsClose(Vector3(2.0f, 2.0f, 2.0f)));
        EXPECT_THAT(v1.GetProjectedOnNormal(Vector3(1.0f, 0.0f, 0.0f)), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestIsFinite)
    {
        //IsFinite
        EXPECT_TRUE(Vector3(1.0f, 1.0f, 1.0f).IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(Vector3(infinity, infinity, infinity).IsFinite());
    }

    struct AngleTestArgs
    {
        AZ::Vector3 current;
        AZ::Vector3 target;
        float angle;
    };

    using AngleTestFixture = ::testing::TestWithParam<AngleTestArgs>;

    TEST_P(AngleTestFixture, TestAngle)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.Angle(param.target), param.angle);
    }

    TEST_P(AngleTestFixture, TestAngleSafe)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafe(param.target), param.angle);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector3,
        AngleTestFixture,
        ::testing::Values(
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, AZ::Constants::HalfPi },
            AngleTestArgs{ Vector3{ 42.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 23.0f, 0.0f }, AZ::Constants::HalfPi },
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ -1.0f, 0.0f, 0.0f }, AZ::Constants::Pi },
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 1.0f, 0.0f }, AZ::Constants::QuarterPi },
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector3{ 1.0f, 1.0f, 0.0f }, Vector3{ -1.0f, -1.0f, 0.0f }, AZ::Constants::Pi }));

    using AngleDegTestFixture = ::testing::TestWithParam<AngleTestArgs>;

    TEST_P(AngleDegTestFixture, TestAngleDeg)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleDeg(param.target), param.angle);
    }

    TEST_P(AngleDegTestFixture, TestAngleDegSafe)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafeDeg(param.target), param.angle);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector3,
        AngleDegTestFixture,
        ::testing::Values(
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, 90.f },
            AngleTestArgs{ Vector3{ 42.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 23.0f, 0.0f }, 90.f },
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ -1.0f, 0.0f, 0.0f }, 180.f },
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 1.0f, 0.0f }, 45.f },
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector3{ 1.0f, 1.0f, 0.0f }, Vector3{ -1.0f, -1.0f, 0.0f }, 180.f }));

    using AngleSafeInvalidAngleTestFixture = ::testing::TestWithParam<AngleTestArgs>;

    TEST_P(AngleSafeInvalidAngleTestFixture, TestInvalidAngle)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafe(param.target), param.angle);
    }

    TEST_P(AngleSafeInvalidAngleTestFixture, TestInvalidAngleDeg)
    {
        auto& param = GetParam();
        EXPECT_FLOAT_EQ(param.current.AngleSafeDeg(param.target), param.angle);
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Vector3,
        AngleSafeInvalidAngleTestFixture,
        ::testing::Values(
            AngleTestArgs{ Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 323432.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector3{ 323432.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, 0.f }));

    TEST(MATH_Vector3, CompareTest)
    {
        Vector3 vA(-100.0f, 10.0f, 10.0f);
        Vector3 vB(35.0f, -11.0f, 10.0f);

        // compare equal
        Vector3 rEq = Vector3::CreateSelectCmpEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        EXPECT_TRUE(rEq.IsClose(Vector3(0.0f, 0.0f, 1.0f)));

        // compare greater equal
        Vector3 rGr = Vector3::CreateSelectCmpGreaterEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        EXPECT_TRUE(rGr.IsClose(Vector3(0.0f, 1.0f, 1.0f)));

        // compare greater
        Vector3 rGrEq = Vector3::CreateSelectCmpGreater(vA, vB, Vector3(1.0f), Vector3(0.0f));
        EXPECT_TRUE(rGrEq.IsClose(Vector3(0.0f, 1.0f, 0.0f)));
    }
}
