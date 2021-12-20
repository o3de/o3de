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
    class TimeTests : public AllocatorsFixture
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

    class TimeSystemTests : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
            m_controlTime = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond());
            m_timeSystem = AZStd::make_unique<AZ::TimeSystem>();
        }

        void TearDown() override
        {
            m_controlTime = AZ::Time::ZeroTimeUs;
            m_timeSystem.reset();
            TeardownAllocator();
        }

        AZ::TimeUs GetDiff(AZ::TimeUs time1, AZ::TimeUs time2) const
        {
            // AZ::TimeUs is unsigned so make sure to not underflow.
            return time1 > time2 ? time1 - time2 : time2 - time1;
        }

        AZ::TimeUs m_controlTime;
        AZStd::unique_ptr<AZ::TimeSystem> m_timeSystem;
    };
    
    TEST_F(TimeSystemTests, GetRealElapsedTimeUs)
    {
        // sleep for a bit to advance time.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));

        // find the delta for the control and from GetRealElapsedTimeUs
        const AZ::TimeUs baseline = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond()) - m_controlTime;
        const AZ::TimeUs elapsedTime = m_timeSystem->GetRealElapsedTimeUs();

        const AZ::TimeUs diff = GetDiff(baseline, elapsedTime);

        // elapsedTime should be within 10 microseconds from baseline.
        EXPECT_LT(diff, AZ::TimeUs{ 10 });
    }

    TEST_F(TimeSystemTests, GetElapsedTimeUs)
    {
        // sleep for a bit to advance time.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));

        // find the delta for the control and from GetElapsedTimeUs
        const AZ::TimeUs baseline = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond()) - m_controlTime;
        const AZ::TimeUs elapsedTime = m_timeSystem->GetElapsedTimeUs();

        const AZ::TimeUs diff = GetDiff(baseline, elapsedTime);

        // elapsedTime should be within 10 microseconds from baseline.
        EXPECT_LT(diff, AZ::TimeUs{ 10 });
    }

    TEST_F(TimeSystemTests, ElapsedTimeScales)
    {
        // slow down 'time'
        m_timeSystem->SetSimulationTickScale(0.5f);

        // sleep for a bit to advance time.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));

        // find the delta for the control and from GetElapsedTimeUs
        const AZ::TimeUs baseline = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond()) - m_controlTime;
        const AZ::TimeUs elapsedTime = m_timeSystem->GetElapsedTimeUs();
        const AZ::TimeUs halfBaseline = (baseline / AZ::TimeUs{ 2 });

        // elapsedTime should be about half of the control.
        const AZ::TimeUs diff = GetDiff(halfBaseline, elapsedTime);

        // elapsedTime should be within 10 microseconds from baseline.
        EXPECT_LT(diff, AZ::TimeUs{ 10 });

        // reset time scale
        m_timeSystem->SetSimulationTickScale(1.0f);
    }

    TEST_F(TimeSystemTests, AdvanceTickDeltaTimes)
    {
        // advance the tick delta to get a clean base.
        m_timeSystem->AdvanceTickDeltaTimes();
        const AZ::TimeUs baselineStart = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond());

        // sleep for a bit to advance time.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));

        // advance the tick delta.
        const AZ::TimeUs delta = m_timeSystem->AdvanceTickDeltaTimes();
        const AZ::TimeUs baselineDelta = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond()) - baselineStart;

        // the delta should be close to the baselineDelta.
        const AZ::TimeUs diff = GetDiff(delta, baselineDelta);
        EXPECT_LT(diff, AZ::TimeUs{ 10 });
    }

    TEST_F(TimeSystemTests, SimulationAndRealTickDeltaTimesWithNoTimeScale)
    {
        // advance the tick delta to get a clean base.
        m_timeSystem->AdvanceTickDeltaTimes();
        const AZ::TimeUs baselineStart = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond());

        // sleep for a bit to advance time.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));

        // advance the tick delta.
        const AZ::TimeUs delta = m_timeSystem->AdvanceTickDeltaTimes();
        const AZ::TimeUs baselineDelta = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond()) - baselineStart;

        // the delta should be close to the baselineDelta.
        AZ::TimeUs diff = GetDiff(delta, baselineDelta);
        EXPECT_LT(diff, AZ::TimeUs{ 10 });

        // the delta should be the same as GetSimulationTickDeltaTimeUs and near GetRealTickDeltaTimeUs
        const AZ::TimeUs simDeltaTime = m_timeSystem->GetSimulationTickDeltaTimeUs();
        EXPECT_EQ(delta, simDeltaTime);

        const AZ::TimeUs realDeltaTime = m_timeSystem->GetRealTickDeltaTimeUs();
        diff = GetDiff(delta, realDeltaTime);
        EXPECT_LT(diff, AZ::TimeUs{ 10 });
    }

    TEST_F(TimeSystemTests, SimulationAndRealTickDeltaTimesWithTimeScale)
    {
        // advance the tick delta to get a clean base.
        m_timeSystem->AdvanceTickDeltaTimes();
        const AZ::TimeUs baselineStart = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond());

        // slow down 'time';
        m_timeSystem->SetSimulationTickScale(0.5f);

        // sleep for a bit to advance time.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));

        // advance the tick delta.
        const AZ::TimeUs delta = m_timeSystem->AdvanceTickDeltaTimes();
        const AZ::TimeUs baselineDelta = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond()) - baselineStart;
        const AZ::TimeUs halfBaselineDelta = (baselineDelta / AZ::TimeUs{ 2 });

        // the delta should be half the baselineDelta
        AZ::TimeUs diff = GetDiff(delta, halfBaselineDelta);
        EXPECT_LT(diff, AZ::TimeUs{ 10 });

        // the delta should be the same as GetSimulationTickDeltaTimeUs
        const AZ::TimeUs simDeltaTime = m_timeSystem->GetSimulationTickDeltaTimeUs();
        EXPECT_EQ(delta, simDeltaTime);

        // the delta should be near half the GetRealTickDeltaTimeUs
        const AZ::TimeUs realDeltaTime = m_timeSystem->GetRealTickDeltaTimeUs();
        const AZ::TimeUs halfRealDeltaTime = (realDeltaTime / AZ::TimeUs{ 2 });
        diff = GetDiff(delta, halfRealDeltaTime);
        EXPECT_LT(diff, AZ::TimeUs{ 10 });

        // reset time scale
        m_timeSystem->SetSimulationTickScale(1.0f);
    }

    TEST_F(TimeSystemTests, SimulationTickDeltaOverride)
    {
        // advance the tick delta to get a clean base.
        m_timeSystem->AdvanceTickDeltaTimes();
        const AZ::TimeUs baselineStart = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond());

        // set the tick delta override
        const AZ::TimeMs tickOverride = AZ::TimeMs{ 3462 };
        m_timeSystem->SetSimulationTickDeltaOverride(tickOverride);

        // sleep for a bit to advance time.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));

        // advance the tick delta.
        const AZ::TimeUs delta = m_timeSystem->AdvanceTickDeltaTimes();
        const AZ::TimeUs baselineDelta = static_cast<AZ::TimeUs>(AZStd::GetTimeNowMicroSecond()) - baselineStart;

        // the delta should be equal to the tickOverride
        EXPECT_EQ(delta, AZ::TimeMsToUs(tickOverride));

        // real tick delta should be near the baselineDelta
        const AZ::TimeUs realDeltaTime = m_timeSystem->GetRealTickDeltaTimeUs();
        const AZ::TimeUs diff = GetDiff(realDeltaTime, baselineDelta);
        EXPECT_LT(diff, AZ::TimeUs{ 10 });

        // reset the tick delta override
        m_timeSystem->SetSimulationTickDeltaOverride(AZ::Time::ZeroTimeMs);
    }
} // namespace UnitTest
