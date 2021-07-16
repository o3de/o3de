/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Vector2, TestConstructors)
    {
        Vector2 vA(0.0f);
        AZ_TEST_ASSERT((vA.GetX() == 0.0f) && (vA.GetY() == 0.0f));
        Vector2 vB(5.0f);
        AZ_TEST_ASSERT((vB.GetX() == 5.0f) && (vB.GetY() == 5.0f));
        Vector2 vC(1.0f, 2.0f);
        AZ_TEST_ASSERT((vC.GetX() == 1.0f) && (vC.GetY() == 2.0f));
    }

    TEST(MATH_Vector2, TestCreate)
    {
        float values[2] = { 10.0f, 20.0f };

        AZ_TEST_ASSERT(Vector2::CreateOne() == Vector2(1.0f, 1.0f));
        AZ_TEST_ASSERT(Vector2::CreateZero() == Vector2(0.0f, 0.0f));

        AZ_TEST_ASSERT(Vector2::CreateFromFloat2(values) == Vector2(10.0f, 20.0f));
        AZ_TEST_ASSERT(Vector2::CreateAxisX() == Vector2(1.0f, 0.0f));
        AZ_TEST_ASSERT(Vector2::CreateAxisY() == Vector2(0.0f, 1.0f));
    }

    TEST(MATH_Vector2, TestCreateFromAngle)
    {
        AZ_TEST_ASSERT(Vector2::CreateFromAngle() == Vector2(0.0f, 1.0f));
        AZ_TEST_ASSERT(Vector2::CreateFromAngle(0.78539816339f).IsClose(Vector2(0.7071067811f, 0.7071067811f)));
        AZ_TEST_ASSERT(Vector2::CreateFromAngle(4.0f).IsClose(Vector2(-0.7568024953f, -0.6536436208f)));
        AZ_TEST_ASSERT(Vector2::CreateFromAngle(-1.0f).IsClose(Vector2(-0.8414709848f, 0.5403023058f)));
    }

    TEST(MATH_Vector2, TestCompareEqual)
    {
        Vector2 vA(-100.0f, 10.0f);
        Vector2 vB(35.0f, 10.0f);
        Vector2 vC(35.0f, 20.0f);

        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        Vector2 compareEqualAB = Vector2::CreateSelectCmpEqual(vA, vB, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareEqualAB.IsClose(Vector2(0.0f, 1.0f)));
        Vector2 compareEqualBC = Vector2::CreateSelectCmpEqual(vB, vC, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareEqualBC.IsClose(Vector2(1.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestCompareGreaterEqual)
    {
        Vector2 vA(-100.0f, 10.0f);
        Vector2 vB(35.0f, 10.0f);
        Vector2 vD(15.0f, 30.0f);

        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        Vector2 compareGreaterEqualAB = Vector2::CreateSelectCmpGreaterEqual(vA, vB, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareGreaterEqualAB.IsClose(Vector2(0.0f, 1.0f)));
        Vector2 compareGreaterEqualBD = Vector2::CreateSelectCmpGreaterEqual(vB, vD, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareGreaterEqualBD.IsClose(Vector2(1.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestCompareGreater)
    {
        Vector2 vA(-100.0f, 10.0f);
        Vector2 vB(35.0f, 10.0f);
        Vector2 vC(35.0f, 20.0f);

        // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        Vector2 compareGreaterAB = Vector2::CreateSelectCmpGreater(vA, vB, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareGreaterAB.IsClose(Vector2(0.0f, 0.0f)));
        Vector2 compareGreaterCA = Vector2::CreateSelectCmpGreater(vC, vA, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareGreaterCA.IsClose(Vector2(1.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestStoreFloat)
    {
        float values[2] = { 0.0f, 0.0f };

        Vector2 vA(1.0f, 2.0f);
        vA.StoreToFloat2(values);
        AZ_TEST_ASSERT(values[0] == 1.0f && values[1] == 2.0f);
    }

    TEST(MATH_Vector2, TestGetSet)
    {
        Vector2 vA(2.0f, 3.0f);
        AZ_TEST_ASSERT(vA == Vector2(2.0f, 3.0f));
        AZ_TEST_ASSERT(vA.GetX() == 2.0f && vA.GetY() == 3.0f);
        vA.SetX(10.0f);
        AZ_TEST_ASSERT(vA == Vector2(10.0f, 3.0f));
        vA.SetY(11.0f);
        AZ_TEST_ASSERT(vA == Vector2(10.0f, 11.0f));
        vA.Set(15.0f);
        AZ_TEST_ASSERT(vA == Vector2(15.0f));
        vA.SetElement(0, 20.0f);
        AZ_TEST_ASSERT(vA.GetX() == 20.0f && vA.GetY() == 15.0f);
        AZ_TEST_ASSERT(vA.GetElement(0) == 20.0f && vA.GetElement(1) == 15.0f);
        AZ_TEST_ASSERT(vA(0) == 20.0f && vA(1) == 15.0f);
        vA.SetElement(1, 21.0f);
        AZ_TEST_ASSERT(vA.GetX() == 20.0f && vA.GetY() == 21.0f);
        AZ_TEST_ASSERT(vA.GetElement(0) == 20.0f && vA.GetElement(1) == 21.0f);
        AZ_TEST_ASSERT(vA(0) == 20.0f && vA(1) == 21.0f);
    }


    TEST(MATH_Vector2, TestGetLength)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(3.0f, 4.0f).GetLengthSq(), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(4.0f, -3.0f).GetLength(), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(4.0f, -3.0f).GetLengthEstimate(), 5.0f);
    }

    TEST(MATH_Vector2, TestGetLengthReciprocal)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(4.0f, -3.0f).GetLengthReciprocal(), 0.2f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(4.0f, -3.0f).GetLengthReciprocalEstimate(), 0.2f);
    }

    TEST(MATH_Vector2, TestGetNormalized)
    {
        AZ_TEST_ASSERT(Vector2(3.0f, 4.0f).GetNormalized().IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector2(3.0f, 4.0f).GetNormalizedEstimate().IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
    }

    TEST(MATH_Vector2, TestGetNormalizedSafe)
    {
        AZ_TEST_ASSERT(Vector2(3.0f, 4.0f).GetNormalizedSafe().IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector2(3.0f, 4.0f).GetNormalizedSafeEstimate().IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector2(0.0f).GetNormalizedSafe() == Vector2(0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector2(0.0f).GetNormalizedSafeEstimate() == Vector2(0.0f, 0.0f));
    }

    TEST(MATH_Vector2, TestNormalize)
    {
        Vector2 v1(4.0f, 3.0f);
        v1.Normalize();
        AZ_TEST_ASSERT(v1.IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));
        v1.Set(4.0f, 3.0f);
        v1.NormalizeEstimate();
        AZ_TEST_ASSERT(v1.IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));
    }

    TEST(MATH_Vector2, TestNormalizeWithLength)
    {
        Vector2 v1(4.0f, 3.0f);
        float length = v1.NormalizeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));
        v1.Set(4.0f, 3.0f);
        length = v1.NormalizeWithLengthEstimate();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));
    }

    TEST(MATH_Vector2, TestNormalizeSafe)
    {
        Vector2 v1(3.0f, 4.0f);
        v1.NormalizeSafe();
        AZ_TEST_ASSERT(v1.IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        AZ_TEST_ASSERT(v1 == Vector2(0.0f, 0.0f));
        v1.Set(3.0f, 4.0f);
        v1.NormalizeSafeEstimate();
        AZ_TEST_ASSERT(v1.IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafeEstimate();
        AZ_TEST_ASSERT(v1 == Vector2(0.0f, 0.0f));
    }

    TEST(MATH_Vector2, TestNormalizeSafeWithLength)
    {
        Vector2 v1(3.0f, 4.0f);
        float length = v1.NormalizeSafeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector2(0.0f, 0.0f));
        v1.Set(3.0f, 4.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector2(0.0f, 0.0f));
    }

    TEST(MATH_Vector2, TestIsNormalized)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 0.0f).IsNormalized());
        AZ_TEST_ASSERT(Vector2(0.7071f, 0.7071f).IsNormalized());
        AZ_TEST_ASSERT(!Vector2(1.0f, 1.0f).IsNormalized());
    }

    TEST(MATH_Vector2, TestSetLength)
    {
        Vector2 v1(3.0f, 4.0f);
        v1.SetLength(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector2(6.0f, 8.0f)));
        v1.Set(3.0f, 4.0f);
        v1.SetLengthEstimate(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector2(6.0f, 8.0f), 1e-3f));
    }

    TEST(MATH_Vector2, TestDistance)
    {
        Vector2 vA(1.0f, 2.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(vA.GetDistanceSq(Vector2(-2.0f, 6.0f)), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(vA.GetDistance(Vector2(5.0f, -1.0f)), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(vA.GetDistanceEstimate(Vector2(5.0f, -1.0f)), 5.0f);
    }

    TEST(MATH_Vector2, TestLerpSlerpNLerp)
    {
        AZ_TEST_ASSERT(Vector2(4.0f, 5.0f).Lerp(Vector2(5.0f, 10.0f), 0.5f).IsClose(Vector2(4.5f, 7.5f)));
        AZ_TEST_ASSERT(Vector2(1.0f, 0.0f).Slerp(Vector2(0.0f, 1.0f), 0.5f).IsClose(Vector2(0.7071f, 0.7071f)));
        AZ_TEST_ASSERT(Vector2(1.0f, 0.0f).Nlerp(Vector2(0.0f, 1.0f), 0.5f).IsClose(Vector2(0.7071f, 0.7071f)));
    }

    TEST(MATH_Vector2, TestGetPerpendicular)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).GetPerpendicular() == Vector2(-2.0f, 1.0f));
    }

    TEST(MATH_Vector2, TestIsClose)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 2.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 3.0f)));

        // Verify tolerance works
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 2.4f), 0.5f));
    }

    TEST(MATH_Vector2, TestIsZero)
    {
        AZ_TEST_ASSERT(Vector2(0.0f).IsZero());
        AZ_TEST_ASSERT(!Vector2(1.0f).IsZero());

        // Verify tolerance works
        AZ_TEST_ASSERT(Vector2(0.05f).IsZero(0.1f));
    }

    TEST(MATH_Vector2, TestEqualityInequality)
    {
        Vector2 vA(1.0f, 2.0f);
        AZ_TEST_ASSERT(vA == Vector2(1.0f, 2.0f));
        AZ_TEST_ASSERT(!(vA == Vector2(5.0f, 6.0f)));
        AZ_TEST_ASSERT(vA != Vector2(7.0f, 8.0f));
        AZ_TEST_ASSERT(!(vA != Vector2(1.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsLessThan)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsLessThan(Vector2(2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsLessThan(Vector2(0.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsLessThan(Vector2(2.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsLessEqualThan)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(0.0f, 3.0f)));
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(2.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsGreaterThan)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 1.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsGreaterEqualThan)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 1.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 3.0f)));
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestMinMax)
    {
        AZ_TEST_ASSERT(Vector2(2.0f, 3.0f).GetMin(Vector2(1.0f, 4.0f)) == Vector2(1.0f, 3.0f));
        AZ_TEST_ASSERT(Vector2(2.0f, 3.0f).GetMax(Vector2(1.0f, 4.0f)) == Vector2(2.0f, 4.0f));
    }

    TEST(MATH_Vector2, TestClamp)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).GetClamp(Vector2(5.0f, -10.0f), Vector2(10.0f, -5.0f)) == Vector2(5.0f, -5.0f));
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).GetClamp(Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f)) == Vector2(1.0f, 2.0f));
    }

    TEST(MATH_Vector2, TestSelect)
    {
        Vector2 vA(1.0f);
        Vector2 vB(2.0f);
        AZ_TEST_ASSERT(vA.GetSelect(Vector2(0.0f, 1.0f), vB) == Vector2(1.0f, 2.0f));
        vA.Select(Vector2(1.0f, 0.0f), vB);
        AZ_TEST_ASSERT(vA == Vector2(2.0f, 1.0f));
    }

    TEST(MATH_Vector2, TestAbs)
    {
        AZ_TEST_ASSERT(Vector2(-1.0f, 2.0f).GetAbs() == Vector2(1.0f, 2.0f));
        AZ_TEST_ASSERT(Vector2(3.0f, -4.0f).GetAbs() == Vector2(3.0f, 4.0f));
    }

    TEST(MATH_Vector2, TestReciprocal)
    {
        AZ_TEST_ASSERT(Vector2(2.0f, 4.0f).GetReciprocal().IsClose(Vector2(0.5f, 0.25f)));
        AZ_TEST_ASSERT(Vector2(2.0f, 4.0f).GetReciprocalEstimate().IsClose(Vector2(0.5f, 0.25f)));
    }

    TEST(MATH_Vector2, TestAdd)
    {
        Vector2 vA(1.0f, 2.0f);
        vA += Vector2(3.0f, 4.0f);
        AZ_TEST_ASSERT(vA == Vector2(4.0f, 6.0f));
        AZ_TEST_ASSERT((Vector2(1.0f, 2.0f) + Vector2(2.0f, -1.0f)) == Vector2(3.0f, 1.0f));
    }

    TEST(MATH_Vector2, TestSub)
    {
        Vector2 vA(10.0f, 11.0f);
        vA -= Vector2(2.0f, 4.0f);
        AZ_TEST_ASSERT(vA == Vector2(8.0f, 7.0f));
        AZ_TEST_ASSERT((Vector2(1.0f, 2.0f) - Vector2(2.0f, -1.0f)) == Vector2(-1.0f, 3.0f));
    }

    TEST(MATH_Vector2, TestMul)
    {
        Vector2 vA(2.0f, 4.0f);
        vA *= Vector2(3.0f, 6.0f);
        AZ_TEST_ASSERT(vA == Vector2(6.0f, 24.0f));

        vA.Set(2.0f, 3.0f);
        vA *= 5.0f;
        AZ_TEST_ASSERT(vA == Vector2(10.0f, 15.0f));

        AZ_TEST_ASSERT((Vector2(3.0f, 2.0f) * Vector2(2.0f, -4.0f)) == Vector2(6.0f, -8.0f));
        AZ_TEST_ASSERT((Vector2(3.0f, 2.0f) * 2.0f) == Vector2(6.0f, 4.0f));
    }

    TEST(MATH_Vector2, TestDiv)
    {
        Vector2 vA(15.0f, 20.0f);
        vA /= Vector2(3.0f, 2.0f);
        AZ_TEST_ASSERT(vA == Vector2(5.0f, 10.0f));

        vA.Set(20.0f, 30.0f);
        vA /= 10.0f;
        AZ_TEST_ASSERT(vA == Vector2(2.0f, 3.0f));

        AZ_TEST_ASSERT((Vector2(30.0f, 20.0f) / Vector2(10.0f, -4.0f)) == Vector2(3.0f, -5.0f));
        AZ_TEST_ASSERT((Vector2(30.0f, 20.0f) / 10.0f) == Vector2(3.0f, 2.0f));
    }

    TEST(MATH_Vector2, TestNegate)
    {
        AZ_TEST_ASSERT((-Vector2(1.0f, -2.0f)) == Vector2(-1.0f, 2.0f));
    }

    TEST(MATH_Vector2, TestDot)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(1.0f, 2.0f).Dot(Vector2(-1.0f, 5.0f)), 9.0f);
    }

    TEST(MATH_Vector2, TestMadd)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).GetMadd(Vector2(2.0f, 6.0f), Vector2(3.0f, 4.0f)) == Vector2(5.0f, 16.0f));
        Vector2 vA(1.0f, 2.0f);
        vA.Madd(Vector2(2.0f, 6.0f), Vector2(3.0f, 4.0f));
        AZ_TEST_ASSERT(vA == Vector2(5.0f, 16.0f));
    }

    TEST(MATH_Vector2, TestProject)
    {
        Vector2 vA(0.5f);
        vA.Project(Vector2(0.0f, 2.0f));
        AZ_TEST_ASSERT(vA == Vector2(0.0f, 0.5f));
        vA.Set(0.5f);
        vA.ProjectOnNormal(Vector2(0.0f, 1.0f));
        AZ_TEST_ASSERT(vA == Vector2(0.0f, 0.5f));
        vA.Set(2.0f, 4.0f);
        AZ_TEST_ASSERT(vA.GetProjected(Vector2(1.0f, 1.0f)) == Vector2(3.0f, 3.0f));
        AZ_TEST_ASSERT(vA.GetProjectedOnNormal(Vector2(1.0f, 0.0f)) == Vector2(2.0f, 0.0f));
    }

    TEST(MATH_Vector2, TestTrig)
    {
        AZ_TEST_ASSERT(Vector2(DegToRad(78.0f), DegToRad(-150.0f)).GetAngleMod().IsClose(Vector2(DegToRad(78.0f), DegToRad(-150.0f))));
        AZ_TEST_ASSERT(Vector2(DegToRad(390.0f), DegToRad(-190.0f)).GetAngleMod().IsClose(Vector2(DegToRad(30.0f), DegToRad(170.0f))));
        AZ_TEST_ASSERT(Vector2(DegToRad(60.0f), DegToRad(105.0f)).GetSin().IsClose(Vector2(0.866f, 0.966f), 0.005f));
        AZ_TEST_ASSERT(Vector2(DegToRad(60.0f), DegToRad(105.0f)).GetCos().IsClose(Vector2(0.5f, -0.259f), 0.005f));
        Vector2 sin, cos;
        Vector2 v1(DegToRad(60.0f), DegToRad(105.0f));
        v1.GetSinCos(sin, cos);
        AZ_TEST_ASSERT(sin.IsClose(Vector2(0.866f, 0.966f), 0.005f));
        AZ_TEST_ASSERT(cos.IsClose(Vector2(0.5f, -0.259f), 0.005f));
    }

    TEST(MATH_Vector2, TestIsFinite)
    {
        AZ_TEST_ASSERT(Vector2(1.0f, 1.0f).IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        AZ_TEST_ASSERT(!Vector2(infinity, infinity).IsFinite());
    }

    TEST(MATH_Vector2, TestAngles)
    {
        using Vec2CalcFunc = float(Vector2::*)(const Vector2&) const;
        auto angleTest = [](Vec2CalcFunc func, const Vector2& self, const Vector2& other, const float target)
        {
            const float epsilon = 0.01f;
            float value = (self.*func)(other);
            AZ_TEST_ASSERT(AZ::IsClose(value, target, epsilon));
        };

        const Vec2CalcFunc angleFuncs[2] = { &Vector2::Angle, &Vector2::AngleSafe };
        for (Vec2CalcFunc angleFunc : angleFuncs)
        {
            angleTest(angleFunc, Vector2{ 1.0f, 0.0f }, Vector2{ 0.0f, 1.0f }, AZ::Constants::HalfPi);
            angleTest(angleFunc, Vector2{ 42.0f, 0.0f }, Vector2{ 0.0f, 23.0f }, AZ::Constants::HalfPi);
            angleTest(angleFunc, Vector2{ 1.0f, 0.0f }, Vector2{ -1.0f, 0.0f }, AZ::Constants::Pi);
            angleTest(angleFunc, Vector2{ 1.0f, 0.0f }, Vector2{ 1.0f, 1.0f }, AZ::Constants::QuarterPi);
            angleTest(angleFunc, Vector2{ 1.0f, 0.0f }, Vector2{ 1.0f, 0.0f }, 0.f);
            angleTest(angleFunc, Vector2{ 1.0f, 1.0f }, Vector2{ -1.0f, -1.0f }, AZ::Constants::Pi);
        }

        const Vec2CalcFunc angleDegFuncs[2] = { &Vector2::AngleDeg, &Vector2::AngleSafeDeg };
        for (Vec2CalcFunc angleDegFunc : angleDegFuncs)
        {
            angleTest(angleDegFunc, Vector2{ 1.0f, 0.0f }, Vector2{ 0.0f, 1.0f }, 90.f);
            angleTest(angleDegFunc, Vector2{ 42.0f, 0.0f }, Vector2{ 0.0f, 23.0f }, 90.f);
            angleTest(angleDegFunc, Vector2{ 1.0f, 0.0f }, Vector2{ -1.0f, 0.0f }, 180.f);
            angleTest(angleDegFunc, Vector2{ 1.0f, 0.0f }, Vector2{ 1.0f, 1.0f }, 45.f);
            angleTest(angleDegFunc, Vector2{ 1.0f, 0.0f }, Vector2{ 1.0f, 0.0f }, 0.f);
            angleTest(angleDegFunc, Vector2{ 1.0f, 1.0f }, Vector2{ -1.0f, -1.0f }, 180.f);
        }

        const Vec2CalcFunc angleSafeFuncs[2] = { &Vector2::AngleSafe, &Vector2::AngleSafeDeg };
        for (Vec2CalcFunc angleSafeFunc : angleSafeFuncs)
        {
            angleTest(angleSafeFunc, Vector2{ 0.0f, 0.0f }, Vector2{ 0.0f, 1.0f }, 0.f);
            angleTest(angleSafeFunc, Vector2{ 0.0f, 0.0f }, Vector2{ 0.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector2{ 1.0f, 0.0f }, Vector2{ 0.0f, 0.0f }, 0.f);
            angleTest(angleSafeFunc, Vector2{ 0.0f, 0.0f }, Vector2{ 0.0f, 323432.0f }, 0.f);
            angleTest(angleSafeFunc, Vector2{ 323432.0f, 0.0f }, Vector2{ 0.0f, 0.0f }, 0.f);
        }
    }
}
