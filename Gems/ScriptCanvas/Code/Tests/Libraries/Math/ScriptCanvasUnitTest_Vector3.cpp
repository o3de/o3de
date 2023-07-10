/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Vector3.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestVector3Functions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestVector3Functions, Absolute_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Absolute(AZ::Vector3(-1, -1, -1));
        EXPECT_EQ(actualResult, AZ::Vector3::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, BuildTangentBasis_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::BuildTangentBasis(AZ::Vector3(1, 0, 0));
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector3(0, 0, 1));
        EXPECT_EQ(AZStd::get<1>(actualResult), AZ::Vector3(0, -1, 0));
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Clamp_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Clamp(AZ::Vector3(-2, 2, 0), AZ::Vector3(-1, -1, -1), AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Vector3(-1, 1, 0));
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Cross_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Cross(AZ::Vector3::CreateOne(), AZ::Vector3::CreateOne());
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Distance_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Distance(AZ::Vector3(-1, 0, 0), AZ::Vector3(1, 0, 0));
        EXPECT_EQ(actualResult, 2);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, DistanceSquared_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::DistanceSquared(AZ::Vector3(-1, 0, 0), AZ::Vector3(1, 0, 0));
        EXPECT_EQ(actualResult, 4);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Dot_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Dot(AZ::Vector3::CreateOne(), AZ::Vector3::CreateOne());
        EXPECT_EQ(actualResult, 3);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, FromValues_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::FromValues(1, 1, 1);
        EXPECT_EQ(actualResult, AZ::Vector3::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, GetElement_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::GetElement(AZ::Vector3::CreateOne(), 0);
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, IsClose_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::IsClose(AZ::Vector3::CreateOne(), AZ::Vector3::CreateOne(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, IsFinite_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::IsFinite(AZ::Vector3::CreateOne());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, IsNormalized_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::IsNormalized(AZ::Vector3(1, 0, 0), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, IsPerpendicular_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::IsPerpendicular(AZ::Vector3(1, 0, 0), AZ::Vector3(0, 1, 0), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, IsZero_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::IsZero(AZ::Vector3::CreateZero(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Length_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Length(AZ::Vector3(3, 4, 0));
        EXPECT_EQ(actualResult, 5);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, LengthReciprocal_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::LengthReciprocal(AZ::Vector3(-3, 4, 0));
        EXPECT_TRUE(AZStd::abs(actualResult - 0.2) < 0.001);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, LengthSquared_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::LengthSquared(AZ::Vector3(3, 4, 0));
        EXPECT_EQ(actualResult, 25);
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Lerp_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Lerp(AZ::Vector3(0, 0, 0), AZ::Vector3(1, 0, 0), 0.5);
        EXPECT_EQ(actualResult, AZ::Vector3(0.5, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Max_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Max(AZ::Vector3(1, 0, 0), AZ::Vector3(0, 1, 1));
        EXPECT_EQ(actualResult, AZ::Vector3::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Min_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Min(AZ::Vector3(1, 0, 0), AZ::Vector3(0, 1, 0));
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, SetX_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::SetX(AZ::Vector3(1, 0, 0), 0);
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, SetY_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::SetY(AZ::Vector3(0, 1, 0), 0);
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, SetZ_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::SetZ(AZ::Vector3(0, 0, 1), 0);
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, MultiplyByNumber_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::MultiplyByNumber(AZ::Vector3(1, 1, 1), 0);
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Negate_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Negate(AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Vector3(-1, -1, -1));
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Normalize_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Normalize(AZ::Vector3(1, 0, 0));
        EXPECT_EQ(actualResult, AZ::Vector3(1, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Project_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Project(AZ::Vector3(1, 0, 0), AZ::Vector3(0, 1, 0));
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Reciprocal_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Reciprocal(AZ::Vector3::CreateOne());
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Vector3::CreateOne()));
#else
        EXPECT_EQ(actualResult, AZ::Vector3::CreateOne());
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, Slerp_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::Slerp(AZ::Vector3::CreateOne(), AZ::Vector3::CreateOne(), 0.5);
        EXPECT_EQ(actualResult, AZ::Vector3::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector3Functions, DirectionTo_Call_GetExpectedResult)
    {
        auto actualResult = Vector3Functions::DirectionTo(AZ::Vector3::CreateZero(), AZ::Vector3(1, 0, 0), 1);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(AZStd::get<0>(actualResult), IsClose(AZ::Vector3(1, 0, 0)));
#else
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector3(1, 0, 0));
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_EQ(AZStd::get<1>(actualResult), 1);
    }
} // namespace ScriptCanvasUnitTest
