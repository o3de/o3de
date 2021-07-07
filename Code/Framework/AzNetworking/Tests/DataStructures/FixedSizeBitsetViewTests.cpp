/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
}
