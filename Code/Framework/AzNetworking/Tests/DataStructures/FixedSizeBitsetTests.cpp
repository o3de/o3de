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

#include <AzNetworking/DataStructures/FixedSizeBitset.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    TEST(FixedSizeBitset, TestSetBit)
    {
        AzNetworking::FixedSizeBitset<128> test;
        test.SetBit(0, true);
        test.SetBit(2, true);

        // 0000 0101
        EXPECT_EQ(test.GetContainer()[0], 0x05);
    }

    TEST(FixedSizeBitset, TestAppend)
    {
        AzNetworking::FixedSizeBitset<63> appendTest1;
        appendTest1.SetBit(1, true);
        AzNetworking::FixedSizeBitset<63> appendTest2;
        appendTest2.SetBit(2, true);
        appendTest2.SetBit(32, true);

        appendTest1 |= appendTest2;

        EXPECT_EQ(appendTest1.GetBit(1), true);
        EXPECT_EQ(appendTest1.GetBit(2), true);
        EXPECT_EQ(appendTest1.GetBit(32), true);
    }

    TEST(FixedSizeBitset, TestAllSet)
    {
        AzNetworking::FixedSizeBitset<63> unusedBitTest(true);
        bool allSet = true;
        for (uint32_t i = 0; i < 63; ++i)
        {
            allSet &= unusedBitTest.GetBit(i);
        }
        EXPECT_EQ(allSet, true);
    }

    TEST(FixedSizeBitset, TestAnySet)
    {
        AzNetworking::FixedSizeBitset<9> unusedBitTest(true);
        for (uint32_t i = 0; i < 9; ++i)
        {
            unusedBitTest.SetBit(i, false);
        }
        EXPECT_FALSE(unusedBitTest.AnySet());
    }
}
