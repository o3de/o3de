/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/DataStructures/FixedSizeBitsetView.h>
#include <AzNetworking/DataStructures/FixedSizeBitset.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    TEST(FixedSizeBitsetView, BasicTests)
    {
        AzNetworking::FixedSizeBitset<100> bitset;

        {
            AzNetworking::FixedSizeBitsetView view(bitset, 10, 1);
            EXPECT_FALSE(bitset.GetBit(10));
            EXPECT_FALSE(view.GetBit(0));

            view.SetBit(0, true);
            EXPECT_TRUE(bitset.GetBit(10));
            EXPECT_TRUE(view.GetBit(0));

            bitset.SetBit(10, false);
            EXPECT_FALSE(view.GetBit(0));
        }

        {
            AzNetworking::FixedSizeBitsetView view(bitset, 20, 1);
            EXPECT_FALSE(view.GetBit(0));
            EXPECT_FALSE(bitset.GetBit(20));

            view.SetBit(0, true);
            EXPECT_TRUE(bitset.GetBit(20));
            EXPECT_TRUE(view.GetBit(0));

            bitset.SetBit(20, false);
            EXPECT_FALSE(view.GetBit(0));
        }
    }

    TEST(FixedSizeBitsetView, EmptyBitset)
    {
        AzNetworking::FixedSizeBitset<32> bitset;
        AzNetworking::FixedSizeBitsetView view(bitset, 10, 0);
        EXPECT_FALSE(view.GetBit(0));
    }

    TEST(FixedSizeBitsetView, TestAnySet)
    {
        const uint32_t VIEW_SIZE = 5;

        AzNetworking::FixedSizeBitset<9> unusedBitTest(true);
        AzNetworking::FixedSizeBitsetView view(unusedBitTest, 0, VIEW_SIZE);
        for (uint32_t i = 0; i < VIEW_SIZE; ++i)
        {
            view.SetBit(i, false);
        }
        EXPECT_FALSE(view.AnySet());

        view.SetBit(0, true);
        EXPECT_TRUE(view.AnySet());

        EXPECT_EQ(view.GetValidBitCount(), VIEW_SIZE);
    }
}
