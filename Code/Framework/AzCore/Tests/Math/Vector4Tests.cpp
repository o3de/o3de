/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector4.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    float values[4] = { 10.0f, 20.0f, 30.0f, 40.0f };

    TEST(MATH_Vector4, TestConstructors)
    {
        Vector4 v1(0.0f);
        AZ_TEST_ASSERT((v1.GetX() == 0.0f) && (v1.GetY() == 0.0f) && (v1.GetZ() == 0.0f) && (v1.GetW() == 0.0f));
        Vector4 v2(5.0f);
        AZ_TEST_ASSERT((v2.GetX() == 5.0f) && (v2.GetY() == 5.0f) && (v2.GetZ() == 5.0f) && (v2.GetW() == 5.0f));
        Vector4 v3(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT((v3.GetX() == 1.0f) && (v3.GetY() == 2.0f) && (v3.GetZ() == 3.0f) && (v3.GetW() == 4.0f));
    }

    TEST(MATH_Vector4, TestCreateFrom)
    {
        Vector4 v4 = Vector4::CreateFromFloat4(values);
        AZ_TEST_ASSERT((v4.GetX() == 10.0f) && (v4.GetY() == 20.0f) && (v4.GetZ() == 30.0f) && (v4.GetW() == 40.0f));
        Vector4 v5 = Vector4::CreateFromVector3(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT((v5.GetX() == 2.0f) && (v5.GetY() == 3.0f) && (v5.GetZ() == 4.0f) && (v5.GetW() == 1.0f));
        Vector4 v6 = Vector4::CreateFromVector3AndFloat(Vector3(2.0f, 3.0f, 4.0f), 5.0f);
        AZ_TEST_ASSERT((v6.GetX() == 2.0f) && (v6.GetY() == 3.0f) && (v6.GetZ() == 4.0f) && (v6.GetW() == 5.0f));
    }

    TEST(MATH_Vector4, TestCreate)
    {
        AZ_TEST_ASSERT(Vector4::CreateOne() == Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        AZ_TEST_ASSERT(Vector4::CreateZero() == Vector4(0.0f));
    }

    TEST(MATH_Vector4, TestCreateAxis)
    {
        AZ_TEST_ASSERT(Vector4::CreateAxisX() == Vector4(1.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector4::CreateAxisY() == Vector4(0.0f, 1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector4::CreateAxisZ() == Vector4(0.0f, 0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(Vector4::CreateAxisW() == Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    TEST(MATH_Vector4, TestCompareEqual)
    {
        Vector4 vA(-100.0f, 10.0f, -1.0f, 0.0f);
        Vector4 vB(35.0f, 10.0f, -5.0f, 0.0f);
        Vector4 vC(35.0f, 20.0f, -1.0f, 0.0f);

        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        Vector4 compareEqualAB = Vector4::CreateSelectCmpEqual(vA, vB, Vector4(1.0f), Vector4(0.0f));
        AZ_TEST_ASSERT(compareEqualAB.IsClose(Vector4(0.0f, 1.0f, 0.0f, 1.0f)));
        Vector4 compareEqualBC = Vector4::CreateSelectCmpEqual(vB, vC, Vector4(1.0f), Vector4(0.0f));
        AZ_TEST_ASSERT(compareEqualBC.IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Vector4, TestCompareGreaterEqual)
    {
        Vector4 vA(-100.0f, 10.0f, -1.0f, 0.0f);
        Vector4 vB(35.0f, 10.0f, -5.0f, 0.0f);
        Vector4 vD(15.0f, 30.0f, 45.0f, 0.0f);

        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        Vector4 compareGreaterEqualAB = Vector4::CreateSelectCmpGreaterEqual(vA, vB, Vector4(1.0f), Vector4(0.0f));
        AZ_TEST_ASSERT(compareGreaterEqualAB.IsClose(Vector4(0.0f, 1.0f, 1.0f, 1.0f)));
        Vector4 compareGreaterEqualBD = Vector4::CreateSelectCmpGreaterEqual(vB, vD, Vector4(1.0f), Vector4(0.0f));
        AZ_TEST_ASSERT(compareGreaterEqualBD.IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Vector4, TestCompareGreater)
    {
        Vector4 vA(-100.0f, 10.0f, -1.0f, 0.0f);
        Vector4 vB(35.0f, 10.0f, -5.0f, 0.0f);
        Vector4 vC(35.0f, 20.0f, -1.0f, 0.0f);

        // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        Vector4 compareGreaterAB = Vector4::CreateSelectCmpGreater(vA, vB, Vector4(1.0f), Vector4(0.0f));
        AZ_TEST_ASSERT(compareGreaterAB.IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        Vector4 compareGreaterCA = Vector4::CreateSelectCmpGreater(vC, vA, Vector4(1.0f), Vector4(0.0f));
        AZ_TEST_ASSERT(compareGreaterCA.IsClose(Vector4(1.0f, 1.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestGetSet)
    {
        Vector4 v1(2.0f, 3.0f, 4.0f, 5.0f);
        AZ_TEST_ASSERT(v1 == Vector4(2.0f, 3.0f, 4.0f, 5.0f));
        v1.SetX(10.0f);
        AZ_TEST_ASSERT(v1 == Vector4(10.0f, 3.0f, 4.0f, 5.0f));
        v1.SetY(11.0f);
        AZ_TEST_ASSERT(v1 == Vector4(10.0f, 11.0f, 4.0f, 5.0f));
        v1.SetZ(12.0f);
        AZ_TEST_ASSERT(v1 == Vector4(10.0f, 11.0f, 12.0f, 5.0f));
        v1.SetW(13.0f);
        AZ_TEST_ASSERT(v1 == Vector4(10.0f, 11.0f, 12.0f, 13.0f));
        v1.Set(15.0f);
        AZ_TEST_ASSERT(v1 == Vector4(15.0f));
        v1.Set(values);
        AZ_TEST_ASSERT((v1.GetX() == 10.0f) && (v1.GetY() == 20.0f) && (v1.GetZ() == 30.0f) && (v1.GetW() == 40.0f));
        v1.Set(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT((v1.GetX() == 2.0f) && (v1.GetY() == 3.0f) && (v1.GetZ() == 4.0f) && (v1.GetW() == 1.0f));
        v1.Set(Vector3(2.0f, 3.0f, 4.0f), 5.0f);
        AZ_TEST_ASSERT((v1.GetX() == 2.0f) && (v1.GetY() == 3.0f) && (v1.GetZ() == 4.0f) && (v1.GetW() == 5.0f));
    }

    TEST(MATH_Vector4, TestIndexOperators)
    {
        Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(v1.GetElement(0) == 1.0f);
        AZ_TEST_ASSERT(v1.GetElement(1) == 2.0f);
        AZ_TEST_ASSERT(v1.GetElement(2) == 3.0f);
        AZ_TEST_ASSERT(v1.GetElement(3) == 4.0f);
        AZ_TEST_ASSERT(v1(0) == 1.0f);
        AZ_TEST_ASSERT(v1(1) == 2.0f);
        AZ_TEST_ASSERT(v1(2) == 3.0f);
        AZ_TEST_ASSERT(v1(3) == 4.0f);
    }

    TEST(MATH_Vector4, TestGetElementSetElement)
    {
        Vector4 v1;
        v1.SetElement(0, 5.0f);
        v1.SetElement(1, 6.0f);
        v1.SetElement(2, 7.0f);
        v1.SetElement(3, 8.0f);
        AZ_TEST_ASSERT(v1.GetElement(0) == 5.0f);
        AZ_TEST_ASSERT(v1.GetElement(1) == 6.0f);
        AZ_TEST_ASSERT(v1.GetElement(2) == 7.0f);
        AZ_TEST_ASSERT(v1.GetElement(3) == 8.0f);
    }

    TEST(MATH_Vector4, TestEquality)
    {
        Vector4 v3(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(v3 == Vector4(1.0f, 2.0f, 3.0f, 4.0));
        AZ_TEST_ASSERT(!(v3 == Vector4(1.0f, 2.0f, 3.0f, 5.0f)));
        AZ_TEST_ASSERT(v3 != Vector4(1.0f, 2.0f, 3.0f, 5.0f));
        AZ_TEST_ASSERT(!(v3 != Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
    }

    TEST(MATH_Vector4, TestGetLength)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(0.0f, 3.0f, 4.0f, 0.0f).GetLengthSq(), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(0.0f, 0.0f, 4.0f, -3.0f).GetLength(), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(0.0f, 0.0f, 4.0f, -3.0f).GetLengthEstimate(), 5.0f);
    }

    TEST(MATH_Vector4, TestGetLengthReciprocal)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(0.0f, 0.0f, 4.0f, -3.0f).GetLengthReciprocal(), 0.2f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(0.0f, 0.0f, 4.0f, -3.0f).GetLengthReciprocalEstimate(), 0.2f);
    }

    TEST(MATH_Vector4, TestGetNormalized)
    {
        AZ_TEST_ASSERT(Vector4(3.0f, 0.0f, 4.0f, 0.0f).GetNormalized().IsClose(Vector4(3.0f / 5.0f, 0.0f, 4.0f / 5.0f, 0.0f)));
        AZ_TEST_ASSERT(Vector4(3.0f, 0.0f, 4.0f, 0.0f).GetNormalizedEstimate().IsClose(Vector4(3.0f / 5.0f, 0.0f, 4.0f / 5.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestGetNormalizedSafe)
    {
        AZ_TEST_ASSERT(Vector4(3.0f, 0.0f, 4.0f, 0.0f).GetNormalizedSafe().IsClose(Vector4(3.0f / 5.0f, 0.0f, 4.0f / 5.0f, 0.0f)));
        AZ_TEST_ASSERT(Vector4(3.0f, 0.0f, 4.0f, 0.0f).GetNormalizedSafeEstimate().IsClose(Vector4(3.0f / 5.0f, 0.0f, 4.0f / 5.0f, 0.0f)));
        AZ_TEST_ASSERT(Vector4(0.0f).GetNormalizedSafe() == Vector4(0.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector4(0.0f).GetNormalizedSafeEstimate() == Vector4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector4, TestNormalize)
    {
        Vector4 v1(4.0f, 3.0f, 0.0f, 0.0f);
        v1.Normalize();
        AZ_TEST_ASSERT(v1.IsClose(Vector4(4.0f / 5.0f, 3.0f / 5.0f, 0.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f, 0.0f);
        v1.NormalizeEstimate();
        AZ_TEST_ASSERT(v1.IsClose(Vector4(4.0f / 5.0f, 3.0f / 5.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestNormalizeWithLength)
    {
        Vector4 v1(4.0f, 3.0f, 0.0f, 0.0f);
        float length = v1.NormalizeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector4(4.0f / 5.0f, 3.0f / 5.0f, 0.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f, 0.0f);
        length = v1.NormalizeWithLengthEstimate();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector4(4.0f / 5.0f, 3.0f / 5.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestNormalizeSafe)
    {
        Vector4 v1(0.0f, 3.0f, 4.0f, 0.0f);
        v1.NormalizeSafe();
        AZ_TEST_ASSERT(v1.IsClose(Vector4(0.0f, 3.0f / 5.0f, 4.0f / 5.0f, 0.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        AZ_TEST_ASSERT(v1 == Vector4(0.0f, 0.0f, 0.0f, 0.0f));
        v1.Set(0.0f, 3.0f, 4.0f, 0.0f);
        v1.NormalizeSafeEstimate();
        AZ_TEST_ASSERT(v1.IsClose(Vector4(0.0f, 3.0f / 5.0f, 4.0f / 5.0f, 0.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafeEstimate();
        AZ_TEST_ASSERT(v1 == Vector4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector4, TestNormalizeSafeWithLength)
    {
        Vector4 v1(0.0f, 3.0f, 4.0f, 0.0f);
        float length = v1.NormalizeSafeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector4(0.0f, 3.0f / 5.0f, 4.0f / 5.0f, 0.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector4(0.0f, 0.0f, 0.0f, 0.0f));
        v1.Set(0.0f, 3.0f, 4.0f, 0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector4(0.0f, 3.0f / 5.0f, 4.0f / 5.0f, 0.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    TEST(MATH_Vector4, TestIsNormalized)
    {
        AZ_TEST_ASSERT(Vector4(1.0f, 0.0f, 0.0f, 0.0f).IsNormalized());
        AZ_TEST_ASSERT(Vector4(0.7071f, 0.7071f, 0.0f, 0.0f).IsNormalized());
        AZ_TEST_ASSERT(!Vector4(1.0f, 1.0f, 0.0f, 0.0f).IsNormalized());
    }

    TEST(MATH_Vector4, TestSetLength)
    {
        Vector4 v1(3.0f, 4.0f, 0.0f, 0.0f);
        v1.SetLength(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector4(6.0f, 8.0f, 0.0f, 0.0f)));
        v1.Set(3.0f, 4.0f, 0.0f, 0.0f);
        v1.SetLengthEstimate(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector4(6.0f, 8.0f, 0.0f, 0.0f), 1e-3f));
    }

    TEST(MATH_Vector4, TestDistance)
    {
        Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistanceSq(Vector4(-2.0f, 6.0f, 3.0f, 4.0f)), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistance(Vector4(-2.0f, 2.0f, -1.0f, 4.0f)), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistanceEstimate(Vector4(-2.0f, 2.0f, -1.0f, 4.0f)), 5.0f);
    }

    TEST(MATH_Vector4, TestIsLessThan)
    {
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(Vector4(0.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(Vector4(1.0f, 2.0f, 4.0f, 5.0f)));
    }

    TEST(MATH_Vector4, TestIsLessEqualThan)
    {
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(Vector4(0.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(Vector4(2.0f, 2.0f, 4.0f, 5.0f)));
    }

    TEST(MATH_Vector4, TestIsGreaterThan)
    {
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(Vector4(0.0f, 1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(Vector4(0.0f, 3.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(Vector4(0.0f, 2.0f, 2.0f, 3.0f)));
    }

    TEST(MATH_Vector4, TestIsGreaterEqualThan)
    {
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(Vector4(0.0f, 1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(Vector4(0.0f, 3.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(Vector4(0.0f, 2.0f, 2.0f, 3.0f)));
    }

    TEST(MATH_Vector4, TestLerpSlerpNLerp)
    {
        AZ_TEST_ASSERT(Vector4(4.0f, 5.0f, 6.0f, 7.0f).Lerp(Vector4(5.0f, 10.0f, 2.0f, 1.0f), 0.5f).IsClose(Vector4(4.5f, 7.5f, 4.0f, 4.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 0.0f, 0.0f, 0.0f).Slerp(Vector4(0.0f, 1.0f, 0.0f, 0.0f), 0.5f).IsClose(Vector4(0.7071f, 0.7071f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 0.0f, 0.0f, 0.0f).Nlerp(Vector4(0.0f, 1.0f, 0.0f, 0.0f), 0.5f).IsClose(Vector4(0.7071f, 0.7071f, 0.0f, 0.0f)));
    }

    TEST(MATH_Vector4, TestDot)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).Dot(Vector4(-1.0f, 5.0f, 3.0f, 2.0f)), 26.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).Dot3(Vector3(-1.0f, 5.0f, 3.0f)), 18.0f);
    }

    TEST(MATH_Vector4, TestIsClose)
    {
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(Vector4(1.0f, 2.0f, 3.0f, 5.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.4f), 0.5f));
    }

    TEST(MATH_Vector4, TestHomogenize)
    {
        Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(v1.GetHomogenized().IsClose(Vector3(0.25f, 0.5f, 0.75f)));
        v1.Homogenize();
        AZ_TEST_ASSERT(v1.IsClose(Vector4(0.25f, 0.5f, 0.75f, 1.0f)));
    }

    TEST(MATH_Vector4, TestMinMax)
    {
        AZ_TEST_ASSERT(Vector4(2.0f, 5.0f, 6.0f, 7.0f).GetMin(Vector4(1.0f, 6.0f, 5.0f, 4.0f)) == Vector4(1.0f, 5.0f, 5.0f, 4.0f));
        AZ_TEST_ASSERT(Vector4(2.0f, 5.0f, 6.0f, 7.0f).GetMax(Vector4(1.0f, 6.0f, 5.0f, 4.0f)) == Vector4(2.0f, 6.0f, 6.0f, 7.0f));
    }

    TEST(MATH_Vector4, TestClamp)
    {
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).GetClamp(Vector4(0.0f, -1.0f, 4.0f, 4.0f), Vector4(2.0f, 1.0f, 10.0f, 4.0f)) == Vector4(1.0f, 1.0f, 4.0f, 4.0f));
    }

    TEST(MATH_Vector4, TestTrig)
    {
        AZ_TEST_ASSERT(Vector4(DegToRad( 78.0f), DegToRad(-150.0f), DegToRad( 190.0f), DegToRad( 78.0f)).GetAngleMod().IsClose(Vector4(DegToRad(78.0f), DegToRad(-150.0f), DegToRad(-170.0f), DegToRad(78.0f))));
        AZ_TEST_ASSERT(Vector4(DegToRad(390.0f), DegToRad(-190.0f), DegToRad(-400.0f), DegToRad(390.0f)).GetAngleMod().IsClose(Vector4(DegToRad(30.0f), DegToRad(170.0f), DegToRad(-40.0f), DegToRad(30.0f))));
        AZ_TEST_ASSERT(Vector4(DegToRad( 60.0f), DegToRad( 105.0f), DegToRad(-174.0f), DegToRad( 60.0f)).GetSin().IsClose(Vector4(0.866f, 0.966f, -0.105f, 0.866f), 0.005f));
        AZ_TEST_ASSERT(Vector4(DegToRad( 60.0f), DegToRad( 105.0f), DegToRad(-174.0f), DegToRad( 60.0f)).GetCos().IsClose(Vector4(0.5f, -0.259f, -0.995f, 0.5f), 0.005f));
        Vector4 sin, cos;
        Vector4 v1(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f), DegToRad(60.0f));
        v1.GetSinCos(sin, cos);
        AZ_TEST_ASSERT(sin.IsClose(Vector4(0.866f, 0.966f, -0.105f, 0.866f), 0.005f));
        AZ_TEST_ASSERT(cos.IsClose(Vector4(0.5f, -0.259f, -0.995f, 0.5f), 0.005f));
    }

    TEST(MATH_Vector4, TestAbs)
    {
        AZ_TEST_ASSERT(Vector4(-1.0f, 2.0f, -5.0f, 1.0f).GetAbs() == Vector4(1.0f, 2.0f, 5.0f, 1.0f));
        AZ_TEST_ASSERT(Vector4(1.0f, -2.0f, 5.0f, -1.0f).GetAbs() == Vector4(1.0f, 2.0f, 5.0f, 1.0f));
    }

    TEST(MATH_Vector4, TestReciprocal)
    {
        AZ_TEST_ASSERT(Vector4(2.0f, 4.0f, 5.0f, 10.0f).GetReciprocal().IsClose(Vector4(0.5f, 0.25f, 0.2f, 0.1f)));
        AZ_TEST_ASSERT(Vector4(2.0f, 4.0f, 5.0f, 10.0f).GetReciprocalEstimate().IsClose(Vector4(0.5f, 0.25f, 0.2f, 0.1f), 1e-3f));
    }

    TEST(MATH_Vector4, TestNegate)
    {
        AZ_TEST_ASSERT((-Vector4(1.0f, 2.0f, -3.0f, -1.0f)) == Vector4(-1.0f, -2.0f, 3.0f, 1.0f));
    }

    TEST(MATH_Vector4, TestAdd)
    {
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) + Vector4(-1.0f, 4.0f, 5.0f, 2.0f)) == Vector4(0.0f, 6.0f, 8.0f, 6.0f));

        Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        AZ_TEST_ASSERT(v1 == Vector4(6.0f, 5.0f, 2.0f, 6.0f));
    }

    TEST(MATH_Vector4, TestSub)
    {
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) - Vector4(-1.0f, 4.0f, 5.0f, 2.0f)) == Vector4(2.0f, -2.0f, -2.0f, 2.0f));

        Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        v1 -= Vector4(2.0f, -1.0f, 3.0f, 1.0f);
        AZ_TEST_ASSERT(v1 == Vector4(4.0f, 6.0f, -1.0f, 5.0f));
    }

    TEST(MATH_Vector4, TestMultiply)
    {
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) * Vector4(-1.0f, 4.0f, 5.0f, 2.0f)) == Vector4(-1.0f, 8.0f, 15.0f, 8.0f));
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f) == Vector4(2.0f, 4.0f, 6.0f, 8.0f));
        AZ_TEST_ASSERT((2.0f * Vector4(1.0f, 2.0f, 3.0f, 4.0f)) == Vector4(2.0f, 4.0f, 6.0f, 8.0f));

        Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        v1 -= Vector4(2.0f, -1.0f, 3.0f, 1.0f);
        v1 *= 3.0f;
        AZ_TEST_ASSERT(v1 == Vector4(12.0f, 18.0f, -3.0f, 15.0f));
    }

    TEST(MATH_Vector4, TestDivide)
    {
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) / Vector4(-1.0f, 4.0f, 5.0f, 2.0f)).IsClose(Vector4(-1.0f, 0.5f, 3.0f / 5.0f, 2.0f)));
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) / 2.0f).IsClose(Vector4(0.5f, 1.0f, 1.5f, 2.0f)));

        Vector4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        v1 -= Vector4(2.0f, -1.0f, 3.0f, 1.0f);
        v1 *= 3.0f;
        v1 /= 2.0f;
        AZ_TEST_ASSERT(v1.IsClose(Vector4(6.0f, 9.0f, -1.5f, 7.5f)));
    }

    TEST(MATH_Vector4, TestAngles)
    {
        using Vec4CalcFunc = float(Vector4::*)(const Vector4&) const;
        auto angleTest = [](Vec4CalcFunc func, const Vector4& self, const Vector4& other, float target)
        {
            const float epsilon = 0.01f;
            float value = (self.*func)(other);
            AZ_TEST_ASSERT(AZ::IsClose(value, target, epsilon));
        };

        const Vec4CalcFunc angleFuncs[2] = { &Vector4::Angle, &Vector4::AngleSafe };
        for (Vec4CalcFunc angleFunc : angleFuncs)
        {
            angleTest(angleFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, AZ::Constants::HalfPi);
            angleTest(angleFunc, Vector4{ 42.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 23.0f, 0.0f, 0.0f }, AZ::Constants::HalfPi);
            angleTest(angleFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ -1.0f, 0.0f, 0.0f, 0.0f }, AZ::Constants::Pi);
            angleTest(angleFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 1.0f, 1.0f, 0.0f, 0.0f }, AZ::Constants::QuarterPi);
            angleTest(angleFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleFunc, Vector4{ 1.0f, 1.0f, 0.0f, 0.0f }, Vector4{ -1.0f, -1.0f, 0.0f, 0.0f }, AZ::Constants::Pi);
        }

        const Vec4CalcFunc angleDegFuncs[2] = { &Vector4::AngleDeg, &Vector4::AngleSafeDeg };
        for (Vec4CalcFunc angleDegFunc : angleDegFuncs)
        {
            angleTest(angleDegFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, 90.f);
            angleTest(angleDegFunc, Vector4{ 42.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 23.0f, 0.0f, 0.0f }, 90.f);
            angleTest(angleDegFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ -1.0f, 0.0f, 0.0f, 0.0f }, 180.f);
            angleTest(angleDegFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 1.0f, 1.0f, 0.0f, 0.0f }, 45.f);
            angleTest(angleDegFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleDegFunc, Vector4{ 1.0f, 1.0f, 0.0f, 0.0f }, Vector4{ -1.0f, -1.0f, 0.0f, 0.0f }, 180.f);
        }

        const Vec4CalcFunc angleSafeFuncs[2] = { &Vector4::AngleSafe, &Vector4::AngleSafeDeg };
        for (Vec4CalcFunc angleSafeFunc : angleSafeFuncs)
        {
            angleTest(angleSafeFunc, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector4{ 1.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 323432.0f, 0.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector4{ 323432.0f, 0.0f, 0.0f, 0.0f }, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.f);
        }
    }
}
