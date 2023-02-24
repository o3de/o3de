/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Metrics/IEventLogger.h>
#include <AzCore/Metrics/EventLoggerFactoryImpl.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class EventLoggerFactoryFixture
        : public LeakDetectionFixture
    {
    public:
        EventLoggerFactoryFixture()
        {
            m_eventLoggerFactory = AZStd::make_unique<AZ::Metrics::EventLoggerFactoryImpl>();
        }
    protected:
        AZStd::unique_ptr<AZ::Metrics::IEventLoggerFactory> m_eventLoggerFactory;
    };

    // Placeholder EventLogger implementation to test factory registration
    class TestEventLogger
        : public AZ::Metrics::IEventLogger
    {
    public:
        AZ_CLASS_ALLOCATOR(TestEventLogger, AZ::SystemAllocator);
        void Flush() override {}

        ResultOutcome RecordDurationEventBegin(const AZ::Metrics::DurationArgs&) override { return AZ::Success(); }
        ResultOutcome RecordDurationEventEnd(const AZ::Metrics::DurationArgs&) override { return AZ::Success(); }
        ResultOutcome RecordCompleteEvent(const AZ::Metrics::CompleteArgs&) override { return AZ::Success(); }
        ResultOutcome RecordInstantEvent(const AZ::Metrics::InstantArgs&) override { return AZ::Success(); }
        ResultOutcome RecordCounterEvent(const AZ::Metrics::CounterArgs&) override { return AZ::Success(); }
        ResultOutcome RecordAsyncEventStart(const AZ::Metrics::AsyncArgs&) override { return AZ::Success(); }
        ResultOutcome RecordAsyncEventInstant(const AZ::Metrics::AsyncArgs&) override { return AZ::Success(); }
        ResultOutcome RecordAsyncEventEnd(const AZ::Metrics::AsyncArgs&) override { return AZ::Success(); }
    };

    TEST_F(EventLoggerFactoryFixture, Factory_CanRegisterEventLogger_Successfully)
    {
        constexpr AZ::Metrics::EventLoggerId loggerId{ 1 };
        constexpr AZ::Metrics::EventLoggerId otherLoggerId{ 2 };

        // Use placeholder implementation
        auto eventLogger = AZStd::make_unique<TestEventLogger>();
        EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(loggerId, AZStd::move(eventLogger)));

        // Attempt to register another event logger using the same loggerId
        // It should fail since it is already registered
        eventLogger = AZStd::make_unique<TestEventLogger>();
        auto registerResult = m_eventLoggerFactory->RegisterEventLogger(loggerId, AZStd::move(eventLogger));
        EXPECT_FALSE(registerResult);
        // Make sure the eventLogger is returned in the failure outcome
        ASSERT_NE(nullptr, registerResult.GetError().get());

        // Registering the event logger using a different event logger id should work
        eventLogger = AZStd::make_unique<TestEventLogger>();
        EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(otherLoggerId, AZStd::move(eventLogger)));

        // Unregister the first logger to validate the Unregister function
        EXPECT_TRUE(m_eventLoggerFactory->UnregisterEventLogger(loggerId));

        // Attempt to register the first event logger again. It should fail
        EXPECT_FALSE(m_eventLoggerFactory->UnregisterEventLogger(loggerId));

        // The second event logger will be unregistered when the EventFactory goes out of scope
    }

    TEST_F(EventLoggerFactoryFixture, Factory_RegisteringAndUnregistering_EventLoggers_DoesNotCauseMemoryCorruption)
    {
        // As the Event Factory sorts the Event Loggers in order of there logger id
        // This test validates that removing event loggers from the front of the array
        // and accessing them does not cause memory courrption

        // Used to generate EventLoggerIds in the range [0, count)
        constexpr size_t EventLoggerIdCount = 10;

        // TestEventLogger is not owned by the Event Factory as it is passed by lvalue reference
        // This is used to register the same event logger with multiple ids
        TestEventLogger testEventLogger;
        // Register event loggers in reverse ID order
        for (uint32_t i = EventLoggerIdCount; i > 0; --i)
        {
            // Subtract to avoid indexing out of bounds
            auto lastIndex = i - 1;
            EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(AZ::Metrics::EventLoggerId{ lastIndex }, testEventLogger));
        }

        // Validate event loggers can be accessed by iterating in order
        for (uint32_t i = 0; i < EventLoggerIdCount; ++i)
        {
            EXPECT_NE(nullptr, m_eventLoggerFactory->FindEventLogger(AZ::Metrics::EventLoggerId{ i }));
        }

        // Unregister event loggers in ascending order
        for (uint32_t i = 0; i < EventLoggerIdCount; ++i)
        {
            EXPECT_TRUE(m_eventLoggerFactory->UnregisterEventLogger(AZ::Metrics::EventLoggerId{ i }));
        }

    }

    TEST_F(EventLoggerFactoryFixture, Factory_NullEventLoggerRegistration_Fails)
    {
        constexpr AZ::Metrics::EventLoggerId loggerId{ 1 };
        AZStd::unique_ptr<TestEventLogger> eventLogger;
        EXPECT_FALSE(m_eventLoggerFactory->RegisterEventLogger(loggerId, AZStd::move(eventLogger)));
    }

    TEST_F(EventLoggerFactoryFixture, Factory_FindEventLoggerById_Succeeds)
    {
        constexpr AZ::Metrics::EventLoggerId loggerId{ 1 };
        constexpr AZ::Metrics::EventLoggerId otherLoggerId{ 2 };
        constexpr AZ::Metrics::EventLoggerId notRegisteredId{ 5 };

        // Register two event loggers
        auto eventLogger = AZStd::make_unique<TestEventLogger>();
        EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(loggerId, AZStd::move(eventLogger)));

        eventLogger = AZStd::make_unique<TestEventLogger>();
        EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(otherLoggerId, AZStd::move(eventLogger)));

        EXPECT_TRUE(m_eventLoggerFactory->IsRegistered(loggerId));
        EXPECT_TRUE(m_eventLoggerFactory->IsRegistered(otherLoggerId));
        EXPECT_FALSE(m_eventLoggerFactory->IsRegistered(notRegisteredId));

        EXPECT_NE(nullptr, m_eventLoggerFactory->FindEventLogger(loggerId));
        EXPECT_NE(nullptr, m_eventLoggerFactory->FindEventLogger(otherLoggerId));
        EXPECT_EQ(nullptr, m_eventLoggerFactory->FindEventLogger(notRegisteredId));
    }

    TEST_F(EventLoggerFactoryFixture, Factory_VisitAllLoggers_Succeeds)
    {
        constexpr AZ::Metrics::EventLoggerId loggerId{ 1 };
        constexpr AZ::Metrics::EventLoggerId otherLoggerId{ 2 };

        // Register two event loggers
        auto eventLogger = AZStd::make_unique<TestEventLogger>();
        EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(loggerId, AZStd::move(eventLogger)));

        eventLogger = AZStd::make_unique<TestEventLogger>();
        EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(otherLoggerId, AZStd::move(eventLogger)));

        constexpr int expectedCount = 2;
        int visitedCount{};
        auto VisitLogger = [&visitedCount](AZ::Metrics::IEventLogger&)
        {
            ++visitedCount;
            return true; // continue visiting
        };
        m_eventLoggerFactory->VisitEventLoggers(VisitLogger);

        EXPECT_EQ(expectedCount, visitedCount);
    }
} // namespace UnitTest
