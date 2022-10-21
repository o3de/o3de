/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Metrics/IEventLogger.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/ring_buffer.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::IO
{
    class GenericStream;
}

namespace AZ::Metrics
{
    // Contains JsonTraceEventLogger specific configuration
    struct JsonTraceLoggerEventConfig
    {
        AZStd::string_view m_loggerName;
    };

    class JsonTraceEventLogger
        : public IEventLogger
    {
    public:
        JsonTraceEventLogger();
        explicit JsonTraceEventLogger(JsonTraceLoggerEventConfig);
        //! Generic stream which owned by the JsonEventLogger
        explicit JsonTraceEventLogger(AZStd::unique_ptr<AZ::IO::GenericStream> stream);
        JsonTraceEventLogger(AZStd::unique_ptr<AZ::IO::GenericStream> stream, JsonTraceLoggerEventConfig);

        ~JsonTraceEventLogger();

        //! Writes and json data to the stream
        void Flush() override;

        ResultOutcome RecordDurationEventBegin(const DurationArgs&) override;
        //! Records an end duration event
        //! Uses the event header to populate the event fields
        ResultOutcome RecordDurationEventEnd(const DurationArgs&) override;

        //! Records a complete event(encapsulates a beging and end duration event)
        //! Uses the event header to populate the event fields
        ResultOutcome RecordCompleteEvent(const CompleteArgs&) override;

        //! Records an instant event
        //! Uses the event header to populate the event fields
        ResultOutcome RecordInstantEvent(const InstantArgs&) override;

        //! Records a Counter
        //! Uses the event header to populate the event fields
        ResultOutcome RecordCounterEvent(const CounterArgs&) override;

        //! Records a start async event
        //! Uses the event header to populate the event fields
        ResultOutcome RecordAsyncEventStart(const AsyncArgs&) override;
        //! Records an instant async event
        //! Uses the event header to populate the event fields
        ResultOutcome RecordAsyncEventInstant(const AsyncArgs&) override;
        //! Records an end async event
        //! Uses the event header to populate the event fields
        ResultOutcome RecordAsyncEventEnd(const AsyncArgs&) override;

        //! Closes the previous stream and associates a new stream
        void ResetStream(AZStd::unique_ptr<AZ::IO::GenericStream> stream);

        static constexpr size_t MaxEventJsonStringSize = 1024;
        using JsonEventString = AZStd::fixed_string<MaxEventJsonStringSize>;
    protected:
        //! Responsible for writing the recorded event data to JSON
        bool FlushRequest(const EventDesc&);

        //! Complete the JSON document by adding the ending ']' bracket
        bool Complete(AZ::IO::GenericStream& stream);

        //! Start the JSON document by adding the opening '[' bracket
        bool Start(AZ::IO::GenericStream& stream);

    protected:
        AZStd::mutex m_flushToStreamMutex;
        AZStd::unique_ptr<AZ::IO::GenericStream> m_stream;

        //! Provides a user friendly name for the event logger
        AZStd::string m_name;
        //! Keep track of whether this is the first event being logged
        //! This is used to prepend a leading comma before the event entry in the trace events array
        AZStd::atomic_bool m_prependComma{ false };

        //! Tracks the number of events written so far
        size_t m_eventCount{};
    };
} // namespace AZ::Metrics
