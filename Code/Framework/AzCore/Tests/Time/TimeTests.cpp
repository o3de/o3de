/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class TimeTests : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_timeSystem = AZStd::make_unique<AZ::TimeSystem>();
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

    TEST_F(TimeTests, QueryCPUThreadTime_ReturnsNonZero)
    {
        (void)AZStd::GetCpuThreadTimeNowMicrosecond();

        // Windows need at least 50 milliseconds of running time to measure the processer tick scale
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(50));

        // The cpu thread time should now return non-zero values
        AZStd::chrono::microseconds initialCpuThreadTime = AZStd::GetCpuThreadTimeNowMicrosecond();
        // Sleep for 1 millisecond (or 1000 microseconds) and check the CPU time
        // less than 1 miilisecond of CPU should have elapsed
        // Now due to the fact that thread sleep can last got longer than it's duration
        // this test is only checking that the new CPUThreadTime is at least greater than or equal to the initial
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
        AZStd::chrono::microseconds newCpuThreadTime = AZStd::GetCpuThreadTimeNowMicrosecond();

        EXPECT_GT((newCpuThreadTime - initialCpuThreadTime).count(), 0);
    }
} // namespace UnitTest
