/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Transform.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestTransformFunctions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestTransformFunctions, FromMatrix3x3_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::FromMatrix3x3(AZ::Matrix3x3::CreateIdentity());
        auto expectedResult = AZ::Transform::CreateIdentity();
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(expectedResult));
#else
        EXPECT_EQ(actualResult, expectedResult);
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, FromMatrix3x3AndTranslation_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::FromMatrix3x3AndTranslation(AZ::Matrix3x3::CreateIdentity(), AZ::Vector3::CreateOne());
        EXPECT_EQ(actualResult.GetUniformScale(), 1);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult.GetRotation(), IsClose(AZ::Quaternion::CreateIdentity()));
#else
        EXPECT_EQ(actualResult.GetTranslation(), AZ::Vector3::CreateOne());
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, FromRotation_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::FromRotation(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Transform::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, FromRotationAndTranslation_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::FromRotationAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne());
        EXPECT_EQ(actualResult.GetUniformScale(), 1);
        EXPECT_EQ(actualResult.GetRotation(), AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult.GetTranslation(), AZ::Vector3::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, FromRotationScaleAndTranslation_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::FromRotationScaleAndTranslation(AZ::Quaternion::CreateIdentity(), 2.0, AZ::Vector3::CreateOne());
        EXPECT_EQ(actualResult.GetUniformScale(), 2);
        EXPECT_EQ(actualResult.GetRotation(), AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult.GetTranslation(), AZ::Vector3::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, FromScale_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::FromScale(5);
        EXPECT_EQ(actualResult.GetUniformScale(), 5);
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, FromTranslation_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::FromTranslation(AZ::Vector3::CreateOne());
        EXPECT_EQ(actualResult.GetTranslation(), AZ::Vector3::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, GetRight_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::GetRight(AZ::Transform::CreateIdentity(), 1);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Vector3(1, 0, 0)));
#else
        EXPECT_EQ(actualResult, AZ::Vector3(1, 0, 0));
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, GetForward_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::GetForward(AZ::Transform::CreateIdentity(), 1);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Vector3(0, 1, 0)));
#else
        EXPECT_EQ(actualResult, AZ::Vector3(0, 1, 0));
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, GetUp_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::GetUp(AZ::Transform::CreateIdentity(), 1);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Vector3(0, 0, 1)));
#else
        EXPECT_EQ(actualResult, AZ::Vector3(0, 0, 1));
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, GetTranslation_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::GetTranslation(AZ::Transform::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Vector3(0, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, IsClose_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::IsClose(AZ::Transform::CreateIdentity(), AZ::Transform::CreateIdentity(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, IsFinite_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::IsFinite(AZ::Transform::CreateIdentity());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, IsOrthogonal_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::IsOrthogonal(AZ::Transform::CreateIdentity(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, MultiplyByUniformScale_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::MultiplyByUniformScale(AZ::Transform::CreateIdentity(), 5);
        EXPECT_EQ(actualResult.GetUniformScale(), 5);
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, MultiplyByVector3_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::MultiplyByVector3(AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne());
        EXPECT_EQ(actualResult, AZ::Vector3::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, MultiplyByVector4_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::MultiplyByVector4(AZ::Transform::CreateIdentity(), AZ::Vector4::CreateOne());
        EXPECT_EQ(actualResult, AZ::Vector4::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, Orthogonalize_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::Orthogonalize(AZ::Transform::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Transform::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, RotationXDegrees_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::RotationXDegrees(0);
        EXPECT_EQ(actualResult, AZ::Transform::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, RotationYDegrees_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::RotationYDegrees(0);
        EXPECT_EQ(actualResult, AZ::Transform::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, RotationZDegrees_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::RotationZDegrees(0);
        EXPECT_EQ(actualResult, AZ::Transform::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestTransformFunctions, ToScale_Call_GetExpectedResult)
    {
        auto actualResult = TransformFunctions::ToScale(AZ::Transform::CreateIdentity());
        EXPECT_EQ(actualResult, 1);
    }
} // namespace ScriptCanvasUnitTest
