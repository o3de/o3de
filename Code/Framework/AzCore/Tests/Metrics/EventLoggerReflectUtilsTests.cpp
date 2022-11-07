/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Metrics/EventLoggerFactoryImpl.h>
#include <AzCore/Metrics/EventLoggerReflectUtils.h>
#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest::EventLoggerReflectUtilsTests
{
    class EventLoggerReflectUtilsFixture
        : public ScopedAllocatorSetupFixture
    {
    public:
        EventLoggerReflectUtilsFixture()
        {
            // Create an event logger factory
            m_eventLoggerFactory = AZStd::make_unique<AZ::Metrics::EventLoggerFactoryImpl>();

            // Create an byte container stream that allows event logger output to be logged in-memory
            auto metricsStream = AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::string>>(&m_metricsOutput);
            // Create the trace event logger that logs to the Google Trace Event format
            auto eventLogger = AZStd::make_unique<AZ::Metrics::JsonTraceEventLogger>(AZStd::move(metricsStream));

            // Register the Google Trace Event Logger with the Event Logger Factory
            EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(m_loggerId, AZStd::move(eventLogger)));

            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            AZ::Metrics::Reflect(m_behaviorContext.get());
        }
    protected:
        AZStd::string m_metricsOutput;
        AZStd::unique_ptr<AZ::Metrics::IEventLoggerFactory> m_eventLoggerFactory;
        AZ::Metrics::EventLoggerId m_loggerId{ 1 };

        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    };

    constexpr const char* EventValueClassName = "EventValue";
    constexpr const char* EventFieldClassName = "EventField";
    constexpr const char* EventArrayClassName = "EventArray";
    constexpr const char* EventObjectClassName = "EventObject";

    TEST_F(EventLoggerReflectUtilsFixture, EventLogger_EventDataTypes_AreAccessibleInScript_Succeeds)
    {
        auto classIter = m_behaviorContext->m_classes.find(EventValueClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventFieldClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventArrayClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventObjectClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);
    }

    TEST_F(EventLoggerReflectUtilsFixture, EventLogger_EventDataTypes_CanBeCreated)
    {
        auto classIter = m_behaviorContext->m_classes.find(EventValueClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventFieldClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventArrayClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventObjectClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);
    }

    TEST_F(EventLoggerReflectUtilsFixture, RecordEvent_CanRecordEventInScript_Succeeds)
    {

    }

} // namespace UnitTest::EventLoggerReflectUtilsTests
