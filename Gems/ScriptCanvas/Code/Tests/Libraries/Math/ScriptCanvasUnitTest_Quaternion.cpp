/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Quaternion.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestQuaternionFunctions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, Conjugate_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::Conjugate(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, ConvertTransformToRotation_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::ConvertTransformToRotation(AZ::Transform::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, Dot_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::Dot(AZ::Quaternion::CreateIdentity(), AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, FromAxisAngleDegrees_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::FromAxisAngleDegrees(AZ::Vector3::CreateZero(), 0);
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, FromMatrix3x3_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::FromMatrix3x3(AZ::Matrix3x3::CreateIdentity());
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Quaternion::CreateIdentity()));
#else
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, FromMatrix4x4_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::FromMatrix4x4(AZ::Matrix4x4::CreateIdentity());
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Quaternion::CreateIdentity()));
#else
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, FromTransform_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::FromTransform(AZ::Transform::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, InvertFull_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::InvertFull(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, IsClose_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::IsClose(AZ::Quaternion::CreateIdentity(), AZ::Quaternion::CreateIdentity(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, IsFinite_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::IsFinite(AZ::Quaternion::CreateIdentity());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, IsIdentity_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::IsIdentity(AZ::Quaternion::CreateIdentity(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, IsZero_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::IsZero(AZ::Quaternion::CreateZero(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, LengthReciprocal_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::LengthReciprocal(AZ::Quaternion::CreateIdentity());
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        constexpr float tolerance = 0.00001f;
        EXPECT_NEAR(actualResult, 1.0f, tolerance);
#else
        EXPECT_EQ(actualResult, 1);
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, LengthSquared_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::LengthSquared(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, Lerp_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::Lerp(AZ::Quaternion::CreateZero(), AZ::Quaternion::CreateIdentity(), 0.5);
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateFromVector3AndValue(AZ::Vector3::CreateZero(), 0.5));
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, MultiplyByNumber_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::MultiplyByNumber(AZ::Quaternion::CreateIdentity(), 0.5);
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateFromVector3AndValue(AZ::Vector3::CreateZero(), 0.5));
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, Negate_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::Negate(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateFromVector3AndValue(AZ::Vector3::CreateZero(), -1));
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, Normalize_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::Normalize(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, RotationXDegrees_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::RotationXDegrees(0);
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, RotationYDegrees_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::RotationYDegrees(0);
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, RotationZDegrees_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::RotationZDegrees(0);
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, ToAngleDegrees_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::ToAngleDegrees(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, 0);
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, CreateFromEulerAngles_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::CreateFromEulerAngles(0, 0, 0);
        EXPECT_EQ(actualResult, AZ::Quaternion::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestQuaternionFunctions, RotateVector3_Call_GetExpectedResult)
    {
        auto actualResult = QuaternionFunctions::RotateVector3(AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne());
        EXPECT_EQ(actualResult, AZ::Vector3::CreateOne());
    }
} // namespace ScriptCanvasUnitTest
