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
        } // SceneCore
    } // SceneAPI
} // AZ