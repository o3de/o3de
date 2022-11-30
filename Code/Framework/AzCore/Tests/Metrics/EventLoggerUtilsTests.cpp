/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Metrics/EventLoggerFactoryImpl.h>
#include <AzCore/Metrics/EventLoggerUtils.h>
#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/ranges/filter_view.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest::EventLoggerUtilsTests
{
    class EventLoggerUtilsFixture
        : public LeakDetectionFixture
    {
    public:
        EventLoggerUtilsFixture()
        {
            // Create an event logger factory
            m_eventLoggerFactory = AZStd::make_unique<AZ::Metrics::EventLoggerFactoryImpl>();

            // Create an byte container stream that allows event logger output to be logged in-memory
            auto metricsStream = AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::string>>(&m_metricsOutput);
            // Create the trace event logger that logs to the Google Trace Event format
            auto eventLogger = AZStd::make_unique<AZ::Metrics::JsonTraceEventLogger>(AZStd::move(metricsStream));

            // Register the Google Trace Event Logger with the Event Logger Factory
            EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(m_loggerId, AZStd::move(eventLogger)));
        }
    protected:
        AZStd::string m_metricsOutput;
        AZStd::unique_ptr<AZ::Metrics::IEventLoggerFactory> m_eventLoggerFactory;
        AZ::Metrics::EventLoggerId m_loggerId{ 1 };
    };

    TEST_F(EventLoggerUtilsFixture, DirectUtilityFunctions_CanRecordMetricsSuccessfully)
    {
        constexpr AZ::Metrics::EventLoggerId loggerId{ 1 };

        const auto startThreadTime = AZStd::chrono::utc_clock::now();

        using EventObjectStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 8>;
        EventObjectStorage argsContainer;
        argsContainer.emplace_back("string", "Hello World");

        AZ::Metrics::DurationArgs durationArgs;
        durationArgs.m_name = "Duration Event";
        durationArgs.m_cat = "Test";
        durationArgs.m_args = argsContainer;
        durationArgs.m_id = "0";

        auto resultOutcome = AZ::Metrics::RecordDurationEventBegin(loggerId, durationArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordDurationEventEnd(loggerId, durationArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);

        AZ::Metrics::CompleteArgs completeArgs;
        completeArgs.m_name = "Complete Event";
        completeArgs.m_cat = "Test";
        completeArgs.m_args = argsContainer;
        completeArgs.m_dur = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::utc_clock::now() - startThreadTime);
        completeArgs.m_id = "0";

        resultOutcome = AZ::Metrics::RecordCompleteEvent(loggerId, completeArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);

        AZ::Metrics::InstantArgs instantArgs;
        instantArgs.m_name = "Instant Event";
        instantArgs.m_cat = "Test";
        instantArgs.m_args = argsContainer;
        instantArgs.m_id = "0";
        instantArgs.m_scope = AZ::Metrics::InstantEventScope::Thread;

        resultOutcome = AZ::Metrics::RecordInstantEvent(loggerId, instantArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);

        AZ::Metrics::CounterArgs counterArgs;
        counterArgs.m_name = "Counter Event";
        counterArgs.m_cat = "Test";
        counterArgs.m_args = argsContainer;
        counterArgs.m_id = "0";

        resultOutcome = AZ::Metrics::RecordCounterEvent(loggerId, counterArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);

        AZ::Metrics::AsyncArgs asyncArgs;
        asyncArgs.m_name = "Async Event";
        asyncArgs.m_cat = "Test";
        asyncArgs.m_args = argsContainer;
        asyncArgs.m_id = "0";
        asyncArgs.m_scope = "Distinguishing Scope";

        resultOutcome = AZ::Metrics::RecordAsyncEventStart(loggerId, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordAsyncEventInstant(loggerId, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordAsyncEventEnd(loggerId, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
    }

    TEST_F(EventLoggerUtilsFixture, RecordEvent_WithEventPhase_CanForwardEventToProperRecordFunction)
    {
        constexpr AZ::Metrics::EventLoggerId loggerId{ 1 };
        constexpr AZ::Metrics::EventLoggerId nonExistantloggerId{ AZStd::numeric_limits<AZ::u32>::max() };

        const auto startThreadTime = AZStd::chrono::utc_clock::now();

        using EventObjectStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 8>;
        EventObjectStorage argsContainer;
        argsContainer.emplace_back("string", "Hello World");

        AZ::Metrics::DurationArgs durationArgs;
        durationArgs.m_name = "Duration Event";
        durationArgs.m_cat = "Test";
        durationArgs.m_args = argsContainer;
        durationArgs.m_id = "0";

        AZ::Metrics::CompleteArgs completeArgs;
        completeArgs.m_name = "Complete Event";
        completeArgs.m_cat = "Test";
        completeArgs.m_args = argsContainer;
        completeArgs.m_dur = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::utc_clock::now() - startThreadTime);
        completeArgs.m_id = "0";

        // Should succeed in recording an event with the proper event phase and events arguments
        auto resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationBegin, durationArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationEnd, durationArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);

        // Should fail due to attempting to query an event logger using a non registered logger id
        resultOutcome = AZ::Metrics::RecordEvent(nonExistantloggerId, AZ::Metrics::EventPhase::DurationBegin, durationArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        // Should fail due to the Counter event phase type being using a structure of duration args
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Counter, durationArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        // Should fail due to the Duration event phase type using a struct of type CompleteArgs
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationEnd, completeArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
    }

    TEST_F(EventLoggerUtilsFixture, RecordEvent_WithEventPhase_SupportsAllEventPhasesWithEventArgsStructures)
    {
        constexpr AZ::Metrics::EventLoggerId loggerId{ 1 };

        const auto startThreadTime = AZStd::chrono::utc_clock::now();

        using EventObjectStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 8>;
        EventObjectStorage argsContainer;
        argsContainer.emplace_back("string", "Hello World");

        AZ::Metrics::DurationArgs durationArgs;
        durationArgs.m_name = "Duration Event";
        durationArgs.m_cat = "Test";
        durationArgs.m_args = argsContainer;
        durationArgs.m_id = "0";

        AZ::Metrics::CompleteArgs completeArgs;
        completeArgs.m_name = "Complete Event";
        completeArgs.m_cat = "Test";
        completeArgs.m_args = argsContainer;
        completeArgs.m_dur = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::utc_clock::now() - startThreadTime);
        completeArgs.m_id = "0";

        AZ::Metrics::InstantArgs instantArgs;
        instantArgs.m_name = "Instant Event";
        instantArgs.m_cat = "Test";
        instantArgs.m_args = argsContainer;
        instantArgs.m_id = "0";
        instantArgs.m_scope = AZ::Metrics::InstantEventScope::Thread;

        AZ::Metrics::CounterArgs counterArgs;
        counterArgs.m_name = "Counter Event";
        counterArgs.m_cat = "Test";
        counterArgs.m_args = argsContainer;
        counterArgs.m_id = "0";

        AZ::Metrics::AsyncArgs asyncArgs;
        asyncArgs.m_name = "Async Event";
        asyncArgs.m_cat = "Test";
        asyncArgs.m_args = argsContainer;
        asyncArgs.m_id = "0";
        asyncArgs.m_scope = "Distinguishing Scope";

        // Duration phases - Should succeed in recording duration events
        auto resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationBegin, durationArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationEnd, durationArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        // Duration phases - Should fail in recording all other events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationBegin, completeArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationBegin, instantArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationBegin, counterArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationBegin, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationEnd, completeArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationEnd, instantArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationEnd, counterArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::DurationEnd, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        // Complete phase - Should succeed in recording complete events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Complete, completeArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        // Complete phase - Should fail in recording all other events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Complete, durationArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Complete, instantArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Complete, counterArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Complete, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        // Instant phase - Should succeed in recording instant events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Instant, instantArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        // Instant phase - Should fail in recording all other events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Instant, durationArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Instant, completeArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Instant, counterArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Instant, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        // Counter phase - Should succeed in recording counter events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Counter, counterArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        // Counter phase - Should fail in recording all other events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Counter, durationArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Counter, completeArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Counter, instantArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::Counter, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        // Async phases - Should succeed in recording async events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncStart, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncInstant, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncEnd, asyncArgs, m_eventLoggerFactory.get());
        EXPECT_TRUE(resultOutcome);
        // Async phases - Should fail in recording all other events
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncStart, durationArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncStart, completeArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncStart, instantArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncStart, counterArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncInstant, durationArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncInstant, completeArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncInstant, instantArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncInstant, counterArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);

        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncEnd, durationArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncEnd, completeArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncEnd, instantArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
        resultOutcome = AZ::Metrics::RecordEvent(loggerId, AZ::Metrics::EventPhase::AsyncEnd, counterArgs, m_eventLoggerFactory.get());
        EXPECT_FALSE(resultOutcome);
    }

    TEST_F(EventLoggerUtilsFixture, UtilityFunctions_CanLookupGlobalRegisteredEventLoggerFactoryInstance)
    {
        constexpr AZ::Metrics::EventLoggerId loggerId{ 1 };

        using EventObjectStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 8>;
        EventObjectStorage argsContainer;
        argsContainer.emplace_back("string", "Hello World");

        AZ::Metrics::DurationArgs durationArgs;
        durationArgs.m_name = "Duration Event";
        durationArgs.m_cat = "Test";
        durationArgs.m_args = argsContainer;
        durationArgs.m_id = "0";
        // Recording a duration event using the member event logger factory always should succeed
        EXPECT_TRUE(AZ::Metrics::RecordDurationEventBegin(loggerId, durationArgs, m_eventLoggerFactory.get()));

        // Since there is not an AZ::Interface registered event logger factory recording should fail the factory argument is not supplied
        EXPECT_FALSE(AZ::Metrics::RecordDurationEventBegin(loggerId, durationArgs));

        // Register the Event Logger Factory with the AZ::Interface<IEventLoggerFactory>
        AZ::Interface<AZ::Metrics::IEventLoggerFactory>::Register(m_eventLoggerFactory.get());
        // The registered global AZ::Interface<IEventLoggerFactory> instance should be used in this case
        EXPECT_TRUE(AZ::Metrics::RecordDurationEventEnd(loggerId, durationArgs));

        // Unregister the Event Logger Factory with the AZ::Interface<IEventLoggerFactory>
        AZ::Interface<AZ::Metrics::IEventLoggerFactory>::Unregister(m_eventLoggerFactory.get());

        // The AZ::Interface<IEventLogerFactory no longer has a registered instance, the recording of the an event should fail again
        AZ::Metrics::CompleteArgs completeArgs;
        completeArgs.m_name = "Complete Event";
        completeArgs.m_cat = "Test";
        completeArgs.m_args = argsContainer;
        completeArgs.m_id = "0";
        EXPECT_FALSE(AZ::Metrics::RecordCompleteEvent(loggerId, completeArgs));
        // Recording should succeed when the member event logger is used though
        EXPECT_TRUE(AZ::Metrics::RecordCompleteEvent(loggerId, completeArgs, m_eventLoggerFactory.get()));
    }
} // namespace UnitTest::EventLoggerUtilsTests
