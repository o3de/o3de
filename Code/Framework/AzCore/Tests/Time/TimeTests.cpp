/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Time/TimeSystem.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class TimeTests
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
            m_timeSystem = AZStd::make_unique<AZ::TimeSystem>();
        }

        void TearDown() override
        {
            m_timeSystem.reset();
            TeardownAllocator();
        }

        AZStd::unique_ptr<AZ::TimeSystem> m_timeSystem;
    };

    TEST_F(TimeTests, TestConversionUsToMs)
    {
        AZ::TimeUs timeUs = AZ::TimeUs{ 1000 };
        AZ::TimeMs timeMs = AZ::TimeUsToMs(timeUs);
        EXPECT_EQ(timeMs, AZ::TimeMs{ 1 });
    }

    TEST_F(TimeTests, TestConversionMsToUs)
    {
        AZ::TimeMs timeMs = AZ::TimeMs{ 1000 };
        AZ::TimeUs timeUs = AZ::TimeMsToUs(timeMs);
        EXPECT_EQ(timeUs, AZ::TimeUs{ 1000000 });
    }

    TEST_F(TimeTests, TestConversionTimeMsToSeconds)
    {
        AZ::TimeMs timeMs = AZ::TimeMs{ 1000 };
        float timeSecondsFloat = AZ::TimeMsToSeconds(timeMs);
        EXPECT_TRUE(AZ::IsClose(timeSecondsFloat, 1.0f));

        double timeSecondsDouble = AZ::TimeMsToSecondsDouble(timeMs);
        EXPECT_TRUE(AZ::IsClose(timeSecondsDouble, 1.0));
    }

    TEST_F(TimeTests, TestConversionSecondsToTimeUs)
    {
        double seconds = 1.0;
        AZ::TimeUs timeUs = AZ::SecondsToTimeUs(seconds);
        EXPECT_EQ(timeUs, AZ::TimeUs{ 1000000 });
    }

    TEST_F(TimeTests, TestConversionSecondsToTimeMs)
    {
        double seconds = 1.0;
        AZ::TimeMs timeMs = AZ::SecondsToTimeMs(seconds);
        EXPECT_EQ(timeMs, AZ::TimeMs{ 1000 });
    }

    TEST_F(TimeTests, TestClocks)
    {
        AZ::TimeUs timeUs = AZ::GetElapsedTimeUs();
        AZ::TimeMs timeMs = AZ::GetElapsedTimeMs();

        AZ::TimeMs timeUsToMs = AZ::TimeUsToMs(timeUs);
        int64_t delta = static_cast<int64_t>(timeMs) - static_cast<int64_t>(timeUsToMs);
        EXPECT_LT(abs(delta), 1);
    }
}
