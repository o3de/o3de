/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/RecentlyUsedIndex.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace AZ::IO
{
    class Streamer_RecentlyUsedIndexTest : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        using RUI = RecentlyUsedIndex<u8>;

        template<size_t Size>
        static void Validate(const RUI& rui, const u8 (&expectedValues)[Size])
        {
            u8 counter = 0;
            auto callback = [&counter, &expectedValues](u8 value)
            {
                EXPECT_EQ(expectedValues[counter], value);
                counter++;
            };
            rui.GetIndicesInOrder(callback);
        }
    };

    TEST_F(Streamer_RecentlyUsedIndexTest, Constructor)
    {
        RUI rui{ 4 };
        Validate(rui, { 0, 1, 2, 3 });
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, GetLeastRecentlyUsed)
    {
        RUI rui{ 4 };
        EXPECT_EQ(0, rui.GetLeastRecentlyUsed());
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, GetMostRecentlyUsed)
    {
        RUI rui{ 4 };
        EXPECT_EQ(3, rui.GetMostRecentlyUsed());
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, Touch_Middle)
    {
        RUI rui{ 4 };
        rui.Touch(2);
        Validate(rui, { 0, 1, 3, 2 });
        EXPECT_EQ(rui.GetLeastRecentlyUsed(), 0);
        EXPECT_EQ(rui.GetMostRecentlyUsed(), 2);
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, Touch_Front)
    {
        RUI rui{ 4 };
        rui.Touch(0);
        Validate(rui, { 1, 2, 3, 0 });
        EXPECT_EQ(rui.GetLeastRecentlyUsed(), 1);
        EXPECT_EQ(rui.GetMostRecentlyUsed(), 0);
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, Touch_Back)
    {
        RUI rui{ 4 };
        rui.Touch(3);
        Validate(rui, { 0, 1, 2, 3 });
        EXPECT_EQ(rui.GetLeastRecentlyUsed(), 0);
        EXPECT_EQ(rui.GetMostRecentlyUsed(), 3);
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, Flush_Middle)
    {
        RUI rui{ 4 };
        rui.Flush(2);
        Validate(rui, { 2, 0, 1, 3 });
        EXPECT_EQ(rui.GetLeastRecentlyUsed(), 2);
        EXPECT_EQ(rui.GetMostRecentlyUsed(), 3);
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, Flush_Front)
    {
        RUI rui{ 4 };
        rui.Flush(0);
        Validate(rui, { 0, 1, 2, 3 });
        EXPECT_EQ(rui.GetLeastRecentlyUsed(), 0);
        EXPECT_EQ(rui.GetMostRecentlyUsed(), 3);
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, Flush_Back)
    {
        RUI rui{ 4 };
        rui.Flush(3);
        Validate(rui, { 3, 0, 1, 2 });
        EXPECT_EQ(rui.GetLeastRecentlyUsed(), 3);
        EXPECT_EQ(rui.GetMostRecentlyUsed(), 2);
    }

    TEST_F(Streamer_RecentlyUsedIndexTest, TouchLeastRecentlyUsed)
    {
        RUI rui{ 4 };
        rui.TouchLeastRecentlyUsed();
        Validate(rui, { 1, 2, 3, 0 });
        EXPECT_EQ(rui.GetLeastRecentlyUsed(), 1);
        EXPECT_EQ(rui.GetMostRecentlyUsed(), 0);
    }
} // namespace AZ::IO
