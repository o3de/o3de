/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Time/TimeSystemComponent.h>
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
            m_timeComponent = new AZ::TimeSystemComponent;
        }

        void TearDown() override
        {
            delete m_timeComponent;
            TeardownAllocator();
        }

        AZ::TimeSystemComponent* m_timeComponent = nullptr;
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

    TEST_F(TimeTests, TestClocks)
    {
        AZ::TimeUs timeUs = AZ::GetElapsedTimeUs();
        AZ::TimeMs timeMs = AZ::GetElapsedTimeMs();

        AZ::TimeMs timeUsToMs = AZ::TimeUsToMs(timeUs);
        int64_t delta = static_cast<int64_t>(timeMs) - static_cast<int64_t>(timeUsToMs);
        EXPECT_LT(abs(delta), 1);
    }
}
