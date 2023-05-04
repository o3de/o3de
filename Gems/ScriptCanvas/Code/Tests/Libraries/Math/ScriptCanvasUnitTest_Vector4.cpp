/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Vector4.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestVector4Functions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestVector4Functions, Absolute_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::Absolute(AZ::Vector4(-1, -1, -1, -1));
        EXPECT_EQ(actualResult, AZ::Vector4::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, Dot_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::Dot(AZ::Vector4::CreateOne(), AZ::Vector4::CreateOne());
        EXPECT_EQ(actualResult, 4);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, FromValues_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::FromValues(1, 1, 1, 1);
        EXPECT_EQ(actualResult, AZ::Vector4::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, GetElement_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::GetElement(AZ::Vector4::CreateOne(), 0);
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, IsClose_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::IsClose(AZ::Vector4::CreateOne(), AZ::Vector4::CreateOne(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, IsFinite_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::IsFinite(AZ::Vector4::CreateOne());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, IsNormalized_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::IsNormalized(AZ::Vector4(1, 0, 0, 0), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, IsZero_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::IsZero(AZ::Vector4::CreateZero(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, Length_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::Length(AZ::Vector4(3, 4, 0, 0));
        EXPECT_EQ(actualResult, 5);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, LengthReciprocal_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::LengthReciprocal(AZ::Vector4(-3, 4, 0, 0));
        EXPECT_TRUE(AZStd::abs(actualResult - 0.2) < 0.001);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, LengthSquared_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::LengthSquared(AZ::Vector4(3, 4, 0, 0));
        EXPECT_EQ(actualResult, 25);
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, SetX_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::SetX(AZ::Vector4(1, 0, 0, 0), 0);
        EXPECT_EQ(actualResult, AZ::Vector4::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, SetY_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::SetY(AZ::Vector4(0, 1, 0, 0), 0);
        EXPECT_EQ(actualResult, AZ::Vector4::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, SetZ_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::SetZ(AZ::Vector4(0, 0, 1, 0), 0);
        EXPECT_EQ(actualResult, AZ::Vector4::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, SetW_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::SetW(AZ::Vector4(0, 0, 0, 1), 0);
        EXPECT_EQ(actualResult, AZ::Vector4::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, MultiplyByNumber_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::MultiplyByNumber(AZ::Vector4(1, 1, 1, 1), 0);
        EXPECT_EQ(actualResult, AZ::Vector4::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, Negate_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::Negate(AZ::Vector4(1, 1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Vector4(-1, -1, -1, -1));
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, Normalize_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::Normalize(AZ::Vector4(1, 0, 0, 0));
        EXPECT_EQ(actualResult, AZ::Vector4(1, 0, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, Reciprocal_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::Reciprocal(AZ::Vector4::CreateOne());
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Vector4::CreateOne()));
#else
        EXPECT_EQ(actualResult, AZ::Vector4::CreateOne());
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestVector4Functions, DirectionTo_Call_GetExpectedResult)
    {
        auto actualResult = Vector4Functions::DirectionTo(AZ::Vector4::CreateZero(), AZ::Vector4(1, 0, 0, 0), 1);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(AZStd::get<0>(actualResult), IsClose(AZ::Vector4(1, 0, 0, 0)));
#else
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector4(1, 0, 0, 0));
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_EQ(AZStd::get<1>(actualResult), 1);
    }
} // namespace ScriptCanvasUnitTest
