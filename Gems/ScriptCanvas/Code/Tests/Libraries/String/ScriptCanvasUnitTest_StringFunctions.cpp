/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/String/StringFunctions.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestStringFunctions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestStringFunctions, ToLower_Call_GetExpectedResult)
    {
        auto actualResult = StringFunctions::ToLower("LOWER");
        EXPECT_EQ(actualResult, "lower");
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, ToUpper_Call_GetExpectedResult)
    {
        auto actualResult = StringFunctions::ToUpper("upper");
        EXPECT_EQ(actualResult, "UPPER");
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, Substring_Call_GetExpectedResult)
    {
        auto actualResult = StringFunctions::Substring("test message", 0, 4);
        EXPECT_EQ(actualResult, "test");
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, ContainsString_Call_GetExpectedResult)
    {
        auto actualResult = StringFunctions::ContainsString("test message", "test", false, true);
        EXPECT_EQ(actualResult, 0);
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, ContainsString_CallWithCaseInsensitive_GetExpectedResult)
    {
        auto actualResult = StringFunctions::ContainsString("test message", "TEST", false, false);
        EXPECT_EQ(actualResult, 0);
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, StartsWith_Call_GetExpectedResult)
    {
        auto actualResult = StringFunctions::StartsWith("test message", "test", true);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, StartsWith_CallWithCaseInsensitive_GetExpectedResult)
    {
        auto actualResult = StringFunctions::StartsWith("test message", "TEST", false);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, EndsWith_Call_GetExpectedResult)
    {
        auto actualResult = StringFunctions::EndsWith("test message", "message", true);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, EndsWith_CallWithCaseInsensitive_GetExpectedResult)
    {
        auto actualResult = StringFunctions::EndsWith("test message", "MESSAGE", false);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, Join_Call_GetExpectedResult)
    {
        auto actualResult = StringFunctions::Join({ "test", "message" }, " ");
        EXPECT_EQ(actualResult, "test message");
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, ReplaceString_Call_GetExpectedResult)
    {
        AZStd::string source = "test message";
        auto actualResult = StringFunctions::ReplaceString(source, "test", "actual", true);
        EXPECT_EQ(actualResult, "actual message");
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, ReplaceString_CallWithCaseInsensitive_GetExpectedResult)
    {
        AZStd::string source = "test message";
        auto actualResult = StringFunctions::ReplaceString(source, "TEST", "actual", false);
        EXPECT_EQ(actualResult, "actual message");
    }

    TEST_F(ScriptCanvasUnitTestStringFunctions, Split_Call_GetExpectedResult)
    {
        auto actualResult = StringFunctions::Split("test message", " ");
        EXPECT_EQ(actualResult.size(), 2);
    }
} // namespace ScriptCanvasUnitTest
