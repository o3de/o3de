/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Color.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;
    
    using ScriptCanvasUnitTestColorFunctions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestColorFunctions, Dot_Call_GetExpectedResult)
    {
        AZ::Color a = AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1));
        AZ::Color b = AZ::Color::CreateFromVector3(AZ::Vector3(0, 0, 0));
        auto actualResult = ColorFunctions::Dot(a, b);
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, Dot3_Call_GetExpectedResult)
    {
        AZ::Color a = AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1));
        AZ::Color b = AZ::Color::CreateFromVector3(AZ::Vector3(0, 0, 0));
        auto actualResult = ColorFunctions::Dot3(a, b);
        EXPECT_EQ(actualResult, 0);
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, FromValues_Call_GetExpectedResult)
    {
        auto actualResult = ColorFunctions::FromValues(1, 1, 1, 1);
        EXPECT_EQ(actualResult, AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1)));
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, FromVector3_Call_GetExpectedResult)
    {
        auto actualResult = ColorFunctions::FromVector3(AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1)));
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, FromVector3AndNumber_Call_GetExpectedResult)
    {
        auto actualResult = ColorFunctions::FromVector3AndNumber(AZ::Vector3(1, 1, 1), 1);
        EXPECT_EQ(actualResult, AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1)));
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, GammaToLinear_Call_GetExpectedResult)
    {
        AZ::Color a = AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1));
        auto actualResult = ColorFunctions::GammaToLinear(a);
        EXPECT_EQ(actualResult, a.GammaToLinear());
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, IsClose_Call_GetExpectedResult)
    {
        AZ::Color a = AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1));
        AZ::Color b = AZ::Color::CreateFromVector3(AZ::Vector3(0, 0, 0));
        auto actualResult = ColorFunctions::IsClose(a, b, 1);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, IsZero_Call_GetExpectedResult)
    {
        AZ::Color a = AZ::Color::CreateFromVector3AndFloat(AZ::Vector3(0, 0, 0), 0);
        auto actualResult = ColorFunctions::IsZero(a, 0.0001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, LinearToGamma_Call_GetExpectedResult)
    {
        AZ::Color a = AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1));
        auto actualResult = ColorFunctions::LinearToGamma(a);
        EXPECT_EQ(actualResult, a.LinearToGamma());
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, MultiplyByNumber_Call_GetExpectedResult)
    {
        AZ::Color a = AZ::Color::CreateFromVector3(AZ::Vector3(0.1, 0.1, 0.1));
        auto actualResult = ColorFunctions::MultiplyByNumber(a, 5);
        EXPECT_EQ(actualResult, AZ::Color::CreateFromVector3AndFloat(AZ::Vector3(0.5, 0.5, 0.5), 5));
    }

    TEST_F(ScriptCanvasUnitTestColorFunctions, One_Call_GetExpectedResult)
    {
        auto actualResult = ColorFunctions::One();
        EXPECT_EQ(actualResult, AZ::Color::CreateFromVector3(AZ::Vector3(1, 1, 1)));
    }
} // namespace ScriptCanvasUnitTest
