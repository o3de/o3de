/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/MathIntrinsics.h>

namespace UnitTest
{
    TEST(MathIntrinsics, CountLeadingZeros)
    {
        // Split up the binary literal in by every 4 bits
        {
            constexpr uint32_t leadingZero32 = 0b1000'0000'0000'0010'0000'0000'0000'0010;
            EXPECT_EQ(0, az_clz_u32(leadingZero32));
            EXPECT_EQ(32, az_clz_u64(leadingZero32));
        }

        {
            constexpr uint64_t leadingZero64 = 0b0000'0100'1111'1011'0001'1000'0010'0110'1000'0000'0000'0010'0000'0000'0000'0010ULL;
            EXPECT_EQ(5, az_clz_u64(leadingZero64));
        }

#if defined(AZ_COMPILER_MSVC)
        {
            // All Zero case
            // When using the clang compiler, this macro calls __builtin_clz,
            // whose return value is undefined when the input is 0
            constexpr uint32_t leadingZero32 = 0b0000'0000'0000'0000'0000'0000'0000'0000;
            EXPECT_EQ(32, az_clz_u32(leadingZero32));
            EXPECT_EQ(64, az_clz_u64(leadingZero32));
        }
#endif
        {
            // All One case
            constexpr uint32_t leadingZero32 = 0b1111'1111'1111'1111'1111'1111'1111'1111;
            EXPECT_EQ(0, az_clz_u32(leadingZero32));
            EXPECT_EQ(32, az_clz_u64(leadingZero32));
        }
    }

    TEST(MathIntrinsics, CountTrailingZeros)
    {
        // Split up the binary literal in by every 4 bits
        {
            constexpr uint32_t trailingZero32 = 0b1000'0000'0000'0010'0000'0000'0010'0000;
            EXPECT_EQ(5, az_ctz_u32(trailingZero32));
            EXPECT_EQ(5, az_ctz_u64(trailingZero32));
        }

        {
            constexpr uint64_t trailingZero64 = 0b0000'0100'1111'1011'0001'1000'0010'0110'1000'0000'0000'0010'0000'0000'0000'0010ULL;
            EXPECT_EQ(1, az_ctz_u64(trailingZero64));
        }

#if defined(AZ_COMPILER_MSVC)
        {
            // All Zero case
            // When using the clang compiler, this macro calls __builtin_ctz,
            // whose return value is undefined when the input is 0
            constexpr uint32_t trailingZero32 = 0b0000'0000'0000'0000'0000'0000'0000'0000;
            EXPECT_EQ(32, az_ctz_u32(trailingZero32));
            EXPECT_EQ(64, az_ctz_u64(trailingZero32));
        }
#endif
        {
            // All One case
            constexpr uint32_t trailingZero32 = 0b1111'1111'1111'1111'1111'1111'1111'1111;
            EXPECT_EQ(0, az_ctz_u32(trailingZero32));
            EXPECT_EQ(0, az_ctz_u64(trailingZero32));
        }
    }

    TEST(MathIntrinsics, CountOneBits)
    {
        // Split up the binary literal in by every 4 bits
        {
            constexpr uint32_t oneBits32 = 0b1010'1010'1011'1010'0010'0000'1010'0001;
            EXPECT_EQ(13, az_popcnt_u32(oneBits32));
            EXPECT_EQ(13, az_popcnt_u64(oneBits32));
        }

        {
            constexpr uint64_t oneBits64 = 0b0000'0100'1111'1011'0001'1000'0010'0110'1000'0000'1111'0010'0001'1000'0000'0010ULL;
            EXPECT_EQ(22, az_popcnt_u64(oneBits64));
        }

        {
            // All Zero case
            constexpr uint32_t oneBits32 = 0b0000'0000'0000'0000'0000'0000'0000'0000;
            EXPECT_EQ(0, az_popcnt_u32(oneBits32));
            EXPECT_EQ(0, az_popcnt_u64(oneBits32));
        }
        {
            // All One case
            constexpr uint32_t oneBits32 = 0b1111'1111'1111'1111'1111'1111'1111'1111;
            EXPECT_EQ(32, az_popcnt_u32(oneBits32));
            EXPECT_EQ(32, az_popcnt_u64(oneBits32));
        }
        {
            // Bitwise-not cases(~0)
            EXPECT_EQ(32, az_popcnt_u32(uint32_t(~0LL)));
            EXPECT_EQ(31, az_popcnt_u32(uint32_t(~1LL)));

            EXPECT_EQ(64, az_popcnt_u64(uint64_t(~0LL)));
            EXPECT_EQ(63, az_popcnt_u64(uint64_t(~1LL)));
        }
    }
}
