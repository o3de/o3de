/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            TEST(PatternMatcherTest, MatchesPattern_MatchingNameWithPostFix_ReturnsTrue)
            {
                PatternMatcher matcher("_postfix", PatternMatcher::MatchApproach::PostFix);
                EXPECT_TRUE(matcher.MatchesPattern("string_with_postfix"));
            }

            TEST(PatternMatcherTest, MatchesPattern_NonMatchingNameWithPostFix_ReturnsFalse)
            {
                PatternMatcher matcher("_postfix", PatternMatcher::MatchApproach::PostFix);
                EXPECT_FALSE(matcher.MatchesPattern("string_with_something_else"));
            }

            TEST(PatternMatcherTest, MatchesPattern_CaseInsensitiveMatchingNameWithPostFix_ReturnsTrue)
            {
                PatternMatcher matcher("_PoStFiX", PatternMatcher::MatchApproach::PostFix);
                EXPECT_TRUE(matcher.MatchesPattern("string_with_postfix"));
            }

            TEST(PatternMatcherTest, MatchesPattern_NonMatchingNameWithPostFixAndEarlyOutForSmallerTestThanPattern_ReturnsFalse)
            {
                PatternMatcher matcher("_postfix", PatternMatcher::MatchApproach::PostFix);
                EXPECT_FALSE(matcher.MatchesPattern("small"));
            }

            TEST(PatternMatcherTest, MatchesPattern_MatchingNameWithPreFix_ReturnsTrue)
            {
                PatternMatcher matcher("prefix_", PatternMatcher::MatchApproach::PreFix);
                EXPECT_TRUE(matcher.MatchesPattern("prefix_for_string"));
            }

            TEST(PatternMatcherTest, MatchesPattern_NonMatchingNameWithPreFix_ReturnsFalse)
            {
                PatternMatcher matcher("prefix_", PatternMatcher::MatchApproach::PreFix);
                EXPECT_FALSE(matcher.MatchesPattern("string_with_something_else"));
            }

            TEST(PatternMatcherTest, MatchesPattern_CaseInsensitiveMatchingNameWithPreFix_ReturnsTrue)
            {
                PatternMatcher matcher("PrEFiX_", PatternMatcher::MatchApproach::PreFix);
                EXPECT_TRUE(matcher.MatchesPattern("prefix_for_string"));
            }

            TEST(PatternMatcherTest, MatchesPattern_MatchingNameWithRegex_ReturnsTrue)
            {
                PatternMatcher matcher("^.{4}$", PatternMatcher::MatchApproach::Regex);
                EXPECT_TRUE(matcher.MatchesPattern("fits"));
            }

            TEST(PatternMatcherTest, MatchesPattern_NonMatchingNameWithRegex_ReturnsFalse)
            {
                PatternMatcher matcher("^.{4}$", PatternMatcher::MatchApproach::Regex);
                EXPECT_FALSE(matcher.MatchesPattern("string_to_long_for_regex"));
            }

            TEST(PatternMatcherTest, MatchesPattern_MatchingPrefixInArrayOfPatterns_ReturnsTrue)
            {
                constexpr auto patterns = AZStd::to_array<AZStd::string_view>({ "postfix", "xxx", "prefix_" });

                PatternMatcher matcher(patterns, PatternMatcher::MatchApproach::PreFix);
                EXPECT_TRUE(matcher.MatchesPattern("prefix_for_string"));
            }

            TEST(PatternMatcherTest, MatchesPattern_NonMatchingPrefixInArrayOfPatterns_ReturnsFalse)
            {
                constexpr auto patterns = AZStd::to_array<AZStd::string_view>({ "postfix", "xxx" });

                PatternMatcher matcher(patterns, PatternMatcher::MatchApproach::PreFix);
                EXPECT_FALSE(matcher.MatchesPattern("prefix_for_string"));
            }

        } // SceneCore
    } // SceneAPI
} // AZ
