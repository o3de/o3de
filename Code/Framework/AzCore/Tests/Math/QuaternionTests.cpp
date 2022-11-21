/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

using namespace AZ;

namespace UnitTest
{
    constexpr float normalizeEpsilon = 0.002f;
    constexpr float values[4] = { 10.0f, 20.0f, 30.0f, 40.0f };

    TEST(MATH_Quaternion, TestHandedness)
    {
        //test to make sure our rotations follow the right hand rule,
        // a positive rotation around z should transform the x axis to the y axis
        Matrix4x4 matrix = Matrix4x4::CreateRotationZ(DegToRad(90.0f));
        Vector3 v = matrix * Vector3(1.0f, 0.0f, 0.0f);
        AZ_TEST_ASSERT(v.IsClose(Vector3(0.0f, 1.0f, 0.0f)));

        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(DegToRad(90.0f));
        v = quat.TransformVector(Vector3(1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(v.IsClose(Vector3(0.0f, 1.0f, 0.0f)));
    }

    TEST(MATH_Quaternion, TestConstruction)
    {
        AZ::Quaternion q1(0.0f, 0.0f, 0.0f, 1.0f);
        EXPECT_TRUE((q1.GetX() == 0.0f) && (q1.GetY() == 0.0f) && (q1.GetZ() == 0.0f) && (q1.GetW() == 1.0f));
        AZ::Quaternion q2(5.0f);
        EXPECT_TRUE((q2.GetX() == 5.0f) && (q2.GetY() == 5.0f) && (q2.GetZ() == 5.0f) && (q2.GetW() == 5.0f));
        AZ::Quaternion q3(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE((q3.GetX() == 1.0f) && (q3.GetY() == 2.0f) && (q3.GetZ() == 3.0f) && (q3.GetW() == 4.0f));
        AZ::Quaternion q4 = AZ::Quaternion::CreateFromVector3AndValue(Vector3(1.0f, 2.0f, 3.0f), 4.0f);
        EXPECT_TRUE((q4.GetX() == 1.0f) && (q4.GetY() == 2.0f) && (q4.GetZ() == 3.0f) && (q4.GetW() == 4.0f));
        AZ::Quaternion q5 = AZ::Quaternion::CreateFromFloat4(values);
        EXPECT_TRUE((q5.GetX() == 10.0f) && (q5.GetY() == 20.0f) && (q5.GetZ() == 30.0f) && (q5.GetW() == 40.0f));
        AZ::Quaternion q6 = AZ::Quaternion::CreateFromVector3(Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_TRUE((q6.GetX() == 1.0f) && (q6.GetY() == 2.0f) && (q6.GetZ() == 3.0f) && (q6.GetW() == 0.0f));
        AZ::Quaternion q7 = AZ::Quaternion::CreateFromAxisAngle(Vector3::CreateAxisZ(), DegToRad(45.0f));
        EXPECT_TRUE(q7.IsClose(AZ::Quaternion::CreateRotationZ(DegToRad(45.0f))));

        AZ::Quaternion q8 = Transform::CreateRotationX(DegToRad(60.0f)).GetRotation();
        EXPECT_TRUE(q8.IsClose(AZ::Quaternion(0.5f, 0.0f, 0.0f, 0.866f)));
        AZ::Quaternion q9 = AZ::Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateRotationX(DegToRad(120.0f)));
        EXPECT_TRUE(q9.IsClose(AZ::Quaternion(0.866f, 0.0f, 0.0f, 0.5f)));
        AZ::Quaternion q10 = AZ::Quaternion::CreateFromMatrix4x4(Matrix4x4::CreateRotationX(DegToRad(120.0f)));
        EXPECT_TRUE(q10.IsClose(AZ::Quaternion(0.866f, 0.0f, 0.0f, 0.5f)));
        AZ::Quaternion q11 = AZ::Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateRotationX(DegToRad(-60.0f)));
        EXPECT_TRUE(q11.IsClose(AZ::Quaternion(-0.5f, 0.0f, 0.0f, 0.866f)));
        AZ::Quaternion q12 = AZ::Quaternion::CreateFromMatrix4x4(Matrix4x4::CreateRotationX(DegToRad(-60.0f)));
        EXPECT_TRUE(q12.IsClose(AZ::Quaternion(-0.5f, 0.0f, 0.0f, 0.866f)));
        AZ::Quaternion q13 = AZ::Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateRotationY(DegToRad(120.0f)));
        EXPECT_TRUE(q13.IsClose(AZ::Quaternion(0.0f, 0.866f, 0.0f, 0.5f)));
        AZ::Quaternion q14 = AZ::Quaternion::CreateFromMatrix4x4(Matrix4x4::CreateRotationY(DegToRad(120.0f)));
        EXPECT_TRUE(q14.IsClose(AZ::Quaternion(0.0f, 0.866f, 0.0f, 0.5f)));
        AZ::Quaternion q15 = AZ::Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateRotationY(DegToRad(-60.0f)));
        EXPECT_TRUE(q15.IsClose(AZ::Quaternion(0.0f, -0.5f, 0.0f, 0.866f)));
        AZ::Quaternion q16 = AZ::Quaternion::CreateFromMatrix4x4(Matrix4x4::CreateRotationY(DegToRad(-60.0f)));
        EXPECT_TRUE(q16.IsClose(AZ::Quaternion(0.0f, -0.5f, 0.0f, 0.866f)));
        AZ::Quaternion q17 = AZ::Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateRotationZ(DegToRad(120.0f)));
        EXPECT_TRUE(q17.IsClose(AZ::Quaternion(0.0f, 0.0f, 0.866f, 0.5f)));
        AZ::Quaternion q18 = AZ::Quaternion::CreateFromMatrix4x4(Matrix4x4::CreateRotationZ(DegToRad(120.0f)));
        EXPECT_TRUE(q18.IsClose(AZ::Quaternion(0.0f, 0.0f, 0.866f, 0.5f)));
        AZ::Quaternion q19 = AZ::Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateRotationZ(DegToRad(-60.0f)));
        EXPECT_TRUE(q19.IsClose(AZ::Quaternion(0.0f, 0.0f, -0.5f, 0.866f)));
        AZ::Quaternion q20 = AZ::Quaternion::CreateFromMatrix4x4(Matrix4x4::CreateRotationZ(DegToRad(-60.0f)));
        EXPECT_TRUE(q20.IsClose(AZ::Quaternion(0.0f, 0.0f, -0.5f, 0.866f)));
    }

    TEST(MATH_Quaternion, TestCreate)
    {
        EXPECT_TRUE(AZ::Quaternion::CreateIdentity() == AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(AZ::Quaternion::CreateZero() == AZ::Quaternion(0.0f));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationX(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.5f, 0.0f, 0.0f, 0.866f)));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.0f, 0.5f, 0.0f, 0.866f)));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationZ(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.0f, 0.0f, 0.5f, 0.866f)));
    }

    TEST(MATH_Quaternion, TestConcatenate)
    {
        Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
        Quaternion q2(-1.0f, -2.0f, -3.0f, -4.0f);
        Quaternion result = q1 * q2;
        EXPECT_TRUE(result.IsClose(Quaternion(-8.0f, -16.0f, -24.0f, -2.0f)));
    }

    TEST(MATH_Quaternion, TestShortestArc)
    {
        Vector3 v1 = Vector3(1.0f, 2.0f, 3.0f).GetNormalized();
        Vector3 v2 = Vector3(-2.0f, 7.0f, -1.0f).GetNormalized();
        Quaternion q3 = AZ::Quaternion::CreateShortestArc(v1, v2); //q3 should transform v1 into v2
        EXPECT_TRUE(v2.IsClose(q3.TransformVector(v1), 1e-3f));
        Quaternion q4 = AZ::Quaternion::CreateShortestArc(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
        EXPECT_TRUE((q4.TransformVector(Vector3(0.0f, 0.0f, 1.0f))).IsClose(Vector3(0.0f, 0.0f, 1.0f), 1e-3f));  //perpendicular vector should be unaffected
        EXPECT_TRUE((q4.TransformVector(Vector3(0.0f, -1.0f, 0.0f))).IsClose(Vector3(1.0f, 0.0f, 0.0f), 1e-3f)); //make sure we rotate the right direction, i.e. actually shortest arc
        v2 = (v1 + Vector3(1e-5f, 1e-5f, 1e-5f)).GetNormalized(); // test almost parallel vectors
        Quaternion q5 = AZ::Quaternion::CreateShortestArc(v1, v2);
        EXPECT_TRUE(v2.IsClose(q5.TransformVector(v1), 1e-3f));
        v2 = (-v1 + Vector3(1e-5f, 1e-5f, 1e-5f)).GetNormalized(); // test almost anti-parallel vectors
        Quaternion q6 = AZ::Quaternion::CreateShortestArc(v1, v2);
        EXPECT_TRUE(v2.IsClose(q6.TransformVector(v1), 1e-3f));
    }

    TEST(MATH_Quaternion, TestGetSet)
    {
        Quaternion q1;
        q1.SetX(10.0f);
        EXPECT_TRUE(q1.GetX() == 10.0f);
        q1.SetY(11.0f);
        EXPECT_TRUE(q1.GetY() == 11.0f);
        q1.SetZ(12.0f);
        EXPECT_TRUE(q1.GetZ() == 12.0f);
        q1.SetW(13.0f);
        EXPECT_TRUE(q1.GetW() == 13.0f);
        q1.Set(15.0f);
        EXPECT_TRUE(q1 == AZ::Quaternion(15.0f));
        q1.Set(2.0f, 3.0f, 4.0f, 5.0f);
        EXPECT_TRUE(q1 == AZ::Quaternion(2.0f, 3.0f, 4.0f, 5.0f));
        q1.Set(Vector3(5.0f, 6.0f, 7.0f), 8.0f);
        EXPECT_TRUE(q1 == AZ::Quaternion(5.0f, 6.0f, 7.0f, 8.0f));
        q1.Set(values);
        EXPECT_TRUE((q1.GetX() == 10.0f) && (q1.GetY() == 20.0f) && (q1.GetZ() == 30.0f) && (q1.GetW() == 40.0f));
    }

    TEST(MATH_Quaternion, TestGetElementSetElement)
    {
        Quaternion q1;
        q1.SetElement(0, 1.0f);
        q1.SetElement(1, 2.0f);
        q1.SetElement(2, 3.0f);
        q1.SetElement(3, 4.0f);
        EXPECT_TRUE(q1.GetElement(0) == 1.0f);
        EXPECT_TRUE(q1.GetElement(1) == 2.0f);
        EXPECT_TRUE(q1.GetElement(2) == 3.0f);
        EXPECT_TRUE(q1.GetElement(3) == 4.0f);
    }

    TEST(MATH_Quaternion, TestIndexOperators)
    {
        Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(q1(0) == 1.0f);
        EXPECT_TRUE(q1(1) == 2.0f);
        EXPECT_TRUE(q1(2) == 3.0f);
        EXPECT_TRUE(q1(3) == 4.0f);
    }

    TEST(MATH_Quaternion, TestIsIdentity)
    {
        Quaternion q1(0.0f, 0.0f, 0.0f, 1.0f);
        EXPECT_TRUE(q1.IsIdentity());
    }

    TEST(MATH_Quaternion, TestConjugate)
    {
        Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(q1.GetConjugate() == AZ::Quaternion(-1.0f, -2.0f, -3.0f, 4.0f));
    }

    TEST(MATH_Quaternion, TestInverse)
    {
        Quaternion q1 = AZ::Quaternion::CreateRotationX(DegToRad(25.0f)) * AZ::Quaternion::CreateRotationY(DegToRad(70.0f));
        EXPECT_TRUE((q1 * q1.GetInverseFast()).IsClose(AZ::Quaternion::CreateIdentity()));
        Quaternion q2 = q1;
        q2.InvertFast();
        EXPECT_TRUE(q1.GetX() == -q2.GetX());
        EXPECT_TRUE(q1.GetY() == -q2.GetY());
        EXPECT_TRUE(q1.GetZ() == -q2.GetZ());
        EXPECT_TRUE(q1.GetW() == q2.GetW());
        EXPECT_TRUE((q1 * q2).IsClose(AZ::Quaternion::CreateIdentity()));
    }

    TEST(MATH_Quaternion, TestGetInverseFull)
    {
        Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE((q1 * q1.GetInverseFull()).IsClose(AZ::Quaternion::CreateIdentity()));
    }

    TEST(MATH_Quaternion, TestDot)
    {
        EXPECT_NEAR(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).Dot(AZ::Quaternion(-1.0f, 5.0f, 3.0f, 2.0f)), 26.0f, normalizeEpsilon);
    }

    TEST(MATH_Quaternion, TestLength)
    {
        EXPECT_NEAR(AZ::Quaternion(-1.0f, 2.0f, 1.0f, 3.0f).GetLengthSq(), 15.0f, normalizeEpsilon);
        EXPECT_NEAR(AZ::Quaternion(-4.0f, 2.0f, 0.0f, 4.0f).GetLength(), 6.0f, normalizeEpsilon);
    }

    TEST(MATH_Quaternion, TestNormalize)
    {
        EXPECT_TRUE(AZ::Quaternion(0.0f, -4.0f, 2.0f, 4.0f).GetNormalized().IsClose(AZ::Quaternion(0.0f, -0.66666f, 0.33333f, 0.66666f)));
        Quaternion q1(2.0f, 0.0f, 4.0f, -4.0f);
        q1.Normalize();
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(0.33333f, 0.0f, 0.66666f, -0.66666f)));
        q1.Set(2.0f, 0.0f, 4.0f, -4.0f);
        float length = q1.NormalizeWithLength();
        EXPECT_NEAR(length, 6.0f, normalizeEpsilon);
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(0.33333f, 0.0f, 0.66666f, -0.66666f)));
    }

    TEST(MATH_Quaternion, TestInterpolation)
    {
        EXPECT_TRUE(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).Lerp(AZ::Quaternion(2.0f, 3.0f, 4.0f, 5.0f), 0.5f).IsClose(AZ::Quaternion(1.5f, 2.5f, 3.5f, 4.5f)));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationX(DegToRad(10.0f)).Slerp(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)), 0.5f).IsClose(AZ::Quaternion(0.045f, 0.259f, 0.0f, 0.965f), 1e-3f));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationX(DegToRad(10.0f)).Squad(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)), AZ::Quaternion::CreateRotationZ(DegToRad(35.0f)), AZ::Quaternion::CreateRotationX(DegToRad(80.0f)), 0.5f).IsClose(AZ::Quaternion(0.2f, 0.132f, 0.083f, 0.967f), 1e-3f));
    }

    TEST(MATH_Quaternion, TestClose)
    {
        EXPECT_TRUE(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));
        EXPECT_TRUE(!AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f)));
        EXPECT_TRUE(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.4f), 0.5f));
    }

    TEST(MATH_Quaternion, TestOperators)
    {
        EXPECT_TRUE((-AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)) == AZ::Quaternion(-1.0f, -2.0f, -3.0f, -4.0f));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) + AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(3.0f, 5.0f, 8.0f, 3.0f)));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) - AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(-1.0f, -1.0f, -2.0f, 5.0f)));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) * AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(8.0f, 11.0f, 16.0f, -27.0f)));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f).IsClose(AZ::Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_TRUE((2.0f * AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)).IsClose(AZ::Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) / 2.0f).IsClose(AZ::Quaternion(0.5f, 1.0f, 1.5f, 2.0f)));
        Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
        q1 += AZ::Quaternion(5.0f, 6.0f, 7.0f, 8.0f);
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(6.0f, 8.0f, 10.0f, 12.0f)));
        q1 -= AZ::Quaternion(3.0f, -1.0f, 5.0f, 7.0f);
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(3.0f, 9.0f, 5.0f, 5.0f)));
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        q1 *= AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f);
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(8.0f, 11.0f, 16.0f, -27.0f)));
        q1 *= 2.0f;
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(16.0f, 22.0f, 32.0f, -54.0f)));
        q1 /= 4.0f;
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(4.0f, 5.5f, 8.0f, -13.5f)));
    }

    TEST(MATH_Quaternion, TestEquality)
    {
        Quaternion q3(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(q3 == AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0));
        EXPECT_TRUE(!(q3 == AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f)));
        EXPECT_TRUE(q3 != AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f));
        EXPECT_TRUE(!(q3 != AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));
    }

    TEST(MATH_Quaternion, TestVectorTransform)
    {
        EXPECT_TRUE((AZ::Quaternion::CreateRotationX(DegToRad(45.0f)).TransformVector(Vector3(4.0f, 1.0f, 0.0f))).IsClose(Vector3(4.0f, 0.7071f, 0.7071f)));
    }

    TEST(MATH_Quaternion, TestGetImaginary)
    {
        Quaternion q1(21.0f, 22.0f, 23.0f, 24.0f);
        EXPECT_TRUE(q1.GetImaginary() == Vector3(21.0f, 22.0f, 23.0f));
    }

    TEST(MATH_Quaternion, TestGetAngle)
    {
        Quaternion q1 = AZ::Quaternion::CreateRotationX(DegToRad(35.0f));
        EXPECT_TRUE(AZ::IsClose(q1.GetAngle(), DegToRad(35.0f)));
    }

    TEST(MATH_Quaternion, TestConcatenation)
    {
        Quaternion q1 = AZ::Quaternion::CreateRotationZ(DegToRad(90.0f));
        Quaternion q2 = AZ::Quaternion::CreateRotationX(DegToRad(90.0f));
        Vector3 v = (q2 * q1).TransformVector(Vector3(1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(v.IsClose(Vector3(0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Quaternion, ToEulerDegrees)
    {
        float halfAngle = 0.5f *  Constants::QuarterPi;
        float sin = sinf(halfAngle);
        float cos = cosf(halfAngle);
        AZ::Quaternion testQuat = AZ::Quaternion::CreateFromVector3AndValue(sin * AZ::Vector3::CreateAxisX(), cos);
        AZ::Vector3 resultVector = testQuat.GetEulerDegrees();
        EXPECT_TRUE(resultVector.IsClose(AZ::Vector3(45.0f, 0.0f, 0.0f)));

        resultVector = ConvertQuaternionToEulerDegrees(testQuat);
        EXPECT_TRUE(resultVector.IsClose(AZ::Vector3(45.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Quaternion, ToEulerRadians)
    {
        constexpr float getEulerRadiansEpsilon = 0.001f;
        float halfAngle = 0.5f *  Constants::HalfPi;
        float sin = sinf(halfAngle);
        float cos = cosf(halfAngle);

        AZ::Quaternion testQuat = AZ::Quaternion::CreateFromVector3AndValue(sin * AZ::Vector3::CreateAxisY(), cos);
        AZ::Vector3 resultVector = testQuat.GetEulerRadians();
        EXPECT_NEAR(Constants::HalfPi, static_cast<float>(resultVector.GetY()), getEulerRadiansEpsilon);

        resultVector = ConvertQuaternionToEulerRadians(testQuat);
        EXPECT_NEAR(Constants::HalfPi, static_cast<float>(resultVector.GetY()), getEulerRadiansEpsilon);
    }

    using QuaternionEulerFixture = ::testing::TestWithParam<AZ::Quaternion>;

    static const AZ::Quaternion TestUnitQuaternions[] = {
        AZ::Quaternion(0.64f, 0.36f, 0.48f, 0.48f),
        AZ::Quaternion(0.70f, -0.34f, 0.10f, 0.62f),
        AZ::Quaternion(-0.38f, 0.34f, 0.70f, -0.50f),
        AZ::Quaternion(0.70f, -0.34f, -0.38f, 0.50f),
        AZ::Quaternion(0.00f, 0.00f, -0.28f, 0.96f),
        AZ::Quaternion(0.24f, -0.64f, 0.72f, 0.12f),
        AZ::Quaternion(-0.66f, 0.62f, 0.42f, 0.06f),
        AZ::Quaternion(0.5f, 0.5f, 0.5f, 0.5f),
        AZ::Quaternion(0.5f, -0.5f, 0.5f, -0.5f),
        AZ::Quaternion(0.34f, 0.62f, -0.34f, -0.62f),
        AZ::Quaternion(-0.1f, -0.7f, 0.1f, 0.7f)
    };

    TEST_P(QuaternionEulerFixture, EulerOrderCorrect)
    {
        // the quaternion should be equivalent to a series of rotations in the order z, then y, then x
        const AZ::Quaternion quaternion = GetParam();
        const AZ::Vector3 euler = quaternion.GetEulerRadians();
        const AZ::Quaternion productOfRotations =
            AZ::Quaternion::CreateRotationX(euler.GetX()) *
            AZ::Quaternion::CreateRotationY(euler.GetY()) *
            AZ::Quaternion::CreateRotationZ(euler.GetZ());
        EXPECT_TRUE(productOfRotations.IsClose(quaternion) || productOfRotations.IsClose(-quaternion));
    }

    TEST_P(QuaternionEulerFixture, QuaternionEulerQuaternionCycle)
    {
        // converting a quaternion to Euler angles and back again should recover the original quaternion
        // note that because the Euler angle representation is not unique, the same is not necessarily true
        // for Euler -> quaternion -> Euler
        const AZ::Quaternion originalQuaternion = GetParam();
        const AZ::Vector3 euler = originalQuaternion.GetEulerRadians();
        const AZ::Quaternion recoveredQuaternion = AZ::Quaternion::CreateFromEulerRadiansXYZ(euler);
        EXPECT_TRUE(recoveredQuaternion.IsClose(originalQuaternion) || recoveredQuaternion.IsClose(-originalQuaternion));
    }

    TEST_P(QuaternionEulerFixture, EulerViaTransformEquivalentToDirectEuler)
    {
        // quaternion -> transform -> Euler -> quaternion should give an equivalent result to
        // quaternion -> Euler -> quaternion
        const AZ::Quaternion originalQuaternion = GetParam();
        const AZ::Vector3 euler1 = originalQuaternion.GetEulerRadians();
        const AZ::Vector3 euler2 = AZ::Transform::CreateFromQuaternion(originalQuaternion).GetEulerRadians();
        const AZ::Quaternion recoveredQuaternion1 = AZ::ConvertEulerRadiansToQuaternion(euler1);
        const AZ::Quaternion recoveredQuaternion2 = AZ::ConvertEulerRadiansToQuaternion(euler2);
        EXPECT_TRUE(recoveredQuaternion1.IsClose(recoveredQuaternion2) || recoveredQuaternion1.IsClose(-recoveredQuaternion2));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Quaternion, QuaternionEulerFixture, ::testing::ValuesIn(TestUnitQuaternions));

    TEST(MATH_Quaternion, FromEulerDegrees)
    {
        const AZ::Vector3 testDegrees(45.0f, 45.0f, 45.0f);
        AZ::Quaternion testQuat;
        testQuat.SetFromEulerDegrees(testDegrees);
        EXPECT_TRUE(testQuat.IsClose(AZ::Quaternion(0.46193981170654296875f, 0.1913417130708694458f, 0.46193981170654296875f, 0.73253774642944335938f), 0.0000001f));
    }

    TEST(MATH_Quaternion, FromEulerRadians)
    {
        const AZ::Vector3 testRadians(Constants::QuarterPi, Constants::QuarterPi, Constants::QuarterPi);
        AZ::Quaternion testQuat;
        testQuat.SetFromEulerRadians(testRadians);
        EXPECT_TRUE(testQuat.IsClose(AZ::Quaternion(0.46193981170654296875f, 0.1913417130708694458f, 0.46193981170654296875f, 0.73253774642944335938f), 0.0000001f));
    }


    TEST(MATH_Quaternion, FromAxisAngle)
    {
        AZ::Quaternion q10 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), Constants::QuarterPi);
        EXPECT_TRUE(q10.IsClose(AZ::Quaternion::CreateRotationZ(Constants::QuarterPi)));

        q10 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), Constants::HalfPi);
        EXPECT_TRUE(q10.IsClose(AZ::Quaternion::CreateRotationY(Constants::HalfPi)));

        q10 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), Constants::TwoPi / 3.0f);
        EXPECT_TRUE(q10.IsClose(AZ::Quaternion::CreateRotationX(Constants::TwoPi / 3.0f)));
    }

    TEST(MATH_Quaternion, ToAxisAngle)
    {
        constexpr float axisAngleEpsilon = 0.002f;
        AZ::Quaternion testQuat = AZ::Quaternion::CreateIdentity();

        Vector3 resultAxis = Vector3::CreateZero();
        float resultAngle{};
        testQuat.ConvertToAxisAngle(resultAxis, resultAngle);
        EXPECT_TRUE(resultAxis.IsClose(Vector3::CreateAxisY()));
        EXPECT_NEAR(0.0f, resultAngle, axisAngleEpsilon);
    }

    TEST(MATH_Quaternion, MatrixConversionTest)
    {
        Matrix4x4 rotMatrix = Matrix4x4::CreateRotationZ(DegToRad(90.0f));
        AZ::Quaternion rotQuat = AZ::Quaternion::CreateRotationZ(DegToRad(90.0f));

        AZ::Quaternion q = AZ::Quaternion::CreateFromMatrix4x4(rotMatrix);
        AZ_TEST_ASSERT(q.IsClose(rotQuat));

        Matrix4x4 m = Matrix4x4::CreateFromQuaternion(rotQuat);
        AZ_TEST_ASSERT(m.IsClose(rotMatrix));
    }

    class QuaternionScaledAxisAngleConversionFixture
        : public ::testing::TestWithParam<AZ::Quaternion>
    {
    public:
        AZ::Quaternion GetAbs(const AZ::Quaternion& in)
        {
            // Take the shortest path for quaternions containing rotations bigger than 180.0°.
            if (in.GetW() < 0.0f)
            {
                return -in;
            }

            return in;
        }
    };

    static const AZ::Quaternion RotationRepresentationConversionTestQuats[] =
    {
        AZ::Quaternion::CreateIdentity(),
        -AZ::Quaternion::CreateIdentity(),
        AZ::Quaternion::CreateRotationX(AZ::Constants::TwoPi),
        AZ::Quaternion::CreateRotationY(AZ::Constants::Pi),
        AZ::Quaternion::CreateRotationZ(AZ::Constants::HalfPi),
        AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi),
        AZ::Quaternion(0.64f, 0.36f, 0.48f, 0.48f),
        AZ::Quaternion(0.70f, -0.34f, 0.10f, 0.62f),
        AZ::Quaternion(-0.38f, 0.34f, 0.70f, -0.50f),
        AZ::Quaternion(0.70f, -0.34f, -0.38f, 0.50f),
        AZ::Quaternion(0.00f, 0.00f, -0.28f, 0.96f),
        AZ::Quaternion(0.24f, -0.64f, 0.72f, 0.12f),
        AZ::Quaternion(-0.66f, 0.62f, 0.42f, 0.06f)
    };

    TEST_P(QuaternionScaledAxisAngleConversionFixture, ScaledAxisAngleQuatRoundtripTests)
    {
        const AZ::Quaternion testQuat = GetAbs(GetParam());

        // Convert test quaternion to scaled axis-angle representation.
        const AZ::Vector3 scaledAxisAngle = testQuat.ConvertToScaledAxisAngle();

        // Convert the scaled axis-angle back into a quaternion.
        AZ::Quaternion backFromScaledAxisAngle = AZ::Quaternion::CreateFromScaledAxisAngle(scaledAxisAngle);

        // Compare the original quaternion with the one after the conversion.
        EXPECT_THAT(testQuat, IsCloseTolerance(backFromScaledAxisAngle, 1e-6f));
    }

    TEST_P(QuaternionScaledAxisAngleConversionFixture, AxisAngleQuatRoundtripTests)
    {
        const AZ::Quaternion testQuat = GetAbs(GetParam());

        // Convert test quaternion to axis-angle representation.
        AZ::Vector3 axis;
        float angle;
        testQuat.ConvertToAxisAngle(axis, angle);

        // Convert the axis-angle back into a quaternion and compare the original quaternion with the one after the conversion.
        const AZ::Quaternion backFromAxisAngle = AZ::Quaternion::CreateFromAxisAngle(axis, angle);
        EXPECT_THAT(testQuat, IsCloseTolerance(backFromAxisAngle, 1e-6f));
    }

    TEST_P(QuaternionScaledAxisAngleConversionFixture, CompareAxisAngleConversionTests)
    {
        const AZ::Quaternion testQuat = GetAbs(GetParam());

        // Convert test quaternion to scaled axis-angle representation.
        const AZ::Vector3 scaledAxisAngle = testQuat.ConvertToScaledAxisAngle();

        // Convert test quaternion to axis-angle representation and scale it manually.
        AZ::Vector3 axis;
        float angle;
        testQuat.ConvertToAxisAngle(axis, angle);

        // Compare the scaled result to the version from the helper that directly converts it to scaled axis-angle.
        AZ::Vector3 scaledResult = axis*angle;
        EXPECT_TRUE(scaledResult.IsClose(scaledAxisAngle, 1e-5f));
    }

    TEST_P(QuaternionScaledAxisAngleConversionFixture, CompareScaledAxisAngleConversionTests)
    {
        const AZ::Quaternion testQuat = GetAbs(GetParam());

        // Convert test quaternion to axis-angle representation and scale it manually.
        AZ::Vector3 axis;
        float angle;
        testQuat.ConvertToAxisAngle(axis, angle);
        AZ::Vector3 scaledResult = axis*angle;

        // Special case handling for identity rotation.
        AZ::Vector3 axisFromScaledResult = scaledResult.GetNormalized();
        float angleFromScaledResult = scaledResult.GetLength();
        if (AZ::IsClose(angleFromScaledResult, 0.0f))
        {
            axisFromScaledResult = AZ::Vector3::CreateAxisY();
        }

        const AZ::Quaternion backFromAxisAngle = AZ::Quaternion::CreateFromAxisAngle(axisFromScaledResult, angleFromScaledResult);
        EXPECT_THAT(testQuat, IsCloseTolerance(backFromAxisAngle, 1e-6f));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Quaternion, QuaternionScaledAxisAngleConversionFixture, ::testing::ValuesIn(RotationRepresentationConversionTestQuats));

    TEST(MATH_Quaternion, ShortestEquivalent)
    {
        const AZ::Quaternion testQuat = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi * 3.0f);

        AZ::Quaternion absQuat = testQuat;
        absQuat.ShortestEquivalent();
        EXPECT_THAT(testQuat.GetShortestEquivalent(), IsCloseTolerance(absQuat, 1e-6f));

        const float angle = absQuat.GetEulerRadians().GetX();
        EXPECT_THAT(angle, testing::FloatEq(-AZ::Constants::HalfPi));
    }

    struct EulerTestArgs
    {
        AZ::Vector3 euler;
        AZ::Quaternion result;
    };

    using AngleRadianTestFixtureXYZ = ::testing::TestWithParam<EulerTestArgs>;

    TEST_P(AngleRadianTestFixtureXYZ, EulerRadiansXYZ)
    {
        auto& param = GetParam();
        EXPECT_THAT(AZ::Quaternion::CreateFromEulerRadiansXYZ(param.euler), IsClose(param.result));
    }

    TEST_P(AngleRadianTestFixtureXYZ, EulerDegreesXYZ)
    {
        auto& param = GetParam();
        EXPECT_THAT(AZ::Quaternion::CreateFromEulerDegreesXYZ(Vector3RadToDeg(param.euler)), IsClose(param.result));
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Quaternion,
        AngleRadianTestFixtureXYZ,
        ::testing::Values(
            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, 0, 0), AZ::Quaternion::CreateRotationX(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, 0, Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) },

            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, 0, 0), AZ::Quaternion::CreateRotationX(-Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, -Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(-Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, 0, -Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(-Constants::QuarterPi) },

            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationX(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationY(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, Constants::QuarterPi, Constants::QuarterPi), AZ::Quaternion::CreateRotationY(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, 0, Constants::QuarterPi), AZ::Quaternion::CreateRotationX(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi)},
                
            EulerTestArgs{ AZ::Vector3(Constants::HalfPi, 0, Constants::QuarterPi), 
                AZ::Quaternion::CreateRotationX(Constants::HalfPi) *
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi)},
            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, -Constants::HalfPi, Constants::QuarterPi), 
                AZ::Quaternion::CreateRotationX(-Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationY(-Constants::HalfPi) *
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi)},
            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, Constants::HalfPi, Constants::TwoOverPi), 
                AZ::Quaternion::CreateRotationX(-Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationY(Constants::HalfPi) *
                AZ::Quaternion::CreateRotationZ(Constants::TwoOverPi)}
                ));

    using AngleRadianTestFixtureYXZ = ::testing::TestWithParam<EulerTestArgs>;

    TEST_P(AngleRadianTestFixtureYXZ, EulerRadiansYXZ)
    {
        auto& param = GetParam();
        EXPECT_THAT(AZ::Quaternion::CreateFromEulerRadiansYXZ(param.euler), IsClose(param.result));
    }

    TEST_P(AngleRadianTestFixtureYXZ, EulerDegreesYXZ)
    {
        auto& param = GetParam();
        EXPECT_THAT(AZ::Quaternion::CreateFromEulerDegreesYXZ(Vector3RadToDeg(param.euler)), IsClose(param.result));
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Quaternion,
        AngleRadianTestFixtureYXZ,
        ::testing::Values(
            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, 0, 0), AZ::Quaternion::CreateRotationX(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, 0, Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) },

            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, 0, 0), AZ::Quaternion::CreateRotationX(-Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, -Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(-Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, 0, -Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(-Constants::QuarterPi) },

            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationX(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, Constants::QuarterPi, Constants::QuarterPi), AZ::Quaternion::CreateRotationY(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, 0, Constants::QuarterPi), AZ::Quaternion::CreateRotationX(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi)},

            EulerTestArgs{ AZ::Vector3(Constants::HalfPi, 0, Constants::QuarterPi), 
                AZ::Quaternion::CreateRotationX(Constants::HalfPi) *
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi)},
            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, -Constants::HalfPi, Constants::QuarterPi), 
                AZ::Quaternion::CreateRotationY(-Constants::HalfPi) *
                AZ::Quaternion::CreateRotationX(-Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi)},
            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, Constants::HalfPi, Constants::TwoOverPi), 
                AZ::Quaternion::CreateRotationY(Constants::HalfPi) *
                AZ::Quaternion::CreateRotationX(-Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationZ(Constants::TwoOverPi)}
                
        ));

    using AngleRadianTestFixtureZYX = ::testing::TestWithParam<EulerTestArgs>;

    TEST_P(AngleRadianTestFixtureZYX, EulerRadiansYXZ)
    {
        auto& param = GetParam();
        EXPECT_THAT(AZ::Quaternion::CreateFromEulerRadiansZYX(param.euler), IsClose(param.result));
    }

    TEST_P(AngleRadianTestFixtureZYX, EulerDegreesYXZ)
    {
        auto& param = GetParam();
        EXPECT_THAT(AZ::Quaternion::CreateFromEulerDegreesZYX(Vector3RadToDeg(param.euler)), IsClose(param.result));
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Quaternion,
        AngleRadianTestFixtureZYX,
        ::testing::Values(
            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, 0, 0), AZ::Quaternion::CreateRotationX(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, 0, Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) },

            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, 0, 0), AZ::Quaternion::CreateRotationX(-Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, -Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(-Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, 0, -Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(-Constants::QuarterPi) },

            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationX(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, Constants::QuarterPi, Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationY(Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(Constants::QuarterPi, 0, Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationX(Constants::QuarterPi)},
                
            EulerTestArgs{ AZ::Vector3(Constants::HalfPi, 0, Constants::QuarterPi), 
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationX(Constants::HalfPi)},
            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, -Constants::HalfPi, Constants::QuarterPi), 
                AZ::Quaternion::CreateRotationZ(Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationY(-Constants::HalfPi) *
                AZ::Quaternion::CreateRotationX(-Constants::QuarterPi)},
            EulerTestArgs{ AZ::Vector3(-Constants::QuarterPi, Constants::HalfPi, Constants::TwoOverPi), 
                AZ::Quaternion::CreateRotationZ(Constants::TwoOverPi) *
                AZ::Quaternion::CreateRotationY(Constants::HalfPi) *
                AZ::Quaternion::CreateRotationX(-Constants::QuarterPi)}
        ));

} // namespace UnitTest
