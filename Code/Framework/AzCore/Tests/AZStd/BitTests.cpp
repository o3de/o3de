/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/bit.h>
#include <AzCore/std/containers/array.h>

namespace UnitTest
{
    class ByteswapFixture : public LeakDetectionFixture
    {
    };

    static_assert(AZStd::byteswap(AZ::u8{ 0xAB }) == AZ::u8{ 0xAB });
    static_assert(AZStd::byteswap(AZ::u16{ 0x1234 }) == AZ::u16{ 0x3412 });
    static_assert(AZStd::byteswap(AZ::u32{ 0x12345678 }) == AZ::u32{ 0x78563412 });
    static_assert(AZStd::byteswap(AZ::u64{ 0x0123456789ABCDEF }) == AZ::u64{ 0xEFCDAB8967452301 });
    static_assert(AZStd::byteswap(AZ::s16{ -2 }) == static_cast<AZ::s16>(0xFEFF));
    static_assert(AZStd::byteswap(AZ::s32{ -2 }) == static_cast<AZ::s32>(0xFEFFFFFF));
    static_assert(AZStd::byteswap(AZ::s64{ -1 }) == AZ::s64{ -1 });
    static_assert(AZStd::byteswap(true) == true);
    static_assert(AZStd::byteswap('a') == 'a');
    static_assert(AZStd::byteswap(char16_t{ 0x1234 }) == char16_t{ 0x3412 });

    //! Independent reference: reverses the object representation without touching AZStd::byteswap.
    template<class T>
    static T ReverseObjectRepresentation(T value)
    {
        auto bytes = AZStd::bit_cast<AZStd::array<AZ::u8, sizeof(T)>>(value);
        AZStd::reverse(bytes.begin(), bytes.end());
        return AZStd::bit_cast<T>(bytes);
    }

    TEST_F(ByteswapFixture, ReversesKnownValues)
    {
        EXPECT_EQ(AZStd::byteswap(AZ::u16{ 0x1234 }), AZ::u16{ 0x3412 });
        EXPECT_EQ(AZStd::byteswap(AZ::u32{ 0x12345678 }), AZ::u32{ 0x78563412 });
        EXPECT_EQ(AZStd::byteswap(AZ::u64{ 0x0123456789ABCDEF }), AZ::u64{ 0xEFCDAB8967452301 });
    }

    TEST_F(ByteswapFixture, LeavesSingleByteTypesUnchanged)
    {
        EXPECT_EQ(AZStd::byteswap(AZ::u8{ 0xAB }), AZ::u8{ 0xAB });
        EXPECT_EQ(AZStd::byteswap(AZ::s8{ -3 }), AZ::s8{ -3 });
        EXPECT_EQ(AZStd::byteswap('a'), 'a');
        EXPECT_EQ(AZStd::byteswap(true), true);
    }

    TEST_F(ByteswapFixture, ReversesSignedValues)
    {
        EXPECT_EQ(AZStd::byteswap(AZ::s16{ -2 }), static_cast<AZ::s16>(0xFEFF));
        EXPECT_EQ(AZStd::byteswap(AZ::s32{ -2 }), static_cast<AZ::s32>(0xFEFFFFFF));
        EXPECT_EQ(AZStd::byteswap(AZ::s64{ -1 }), AZ::s64{ -1 });
    }

    TEST_F(ByteswapFixture, MatchesReversedObjectRepresentation)
    {
        AZ::u64 bits = 1;
        for (int iteration = 0; iteration < 1000; ++iteration)
        {
            bits = bits * 6364136223846793005ULL + 1442695040888963407ULL;

            EXPECT_EQ(AZStd::byteswap(static_cast<AZ::u16>(bits)), ReverseObjectRepresentation(static_cast<AZ::u16>(bits)));
            EXPECT_EQ(AZStd::byteswap(static_cast<AZ::u32>(bits)), ReverseObjectRepresentation(static_cast<AZ::u32>(bits)));
            EXPECT_EQ(AZStd::byteswap(bits), ReverseObjectRepresentation(bits));
            EXPECT_EQ(AZStd::byteswap(static_cast<AZ::s16>(bits)), ReverseObjectRepresentation(static_cast<AZ::s16>(bits)));
            EXPECT_EQ(AZStd::byteswap(static_cast<AZ::s32>(bits)), ReverseObjectRepresentation(static_cast<AZ::s32>(bits)));
            EXPECT_EQ(AZStd::byteswap(static_cast<AZ::s64>(bits)), ReverseObjectRepresentation(static_cast<AZ::s64>(bits)));
        }
    }

    TEST_F(ByteswapFixture, IsItsOwnInverse)
    {
        AZ::u64 bits = 1;
        for (int iteration = 0; iteration < 1000; ++iteration)
        {
            bits = bits * 6364136223846793005ULL + 1442695040888963407ULL;

            EXPECT_EQ(AZStd::byteswap(AZStd::byteswap(static_cast<AZ::u16>(bits))), static_cast<AZ::u16>(bits));
            EXPECT_EQ(AZStd::byteswap(AZStd::byteswap(static_cast<AZ::u32>(bits))), static_cast<AZ::u32>(bits));
            EXPECT_EQ(AZStd::byteswap(AZStd::byteswap(bits)), bits);
        }
    }
} // namespace UnitTest
