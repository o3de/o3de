/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/NetworkTime/RewindableArray.h>
#include <Multiplayer/NetworkTime/RewindableFixedVector.h>
#include <Multiplayer/NetworkTime/RewindableObject.h>
#include <Source/NetworkTime/NetworkTime.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Time/TimeSystemComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class RewindableContainerTests
        : public AllocatorsFixture
    {
    public:
        Multiplayer::NetworkTime m_networkTime;
        AZ::LoggerSystemComponent m_loggerComponent;
        AZ::TimeSystemComponent m_timeComponent;
    };

    static constexpr uint32_t RewindableContainerSize = 7;

    TEST_F(RewindableContainerTests, BasicVectorTest)
    {
        Multiplayer::RewindableFixedVector<uint32_t, RewindableContainerSize> test(0, 0);

        // Test push_back
        for (uint32_t idx = 0; idx < RewindableContainerSize; ++idx)
        {
            test.push_back(idx);
            EXPECT_EQ(idx, test[idx]);
            Multiplayer::GetNetworkTime()->IncrementHostFrameId();
        }

        // Test rewind for all pushed values and overall size
        for (uint32_t idx = 0; idx < RewindableContainerSize; ++idx)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(idx), AZ::TimeMs{ 0 }, AzNetworking::InvalidConnectionId);
            EXPECT_EQ(idx + 1, test.size());
            EXPECT_EQ(idx, test.back());
        }

        // Test pop_back
        test.pop_back();
        EXPECT_EQ(RewindableContainerSize - 1, test.size());
        Multiplayer::GetNetworkTime()->IncrementHostFrameId();

        // Test iterator
        uint32_t iterCount = 0;
        auto iter = test.begin();
        while (iter != test.end())
        {
            ++iterCount;
            ++iter;
        }
        EXPECT_EQ(RewindableContainerSize - 1, iterCount);

        // Test clear and empty
        test.clear();
        EXPECT_EQ(0, test.size());
        Multiplayer::GetNetworkTime()->IncrementHostFrameId();
        EXPECT_TRUE(test.empty());

        // Test rewind for pop_back and clear
        Multiplayer::ScopedAlterTime pop_time(static_cast<Multiplayer::HostFrameId>(RewindableContainerSize), AZ::TimeMs{ 0 }, AzNetworking::InvalidConnectionId);
        EXPECT_EQ(RewindableContainerSize - 1, test.size());
        Multiplayer::ScopedAlterTime clear_time(static_cast<Multiplayer::HostFrameId>(RewindableContainerSize + 1), AZ::TimeMs{ 0 }, AzNetworking::InvalidConnectionId);
        EXPECT_EQ(0, test.size());

        // Test copy_values and resize_no_construct
        test.resize_no_construct(RewindableContainerSize);
        test.copy_values(&test[RewindableContainerSize-1], 1);
        EXPECT_EQ(1, test.size());
        test.resize_no_construct(RewindableContainerSize);
        EXPECT_EQ(test[0], test[RewindableContainerSize - 1]);
    }

    TEST_F(RewindableContainerTests, BasicArrayTest)
    {
        Multiplayer::RewindableArray<uint32_t, RewindableContainerSize> test;

        test.fill(0);
        Multiplayer::GetNetworkTime()->IncrementHostFrameId();
        // Test push_back
        for (uint32_t idx = 0; idx < RewindableContainerSize; ++idx)
        {
            test[idx] = idx;
            EXPECT_EQ(idx, test[idx].Get());
            Multiplayer::GetNetworkTime()->IncrementHostFrameId();
        }

        // Test rewind for all values and overall size
        for (uint32_t idx = 1; idx <= RewindableContainerSize; ++idx)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(idx), AZ::TimeMs{ 0 }, AzNetworking::InvalidConnectionId);
            for (uint32_t testIdx = 0; testIdx < RewindableContainerSize; ++testIdx)
            {
                if (testIdx < idx)
                {
                    EXPECT_EQ(testIdx, test[testIdx].Get());
                }
                else
                {
                    EXPECT_EQ(0, test[testIdx].Get());
                }
            }
        }
    }
}
