/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/IEventScheduler.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class ScheduledEventTests
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
            AZ::NameDictionary::Create();

            m_loggerComponent = AZStd::make_unique<AZ::LoggerSystemComponent>();
            m_timeSystem = AZStd::make_unique<AZ::TimeSystem>();
            m_eventSchedulerComponent = AZStd::make_unique<AZ::EventSchedulerSystemComponent>();

            m_testEvent = AZStd::make_unique<AZ::ScheduledEvent>([this] { TestBasicEvent(); }, AZ::Name("UnitTestEvent fire once event"));
            m_testRequeue = AZStd::make_unique<AZ::ScheduledEvent>([this] { TestAutoRequeuedEvent(); }, AZ::Name("UnitTestEvent auto Requeue"));
        }

        void TearDown() override
        {
            m_testEvent.reset();
            m_testRequeue.reset();

            m_eventSchedulerComponent.reset();
            m_timeSystem.reset();
            m_loggerComponent.reset();

            AZ::NameDictionary::Destroy();
        }

        void TestBasicEvent()
        {
            ++m_basicEventTriggerCount;
        }

        void TestAutoRequeuedEvent()
        {
            ++m_requeuedEventTriggerCount;
        }

        uint32_t m_basicEventTriggerCount = 0;
        uint32_t m_requeuedEventTriggerCount = 0;

        AZStd::unique_ptr<AZ::ScheduledEvent> m_testEvent;
        AZStd::unique_ptr<AZ::ScheduledEvent> m_testRequeue;

        AZStd::unique_ptr<AZ::LoggerSystemComponent> m_loggerComponent;
        AZStd::unique_ptr<AZ::TimeSystem> m_timeSystem;
        AZStd::unique_ptr<AZ::EventSchedulerSystemComponent> m_eventSchedulerComponent;
    };

    TEST_F(ScheduledEventTests, TestFireOnce)
    {
        m_testEvent->Enqueue(AZ::TimeMs(100));
        constexpr AZ::TimeMs TotalIterationTimeMs = AZ::TimeMs{ 1050 };
        const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();
        for (;;)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            m_eventSchedulerComponent->OnTick(0.0f, AZ::ScriptTimePoint());
            if (AZ::GetElapsedTimeMs() - startTimeMs > TotalIterationTimeMs)
            {
                break;
            }
        }
        EXPECT_EQ(m_basicEventTriggerCount, 1);
    }

    TEST_F(ScheduledEventTests, TestRequeue)
    {
        m_testRequeue->Enqueue(AZ::TimeMs(200), true);
        constexpr AZ::TimeMs TotalIterationTimeMs = AZ::TimeMs{ 1050 };
        const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();
        for (;;)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            m_eventSchedulerComponent->OnTick(0.0f, AZ::ScriptTimePoint());
            if (AZ::GetElapsedTimeMs() - startTimeMs > TotalIterationTimeMs)
            {
                break;
            }
        }
        // Use EXPECT_GT in case the OS oversleeps long enough to cause unexpected extra timer pops
        EXPECT_GT(m_requeuedEventTriggerCount, 1);
    }
}
