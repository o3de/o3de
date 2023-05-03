/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest-param-test.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <string.h>

namespace AZ
{
    using namespace UnitTest;

    using StringFuncTest = LeakDetectionFixture;

    TEST_F(StringFuncTest, Equal_CaseSensitive_OnNonNullTerminatedStringView_Success)
    {
        constexpr AZStd::string_view testString1 = "Hello World IceCream";
        constexpr AZStd::string_view testString2 = "IceCream World Hello";
        constexpr bool caseSensitive = true;
        EXPECT_TRUE(AZ::StringFunc::Equal(testString1.substr(6,5), testString2.substr(9,5), caseSensitive));
    }
    TEST_F(StringFuncTest, Equal_CaseInsensitive_OnNonNullTerminatedStringView_Success)
    {
        constexpr AZStd::string_view testString1 = "Hello World IceCream";
        constexpr AZStd::string_view testString2 = "IceCream woRLd Hello";
        constexpr bool caseSensitive = false;
        EXPECT_TRUE(AZ::StringFunc::Equal(testString1.substr(6, 5), testString2.substr(9, 5), caseSensitive));
    }
    TEST_F(StringFuncTest, Equal_CaseSensitive_OnNonNullTerminatedStringView_WithDifferentCases_Fails)
    {
        constexpr AZStd::string_view testString1 = "Hello World IceCream";
        constexpr AZStd::string_view testString2 = "IceCream woRLd Hello";
        constexpr bool caseSensitive = true;
        EXPECT_FALSE(AZ::StringFunc::Equal(testString1.substr(6, 5), testString2.substr(9, 5), caseSensitive));
    }
    TEST_F(StringFuncTest, Equal_OnNonNullTerminatedStringView_WithDifferentSize_Fails)
    {
        constexpr AZStd::string_view testString1 = "Hello World IceCream";
        constexpr AZStd::string_view testString2 = "IceCream World Hello";

        EXPECT_FALSE(AZ::StringFunc::Equal(testString1.substr(6, 6), testString2.substr(9, 5)));
    }

    // Strip out any trailing path separators

    TEST_F(StringFuncTest, Strip_ValidInputExtraEndingPathSeparators_Success)
    {
        AZStd::string       samplePath = "F:\\w s 1\\dev\\/";
        AZStd::string       expectedResult = "F:\\w s 1\\dev";
        const char*         stripCharacters = AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING  AZ_WRONG_FILESYSTEM_SEPARATOR_STRING;

        AZ::StringFunc::Strip(samplePath, stripCharacters, false, false, true);

        ASSERT_TRUE(samplePath == expectedResult);
    }
    
    TEST_F(StringFuncTest, Strip_AllEmptyResult1_Success)
    {
        AZStd::string input = "aa";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken);

        ASSERT_TRUE(input.empty());
    }

    TEST_F(StringFuncTest, Strip_AllEmptyResult2_Success)
    {
        AZStd::string input = "aaaa";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken);

        ASSERT_TRUE(input.empty());
    }

    TEST_F(StringFuncTest, Strip_BeginEndCaseSensitiveEmptyResult1_Success)
    {
        AZStd::string input = "aa";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken, true, true, true);

        ASSERT_TRUE(input.empty());
    }

    TEST_F(StringFuncTest, Strip_BeginEndCaseSensitiveEmptyResult2_Success)
    {
        AZStd::string input = "aaaa";
        AZStd::string expectedResult = "aa";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken, true, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_BeginEndCaseInsensitiveEmptyResult1_Success)
    {
        AZStd::string input = "aa";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_TRUE(input.empty());
    }

    TEST_F(StringFuncTest, Strip_BeginEndCaseInsensitiveEmptyResult2_Success)
    {
        AZStd::string input = "aaaa";
        AZStd::string expectedResult = "aa";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_EQ(input, expectedResult);
    }
    TEST_F(StringFuncTest, Join_PathsWithNoOverlappingDirectoriesDisabled_Success)
    {
        AZStd::string path1 = "1/2/3/4";
        AZStd::string path2 = "5/6";
        AZStd::string expectedResult = "1/2/3/4/5/6";
        AZStd::string joinResult;

        AZ::StringFunc::Path::Normalize(path1);
        AZ::StringFunc::Path::Normalize(path2);
        AZ::StringFunc::Path::Normalize(expectedResult);
        AZ::StringFunc::Path::Join(path1.c_str(), path2.c_str(), joinResult);

        ASSERT_EQ(joinResult, expectedResult);
    }

    TEST_F(StringFuncTest, Join_PathsWithOverlappingDirectoriesDisabled_Success)
    {
        AZStd::string path1 = "1/2/3/4";
        AZStd::string path2 = "3/4/5/6";
        AZStd::string expectedResult = "1/2/3/4/3/4/5/6";
        AZStd::string joinResult;

        AZ::StringFunc::Path::Normalize(path1);
        AZ::StringFunc::Path::Normalize(path2);
        AZ::StringFunc::Path::Normalize(expectedResult);
        AZ::StringFunc::Path::Join(path1.c_str(), path2.c_str(), joinResult);

        ASSERT_EQ(joinResult, expectedResult);
    }

    TEST_F(StringFuncTest, Join_PathsWithNoOverlappingDirectories_Success)
    {
        AZStd::string path1 = "1/2/3/4";
        AZStd::string path2 = "5/6";
        AZStd::string expectedResult = "1/2/3/4/5/6";
        AZStd::string joinResult;

        AZ::StringFunc::Path::Normalize(path1);
        AZ::StringFunc::Path::Normalize(path2);
        AZ::StringFunc::Path::Normalize(expectedResult);
        AZ::StringFunc::Path::Join(path1.c_str(), path2.c_str(), joinResult);

        ASSERT_EQ(joinResult, expectedResult);
    }

    TEST_F(StringFuncTest, Join_PathsWithOverlappingDirectories_Success)
    {
        AZStd::string path1 = "1/2/3/4";
        AZStd::string path2 = "3/4/5/6";
        AZStd::string expectedResult = "1/2/3/4/3/4/5/6";
        AZStd::string joinResult;

        AZ::StringFunc::Path::Normalize(path1);
        AZ::StringFunc::Path::Normalize(path2);
        AZ::StringFunc::Path::Normalize(expectedResult);
        AZ::StringFunc::Path::Join(path1.c_str(), path2.c_str(), joinResult);

        ASSERT_EQ(joinResult, expectedResult);
    }

    TEST_F(StringFuncTest, Join_PathsWithSecondNameOverlappingDirectory_Success)
    {
        AZStd::string path1 = "1/2/3/4";
        AZStd::string path2 = "3";
        AZStd::string expectedResult = "1/2/3/4/3";
        AZStd::string joinResult;

        AZ::StringFunc::Path::Normalize(path1);
        AZ::StringFunc::Path::Normalize(path2);
        AZ::StringFunc::Path::Normalize(expectedResult);
        AZ::StringFunc::Path::Join(path1.c_str(), path2.c_str(), joinResult);

        ASSERT_EQ(joinResult, expectedResult);
    }

    TEST_F(StringFuncTest, Join_NonPathJoin_CanJoinRange)
    {
        AZStd::string result;
        AZ::StringFunc::Join(result, AZStd::initializer_list<const char*>{ "1", "2", "3", "4", "3" }, '/');
        EXPECT_EQ("1/2/3/4/3", result);

        result.clear();

        // Try joining with a string literal instead of a char literal
        AZ::StringFunc::Join(result, AZStd::initializer_list<const char*>{ "1", "2", "3", "4", "3" }, "/");
        EXPECT_EQ("1/2/3/4/3", result);
    }

    TEST_F(StringFuncTest, Tokenize_SingleDelimeter_Empty)
    {
        AZStd::string input = "";
        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(input.c_str(), tokens, ' ');
        ASSERT_EQ(tokens.size(), 0);
    }

    TEST_F(StringFuncTest, Tokenize_SingleDelimeter)
    {
        AZStd::string input = "a b,c";
        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(input.c_str(), tokens, ' ');

        ASSERT_EQ(tokens.size(), 2);
        ASSERT_TRUE(tokens[0] == "a");
        ASSERT_TRUE(tokens[1] == "b,c");
    }

    TEST_F(StringFuncTest, Tokenize_MultiDelimeter_Empty)
    {
        AZStd::string input = "";
        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(input.c_str(), tokens, " ,");
        ASSERT_EQ(tokens.size(), 0);
    }

    TEST_F(StringFuncTest, Tokenize_MultiDelimeters)
    {
        AZStd::string input = " -a +b +c -d-e";
        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(input.c_str(), tokens, "-+");

        ASSERT_EQ(tokens.size(), 5);
        ASSERT_TRUE(tokens[0] == "a ");
        ASSERT_TRUE(tokens[1] == "b ");
        ASSERT_TRUE(tokens[2] == "c ");
        ASSERT_TRUE(tokens[3] == "d");
        ASSERT_TRUE(tokens[4] == "e");
    }

    TEST_F(StringFuncTest, Tokenize_SubstringDelimeters_Empty)
    {
        AZStd::string input = "";
        AZStd::vector<AZStd::string> tokens;
        AZStd::vector<AZStd::string_view> delimeters = {" -", " +"};
        AZ::StringFunc::Tokenize(input.c_str(), tokens, delimeters);
        ASSERT_EQ(tokens.size(), 0);
    }

    TEST_F(StringFuncTest, Tokenize_SubstringDelimeters)
    {
        AZStd::string input = " -a +b +c -d-e";
        AZStd::vector<AZStd::string> tokens;
        AZStd::vector<AZStd::string_view> delimeters = { " -", " +" };
        AZ::StringFunc::Tokenize(input.c_str(), tokens, delimeters);

        ASSERT_EQ(tokens.size(), 4);
        ASSERT_TRUE(tokens[0] == "a");
        ASSERT_TRUE(tokens[1] == "b");
        ASSERT_TRUE(tokens[2] == "c");
        ASSERT_TRUE(tokens[3] == "d-e"); // Test for something like a guid, which contain typical separator characters
    }

    TEST_F(StringFuncTest, TokenizeVisitor_EmptyString_DoesNotInvokeVisitor)
    {
        int visitedCount{};
        auto visitor = [&visitedCount](AZStd::string_view)
        {
            ++visitedCount;
        };
        AZ::StringFunc::TokenizeVisitor("", visitor, " ");
        EXPECT_EQ(0, visitedCount);
    }

    TEST_F(StringFuncTest, TokenizeVisitor_NonDelimitedString_InvokeVisitorOnce)
    {
        int visitedCount{};
        auto visitor = [&visitedCount](AZStd::string_view)
        {
            ++visitedCount;
        };
        AZ::StringFunc::TokenizeVisitor("Hello", visitor, " ");
        EXPECT_EQ(1, visitedCount);
    }

    TEST_F(StringFuncTest, TokenizeVisitor_DelimitedString_InvokeVisitorMultipleTimes)
    {
        int visitedCount{};
        auto visitor = [&visitedCount](AZStd::string_view)
        {
            ++visitedCount;
        };
        AZ::StringFunc::TokenizeVisitor("Hello World", visitor, " ");
        EXPECT_EQ(2, visitedCount);

        visitedCount = {};
        AZ::StringFunc::TokenizeVisitor("Hello  World   Again", visitor, " ");
        EXPECT_EQ(3, visitedCount);
    }

    TEST_F(StringFuncTest, TokenizeVisitor_EmptyStringOption_InvokeVisitorWithEmptyString)
    {
        int visitedCount{};
        bool emptyStringFound{};
        auto visitor = [&visitedCount, &emptyStringFound](AZStd::string_view token)
        {
            ++visitedCount;
            if (token.empty())
            {
                emptyStringFound = true;
            }
        };
        AZ::StringFunc::TokenizeVisitor("Hello,,World", visitor, ",", true);
        EXPECT_EQ(3, visitedCount);
        EXPECT_TRUE(emptyStringFound);

        visitedCount = {};
        emptyStringFound = {};
        AZ::StringFunc::TokenizeVisitor(",Hello,World", visitor, ",", true);
        EXPECT_EQ(3, visitedCount);
        EXPECT_TRUE(emptyStringFound);
    }

    TEST_F(StringFuncTest, TokenizeVisitor_WhiteSpaceOption_InvokeVisitorWithWhiteSpaceAroundTokenString)
    {
        int visitedCount{};
        bool trailingWhitespaceTokenFound{};
        bool leadingWhitespaceTokenFound{};
        auto visitor = [&visitedCount, &trailingWhitespaceTokenFound, &leadingWhitespaceTokenFound](AZStd::string_view token)
        {
            ++visitedCount;
            if (token.starts_with(' '))
            {
                leadingWhitespaceTokenFound = true;
            }
            if (token.ends_with(' '))
            {
                trailingWhitespaceTokenFound = true;
            }
        };
        AZ::StringFunc::TokenizeVisitor("Hello  ,    World", visitor, ",", false, true);
        EXPECT_EQ(2, visitedCount);
        EXPECT_TRUE(trailingWhitespaceTokenFound);
        EXPECT_TRUE(leadingWhitespaceTokenFound);

        visitedCount = {};
        trailingWhitespaceTokenFound = {};
        leadingWhitespaceTokenFound = {};
        AZ::StringFunc::TokenizeVisitor("Hello, World", visitor, ",", false, true);
        EXPECT_EQ(2, visitedCount);
        EXPECT_FALSE(trailingWhitespaceTokenFound);
        EXPECT_TRUE(leadingWhitespaceTokenFound);

        visitedCount = {};
        trailingWhitespaceTokenFound = {};
        leadingWhitespaceTokenFound = {};
        AZ::StringFunc::TokenizeVisitor("Hello ,World", visitor, ",", false, true);
        EXPECT_EQ(2, visitedCount);
        EXPECT_TRUE(trailingWhitespaceTokenFound);
        EXPECT_FALSE(leadingWhitespaceTokenFound);
    }

    TEST_F(StringFuncTest, TokenizeVisitor_DocExampleAboveFunctionDeclaration_Succeeds)
    {
        constexpr AZStd::array visitTokens = { "Hello", "World", "", "More", "", "", "Tokens" };
        size_t visitIndex{};
        AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                  // capture. Newer versions issue unused warning
        auto visitor = [&visitIndex, &visitTokens](AZStd::string_view token)
        AZ_POP_DISABLE_WARNING
        {
            if (visitIndex > visitTokens.size())
            {
                ADD_FAILURE() << "More tokens have been visited than are expected:" << visitTokens.size();
                return;
            }

            if (token != visitTokens[visitIndex])
            {
                AZStd::fixed_string<64> result{ token };
                ADD_FAILURE() << "Visited token \"" << result.c_str()  << "\" does not match token \"" << visitTokens[visitIndex] << "\" at index (" << visitIndex << ") in the visitTokens array";
            }
            ++visitIndex;
        };
        AZ::StringFunc::TokenizeVisitor("Hello World  More   Tokens", visitor, " ", true, true);
        AZStd::vector<AZStd::string> resultTokens;
        AZ::StringFunc::Tokenize("Hello World  More   Tokens", resultTokens, ' ', true, true);
        ASSERT_EQ(visitTokens.size(), resultTokens.size());
        EXPECT_TRUE(AZStd::equal(resultTokens.begin(), resultTokens.end(), visitTokens.begin()));
    }

    TEST_F(StringFuncTest, TokenizeVisitorReverse_DocExampleAboveFunctionDeclaration_Succeeds)
    {
        constexpr AZStd::array visitTokens = { "Hello", "World", "", "More", "", "", "Tokens" };
        size_t visitIndex = visitTokens.size() - 1;
        AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                  // capture. Newer versions issue unused warning
        auto visitor = [&visitIndex, &visitTokens](AZStd::string_view token)
        AZ_POP_DISABLE_WARNING
        {
            if (visitIndex > visitTokens.size())
            {
                ADD_FAILURE() << "More tokens have been visited than are expected:" << visitTokens.size();
                return;
            }

            if (token != visitTokens[visitIndex])
            {
                AZStd::fixed_string<64> result{ token };
                ADD_FAILURE() << "Visited token \"" << result.c_str() << "\" does not match token \"" << visitTokens[visitIndex] << "\" at index (" << visitIndex << ") in the visitTokens array";
            }
            --visitIndex;
        };
        AZ::StringFunc::TokenizeVisitorReverse("Hello World  More   Tokens", visitor, " ", true, true);
        AZStd::vector<AZStd::string> resultTokens;

        // Visitor functor which replaces the regular Tokenize behavior of inserting a new token into a vector of strings
        auto addToVectorVisitor = [&resultTokens](AZStd::string_view token)
        {
            resultTokens.push_back(token);
        };
        AZ::StringFunc::TokenizeVisitorReverse("Hello World  More   Tokens", addToVectorVisitor, ' ', true, true);
        ASSERT_EQ(visitTokens.size(), resultTokens.size());
        EXPECT_TRUE(AZStd::equal(resultTokens.begin(), resultTokens.end(), visitTokens.rbegin()));
    }

    TEST_F(StringFuncTest, TokenizeNext_WithEmptyString_ReturnsInvalid)
    {
        AZStd::string_view input{ "" };
        AZStd::optional<AZStd::string_view> token = AZ::StringFunc::TokenizeNext(input, ",");
        EXPECT_FALSE(token);
    }
    TEST_F(StringFuncTest, TokenizeNext_WithNonEmptyString_ReturnsValid)
    {
        AZStd::string_view input{ " " };
        AZStd::optional<AZStd::string_view> token = AZ::StringFunc::TokenizeNext(input, ",");
        EXPECT_TRUE(token);
        EXPECT_EQ(" ", *token);
    }

    TEST_F(StringFuncTest, TokenizeNext_DocExample1AboveFunctionDeclaration_Succeeds)
    {
        AZStd::string_view input = "Hello World";
        AZStd::optional<AZStd::string_view> output;
        output = StringFunc::TokenizeNext(input, " ");
        EXPECT_EQ("World", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("Hello", output);
        output = StringFunc::TokenizeNext(input, " ");
        EXPECT_EQ("", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("World", output);
        output = StringFunc::TokenizeNext(input, " ");
        EXPECT_EQ("", input);
        EXPECT_FALSE(output);
    }

    TEST_F(StringFuncTest, TokenizeNext_DocExample2AboveFunctionDeclaration_Succeeds)
    {
        AZStd::string_view input = "Hello World  More   Tokens";
        AZStd::optional<AZStd::string_view> output;
        output = StringFunc::TokenizeNext(input, ' ');
        EXPECT_EQ("World  More   Tokens", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("Hello", output);
        output = StringFunc::TokenizeNext(input, ' ');
        EXPECT_EQ(" More   Tokens", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("World", output);
        output = StringFunc::TokenizeNext(input, ' ');
        EXPECT_EQ("More   Tokens", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("", output);
        output = StringFunc::TokenizeNext(input, ' ');
        EXPECT_EQ("  Tokens", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("More", output);
        output = StringFunc::TokenizeNext(input, ' ');
        EXPECT_EQ(" Tokens", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("", output);
        output = StringFunc::TokenizeNext(input, ' ');
        EXPECT_EQ("Tokens", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("", output);
        output = StringFunc::TokenizeNext(input, ' ');
        EXPECT_EQ("", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("Tokens", output);
        output = StringFunc::TokenizeNext(input, ' ');
        EXPECT_EQ("", input);
        EXPECT_FALSE(output);
    }

    TEST_F(StringFuncTest, TokenizeLast_DocExample1AboveFunctionDeclaration_Succeeds)
    {
        AZStd::string_view input = "Hello World";
        AZStd::optional<AZStd::string_view> output;
        output = StringFunc::TokenizeLast(input, " ");
        EXPECT_EQ("Hello", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("World", output);
        output = StringFunc::TokenizeLast(input, " ");
        EXPECT_EQ("", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("Hello", output);
        output = StringFunc::TokenizeLast(input, " ");
        EXPECT_EQ("", input);
        EXPECT_FALSE(output);
    }

    TEST_F(StringFuncTest, TokenizeLast_DocExample2AboveFunctionDeclaration_Succeeds)
    {
        AZStd::string_view input = "Hello World  More   Tokens";
        AZStd::optional<AZStd::string_view> output;
        output = StringFunc::TokenizeLast(input, ' ');
        EXPECT_EQ("Hello World  More  ", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("Tokens", output);
        output = StringFunc::TokenizeLast(input, ' ');
        EXPECT_EQ("Hello World  More ", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("", output);
        output = StringFunc::TokenizeLast(input, ' ');
        EXPECT_EQ("Hello World  More", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("", output);
        output = StringFunc::TokenizeLast(input, ' ');
        EXPECT_EQ("Hello World ", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("More", output);
        output = StringFunc::TokenizeLast(input, ' ');
        EXPECT_EQ("Hello World", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("", output);
        output = StringFunc::TokenizeLast(input, ' ');
        EXPECT_EQ("Hello", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("World", output);
        output = StringFunc::TokenizeLast(input, ' ');
        EXPECT_EQ("", input);
        EXPECT_TRUE(output);
        EXPECT_EQ("Hello", output);
        output = StringFunc::TokenizeLast(input, ' ');
        EXPECT_EQ("", input);
        EXPECT_FALSE(output);
    }

    TEST_F(StringFuncTest, GroupDigits_BasicFunctionality)
    {
        // Test a bunch of numbers and other inputs
        const AZStd::pair<AZStd::string, AZStd::string> inputsAndExpectedOutputs[] = 
        {
            { "0", "0" },
            { "10", "10" },
            { "100", "100" },
            { "1000", "1,000" },
            { "10000", "10,000" },
            { "100000", "100,000" },
            { "1000000", "1,000,000" },
            { "0.0", "0.0"},
            { "10.0", "10.0"},
            { "100.0", "100.0" },
            { "1000.0", "1,000.0" },
            { "10000.0", "10,000.0" },
            { "100000.0", "100,000.0" },
            { "1000000.0", "1,000,000.0" },
            { "-0.0", "-0.0" },
            { "-10.0", "-10.0" },
            { "-100.0", "-100.0" },
            { "-1000.0", "-1,000.0" },
            { "-10000.0", "-10,000.0" },
            { "-100000.0", "-100,000.0" },
            { "-1000000.0", "-1,000,000.0" },
            { "foo", "foo" },
            { "foo123.0", "foo123.0" },
            { "foo1234.0", "foo1,234.0" }
        };

        static const size_t BUFFER_SIZE = 32;
        char buffer[BUFFER_SIZE];

        for (const auto& inputAndExpectedOutput : inputsAndExpectedOutputs)
        {
            azstrncpy(buffer, BUFFER_SIZE, inputAndExpectedOutput.first.c_str(), inputAndExpectedOutput.first.length() + 1);
            auto endPos = StringFunc::NumberFormatting::GroupDigits(buffer, BUFFER_SIZE);
            EXPECT_STREQ(buffer, inputAndExpectedOutput.second.c_str());
            EXPECT_EQ(inputAndExpectedOutput.second.length(), endPos);
        }

        // Test valid inputs
        AZ_TEST_START_ASSERTTEST;
        StringFunc::NumberFormatting::GroupDigits(nullptr, 0);  // Should assert twice, for null pointer and 0 being <= decimalPosHint
        StringFunc::NumberFormatting::GroupDigits(buffer, BUFFER_SIZE, 0, ',', '.', 0, 0);  // Should assert for having a non-positive grouping size
        AZ_TEST_STOP_ASSERTTEST(3);

        // Test buffer overruns
        static const size_t SMALL_BUFFER_SIZE = 8;
        char smallBuffer[SMALL_BUFFER_SIZE];
        char prevSmallBuffer[SMALL_BUFFER_SIZE];

        for (const auto& inputAndExpectedOutput : inputsAndExpectedOutputs)
        {
            azstrncpy(smallBuffer, SMALL_BUFFER_SIZE, inputAndExpectedOutput.first.c_str(), AZStd::min(inputAndExpectedOutput.first.length() + 1, SMALL_BUFFER_SIZE - 1));
            smallBuffer[SMALL_BUFFER_SIZE - 1] = 0;  // Force null-termination
            memcpy(prevSmallBuffer, smallBuffer, SMALL_BUFFER_SIZE);
            auto endPos = StringFunc::NumberFormatting::GroupDigits(smallBuffer, SMALL_BUFFER_SIZE);

            if (inputAndExpectedOutput.second.length() >= SMALL_BUFFER_SIZE)
            {
                EXPECT_STREQ(smallBuffer, prevSmallBuffer);  // No change if buffer overruns
            }
            else
            {
                EXPECT_STREQ(smallBuffer, inputAndExpectedOutput.second.c_str());
                EXPECT_EQ(inputAndExpectedOutput.second.length(), endPos);
            }
        }
    }

    TEST_F(StringFuncTest, GroupDigits_DifferentSettings)
    {
        struct TestSettings
        {
            AZStd::string m_input;
            AZStd::string m_output;
            size_t m_decimalPosHint;
            char m_digitSeparator;
            char m_decimalSeparator;
            int m_groupingSize;
            int m_firstGroupingSize;
        };

        const TestSettings testSettings[] =
        {
            { "123456789.0123", "123,456,789.0123", 9, ',', '.', 3, 0 },  // Correct decimal position hint
            { "123456789.0123", "123,456,789.0123", 8, ',', '.', 3, 0 },  // Incorrect decimal position hint
            { "123456789,0123", "123.456.789,0123", 0, '.', ',', 3, 0 },  // Swap glyphs used for decimal and grouping
            { "123456789.0123", "1,23,45,67,89.0123", 8, ',', '.', 2, 0 },  // Different grouping size
            { "123456789.0123", "12,34,56,789.0123", 8, ',', '.', 2, 3 },  // Customized grouping size for first group
        };

        static const size_t BUFFER_SIZE = 32;
        char buffer[BUFFER_SIZE];

        for (const auto& settings : testSettings)
        {
            azstrncpy(buffer, BUFFER_SIZE, settings.m_input.c_str(), settings.m_input.length() + 1);
            auto endPos = StringFunc::NumberFormatting::GroupDigits(buffer, BUFFER_SIZE, settings.m_decimalPosHint, settings.m_digitSeparator, 
                settings.m_decimalSeparator, settings.m_groupingSize, settings.m_firstGroupingSize);
            EXPECT_STREQ(buffer, settings.m_output.c_str());
            EXPECT_EQ(settings.m_output.length(), endPos);
        }
    }

    TEST_F(StringFuncTest, CalculateBranchToken_ValidInput_Success)
    {
        AZStd::string       samplePath = "F:\\w s 1\\dev";
        AZStd::string       expectedToken = "0x68E564C5";
        AZStd::string       resultToken;

        AZ::StringFunc::AssetPath::CalculateBranchToken(samplePath, resultToken);

        ASSERT_TRUE(resultToken == expectedToken);
    }

    TEST_F(StringFuncTest, CalculateBranchToken_ValidInputExtra_Success)
    {
        AZStd::string       samplePath = "F:\\w s 1\\dev\\";
        AZStd::string       expectedToken = "0x68E564C5";
        AZStd::string       resultToken;

        AZ::StringFunc::AssetPath::CalculateBranchToken(samplePath, resultToken);

        ASSERT_TRUE(resultToken == expectedToken);
    }

    TEST_F(StringFuncTest, HasDrive_EmptyInput_NoDriveFound)
    {
        EXPECT_FALSE(AZ::StringFunc::Path::HasDrive(""));
    }

    TEST_F(StringFuncTest, HasDrive_InputContainsDrive_DriveFound)
    {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AZStd::string input;

        input = "F:\\test\\to\\get\\drive\\";

        EXPECT_TRUE(AZ::StringFunc::Path::HasDrive(input.c_str()));
#endif
    }

    TEST_F(StringFuncTest, HasDrive_InputDoesNotContainDrive_NoDriveFound)
    {
        AZStd::string input;

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        input = "test\\with\\no\\drive\\";
#else
        input = "test/with/no/drive/";
#endif

        EXPECT_FALSE(AZ::StringFunc::Path::HasDrive(input.c_str()));
    }

    TEST_F(StringFuncTest, HasDrive_CheckAllFileSystemFormats_InputContainsDrive_DriveFound)
    {
        AZStd::string input1 = "F:\\test\\to\\get\\drive\\";
        AZStd::string input2 = "/test/to/get/drive/";

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        EXPECT_TRUE(AZ::StringFunc::Path::HasDrive(input1.c_str(), true));
#endif
        EXPECT_TRUE(AZ::StringFunc::Path::HasDrive(input2.c_str(), true));
    }

    TEST_F(StringFuncTest, HasDrive_CheckAllFileSystemFormats_InputDoesNotContainDrive_NoDriveFound)
    {
        AZStd::string input1 = "test\\with\\no\\drive\\";
        AZStd::string input2 = "test/with/no/drive/";

        EXPECT_FALSE(AZ::StringFunc::Path::HasDrive(input1.c_str(), true));
        EXPECT_FALSE(AZ::StringFunc::Path::HasDrive(input2.c_str(), true));
    }

    TEST_F(StringFuncTest, GetDrive_UseSameStringForInOut_Success)
    {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AZStd::string input = "F:\\test\\to\\get\\drive\\";
        AZStd::string expectedDriveResult = "F:";

        bool result = AZ::StringFunc::Path::GetDrive(input.c_str(), input);

        ASSERT_TRUE(result);
        ASSERT_EQ(input, expectedDriveResult);
#endif
    }

    TEST_F(StringFuncTest, IsValid_LongPathComponent_Success)
    {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AZStd::string longFilename = "F:\\folder\\1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890.txt";
        AZStd::string longFolder = "F:\\1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\\filename.ext";
        bool result = AZ::StringFunc::Path::IsValid(longFilename.c_str(), true, true);
        EXPECT_TRUE(result);

        result = AZ::StringFunc::Path::IsValid(longFolder.c_str(), true, true);
        EXPECT_TRUE(result);
#else
        AZStd::string longFilename = "/folder/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890.txt";
        AZStd::string longFolder = "/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890/filename.ext";
        bool result = AZ::StringFunc::Path::IsValid(longFilename.c_str(), false, true);
        EXPECT_TRUE(result);

        result = AZ::StringFunc::Path::IsValid(longFolder.c_str(), false, true);
        EXPECT_TRUE(result);
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    }

    //! Strip

    TEST_F(StringFuncTest, Strip_InternalCharactersAll_Success)
    {
        AZStd::string input = "Hello World";
        AZStd::string expectedResult = "Heo Word";
        const char stripToken = 'l';

        AZ::StringFunc::Strip(input, stripToken);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharactersBeginning_NoChange)
    {
        AZStd::string input = "Hello World";
        AZStd::string expectedResult = "Hello World";
        const char stripToken = 'l';

        AZ::StringFunc::Strip(input, stripToken, false, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharactersEnd_NoChange)
    {
        AZStd::string input = "Hello World";
        AZStd::string expectedResult = "Hello World";
        const char stripToken = 'l';

        AZ::StringFunc::Strip(input, stripToken, false, false, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharacterBeginningEnd_NoChange)
    {
        AZStd::string input = "Hello World";
        AZStd::string expectedResult = "Hello World";
        const char stripToken = 'l';

        AZ::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharactersBeginningEndInsensitive_NoChange)
    {
        AZStd::string input = "HeLlo HeLlo HELlO";
        AZStd::string expectedResult = "HeLlo HeLlo HELlO";
        const char stripToken = 'l';

        AZ::StringFunc::Strip(input, stripToken, true, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharactersBeginningEndString_Success)
    {
        AZStd::string input = "HeLlo HeLlo HELlO";
        AZStd::string expectedResult = " HeLlo ";
        const char* stripToken = "hello";

        AZ::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_Beginning_Success)
    {
        AZStd::string input = "AbrAcadabra";
        AZStd::string expectedResult = "brAcadabra";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken, false, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_End_Success)
    {
        AZStd::string input = "AbrAcadabra";
        AZStd::string expectedResult = "AbrAcadabr";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken, false, false, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_BeginningEnd_Success)
    {
        AZStd::string input = "AbrAcadabra";
        AZStd::string expectedResult = "brAcadabr";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_BeginningEndCaseSensitive_EndOnly)
    {
        AZStd::string input = "AbrAcadabra";
        AZStd::string expectedResult = "AbrAcadabr";
        const char stripToken = 'a';

        AZ::StringFunc::Strip(input, stripToken, true, true, true);

        ASSERT_EQ(input, expectedResult);
    }


    using StringFuncPathTest = LeakDetectionFixture;

    TEST_F(StringFuncPathTest, GetParentDir_InvokedOnAbsoluteDirectory_ReturnsSameDirectory)
    {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        const char* input = R"str(C:\path\to\some\resource\)str";
        const char* expectedResult = R"str(C:\path\to\some)str";
#else
        const char* input = R"str(/path/to/some/resource/)str";
        const char* expectedResult = R"str(/path/to/some)str";
#endif
        AZStd::optional<AZStd::string> result = AZ::StringFunc::Path::GetParentDir(input);
        EXPECT_TRUE(result.has_value());
        EXPECT_STREQ(expectedResult, result->c_str());
    }

    TEST_F(StringFuncPathTest, GetParentDir_InvokedOnRelativeDirectory_ReturnsSameDirectory)
    {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        const char* input = R"str(some\resource\)str";
        const char* expectedResult = R"str(some)str";
#else
        const char* input = R"str(some/resource/)str";
        const char* expectedResult = R"str(some)str";
#endif
        AZStd::optional<AZStd::string> result = AZ::StringFunc::Path::GetParentDir(input);
        EXPECT_TRUE(result.has_value());
        EXPECT_STREQ(expectedResult, result->c_str());
    }
    TEST_F(StringFuncPathTest, GetParentDir_InvokedOnRelativePathWithoutTrailingSlash_ReturnsParentDirectory)
    {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        const char* input = R"str(some\resource.exe)str";
        const char* expectedResult = R"str(some)str";
#else
        const char* input = R"str(some/resource.exe)str";
        const char* expectedResult = R"str(some)str";
#endif
        AZStd::optional<AZStd::string> result = AZ::StringFunc::Path::GetParentDir(input);
        EXPECT_TRUE(result.has_value());
        EXPECT_STREQ(expectedResult, result->c_str());
    }

    TEST_F(StringFuncPathTest, GetParentDir_InvokedOnRoot_ReturnsRoot)
    {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        const char* input = R"str(C:\)str";
        const char* expectedResult = R"str(C:\)str";
#else
        const char* input = R"str(/)str";
        const char* expectedResult = R"str(/)str";
#endif
        AZStd::optional<AZStd::string> result = AZ::StringFunc::Path::GetParentDir(input);
        EXPECT_TRUE(result.has_value());
        EXPECT_STREQ(expectedResult, result->c_str());
    }

    TEST_F(StringFuncPathTest, GetParentDir_InvokedOnEmptyPath_ReturnFalse)
    {
        const char* input = "";
        AZStd::optional<AZStd::string> result = AZ::StringFunc::Path::GetParentDir(input);
        EXPECT_FALSE(result.has_value());
    }

    TEST_F(StringFuncPathTest, GetParentDir_InvokedOnPathWithoutParent_ReturnsFalse)
    {
        const char* input = "Test.exe";
        AZStd::optional<AZStd::string> result = AZ::StringFunc::Path::GetParentDir(input);
        EXPECT_FALSE(result.has_value());
    }

    TEST_F(StringFuncPathTest, GetParentDir_InvokedOnWindowsDriveWithoutTrailingSlash_ReturnsTrue)
    {
        const char* input = R"str(C:)str";
        AZStd::optional<AZStd::string> result = AZ::StringFunc::Path::GetParentDir(input);
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        EXPECT_TRUE(result.has_value());
#else
        EXPECT_FALSE(result.has_value());
#endif
    }

    TEST_F(StringFuncPathTest, ReplaceExtension_WithoutDot)
    {
        AZStd::string s = "D:\\p4\\some.file";
        AZ::StringFunc::Path::ReplaceExtension(s, "xml");
        EXPECT_STREQ("D:\\p4\\some.xml", s.c_str());
    }

    TEST_F(StringFuncPathTest, ReplaceExtension_WithDot)
    {
        AZStd::string s = "D:\\p4\\some.file";
        AZ::StringFunc::Path::ReplaceExtension(s, ".xml");
        EXPECT_STREQ("D:\\p4\\some.xml", s.c_str());
    }

    TEST_F(StringFuncPathTest, ReplaceExtension_Empty)
    {
        AZStd::string s = "D:\\p4\\some.file";
        AZ::StringFunc::Path::ReplaceExtension(s, "");
        EXPECT_STREQ("D:\\p4\\some", s.c_str());
    }

    TEST_F(StringFuncPathTest, ReplaceExtension_Null)
    {
        AZStd::string s = "D:\\p4\\some.file";
        AZ::StringFunc::Path::ReplaceExtension(s, nullptr);
        EXPECT_STREQ("D:\\p4\\some", s.c_str());
    }

    class TestPathStringArgs
    {
    public:
        TestPathStringArgs(const char* input, const char* expected_output)
            : m_input(input),
            m_expected(expected_output)
        {}

        const char* m_input;
        const char* m_expected;

    };

    class StringPathFuncTest
    : public StringFuncTest,
      public ::testing::WithParamInterface<TestPathStringArgs>
    {
    public:
        StringPathFuncTest() = default;
        ~StringPathFuncTest() override = default;
    };


    TEST_P(StringPathFuncTest, TestNormalizePath)
    {
        const TestPathStringArgs& param = GetParam();

        AZStd::string input = AZStd::string(param.m_input);
        AZStd::string expected = AZStd::string(param.m_expected);

        bool result = AZ::StringFunc::Path::Normalize(input);
        EXPECT_TRUE(result);
        EXPECT_STREQ(input.c_str(), expected.c_str());
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    INSTANTIATE_TEST_CASE_P(
        PathWithSingleDotSubFolders,
        StringPathFuncTest, 
        ::testing::Values(
            TestPathStringArgs("F:\\test\\to\\get\\.\\drive\\", "F:\\test\\to\\get\\drive\\")
        ));

    INSTANTIATE_TEST_CASE_P(
        PathWithDoubleDotSubFolders,
        StringPathFuncTest, 
        ::testing::Values(
            TestPathStringArgs("C:\\One\\Two\\..\\Three\\",                                          "C:\\One\\Three\\"),
            TestPathStringArgs("C:\\One\\..\\..\\Two\\",                                             "C:\\Two\\"),
            TestPathStringArgs("C:\\One\\Two\\Three\\..\\",                                          "C:\\One\\Two\\"),
            TestPathStringArgs("F:\\test\\to\\get\\..\\blue\\orchard\\..\\drive\\",                  "F:\\test\\to\\blue\\drive\\"),
            TestPathStringArgs("F:\\test\\to\\.\\.\\get\\..\\.\\.\\drive\\",                         "F:\\test\\to\\drive\\"),
            TestPathStringArgs("F:\\..\\test\\to\\.\\.\\get\\..\\.\\.\\drive\\",                     "F:\\test\\to\\drive\\"),
            TestPathStringArgs("F:\\..\\..\\..\\test\\to\\.\\.\\get\\..\\.\\.\\drive\\",             "F:\\test\\to\\drive\\"),
            TestPathStringArgs("F:\\..\\..\\..\\test\\to\\.\\.\\get\\..\\.\\.\\drive\\..\\..\\..\\", "F:\\"),
            TestPathStringArgs("F:\\..\\",                                                           "F:\\")
   ));
#else
    INSTANTIATE_TEST_CASE_P(
        PathWithSingleDotSubFolders,
        StringPathFuncTest, ::testing::Values(
            TestPathStringArgs("/test/to/get/./drive/", "/test/to/get/drive/")
        ));

    INSTANTIATE_TEST_CASE_P(
        PathWithDoubleDotSubFolders,
        StringPathFuncTest, 
        ::testing::Values(
            TestPathStringArgs("/one/two/../three/",                     "/one/three/"),
            TestPathStringArgs("/one/../../two/",                        "/two/"),
            TestPathStringArgs("/one/two/three/../",                     "/one/two/"),
            TestPathStringArgs("/test/to/get/../blue/orchard/../drive/", "/test/to/blue/drive/"),
            TestPathStringArgs("/test/to/././get./././.drive/",          "/test/to/get./.drive/"),
            TestPathStringArgs("/../../test/to/././get/../././drive/",   "/test/to/drive/")
    ));
#endif //AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    
}
