/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Vector3, TestConstructors)
    {
        Vector3 v1(0.0f);
        AZ_TEST_ASSERT((v1.GetX() == 0.0f) && (v1.GetY() == 0.0f) && (v1.GetZ() == 0.0f));
        Vector3 v2(5.0f);
        AZ_TEST_ASSERT((v2.GetX() == 5.0f) && (v2.GetY() == 5.0f) && (v2.GetZ() == 5.0f));
        Vector3 v3(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT((v3.GetX() == 1.0f) && (v3.GetY() == 2.0f) && (v3.GetZ() == 3.0f));
    }

    TEST(MATH_Vector3, TestCreateFunctions)
    {
        AZ_TEST_ASSERT(Vector3::CreateOne() == Vector3(1.0f, 1.0f, 1.0f));
        AZ_TEST_ASSERT(Vector3::CreateZero() == Vector3(0.0f));
        float values[3] = { 10.0f, 20.0f, 30.0f };
        AZ_TEST_ASSERT(Vector3::CreateFromFloat3(values) == Vector3(10.0f, 20.0f, 30.0f));
        AZ_TEST_ASSERT(Vector3::CreateAxisX() == Vector3(1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3::CreateAxisY() == Vector3(0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3::CreateAxisZ() == Vector3(0.0f, 0.0f, 1.0f));
    }

    TEST(MATH_Vector3, TestCompareEqual)
    {
        Vector3 vA(-100.0f, 10.0f, -1.0f);
        Vector3 vB(35.0f, 10.0f, -5.0f);
        Vector3 vC(35.0f, 20.0f, -1.0f);

        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        Vector3 compareEqualAB = Vector3::CreateSelectCmpEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(compareEqualAB.IsClose(Vector3(0.0f, 1.0f, 0.0f)));
        Vector3 compareEqualBC = Vector3::CreateSelectCmpEqual(vB, vC, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(compareEqualBC.IsClose(Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestCompareGreaterEqual)
    {
        Vector3 vA(-100.0f, 10.0f, -1.0f);
        Vector3 vB(35.0f, 10.0f, -5.0f);
        Vector3 vD(15.0f, 30.0f, 45.0f);

        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        Vector3 compareGreaterEqualAB = Vector3::CreateSelectCmpGreaterEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(compareGreaterEqualAB.IsClose(Vector3(0.0f, 1.0f, 1.0f)));
        Vector3 compareGreaterEqualBD = Vector3::CreateSelectCmpGreaterEqual(vB, vD, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(compareGreaterEqualBD.IsClose(Vector3(1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestCompareGreater)
    {
        Vector3 vA(-100.0f, 10.0f, -1.0f);
        Vector3 vB(35.0f, 10.0f, -5.0f);
        Vector3 vC(35.0f, 20.0f, -1.0f);

        // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        Vector3 compareGreaterAB = Vector3::CreateSelectCmpGreater(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(compareGreaterAB.IsClose(Vector3(0.0f, 0.0f, 1.0f)));
        Vector3 compareGreaterCA = Vector3::CreateSelectCmpGreater(vC, vA, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(compareGreaterCA.IsClose(Vector3(1.0f, 1.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestStoreFloat)
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        float values[3];

        v1.StoreToFloat3(values);
        AZ_TEST_ASSERT(values[0] == 1.0f && values[1] == 2.0f && values[2] == 3.0f);

        float values4[4];
        v1.StoreToFloat4(values4);
        AZ_TEST_ASSERT(values4[0] == 1.0f && values4[1] == 2.0f && values4[2] == 3.0f);
    }

    TEST(MATH_Vector3, TestGetSet)
    {
        Vector3 v1(2.0f, 3.0f, 4.0f);
        float values[3] = { 1.0f, 2.0f, 3.0f };

        AZ_TEST_ASSERT(v1 == Vector3(2.0f, 3.0f, 4.0f));
        v1.SetX(10.0f);
        AZ_TEST_ASSERT(v1 == Vector3(10.0f, 3.0f, 4.0f));
        v1.SetY(11.0f);
        AZ_TEST_ASSERT(v1 == Vector3(10.0f, 11.0f, 4.0f));
        v1.SetZ(12.0f);
        AZ_TEST_ASSERT(v1 == Vector3(10.0f, 11.0f, 12.0f));
        v1.Set(15.0f);
        AZ_TEST_ASSERT(v1 == Vector3(15.0f));
        v1.Set(values);
        AZ_TEST_ASSERT((v1.GetX() == 1.0f) && (v1.GetY() == 2.0f) && (v1.GetZ() == 3.0f));
    }

    TEST(MATH_Vector3, TestIndexElement)
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(v1(0) == 1.0f);
        AZ_TEST_ASSERT(v1(1) == 2.0f);
        AZ_TEST_ASSERT(v1(2) == 3.0f);
        AZ_TEST_ASSERT(v1.GetElement(0) == 1.0f);
        AZ_TEST_ASSERT(v1.GetElement(1) == 2.0f);
        AZ_TEST_ASSERT(v1.GetElement(2) == 3.0f);
        v1.SetElement(0, 5.0f);
        v1.SetElement(1, 6.0f);
        v1.SetElement(2, 7.0f);
        AZ_TEST_ASSERT(v1.GetElement(0) == 5.0f);
        AZ_TEST_ASSERT(v1.GetElement(1) == 6.0f);
        AZ_TEST_ASSERT(v1.GetElement(2) == 7.0f);
    }

    TEST(MATH_Vector3, TestGetLength)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(3.0f, 4.0f, 0.0f).GetLengthSq(), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLength(), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLengthEstimate(), 5.0f);
    }

    TEST(MATH_Vector3, TestGetLengthReciprocal)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocal(), 0.2f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocalEstimate(), 0.2f);
    }

    TEST(MATH_Vector3, TestGetNormalized)
    {
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalized().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedEstimate().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
    }

    TEST(MATH_Vector3, TestGetNormalizedSafe)
    {
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafe().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafeEstimate().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector3(0.0f).GetNormalizedSafe() == Vector3(0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3(0.0f).GetNormalizedSafeEstimate() == Vector3(0.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector3, TestNormalize)
    {
        Vector3 v1(4.0f, 3.0f, 0.0f);
        v1.Normalize();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f);
        v1.NormalizeEstimate();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestNormalizeWithLength)
    {
        Vector3 v1(4.0f, 3.0f, 0.0f);
        float length = v1.NormalizeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f);
        length = v1.NormalizeWithLengthEstimate();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
    }

    TEST(MATH_Vector3, TestNormalizeSafe)
    {
        Vector3 v1(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafe();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        AZ_TEST_ASSERT(v1 == Vector3(0.0f, 0.0f, 0.0f));
        v1.Set(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafeEstimate();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafeEstimate();
        AZ_TEST_ASSERT(v1 == Vector3(0.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector3, TestNormalizeSafeWithLength)
    {
        Vector3 v1(0.0f, 3.0f, 4.0f);
        float length = v1.NormalizeSafeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector3(0.0f, 0.0f, 0.0f));
        v1.Set(0.0f, 3.0f, 4.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector3(0.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector3, TestIsNormalized)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 0.0f, 0.0f).IsNormalized());
        AZ_TEST_ASSERT(Vector3(0.7071f, 0.7071f, 0.0f).IsNormalized());
        AZ_TEST_ASSERT(!Vector3(1.0f, 1.0f, 0.0f).IsNormalized());
    }

    TEST(MATH_Vector3, TestSetLength)
    {
        Vector3 v1(3.0f, 4.0f, 0.0f);
        v1.SetLength(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(6.0f, 8.0f, 0.0f)));
        v1.Set(3.0f, 4.0f, 0.0f);
        v1.SetLengthEstimate(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(6.0f, 8.0f, 0.0f), 1e-3f));
    }

    TEST(MATH_Vector3, TestDistance)
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistanceSq(Vector3(-2.0f, 6.0f, 3.0f)), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistance(Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistanceEstimate(Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);
    }

    TEST(MATH_Vector3, TestLerpSlerpNLerp)
    {
        AZ_TEST_ASSERT(Vector3(4.0f, 5.0f, 6.0f).Lerp(Vector3(5.0f, 10.0f, 2.0f), 0.5f).IsClose(Vector3(4.5f, 7.5f, 4.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 0.0f, 0.0f).Slerp(Vector3(0.0f, 1.0f, 0.0f), 0.5f).IsClose(Vector3(0.7071f, 0.7071f, 0.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 0.0f, 0.0f).Nlerp(Vector3(0.0f, 1.0f, 0.0f), 0.5f).IsClose(Vector3(0.7071f, 0.7071f, 0.0f)));
    }

    TEST(MATH_Vector3, TestDotProduct)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(1.0f, 2.0f, 3.0f).Dot(Vector3(-1.0f, 5.0f, 3.0f)), 18.0f);
    }

    TEST(MATH_Vector3, TestCrossProduct)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).Cross(Vector3(3.0f, 1.0f, -1.0f)) == Vector3(-5.0f, 10.0f, -5.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).CrossXAxis() == Vector3(0.0f, 3.0f, -2.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).CrossYAxis() == Vector3(-3.0f, 0.0f, 1.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).CrossZAxis() == Vector3(2.0f, -1.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).XAxisCross() == Vector3(0.0f, -3.0f, 2.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).YAxisCross() == Vector3(3.0f, 0.0f, -1.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).ZAxisCross() == Vector3(-2.0f, 1.0f, 0.0f));
    }

    TEST(MATH_Vector3, TestIsClose)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 4.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 3.4f), 0.5f));
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
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(0.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(2.0f, 2.0f, 4.0f)));
    }

    TEST(MATH_Vector3, TestIsLessEqualThan)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(0.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(2.0f, 2.0f, 4.0f)));
    }

    TEST(MATH_Vector3, TestIsGreaterThan)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 1.0f, 2.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 3.0f, 2.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 2.0f, 2.0f)));
    }

    TEST(MATH_Vector3, TestIsGreaterEqualThan)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 1.0f, 2.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 3.0f, 2.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 2.0f, 2.0f)));
    }

    TEST(MATH_Vector3, TestMinMax)
    {
        AZ_TEST_ASSERT(Vector3(2.0f, 5.0f, 6.0f).GetMin(Vector3(1.0f, 6.0f, 5.0f)) == Vector3(1.0f, 5.0f, 5.0f));
        AZ_TEST_ASSERT(Vector3(2.0f, 5.0f, 6.0f).GetMax(Vector3(1.0f, 6.0f, 5.0f)) == Vector3(2.0f, 6.0f, 6.0f));
    }

    TEST(MATH_Vector3, TestClamp)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).GetClamp(Vector3(0.0f, -1.0f, 4.0f), Vector3(2.0f, 1.0f, 10.0f)) == Vector3(1.0f, 1.0f, 4.0f));
    }

    TEST(MATH_Vector3, TestTrig)
    {
        AZ_TEST_ASSERT(Vector3(DegToRad(78.0f), DegToRad(-150.0f), DegToRad(190.0f)).GetAngleMod().IsClose(Vector3(DegToRad(78.0f), DegToRad(-150.0f), DegToRad(-170.0f))));
        AZ_TEST_ASSERT(Vector3(DegToRad(390.0f), DegToRad(-190.0f), DegToRad(-400.0f)).GetAngleMod().IsClose(Vector3(DegToRad(30.0f), DegToRad(170.0f), DegToRad(-40.0f))));
        AZ_TEST_ASSERT(Vector3(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f)).GetSin().IsClose(Vector3(0.866f, 0.966f, -0.105f), 0.005f));
        AZ_TEST_ASSERT(Vector3(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f)).GetCos().IsClose(Vector3(0.5f, -0.259f, -0.995f), 0.005f));
        Vector3 sin, cos;
        Vector3 v1(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f));
        v1.GetSinCos(sin, cos);
        AZ_TEST_ASSERT(sin.IsClose(Vector3(0.866f, 0.966f, -0.105f), 0.005f));
        AZ_TEST_ASSERT(cos.IsClose(Vector3(0.5f, -0.259f, -0.995f), 0.005f));
    }

    TEST(MATH_Vector3, TestAbs)
    {
        AZ_TEST_ASSERT(Vector3(-1.0f, 2.0f, -5.0f).GetAbs() == Vector3(1.0f, 2.0f, 5.0f));
    }

    TEST(MATH_Vector3, TestReciprocal)
    {
        AZ_TEST_ASSERT(Vector3(2.0f, 4.0f, 5.0f).GetReciprocal().IsClose(Vector3(0.5f, 0.25f, 0.2f)));
        AZ_TEST_ASSERT(Vector3(2.0f, 4.0f, 5.0f).GetReciprocalEstimate().IsClose(Vector3(0.5f, 0.25f, 0.2f), 1e-3f));
    }

    TEST(MATH_Vector3, TestNegate)
    {
        AZ_TEST_ASSERT((-Vector3(1.0f, 2.0f, -3.0f)) == Vector3(-1.0f, -2.0f, 3.0f));
    }

    TEST(MATH_Vector3, TestAdd)
    {
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) + Vector3(-1.0f, 4.0f, 5.0f)) == Vector3(0.0f, 6.0f, 8.0f));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        AZ_TEST_ASSERT(v1 == Vector3(6.0f, 5.0f, 2.0f));
    }

    TEST(MATH_Vector3, TestSub)
    {
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) - Vector3(-1.0f, 4.0f, 5.0f)) == Vector3(2.0f, -2.0f, -2.0f));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        v1 -= Vector3(2.0f, -1.0f, 3.0f);
        AZ_TEST_ASSERT(v1 == Vector3(4.0f, 6.0f, -1.0f));
    }

    TEST(MATH_Vector3, TestMul)
    {
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) * Vector3(-1.0f, 4.0f, 5.0f)) == Vector3(-1.0f, 8.0f, 15.0f));
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) * 2.0f) == Vector3(2.0f, 4.0f, 6.0f));
        AZ_TEST_ASSERT((2.0f * Vector3(1.0f, 2.0f, 3.0f)) == Vector3(2.0f, 4.0f, 6.0f));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        v1 -= Vector3(2.0f, -1.0f, 3.0f);
        v1 *= 3.0f;
        AZ_TEST_ASSERT(v1 == Vector3(12.0f, 18.0f, -3.0f));
    }

    TEST(MATH_Vector3, TestDiv)
    {
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) / Vector3(-1.0f, 4.0f, 5.0f)).IsClose(Vector3(-1.0f, 0.5f, 3.0f / 5.0f)));
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) / 2.0f).IsClose(Vector3(0.5f, 1.0f, 1.5f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        v1 -= Vector3(2.0f, -1.0f, 3.0f);
        v1 *= 3.0f;
        v1 /= 2.0f;
        AZ_TEST_ASSERT(v1.IsClose(Vector3(6.0f, 9.0f, -1.5f)));
    }

    TEST(MATH_Vector3, TestBuildTangentBasis)
    {
        Vector3 v1 = Vector3(1.0f, 1.0f, 2.0f).GetNormalized();
        Vector3 v2, v3;

        v1.BuildTangentBasis(v2, v3);
        AZ_TEST_ASSERT(v2.IsNormalized());
        AZ_TEST_ASSERT(v3.IsNormalized());
        AZ_TEST_ASSERT(fabsf(v2.Dot(v1)) < 0.001f);
        AZ_TEST_ASSERT(fabsf(v3.Dot(v1)) < 0.001f);
        AZ_TEST_ASSERT(fabsf(v2.Dot(v3)) < 0.001f);
    }

    TEST(MATH_Vector3, TestMadd)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).GetMadd(Vector3(2.0f, 1.0f, 4.0f), Vector3(1.0f, 2.0f, 4.0f)) == Vector3(3.0f, 4.0f, 16.0f));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        v1.Madd(Vector3(2.0f, 1.0f, 4.0f), Vector3(1.0f, 2.0f, 4.0f));
        AZ_TEST_ASSERT(v1 == Vector3(3.0f, 4.0f, 16.0f));
    }

    TEST(MATH_Vector3, TestIsPerpendicular)
    {
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 0.0f).IsPerpendicular(Vector3(0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 0.0f).IsPerpendicular(Vector3(0.0f, 1.0f, 1.0f)));
    }

    TEST(MATH_Vector3, TestGetOrthogonalVector)
    {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2 = v1.GetOrthogonalVector();
        AZ_TEST_ASSERT(v1.IsPerpendicular(v2));
        v1 = Vector3::CreateAxisX();
        v2 = v1.GetOrthogonalVector();
        AZ_TEST_ASSERT(v1.IsPerpendicular(v2));
    }

    TEST(MATH_Vector3, TestProject)
    {
        Vector3 v1(0.5f, 0.5f, 0.5f);
        v1.Project(Vector3(0.0f, 2.0f, 1.0f));
        AZ_TEST_ASSERT(v1 == Vector3(0.0f, 0.6f, 0.3f));
        v1.Set(0.5f, 0.5f, 0.5f);
        v1.ProjectOnNormal(Vector3(0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(v1 == Vector3(0.0f, 0.5f, 0.0f));
        v1.Set(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(v1.GetProjected(Vector3(1.0f, 1.0f, 1.0f)) == Vector3(2.0f, 2.0f, 2.0f));
        AZ_TEST_ASSERT(v1.GetProjectedOnNormal(Vector3(1.0f, 0.0f, 0.0f)) == Vector3(1.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector3, TestIsFinite)
    {
        //IsFinite
        AZ_TEST_ASSERT(Vector3(1.0f, 1.0f, 1.0f).IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        AZ_TEST_ASSERT(!Vector3(infinity, infinity, infinity).IsFinite());
    }

    TEST(MATH_Vector3, TestAngles)
    {
        using Vec3CalcFunc = float(Vector3::*)(const Vector3&) const;
        auto angleTest = [](Vec3CalcFunc func, const Vector3& self, const Vector3& other, float target)
        {
            const float epsilon = 0.01f;
            float value = (self.*func)(other);
            AZ_TEST_ASSERT(AZ::IsClose(value, target, epsilon));
        };

        const Vec3CalcFunc angleFuncs[2] = { &Vector3::Angle, &Vector3::AngleSafe };
        for (Vec3CalcFunc angleFunc : angleFuncs)
        {
            angleTest(angleFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, AZ::Constants::HalfPi);
            angleTest(angleFunc, Vector3{ 42.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 23.0f, 0.0f }, AZ::Constants::HalfPi);
            angleTest(angleFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ -1.0f, 0.0f, 0.0f }, AZ::Constants::Pi);
            angleTest(angleFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 1.0f, 0.0f }, AZ::Constants::QuarterPi);
            angleTest(angleFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleFunc, Vector3{ 1.0f, 1.0f, 0.0f }, Vector3{ -1.0f, -1.0f, 0.0f }, AZ::Constants::Pi);
        }

        const Vec3CalcFunc angleDegFuncs[2] = { &Vector3::AngleDeg, &Vector3::AngleSafeDeg };
        for (Vec3CalcFunc angleDegFunc : angleDegFuncs)
        {
            angleTest(angleDegFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, 90.f);
            angleTest(angleDegFunc, Vector3{ 42.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 23.0f, 0.0f }, 90.f);
            angleTest(angleDegFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ -1.0f, 0.0f, 0.0f }, 180.f);
            angleTest(angleDegFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 1.0f, 0.0f }, 45.f);
            angleTest(angleDegFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleDegFunc, Vector3{ 1.0f, 1.0f, 0.0f }, Vector3{ -1.0f, -1.0f, 0.0f }, 180.f);
        }

        const Vec3CalcFunc angleSafeFuncs[2] = { &Vector3::AngleSafe, &Vector3::AngleSafeDeg };
        for (Vec3CalcFunc angleSafeFunc : angleSafeFuncs)
        {
            angleTest(angleSafeFunc, Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 323432.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector3{ 323432.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, 0.f);
        }
    }

    TEST(MATH_Vector3, CompareTest)
    {
        Vector3 vA(-100.0f, 10.0f, 10.0f);
        Vector3 vB(35.0f, -11.0f, 10.0f);

        // compare equal
        Vector3 rEq = Vector3::CreateSelectCmpEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(rEq.IsClose(Vector3(0.0f, 0.0f, 1.0f)));

        // compare greater equal
        Vector3 rGr = Vector3::CreateSelectCmpGreaterEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(rGr.IsClose(Vector3(0.0f, 1.0f, 1.0f)));

        // compare greater
        Vector3 rGrEq = Vector3::CreateSelectCmpGreater(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(rGrEq.IsClose(Vector3(0.0f, 1.0f, 0.0f)));
    }
}
