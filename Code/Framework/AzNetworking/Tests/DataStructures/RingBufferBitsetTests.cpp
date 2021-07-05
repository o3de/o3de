/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/DataStructures/RingBufferBitset.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    AzNetworking::RingbufferBitset<128> test;

    TEST(RingbufferBitset, Push2Bits)
    {
        test.PushBackBits(2);

        test.SetBit(0, true);
        test.SetBit(1, true);

        EXPECT_TRUE(test.GetBit(0));
        EXPECT_TRUE(test.GetBit(1));
    }

    TEST(RingbufferBitset, Push4Bits)
    {
        test.PushBackBits(2);

        EXPECT_FALSE(test.GetBit(0));
        EXPECT_FALSE(test.GetBit(1));
        EXPECT_TRUE (test.GetBit(2));
        EXPECT_TRUE (test.GetBit(3));
    }

    TEST(RingbufferBitset, Push44Bits)
    {
        test.PushBackBits(40);
        test.SetBit(41, true);

        EXPECT_FALSE(test.GetBit(40));
        EXPECT_TRUE (test.GetBit(41));
        EXPECT_TRUE (test.GetBit(42));
        EXPECT_TRUE (test.GetBit(43));
    }

    TEST(RingbufferBitset, Push84Bits)
    {
        test.PushBackBits(40);
        test.SetBit(80, true);

        EXPECT_TRUE(test.GetBit(80));
        EXPECT_TRUE(test.GetBit(81));
        EXPECT_TRUE(test.GetBit(82));
        EXPECT_TRUE(test.GetBit(83));
    }

    TEST(RingbufferBitset, Push124Bits)
    {
        test.PushBackBits(40);

        EXPECT_TRUE(test.GetBit(120));
        EXPECT_TRUE(test.GetBit(121));
        EXPECT_TRUE(test.GetBit(122));
        EXPECT_TRUE(test.GetBit(123));
    }

    TEST(RingbufferBitset, TestWrap)
    {
        // Bits should reset to false after wrapping and falling out of our window
        test.PushBackBits(40);

        EXPECT_FALSE(test.GetBit(160 - 128));
        EXPECT_FALSE(test.GetBit(161 - 128));
        EXPECT_FALSE(test.GetBit(162 - 128));
        EXPECT_FALSE(test.GetBit(163 - 128));
    }
}
