/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/utility/charconv.h>


namespace UnitTest
{
    class CharconvTestFixture
        : public UnitTest::LeakDetectionFixture
    {};

    TEST_F(CharconvTestFixture, ToChars_CanConvertIntegralValue_Success)
    {
        constexpr int8_t int8Value = -2;
        constexpr uint8_t uint8Value = 2;
        constexpr int16_t int16Value = -256 - 2;
        constexpr uint16_t uint16Value = 256 + 2;
        constexpr int32_t int32Value = -65536 - 2;
        constexpr uint32_t uint32Value = 65536 + 2;
        constexpr int64_t int64Value = -4294967296 - 2;
        constexpr uint64_t uint64Value = 4294967296 + 2;

        {
            char convertedInt[32];
            AZStd::to_chars_result result = AZStd::to_chars(AZStd::ranges::begin(convertedInt),
                AZStd::ranges::end(convertedInt), int8Value);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            // Null-terminate the buffer
            // Assert that the result ptr not outside of the convertedInt array range
            ASSERT_LT(result.ptr - convertedInt, AZStd::ranges::size(convertedInt));
            *result.ptr = '\0';
            EXPECT_STREQ("-2", convertedInt);
        }
        {
            char convertedInt[32];
            AZStd::to_chars_result result = AZStd::to_chars(AZStd::ranges::begin(convertedInt),
                AZStd::ranges::end(convertedInt), uint8Value);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            // Null-terminate the buffer
            // Assert that the result ptr not outside of the convertedInt array range
            ASSERT_LT(result.ptr - convertedInt, AZStd::ranges::size(convertedInt));
            *result.ptr = '\0';
            EXPECT_STREQ("2", convertedInt);
        }
        {
            char convertedInt[32];
            AZStd::to_chars_result result = AZStd::to_chars(AZStd::ranges::begin(convertedInt),
                AZStd::ranges::end(convertedInt), int16Value);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            // Null-terminate the buffer
            // Assert that the result ptr not outside of the convertedInt array range
            ASSERT_LT(result.ptr - convertedInt, AZStd::ranges::size(convertedInt));
            *result.ptr = '\0';
            EXPECT_STREQ("-258", convertedInt);
        }
        {
            char convertedInt[32];
            AZStd::to_chars_result result = AZStd::to_chars(AZStd::ranges::begin(convertedInt),
                AZStd::ranges::end(convertedInt), uint16Value);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            // Null-terminate the buffer
            // Assert that the result ptr not outside of the convertedInt array range
            ASSERT_LT(result.ptr - convertedInt, AZStd::ranges::size(convertedInt));
            *result.ptr = '\0';
            EXPECT_STREQ("258", convertedInt);
        }
        {
            char convertedInt[32];
            AZStd::to_chars_result result = AZStd::to_chars(AZStd::ranges::begin(convertedInt),
                AZStd::ranges::end(convertedInt), int32Value);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            // Null-terminate the buffer
            // Assert that the result ptr not outside of the convertedInt array range
            ASSERT_LT(result.ptr - convertedInt, AZStd::ranges::size(convertedInt));
            *result.ptr = '\0';
            EXPECT_STREQ("-65538", convertedInt);
        }
        {
            char convertedInt[32];
            AZStd::to_chars_result result = AZStd::to_chars(AZStd::ranges::begin(convertedInt),
                AZStd::ranges::end(convertedInt), uint32Value);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            // Null-terminate the buffer
            // Assert that the result ptr not outside of the convertedInt array range
            ASSERT_LT(result.ptr - convertedInt, AZStd::ranges::size(convertedInt));
            *result.ptr = '\0';
            EXPECT_STREQ("65538", convertedInt);
        }
        {
            char convertedInt[32];
            AZStd::to_chars_result result = AZStd::to_chars(AZStd::ranges::begin(convertedInt),
                AZStd::ranges::end(convertedInt), int64Value);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            // Null-terminate the buffer
            // Assert that the result ptr not outside of the convertedInt array range
            ASSERT_LT(result.ptr - convertedInt, AZStd::ranges::size(convertedInt));
            *result.ptr = '\0';
            EXPECT_STREQ("-4294967298", convertedInt);
        }
        {
            char convertedInt[32];
            AZStd::to_chars_result result = AZStd::to_chars(AZStd::ranges::begin(convertedInt),
                AZStd::ranges::end(convertedInt), uint64Value);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            // Null-terminate the buffer
            // Assert that the result ptr not outside of the convertedInt array range
            ASSERT_LT(result.ptr - convertedInt, AZStd::ranges::size(convertedInt));
            *result.ptr = '\0';
            EXPECT_STREQ("4294967298", convertedInt);
        }
    }

    TEST_F(CharconvTestFixture, FromChars_CanConvertString_Success)
    {
        {
            int8_t intValue{};
            AZStd::string_view numberString = "-2";
            AZStd::from_chars_result result = AZStd::from_chars(numberString.data(),
                numberString.data() + numberString.size(), intValue);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            EXPECT_EQ(numberString.end(), result.ptr);
            EXPECT_EQ(-2, intValue);
        }
        {
            uint8_t intValue{};
            AZStd::string_view numberString = "2";
            AZStd::from_chars_result result = AZStd::from_chars(numberString.data(),
                numberString.data() + numberString.size(), intValue);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            EXPECT_EQ(numberString.end(), result.ptr);
            EXPECT_EQ(2, intValue);
        }
        {
            int16_t intValue{};
            AZStd::string_view numberString = "-258";
            AZStd::from_chars_result result = AZStd::from_chars(numberString.data(),
                numberString.data() + numberString.size(), intValue);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            EXPECT_EQ(numberString.end(), result.ptr);
            EXPECT_EQ(-258, intValue);
        }
        {
            uint16_t intValue{};
            AZStd::string_view numberString = "258";
            AZStd::from_chars_result result = AZStd::from_chars(numberString.data(),
                numberString.data() + numberString.size(), intValue);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            EXPECT_EQ(numberString.end(), result.ptr);
            EXPECT_EQ(258, intValue);
        }
        {
            int32_t intValue{};
            AZStd::string_view numberString = "-65538";
            AZStd::from_chars_result result = AZStd::from_chars(numberString.data(),
                numberString.data() + numberString.size(), intValue);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            EXPECT_EQ(numberString.end(), result.ptr);
            EXPECT_EQ(-65538, intValue);
        }
        {
            uint32_t intValue{};
            AZStd::string_view numberString = "65538";
            AZStd::from_chars_result result = AZStd::from_chars(numberString.data(),
                numberString.data() + numberString.size(), intValue);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            EXPECT_EQ(numberString.end(), result.ptr);
            EXPECT_EQ(65538, intValue);
        }
        {
            int64_t intValue{};
            AZStd::string_view numberString = "-4294967298";
            AZStd::from_chars_result result = AZStd::from_chars(numberString.data(),
                numberString.data() + numberString.size(), intValue);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            EXPECT_EQ(numberString.end(), result.ptr);
            EXPECT_EQ(-4294967298, intValue);
        }
        {
            uint64_t intValue{};
            AZStd::string_view numberString = "4294967298";
            AZStd::from_chars_result result = AZStd::from_chars(numberString.data(),
                numberString.data() + numberString.size(), intValue);

            EXPECT_EQ(AZStd::errc{}, result.ec);
            EXPECT_EQ(numberString.end(), result.ptr);
            EXPECT_EQ(4294967298, intValue);
        }
    }
}
