/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Math/SimdMath.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Vector2, TestConstructors)
    {
        Vector2 vA(0.0f);
        EXPECT_FLOAT_EQ(vA.GetX(), 0.0f);
        EXPECT_FLOAT_EQ(vA.GetY(), 0.0f);
        Vector2 vB(5.0f);
        EXPECT_FLOAT_EQ(vB.GetX(), 5.0f);
        EXPECT_FLOAT_EQ(vB.GetY(), 5.0f);
        Vector2 vC(1.0f, 2.0f);
        EXPECT_FLOAT_EQ(vC.GetX(), 1.0f);
        EXPECT_FLOAT_EQ(vC.GetY(), 2.0f);
        EXPECT_THAT(Vector2(Vector4(1.0f, 3.0f, 2.0f, 4.0f)), IsClose(Vector2(1.0f, 3.0f)));
        EXPECT_THAT(Vector2(Vector3(1.0f, 3.0f, 2.0f)), IsClose(Vector2(1.0f, 3.0f)));
    }

    TEST(MATH_Vector2, TestCreate)
    {
        float values[2] = { 10.0f, 20.0f };

        EXPECT_THAT(Vector2::CreateOne(), IsClose(Vector2(1.0f, 1.0f)));
        EXPECT_THAT(Vector2::CreateZero(), IsClose(Vector2(0.0f, 0.0f)));

        EXPECT_THAT(Vector2::CreateFromFloat2(values), IsClose(Vector2(10.0f, 20.0f)));
        EXPECT_THAT(Vector2::CreateAxisX(), IsClose(Vector2(1.0f, 0.0f)));
        EXPECT_THAT(Vector2::CreateAxisY(), IsClose(Vector2(0.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestCreateFromAngle)
    {
        EXPECT_THAT(Vector2::CreateFromAngle(), IsClose(Vector2(0.0f, 1.0f)));
        EXPECT_THAT(Vector2::CreateFromAngle(0.78539816339f), IsClose(Vector2(0.7071067811f, 0.7071067811f)));
        EXPECT_THAT(Vector2::CreateFromAngle(4.0f), IsClose(Vector2(-0.7568024953f, -0.6536436208f)));
        EXPECT_THAT(Vector2::CreateFromAngle(-1.0f), IsClose(Vector2(-0.8414709848f, 0.5403023058f)));
    }

    TEST(MATH_Vector2, TestCompareEqual)
    {
        Vector2 vA(-100.0f, 10.0f);
        Vector2 vB(35.0f, 10.0f);
        Vector2 vC(35.0f, 20.0f);

        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        Vector2 compareEqualAB = Vector2::CreateSelectCmpEqual(vA, vB, Vector2(1.0f), Vector2(0.0f));
        EXPECT_THAT(compareEqualAB, IsClose(Vector2(0.0f, 1.0f)));
        Vector2 compareEqualBC = Vector2::CreateSelectCmpEqual(vB, vC, Vector2(1.0f), Vector2(0.0f));
        EXPECT_THAT(compareEqualBC, IsClose(Vector2(1.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestCompareGreaterEqual)
    {
        Vector2 vA(-100.0f, 10.0f);
        Vector2 vB(35.0f, 10.0f);
        Vector2 vD(15.0f, 30.0f);

        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        Vector2 compareGreaterEqualAB = Vector2::CreateSelectCmpGreaterEqual(vA, vB, Vector2(1.0f), Vector2(0.0f));
        EXPECT_THAT(compareGreaterEqualAB, IsClose(Vector2(0.0f, 1.0f)));
        Vector2 compareGreaterEqualBD = Vector2::CreateSelectCmpGreaterEqual(vB, vD, Vector2(1.0f), Vector2(0.0f));
        EXPECT_THAT(compareGreaterEqualBD, IsClose(Vector2(1.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestCompareGreater)
    {
        Vector2 vA(-100.0f, 10.0f);
        Vector2 vB(35.0f, 10.0f);
        Vector2 vC(35.0f, 20.0f);

        // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        Vector2 compareGreaterAB = Vector2::CreateSelectCmpGreater(vA, vB, Vector2(1.0f), Vector2(0.0f));
        EXPECT_THAT(compareGreaterAB, IsClose(Vector2(0.0f, 0.0f)));
        Vector2 compareGreaterCA = Vector2::CreateSelectCmpGreater(vC, vA, Vector2(1.0f), Vector2(0.0f));
        EXPECT_THAT(compareGreaterCA, IsClose(Vector2(1.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestStoreFloat)
    {
        float values[2] = { 0.0f, 0.0f };

        Vector2 vA(1.0f, 2.0f);
        vA.StoreToFloat2(values);
        EXPECT_EQ(values[0], 1.0f);
        EXPECT_EQ(values[1], 2.0f);
    }

    TEST(MATH_Vector2, TestGetSet)
    {
        Vector2 vA(2.0f, 3.0f);

        EXPECT_TRUE(vA == Vector2(2.0f, 3.0f));
        EXPECT_FLOAT_EQ(vA.GetX(), 2.0f);
        EXPECT_FLOAT_EQ(vA.GetY(), 3.0f);

        vA.SetX(10.0f);
        EXPECT_TRUE(vA == Vector2(10.0f, 3.0f));

        vA.SetY(11.0f);
        EXPECT_TRUE(vA == Vector2(10.0f, 11.0f));

        vA.Set(15.0f);
        EXPECT_TRUE(vA == Vector2(15.0f));

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
        EXPECT_FLOAT_EQ(Vector2(3.0f, 4.0f).GetLengthSq(), 25.0f);
        EXPECT_FLOAT_EQ(Vector2(4.0f, -3.0f).GetLength(), 5.0f);
        EXPECT_FLOAT_EQ(Vector2(4.0f, -3.0f).GetLengthEstimate(), 5.0f);
    }

    TEST(MATH_Vector2, TestGetLengthReciprocal)
    {
        EXPECT_NEAR(Vector2(4.0f, -3.0f).GetLengthReciprocal(), 0.2f, 0.01f);
        EXPECT_NEAR(Vector2(4.0f, -3.0f).GetLengthReciprocalEstimate(), 0.2f, 0.01f);
    }

    TEST(MATH_Vector2, TestGetNormalized)
    {
        EXPECT_THAT(Vector2(3.0f, 4.0f).GetNormalized(), IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        EXPECT_THAT(Vector2(3.0f, 4.0f).GetNormalizedEstimate(), IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
    }

    TEST(MATH_Vector2, TestGetNormalizedSafe)
    {
        EXPECT_THAT(Vector2(3.0f, 4.0f).GetNormalizedSafe(), IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        EXPECT_THAT(Vector2(3.0f, 4.0f).GetNormalizedSafeEstimate(), IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        EXPECT_THAT(Vector2(0.0f).GetNormalizedSafe(), IsClose(Vector2(0.0f, 0.0f)));
        EXPECT_THAT(Vector2(0.0f).GetNormalizedSafeEstimate(), IsClose(Vector2(0.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestNormalize)
    {
        Vector2 v1(4.0f, 3.0f);
        v1.Normalize();
        EXPECT_THAT(v1, IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));
        v1.Set(4.0f, 3.0f);
        v1.NormalizeEstimate();
        EXPECT_THAT(v1, IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));
    }

    TEST(MATH_Vector2, TestNormalizeWithLength)
    {
        Vector2 v1(4.0f, 3.0f);
        float length = v1.NormalizeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));
        v1.Set(4.0f, 3.0f);
        length = v1.NormalizeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));
    }

    TEST(MATH_Vector2, TestNormalizeSafe)
    {
        Vector2 v1(3.0f, 4.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        EXPECT_THAT(v1, IsClose(Vector2(0.0f, 0.0f)));
        v1.Set(3.0f, 4.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafeEstimate();
        EXPECT_THAT(v1, IsClose(Vector2(0.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestNormalizeSafeWithLength)
    {
        Vector2 v1(3.0f, 4.0f);
        float length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(Vector2(0.0f, 0.0f)));
        v1.Set(3.0f, 4.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 5.0f);
        EXPECT_THAT(v1, IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthEstimate();
        EXPECT_FLOAT_EQ(length, 0.0f);
        EXPECT_THAT(v1, IsClose(Vector2(0.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestIsNormalized)
    {
        EXPECT_TRUE(Vector2(1.0f, 0.0f).IsNormalized());
        EXPECT_TRUE(Vector2(0.7071f, 0.7071f).IsNormalized());
        EXPECT_FALSE(Vector2(1.0f, 1.0f).IsNormalized());
    }

    TEST(MATH_Vector2, TestSetLength)
    {
        Vector2 v1(3.0f, 4.0f);
        v1.SetLength(10.0f);
        EXPECT_THAT(v1, IsClose(Vector2(6.0f, 8.0f)));
        v1.Set(3.0f, 4.0f);
        v1.SetLengthEstimate(10.0f);
        EXPECT_THAT(v1, IsCloseTolerance(Vector2(6.0f, 8.0f), 1e-3f));
    }

    TEST(MATH_Vector2, TestDistance)
    {
        Vector2 vA(1.0f, 2.0f);
        EXPECT_FLOAT_EQ(vA.GetDistanceSq(Vector2(-2.0f, 6.0f)), 25.0f);
        EXPECT_FLOAT_EQ(vA.GetDistance(Vector2(5.0f, -1.0f)), 5.0f);
        EXPECT_FLOAT_EQ(vA.GetDistanceEstimate(Vector2(5.0f, -1.0f)), 5.0f);
    }

    TEST(MATH_Vector2, TestLerpSlerpNLerp)
    {
        EXPECT_THAT(Vector2(4.0f, 5.0f).Lerp(Vector2(5.0f, 10.0f), 0.5f), IsClose(Vector2(4.5f, 7.5f)));
        EXPECT_THAT(Vector2(1.0f, 0.0f).Slerp(Vector2(0.0f, 1.0f), 0.5f), IsClose(Vector2(0.7071f, 0.7071f)));
        EXPECT_THAT(Vector2(1.0f, 0.0f).Nlerp(Vector2(0.0f, 1.0f), 0.5f), IsClose(Vector2(0.7071f, 0.7071f)));
    }

    TEST(MATH_Vector2, TestGetPerpendicular)
    {
        EXPECT_THAT(Vector2(1.0f, 2.0f).GetPerpendicular(), IsClose(Vector2(-2.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestIsClose)
    {
        EXPECT_TRUE(Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 2.0f)));
        EXPECT_FALSE(Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 3.0f)));

        // Verify tolerance works
        EXPECT_TRUE(Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 2.4f), 0.5f));
    }

    TEST(MATH_Vector2, TestIsZero)
    {
        EXPECT_TRUE(Vector2(0.0f).IsZero());
        EXPECT_FALSE(Vector2(1.0f).IsZero());

        // Verify tolerance works
        EXPECT_TRUE(Vector2(0.05f).IsZero(0.1f));
    }

    TEST(MATH_Vector2, TestEqualityInequality)
    {
        Vector2 vA(1.0f, 2.0f);
        EXPECT_TRUE(vA == Vector2(1.0f, 2.0f));
        EXPECT_FALSE((vA == Vector2(5.0f, 6.0f)));
        EXPECT_TRUE(vA != Vector2(7.0f, 8.0f));
        EXPECT_FALSE((vA != Vector2(1.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsLessThan)
    {
        EXPECT_TRUE(Vector2(1.0f, 2.0f).IsLessThan(Vector2(2.0f, 3.0f)));
        EXPECT_FALSE(Vector2(1.0f, 2.0f).IsLessThan(Vector2(0.0f, 3.0f)));
        EXPECT_FALSE(Vector2(1.0f, 2.0f).IsLessThan(Vector2(2.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsLessEqualThan)
    {
        EXPECT_TRUE(Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(2.0f, 3.0f)));
        EXPECT_FALSE(Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(0.0f, 3.0f)));
        EXPECT_TRUE(Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(2.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsGreaterThan)
    {
        EXPECT_TRUE(Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 1.0f)));
        EXPECT_FALSE(Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 3.0f)));
        EXPECT_FALSE(Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestIsGreaterEqualThan)
    {
        EXPECT_TRUE(Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 1.0f)));
        EXPECT_FALSE(Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 3.0f)));
        EXPECT_TRUE(Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestMinMax)
    {
        EXPECT_THAT(Vector2(2.0f, 3.0f).GetMin(Vector2(1.0f, 4.0f)), IsClose(Vector2(1.0f, 3.0f)));
        EXPECT_THAT(Vector2(2.0f, 3.0f).GetMax(Vector2(1.0f, 4.0f)), IsClose(Vector2(2.0f, 4.0f)));
    }

    TEST(MATH_Vector2, TestClamp)
    {
        EXPECT_THAT(Vector2(1.0f, 2.0f).GetClamp(Vector2(5.0f, -10.0f), Vector2(10.0f, -5.0f)), IsClose(Vector2(5.0f, -5.0f)));
        EXPECT_THAT(Vector2(1.0f, 2.0f).GetClamp(Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f)), IsClose(Vector2(1.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestSelect)
    {
        Vector2 vA(1.0f);
        Vector2 vB(2.0f);
        EXPECT_THAT(vA.GetSelect(Vector2(0.0f, 1.0f), vB), IsClose(Vector2(1.0f, 2.0f)));
        vA.Select(Vector2(1.0f, 0.0f), vB);
        EXPECT_THAT(vA, IsClose(Vector2(2.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestAbs)
    {
        EXPECT_THAT(Vector2(-1.0f, 2.0f).GetAbs(), IsClose(Vector2(1.0f, 2.0f)));
        EXPECT_THAT(Vector2(3.0f, -4.0f).GetAbs(), IsClose(Vector2(3.0f, 4.0f)));
    }

    TEST(MATH_Vector2, TestReciprocal)
    {
        EXPECT_THAT(Vector2(2.0f, 4.0f).GetReciprocal(), IsClose(Vector2(0.5f, 0.25f)));
        EXPECT_THAT(Vector2(2.0f, 4.0f).GetReciprocalEstimate(), IsClose(Vector2(0.5f, 0.25f)));
    }

    TEST(MATH_Vector2, TestAdd)
    {
        Vector2 vA(1.0f, 2.0f);
        vA += Vector2(3.0f, 4.0f);
        EXPECT_THAT(vA, IsClose(Vector2(4.0f, 6.0f)));
        EXPECT_THAT((Vector2(1.0f, 2.0f) + Vector2(2.0f, -1.0f)), IsClose(Vector2(3.0f, 1.0f)));
    }

    TEST(MATH_Vector2, TestSub)
    {
        Vector2 vA(10.0f, 11.0f);
        vA -= Vector2(2.0f, 4.0f);
        EXPECT_THAT(vA, IsClose(Vector2(8.0f, 7.0f)));
        EXPECT_THAT((Vector2(1.0f, 2.0f) - Vector2(2.0f, -1.0f)), IsClose(Vector2(-1.0f, 3.0f)));
    }

    TEST(MATH_Vector2, TestMul)
    {
        Vector2 vA(2.0f, 4.0f);
        vA *= Vector2(3.0f, 6.0f);
        EXPECT_THAT(vA, IsClose(Vector2(6.0f, 24.0f)));

        vA.Set(2.0f, 3.0f);
        vA *= 5.0f;
        EXPECT_THAT(vA, IsClose(Vector2(10.0f, 15.0f)));

        EXPECT_THAT((Vector2(3.0f, 2.0f) * Vector2(2.0f, -4.0f)), IsClose(Vector2(6.0f, -8.0f)));
        EXPECT_THAT((Vector2(3.0f, 2.0f) * 2.0f), IsClose(Vector2(6.0f, 4.0f)));
    }

    TEST(MATH_Vector2, TestDiv)
    {
        Vector2 vA(15.0f, 20.0f);
        vA /= Vector2(3.0f, 2.0f);
        EXPECT_THAT(vA, IsClose(Vector2(5.0f, 10.0f)));

        vA.Set(20.0f, 30.0f);
        vA /= 10.0f;
        EXPECT_THAT(vA, IsClose(Vector2(2.0f, 3.0f)));

        EXPECT_THAT((Vector2(30.0f, 20.0f) / Vector2(10.0f, -4.0f)), IsClose(Vector2(3.0f, -5.0f)));
        EXPECT_THAT((Vector2(30.0f, 20.0f) / 10.0f), IsClose(Vector2(3.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestNegate)
    {
        EXPECT_THAT((-Vector2(1.0f, -2.0f)), IsClose(Vector2(-1.0f, 2.0f)));
    }

    TEST(MATH_Vector2, TestDot)
    {
        EXPECT_FLOAT_EQ(Vector2(1.0f, 2.0f).Dot(Vector2(-1.0f, 5.0f)), 9.0f);
    }

    TEST(MATH_Vector2, TestMadd)
    {
        EXPECT_THAT(Vector2(1.0f, 2.0f).GetMadd(Vector2(2.0f, 6.0f), Vector2(3.0f, 4.0f)), IsClose(Vector2(5.0f, 16.0f)));
        Vector2 vA(1.0f, 2.0f);
        vA.Madd(Vector2(2.0f, 6.0f), Vector2(3.0f, 4.0f));
        EXPECT_THAT(vA, IsClose(Vector2(5.0f, 16.0f)));
    }

    TEST(MATH_Vector2, TestProject)
    {
        Vector2 vA(0.5f);
        vA.Project(Vector2(0.0f, 2.0f));
        EXPECT_THAT(vA, IsClose(Vector2(0.0f, 0.5f)));
        vA.Set(0.5f);
        vA.ProjectOnNormal(Vector2(0.0f, 1.0f));
        EXPECT_THAT(vA, IsClose(Vector2(0.0f, 0.5f)));
        vA.Set(2.0f, 4.0f);
        EXPECT_THAT(vA.GetProjected(Vector2(1.0f, 1.0f)), IsClose(Vector2(3.0f, 3.0f)));
        EXPECT_THAT(vA.GetProjectedOnNormal(Vector2(1.0f, 0.0f)), IsClose(Vector2(2.0f, 0.0f)));
    }

    TEST(MATH_Vector2, TestTrig)
    {
        EXPECT_THAT(Vector2(DegToRad(78.0f), DegToRad(-150.0f)).GetAngleMod(), IsClose(Vector2(DegToRad(78.0f), DegToRad(-150.0f))));
        EXPECT_THAT(Vector2(DegToRad(390.0f), DegToRad(-190.0f)).GetAngleMod(), IsClose(Vector2(DegToRad(30.0f), DegToRad(170.0f))));
        EXPECT_THAT(Vector2(DegToRad(60.0f), DegToRad(105.0f)).GetSin(), IsCloseTolerance(Vector2(0.866f, 0.966f), 0.005f));
        EXPECT_THAT(Vector2(DegToRad(60.0f), DegToRad(105.0f)).GetCos(), IsCloseTolerance(Vector2(0.5f, -0.259f), 0.005f));
        Vector2 sin, cos;
        Vector2 v1(DegToRad(60.0f), DegToRad(105.0f));
        v1.GetSinCos(sin, cos);
        EXPECT_THAT(sin, IsCloseTolerance(Vector2(0.866f, 0.966f), 0.005f));
        EXPECT_THAT(cos, IsCloseTolerance(Vector2(0.5f, -0.259f), 0.005f));
    }

    TEST(MATH_Vector2, TestIsFinite)
    {
        EXPECT_TRUE(Vector2(1.0f, 1.0f).IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(Vector2(infinity, infinity).IsFinite());
    }
    struct AngleTestArgs
    {
        AZ::Vector2 current;
        AZ::Vector2 target;
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
        MATH_Vector2,
        AngleTestFixture,
        ::testing::Values(
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ 0.0f, 1.0f }, AZ::Constants::HalfPi },
            AngleTestArgs{ Vector2{ 42.0f, 0.0f }, Vector2{ 0.0f, 23.0f }, AZ::Constants::HalfPi },
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ -1.0f, 0.0f }, AZ::Constants::Pi },
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ 1.0f, 1.0f }, AZ::Constants::QuarterPi },
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ 1.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector2{ 1.0f, 1.0f }, Vector2{ -1.0f, -1.0f }, AZ::Constants::Pi }));

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
        MATH_Vector2,
        AngleDegTestFixture,
        ::testing::Values(
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ 0.0f, 1.0f }, 90.f },
            AngleTestArgs{ Vector2{ 42.0f, 0.0f }, Vector2{ 0.0f, 23.0f }, 90.f },
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ -1.0f, 0.0f }, 180.f },
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ 1.0f, 1.0f }, 45.f },
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ 1.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector2{ 1.0f, 1.0f }, Vector2{ -1.0f, -1.0f }, 180.f }));

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
        MATH_Vector2,
        AngleSafeInvalidAngleTestFixture,
        ::testing::Values(
            AngleTestArgs{ Vector2{ 0.0f, 0.0f }, Vector2{ 0.0f, 1.0f }, 0.f },
            AngleTestArgs{ Vector2{ 0.0f, 0.0f }, Vector2{ 0.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector2{ 1.0f, 0.0f }, Vector2{ 0.0f, 0.0f }, 0.f },
            AngleTestArgs{ Vector2{ 0.0f, 0.0f }, Vector2{ 0.0f, 323432.0f }, 0.f },
            AngleTestArgs{ Vector2{ 323432.0f, 0.0f }, Vector2{ 0.0f, 0.0f }, 0.f }));
} // namespace UnitTest
