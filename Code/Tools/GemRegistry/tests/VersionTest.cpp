/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <AzCore/std/containers/unordered_map.h>

#include <AzTest/AzTest.h>
#include <GemRegistry/Version.h>

using Gems::GemVersion;
using Gems::EngineVersion;

class VersionTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_errParseFailed            = "Failed to parse valid version string.";
        m_errParseInvalidSucceeded  = "Parsing invalid version string succeeded.";
        m_errParseInvalid           = "ParseFromString resulted in incorrect version.";
        m_errCompareIncorrect       = "Result of Compare is incorrect.";
        m_errToStringIncorrect      = "ToString result is incorrect.";
        m_errHasherIncorrect        = "Did not get back the same value from hash map.";
    }

    const char* m_errParseFailed;
    const char* m_errParseInvalidSucceeded;
    const char* m_errParseInvalid;
    const char* m_errCompareIncorrect;
    const char* m_errToStringIncorrect;
    const char* m_errHasherIncorrect;
};

TEST_F(VersionTest, InitializerListConstructor_ValidValues_ReturnSameValues)
{
    GemVersion v0 = { 1, 2, 3 };

    ASSERT_EQ(v0.m_parts[0], 1) << m_errParseInvalid;
    ASSERT_EQ(v0.m_parts[1], 2) << m_errParseInvalid;
    ASSERT_EQ(v0.m_parts[2], 3) << m_errParseInvalid;

    EngineVersion v1 = { 1, 2, 3, 4 };

    ASSERT_EQ(v1.m_parts[0], 1) << m_errParseInvalid;
    ASSERT_EQ(v1.m_parts[1], 2) << m_errParseInvalid;
    ASSERT_EQ(v1.m_parts[2], 3) << m_errParseInvalid;
    ASSERT_EQ(v1.m_parts[3], 4) << m_errParseInvalid;
}

TEST_F(VersionTest, ParseFromString_ValidString_ReturnSuccessOutcomeWithCorrectValues)
{
    auto version0Outcome = GemVersion::ParseFromString("1.2.3");
    ASSERT_TRUE(version0Outcome.IsSuccess()) << m_errParseFailed;

    GemVersion v0 = version0Outcome.GetValue();

    ASSERT_EQ(v0.GetMajor(), 1)                        << m_errParseInvalid;
    ASSERT_EQ(v0.GetMinor(), 2)                        << m_errParseInvalid;
    ASSERT_EQ(v0.GetPatch(), 3)                        << m_errParseInvalid;

    auto version1Outcome = EngineVersion::ParseFromString("1.2.3.4");
    ASSERT_TRUE(version1Outcome.IsSuccess()) << m_errParseFailed;

    EngineVersion v1 = version1Outcome.GetValue();

    ASSERT_EQ(v1.m_parts[0], 1) << m_errParseInvalid;
    ASSERT_EQ(v1.m_parts[1], 2) << m_errParseInvalid;
    ASSERT_EQ(v1.m_parts[2], 3) << m_errParseInvalid;
    ASSERT_EQ(v1.m_parts[3], 4) << m_errParseInvalid;
}

TEST_F(VersionTest, ParseFromString_EmptyString_ReturnFailureOutcome)
{
    ASSERT_FALSE(GemVersion::ParseFromString("").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(EngineVersion::ParseFromString("").IsSuccess()) << m_errParseInvalidSucceeded;
}

TEST_F(VersionTest, ParseFromString_InvalidPartSize_ReturnFailureOutcome)
{
    ASSERT_FALSE(GemVersion::ParseFromString("1.2").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(GemVersion::ParseFromString("1.2.3.4").IsSuccess()) << m_errParseInvalidSucceeded;

    ASSERT_FALSE(EngineVersion::ParseFromString("1.4.2.1.1").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(EngineVersion::ParseFromString("1.2.3").IsSuccess()) << m_errParseInvalidSucceeded;
}

TEST_F(VersionTest, ParseFromString_InvalidCharactersString_ReturnFailureOutcome)
{
    ASSERT_FALSE(GemVersion::ParseFromString("NotAVersion").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(GemVersion::ParseFromString("NotAVersion.2.3").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(GemVersion::ParseFromString("1.NotAVersion.3").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(GemVersion::ParseFromString("1.2.NotAVersion").IsSuccess()) << m_errParseInvalidSucceeded;

    ASSERT_FALSE(EngineVersion::ParseFromString("NotBVersion").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(EngineVersion::ParseFromString("NotBVersion.2.3").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(EngineVersion::ParseFromString("1.NotBVersion.3").IsSuccess()) << m_errParseInvalidSucceeded;
    ASSERT_FALSE(EngineVersion::ParseFromString("1.2.NotBVersion").IsSuccess()) << m_errParseInvalidSucceeded;
}

TEST_F(VersionTest, ParseFromString_InvalidSeparator_ReturnFailureOutcome)
{
    ASSERT_FALSE(GemVersion::ParseFromString("1,2,3").IsSuccess()) << m_errParseInvalidSucceeded;

    ASSERT_FALSE(EngineVersion::ParseFromString("1,2,3,4").IsSuccess()) << m_errParseInvalidSucceeded;
}

TEST_F(VersionTest, Compare_DifferentMajor_ReturnLesserThanZero)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 2, 0, 0 };

    ASSERT_LT(GemVersion::Compare(v1, v2), 0) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 2, 0, 0, 0 };

    ASSERT_LT(EngineVersion::Compare(v3, v4), 0) << m_errCompareIncorrect;
}

TEST_F(VersionTest, Compare_DifferentMinor_ReturnLesserThanZero)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 1, 1, 0 };

    ASSERT_LT(GemVersion::Compare(v1, v2), 0) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 1, 1, 0, 0 };

    ASSERT_LT(EngineVersion::Compare(v3, v4), 0) << m_errCompareIncorrect;
}

TEST_F(VersionTest, Compare_DifferentMajorAndMinor_ReturnGreaterThanZero)
{
    GemVersion v1 = { 2, 0, 0 };
    GemVersion v2 = { 1, 1, 0 };

    ASSERT_GT(GemVersion::Compare(v1, v2), 0) << m_errCompareIncorrect;

    EngineVersion v3 = { 2, 0, 0, 0 };
    EngineVersion v4 = { 1, 1, 0, 0 };

    ASSERT_GT(EngineVersion::Compare(v3, v4), 0) << m_errCompareIncorrect;
}

TEST_F(VersionTest, Compare_SameValue_ReturnZero)
{
    GemVersion v1 = { 1, 1, 0 };
    GemVersion v2 = { 1, 1, 0 };

    ASSERT_EQ(GemVersion::Compare(v1, v2), 0) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 1, 0, 0 };
    EngineVersion v4 = { 1, 1, 0, 0 };

    ASSERT_EQ(EngineVersion::Compare(v3, v4), 0) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareLesserThan_DifferentMajor_ReturnTrue)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 2, 0, 0 };

    ASSERT_TRUE(v1 < v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 2, 0, 0, 0 };

    ASSERT_TRUE(v3 < v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareLesserThan_SameMajor_ReturnTrue)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 1, 1, 0 };

    ASSERT_TRUE(v1 < v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 1, 1, 0, 0 };

    ASSERT_TRUE(v3 < v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareLesserEqualsTo_DifferentMajor_ReturnTrue)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 2, 0, 0 };

    ASSERT_TRUE(v1 <= v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 2, 0, 0, 0 };

    ASSERT_TRUE(v3 <= v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareLesserEqualsTo_SameMajorSmallerMinor_ReturnTrue)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 1, 1, 0 };

    ASSERT_TRUE(v1 <= v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 1, 1, 0, 0 };

    ASSERT_TRUE(v3 <= v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareLesserEqualsTo_Samevalues_ReturnTrue)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 1, 0, 0 };

    ASSERT_TRUE(v1 <= v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 1, 0, 0, 0 };

    ASSERT_TRUE(v3 <= v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareGreaterThan_DifferentMajor_ReturnTrue)
{
    GemVersion v1 = { 2, 0, 0 };
    GemVersion v2 = { 1, 0, 0 };

    ASSERT_TRUE(v1 > v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 2, 0, 0, 0 };
    EngineVersion v4 = { 1, 0, 0, 0 };

    ASSERT_TRUE(v3 > v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareGreaterThan_SameMajor_ReturnTrue)
{
    GemVersion v1 = { 1, 1, 0 };
    GemVersion v2 = { 1, 0, 0 };

    ASSERT_TRUE(v1 > v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 1, 0, 0 };
    EngineVersion v4 = { 1, 0, 0, 0 };

    ASSERT_TRUE(v3 > v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareGreaterEqualsTo_DifferentMajor_ReturnTrue)
{
    GemVersion v1 = { 2, 0, 0 };
    GemVersion v2 = { 1, 0, 0 };

    ASSERT_TRUE(v1 >= v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 2, 0, 0, 0 };
    EngineVersion v4 = { 1, 0, 0, 0 };

    ASSERT_TRUE(v3 >= v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareGreaterEqualsTo_SameMajorSmallerMinor_ReturnTrue)
{
    GemVersion v1 = { 1, 1, 0 };
    GemVersion v2 = { 1, 0, 0 };

    ASSERT_TRUE(v1 >= v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 1, 0, 0 };
    EngineVersion v4 = { 1, 0, 0, 0 };

    ASSERT_TRUE(v3 >= v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareGreaterEqualsTo_Samevalues_ReturnTrue)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 1, 0, 0 };

    ASSERT_TRUE(v1 >= v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 1, 0, 0, 0 };

    ASSERT_TRUE(v3 >= v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareEqualsTo_SameValue_ReturnTrue)
{
    GemVersion v1 = { 1, 1, 0 };
    GemVersion v2 = { 1, 1, 0 };

    ASSERT_TRUE(v1 == v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 1, 0, 0 };
    EngineVersion v4 = { 1, 1, 0, 0 };

    ASSERT_TRUE(v3 == v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareEqualsTo_DifferentValue_ReturnFalse)
{
    GemVersion v1 = { 1, 1, 0 };
    GemVersion v2 = { 0, 1, 0 };

    ASSERT_FALSE(v1 == v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 1, 0, 0 };
    EngineVersion v4 = { 0, 1, 0, 0 };

    ASSERT_FALSE(v3 == v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareNotEqualsTo_DifferentValues_ReturnTrue)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 2, 0, 0 };

    ASSERT_TRUE(v1 != v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 1, 0, 0, 0 };
    EngineVersion v4 = { 2, 0, 0, 0 };

    ASSERT_TRUE(v3 != v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, CompareNotEqualsTo_SameValues_ReturnFalse)
{
    GemVersion v1 = { 2, 0, 1 };
    GemVersion v2 = { 2, 0, 1 };

    ASSERT_FALSE(v1 != v2) << m_errCompareIncorrect;

    EngineVersion v3 = { 2, 0, 1, 0 };
    EngineVersion v4 = { 2, 0, 1, 0 };

    ASSERT_FALSE(v3 != v4) << m_errCompareIncorrect;
}

TEST_F(VersionTest, ToString_VariousValues_ReturnsCorrectStringOutput)
{
    GemVersion v1 = { 1, 0, 0 };
    GemVersion v2 = { 2, 0, 0 };
    GemVersion v3 = { 1, 1, 0 };
    GemVersion v4 = { 1, 1, 0 };

    ASSERT_STREQ(v1.ToString().c_str(), "1.0.0")        << m_errToStringIncorrect;
    ASSERT_STREQ(v2.ToString().c_str(), "2.0.0")        << m_errToStringIncorrect;
    ASSERT_STREQ(v3.ToString().c_str(), "1.1.0")        << m_errToStringIncorrect;
    ASSERT_STREQ(v4.ToString().c_str(), "1.1.0")        << m_errToStringIncorrect;


    EngineVersion v5 = { 1, 0, 0, 0 };
    EngineVersion v6 = { 2, 0, 0, 0 };
    EngineVersion v7 = { 1, 1, 0, 0 };
    EngineVersion v8 = { 1, 1, 0, 0 };

    ASSERT_STREQ(v5.ToString().c_str(), "1.0.0.0") << m_errToStringIncorrect;
    ASSERT_STREQ(v6.ToString().c_str(), "2.0.0.0") << m_errToStringIncorrect;
    ASSERT_STREQ(v7.ToString().c_str(), "1.1.0.0") << m_errToStringIncorrect;
    ASSERT_STREQ(v8.ToString().c_str(), "1.1.0.0") << m_errToStringIncorrect;
}

TEST_F(VersionTest, ToString_ToStringFromParseStringValue_ReturnSameStringFromInput)
{
    const char* version0String = "1.2.3";

    auto version0Outcome = GemVersion::ParseFromString(version0String);
    GemVersion v0 = version0Outcome.GetValue();

    ASSERT_TRUE(version0Outcome.IsSuccess()) << m_errParseFailed;
    ASSERT_STREQ(v0.ToString().c_str(), version0String) << m_errToStringIncorrect;


    const char* version1String = "1.2.3.4";

    auto version1Outcome = EngineVersion::ParseFromString(version1String);
    EngineVersion v1 = version1Outcome.GetValue();

    ASSERT_TRUE(version1Outcome.IsSuccess()) << m_errParseFailed;
    ASSERT_STREQ(v1.ToString().c_str(), version1String) << m_errToStringIncorrect;
}

TEST_F(VersionTest, Hasher_DifferentValues_GetBackSameValue)
{
    {
        AZStd::unordered_map<GemVersion, int> unorderedMap;
        GemVersion v1 = { 1, 0, 0 };
        int v1value = 1;
        GemVersion v2 = { 2, 0, 0 };
        int v2value = 2;
        GemVersion v3 = { 1, 1, 0 };
        int v3value = 3;
        GemVersion v4 = { 1, 1, 1 };
        int v4value = 4;

        unorderedMap[v1] = v1value;
        unorderedMap[v2] = v2value;
        unorderedMap[v3] = v3value;
        unorderedMap[v4] = v4value;

        ASSERT_EQ(unorderedMap[v1], v1value) << m_errHasherIncorrect;
        ASSERT_EQ(unorderedMap[v2], v2value) << m_errHasherIncorrect;
        ASSERT_EQ(unorderedMap[v3], v3value) << m_errHasherIncorrect;
        ASSERT_EQ(unorderedMap[v4], v4value) << m_errHasherIncorrect;
    }

    {
        AZStd::unordered_map<EngineVersion, int> unorderedMap;
        EngineVersion v1 = { 1, 0, 0, 0 };
        int v1value = 1;
        EngineVersion v2 = { 2, 0, 0, 0 };
        int v2value = 2;
        EngineVersion v3 = { 1, 1, 0, 0 };
        int v3value = 3;
        EngineVersion v4 = { 1, 1, 1, 0 };
        int v4value = 4;

        unorderedMap[v1] = v1value;
        unorderedMap[v2] = v2value;
        unorderedMap[v3] = v3value;
        unorderedMap[v4] = v4value;

        ASSERT_EQ(unorderedMap[v1], v1value) << m_errHasherIncorrect;
        ASSERT_EQ(unorderedMap[v2], v2value) << m_errHasherIncorrect;
        ASSERT_EQ(unorderedMap[v3], v3value) << m_errHasherIncorrect;
        ASSERT_EQ(unorderedMap[v4], v4value) << m_errHasherIncorrect;
    }
}

TEST_F(VersionTest, Hasher_SameValues_GetBackSameValue)
{
    {
        AZStd::unordered_map<GemVersion, int> unorderedMap;
        GemVersion v1 = { 1, 1, 1 };
        GemVersion v2 = { 1, 1, 1 };
        int v12value = 1;

        unorderedMap[v1] = v12value;

        ASSERT_EQ(unorderedMap.at(v1), v12value) << m_errHasherIncorrect;
        ASSERT_EQ(unorderedMap.at(v2), v12value) << m_errHasherIncorrect;
    }

    {
        AZStd::unordered_map<EngineVersion, int> unorderedMap;
        EngineVersion v1 = { 1, 1, 1, 0 };
        EngineVersion v2 = { 1, 1, 1, 0 };
        int v12value = 1;

        unorderedMap[v1] = v12value;

        ASSERT_EQ(unorderedMap.at(v1), v12value) << m_errHasherIncorrect;
        ASSERT_EQ(unorderedMap.at(v2), v12value) << m_errHasherIncorrect;
    }
}