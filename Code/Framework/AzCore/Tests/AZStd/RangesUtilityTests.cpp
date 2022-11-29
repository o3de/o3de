/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/ranges/ranges_to.h>

namespace UnitTest
{
    class RangesUtilityTestFixture
        : public LeakDetectionFixture
    {};


    TEST_F(RangesUtilityTestFixture, RangesTo_ConvertsContainer_To_Vector_AndBack_Succeeds)
    {
        AZStd::string testString{ "Hello World" };
        {
            // Test convert from AZStd::string -> AZStd::vector<char>
            auto convertedVector = AZStd::ranges::to<AZStd::vector<char>>(testString);
            EXPECT_THAT(convertedVector, ::testing::Pointwise(::testing::Eq(), testString));

            // Test back from convert from AZStd::vector<char> -> AZStd::string
            auto convertedString = AZStd::ranges::to<AZStd::string>(convertedVector);
            EXPECT_EQ(testString, convertedString);
        }
    }

    TEST_F(RangesUtilityTestFixture, RangesTo_TemplateTemplateOverload_Conversion_Succeeds)
    {
        AZStd::string testString{ "Hello World" };
        {
            // Test convert from AZStd::string -> AZStd::vector<char>
            auto convertedVector = AZStd::ranges::to<AZStd::vector>(testString);
            EXPECT_THAT(convertedVector, ::testing::Pointwise(::testing::Eq(), testString));

            // Test back from convert from AZStd::vector<char> -> AZStd::string
            auto convertedString = AZStd::ranges::to<AZStd::basic_string>(convertedVector);
            EXPECT_EQ(testString, convertedString);
        }
    }

    TEST_F(RangesUtilityTestFixture, RangesTo_ToAdaptor_Succeeds)
    {
        AZStd::string testString{ "Hello World" };
        // Test adaptor container class overload
        auto convertedClassVector = testString | AZStd::ranges::to<AZStd::vector<char>>();
        EXPECT_THAT(convertedClassVector, ::testing::Pointwise(::testing::Eq(), testString));

        // Test adaptor container template template overload
        auto convertedTemplateVector = testString | AZStd::ranges::to<AZStd::vector>();
        EXPECT_THAT(convertedTemplateVector, ::testing::Pointwise(::testing::Eq(), testString));

        // Test adaptor container class overload with allocator argument
        convertedClassVector = testString | AZStd::ranges::to<AZStd::vector<char>>(AZStd::allocator{});
        EXPECT_THAT(convertedClassVector, ::testing::Pointwise(::testing::Eq(), testString));

        // Test adaptor container template template overload with allocator argument
        convertedTemplateVector = testString | AZStd::ranges::to<AZStd::vector>(AZStd::allocator{});
        EXPECT_THAT(convertedTemplateVector, ::testing::Pointwise(::testing::Eq(), testString));
    }
}
