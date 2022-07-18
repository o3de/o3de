/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Vector2.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestVector2Functions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestVector2Functions, Absolute_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Absolute(AZ::Vector2(-1, -1));
        EXPECT_EQ(actualResult, AZ::Vector2::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Angle_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Angle(AZ::Constants::Pi / 2);
        EXPECT_EQ(actualResult, AZ::Vector2(1, 0));
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Clamp_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Clamp(AZ::Vector2(-2, 2), AZ::Vector2(-1, -1), AZ::Vector2(1, 1));
        EXPECT_EQ(actualResult, AZ::Vector2(-1, 1));
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Distance_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Distance(AZ::Vector2(-1, 0), AZ::Vector2(1, 0));
        EXPECT_EQ(actualResult, 2);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, DistanceSquared_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::DistanceSquared(AZ::Vector2(-1, 0), AZ::Vector2(1, 0));
        EXPECT_EQ(actualResult, 4);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Dot_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Dot(AZ::Vector2::CreateOne(), AZ::Vector2::CreateOne());
        EXPECT_EQ(actualResult, 2);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, FromValues_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::FromValues(1, 1);
        EXPECT_EQ(actualResult, AZ::Vector2::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, GetElement_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::GetElement(AZ::Vector2::CreateOne(), 0);
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, IsClose_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::IsClose(AZ::Vector2::CreateOne(), AZ::Vector2::CreateOne(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, IsFinite_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::IsFinite(AZ::Vector2::CreateOne());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, IsNormalized_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::IsNormalized(AZ::Vector2(1, 0), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, IsZero_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::IsZero(AZ::Vector2::CreateZero(), 0.001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Length_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Length(AZ::Vector2(3, 4));
        EXPECT_EQ(actualResult, 5);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, LengthSquared_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::LengthSquared(AZ::Vector2(3, 4));
        EXPECT_EQ(actualResult, 25);
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Lerp_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Lerp(AZ::Vector2(0, 0), AZ::Vector2(1, 0), 0.5);
        EXPECT_EQ(actualResult, AZ::Vector2(0.5, 0));
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Max_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Max(AZ::Vector2(1, 0), AZ::Vector2(0, 1));
        EXPECT_EQ(actualResult, AZ::Vector2::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Min_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Min(AZ::Vector2(1, 0), AZ::Vector2(0, 1));
        EXPECT_EQ(actualResult, AZ::Vector2::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, SetX_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::SetX(AZ::Vector2(1, 0), 0);
        EXPECT_EQ(actualResult, AZ::Vector2::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, SetY_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::SetY(AZ::Vector2(0, 1), 0);
        EXPECT_EQ(actualResult, AZ::Vector2::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, MultiplyByNumber_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::MultiplyByNumber(AZ::Vector2(1, 1), 0);
        EXPECT_EQ(actualResult, AZ::Vector2::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Negate_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Negate(AZ::Vector2(1, 1));
        EXPECT_EQ(actualResult, AZ::Vector2(-1, -1));
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Normalize_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Normalize(AZ::Vector2(1, 0));
        EXPECT_EQ(actualResult, AZ::Vector2(1, 0));
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Project_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Project(AZ::Vector2(1, 0), AZ::Vector2(0, 1));
        EXPECT_EQ(actualResult, AZ::Vector2::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, Slerp_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::Slerp(AZ::Vector2::CreateOne(), AZ::Vector2::CreateOne(), 0.5);
        EXPECT_EQ(actualResult, AZ::Vector2::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, ToPerpendicular_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::ToPerpendicular(AZ::Vector2(1, 0));
        EXPECT_EQ(actualResult, AZ::Vector2(0, 1));
    }

    TEST_F(ScriptCanvasUnitTestVector2Functions, DirectionTo_Call_GetExpectedResult)
    {
        auto actualResult = Vector2Functions::DirectionTo(AZ::Vector2::CreateZero(), AZ::Vector2(1, 0), 1);
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector2(1, 0));
        EXPECT_EQ(AZStd::get<1>(actualResult), 1);
    }
} // namespace ScriptCanvasUnitTest
