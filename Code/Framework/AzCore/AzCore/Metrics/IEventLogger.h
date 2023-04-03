/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Platform.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/utils.h>

namespace AZ::IO
{
    class GenericStream;
}

namespace AZ::Metrics::Internal
{
    //! Wraps the Metrics settings prefix key of "/O3DE/Metrics"
    //! which is used as the parent object of any "/O3DE/Metrics/<EventLoggerName>" keys
    //! The "/O3DE/Metrics/<EventLoggerName>" is the anchor object where settings
    //! for an Event Logger are queried.
    //! Currently  supports the following settings
    //!
    //! * "/O3DE/Metrics/<EventLoggerName>/Active" - If set to false, the event logger will not record new events
    //!   If not set or true, the event logger will record events
    struct SettingsKey_t
    {
        using StringType = AZStd::fixed_string<128>;

        constexpr StringType operator()(AZStd::string_view name) const
        {
            constexpr size_t MaxTotalKeySize = StringType{}.max_size();
            // The +1 is for the '/' separator
            [[maybe_unused]] const size_t maxNameSize = MaxTotalKeySize - (MetricsSettingsPrefix.size() + 1);

            AZ_Assert(name.size() <= maxNameSize,
                R"(The size of the event logger name "%.*s" is too long. It must be <= %zu characters)",
                AZ_STRING_ARG(name), maxNameSize);
            StringType settingsKey(MetricsSettingsPrefix);
            settingsKey += '/';
            settingsKey += name;

            return settingsKey;
        }

        constexpr operator AZStd::string_view() const
        {
            return MetricsSettingsPrefix;
        }

    private:
        AZStd::string_view MetricsSettingsPrefix = "/O3DE/Metrics";
    };
}

namespace AZ::Metrics
{
    //! Settings key object which supports a call operator
    //! which accepts the name of an event logger and returns
    //! a fixed_string acting as an anchor object for all settings
    //! associated with an event logger using that name
    constexpr Internal::SettingsKey_t SettingsKey{};

    //! Represents the "args" field where key, value entries
    //! within the per event data is stored
    constexpr AZStd::string_view ArgsKey = "args";

    //! Event fields structure that can be used to record event argument data
    //! NOTE: the EventValue does not own any data
    //! Any string or array data must outlive the EventValue
    struct EventValue;

    //! Encapsulates an EventField paired with a field name.
    //! Represents a single json key, value pair field
    struct EventField;

    //! AZStd/std::span requires a complete type, so an EventValue pointer and a size member
    //! is stored in the public API exposes a span interface
    //! https://eel.is/c++draft/containers#span.overview-4
    //! The reason why EventValue can't be complete is because it is a recursive type
    //! that has a reference to an array of child event values
    struct EventArray
    {
        constexpr EventArray() = default;
        constexpr explicit EventArray(AZStd::span<EventValue> arrayValues);

        constexpr void SetArrayValues (AZStd::span<EventValue> arrayValues);
        constexpr AZStd::span<EventValue> GetArrayValues() const;

    private:
        EventValue* m_arrayAddress{};
        size_t m_arraySize{};
    };

    //! AZStd/std::span requires a complete type, so an EventField pointer and a size member
    //! is stored in the public API exposes a span interface
    struct EventObject
    {
        constexpr EventObject() = default;
        constexpr explicit EventObject(AZStd::span<EventField> objectFields);

        constexpr void SetObjectFields(AZStd::span<EventField> objectFields);
        constexpr AZStd::span<EventField> GetObjectFields() const;

    private:
        EventField* m_objectAddress{};
        size_t m_objectSize{};
    };

    //! Implementation of Event Value struct used to reference JSON like types
    struct EventValue
    {
        constexpr EventValue();
        template<class T, class Alt = AZStd::variant_detail::best_alternative_t<T,
            AZStd::string_view,
            bool,
            AZ::s64,
            AZ::u64,
            double,
            EventArray,
            EventObject>, class = AZStd::enable_if_t<AZStd::constructible_from<Alt, T>>>
        constexpr EventValue(T&& value);

        template<class T, class... Args>
        constexpr explicit EventValue(AZStd::in_place_type_t<T>, Args&&... args);

        template<class T, class U, class... Args>
        constexpr explicit EventValue(AZStd::in_place_type_t<T>, AZStd::initializer_list<U> iList, Args&&... args);

        using ArgsVariant = AZStd::variant<
            AZStd::string_view,
            bool,
            AZ::s64,
            AZ::u64,
            double,
            EventArray, // Used to encapsulate a span of <EventValue> to represent arguments to output to the Google Trace format "args" object
            EventObject  // Used to encapsulate a span of <EventField> to represent arguments to output to the Google Trace format "args" object
        >;

        //! Variant instance for storing the value of an event entry(string, bool integer, double, array, object)
        ArgsVariant m_value;
    };

    //! Event field can now be defined now that EventValue is complete
    struct EventField
    {
        constexpr EventField();
        constexpr EventField(AZStd::string_view name, EventValue value);

        //! Name of the field
        AZStd::string_view m_name;
        //! Value of the field
        EventValue m_value;
    };

    // Helper Type aliases that can be used to provide storage for JSON array and JSON object types inside of a function
    // The fixed_vector capacity can be upped if more storage is needed
    using EventArrayStorage = AZStd::fixed_vector<AZ::Metrics::EventValue, 32>;
    using EventObjectStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 32>;

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(EventPhase, char,
        (DurationBegin, 'B'),
        (DurationEnd, 'E'),
        (Complete, 'X'),
        (Instant, 'i'),
        (Counter, 'C'),
        (AsyncStart, 'b'),
        (AsyncInstant, 'n'),
        (AsyncEnd, 'e'),
        (FlowStart, 's'),
        (FlowInstant, 't'),
        (FlowEnd, 'f'),
        (ObjectCreated, 'N'),
        (ObjectSnapshot, 'O'),
        (ObjectDestroyed, 'D'),
        (Metadata, 'M'),
        (MemoryDumpGlobal, 'V'),
        (MemoryDumpProcess, 'v'),
        (Mark, 'R'),
        (ClockSync, 'c'),
        (ContextEnter, '('),
        (ContentLeave, ')')
        );

    struct EventDesc
    {
        EventDesc();

        void SetName(AZStd::string_view name);
        AZStd::string_view GetName() const;

        void SetCategory(AZStd::string_view category);
        AZStd::string_view GetCategory() const;

        void SetEventPhase(EventPhase phase);
        EventPhase GetEventPhase() const;

        void SetId(AZStd::optional<AZStd::string_view> id);
        AZStd::optional<AZStd::string_view> GetId() const;

        void SetTimestamp(AZStd::chrono::microseconds);
        AZStd::chrono::microseconds GetTimestamp() const;

        void SetThreadTimestamp(AZStd::optional<AZStd::chrono::microseconds>);
        AZStd::optional<AZStd::chrono::microseconds> GetThreadTimestamp() const;

        void SetProcessId(AZ::Platform::ProcessId processId);
        AZ::Platform::ProcessId GetProcessId() const;

        void SetThreadId(AZStd::thread::id);
        AZStd::thread::id GetThreadId() const;

        //! Sets the fields of the "args" JSON object on the event description data
        void SetArgs(AZStd::span<EventField> argsFields);

        //! Gets a reference to the "args" JSON object on the event description data
        EventObject& GetArgs() &;
        const EventObject& GetArgs() const&;
        EventObject&& GetArgs() &&;

        void SetExtraParams(AZStd::span<EventField> extraParams);
        AZStd::span<EventField> GetExtraParams() const;
    private:

        //! Name of the Event
        AZStd::string_view m_name;
        //! Comma separated list of categories associated with the event
        AZStd::string_view m_cat;
        //! Id to associate with the event
        AZStd::optional<AZStd::string_view> m_id;
        //! Timestamp in microseconds when event occurs
        AZStd::chrono::microseconds m_ts;
        //! Thread timestamp of the event
        AZStd::optional<AZStd::chrono::microseconds> m_tts;
        //! Process Id of application
        AZ::Platform::ProcessId m_pid;
        //! Thread Id of thread which recorded the event
        AZStd::thread::id m_tid;

        //! Default value of an Event Header is a phase event
        //! This will be populated by the EventLogger
        EventPhase m_phase{ EventPhase::Complete };


        //! fields that will be written to the "args" JSON object when the event data is recorded.
        //! It defaults to a JSON object with no fields
        EventField m_argsField;

        //! A span of event parameters that is custom to the event being logged
        //! For example the complete event supports an additional "dur" and "tdur" field
        //! Which is populated with the tracing clock duration in microseconds and thread tracing clock duration
        AZStd::span<EventField> m_extraParams;
    };


    //! Base structure which represents the common args that event record event function accepts
    struct EventArgs
    {
        //! Name of the Event
        AZStd::string_view m_name;
        //! Comma separated list of categories associated with the event
        AZStd::string_view m_cat;
        //! Arguments used to fill the "args" field for each trace event
        AZStd::span<EventField> m_args;
    };

    //! Structure which represents arguments associated with the duration trace events
    //! Duration events come in pairs(Begin and End), which can be used to mark a duration of time
    //! that has passed on single thrad
    //! https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview#heading=h.nso4gcezn7n1
    struct DurationArgs
        : EventArgs
    {
        //! Id to associate with the event
        AZStd::optional<AZStd::string_view> m_id;
    };

    //! Structure which represents arguments associated with the complete trace events
    //! Complete events combines a pair of duration begin and end events into a single event
    //! It should be used to record an duration of time that has passed
    //! https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview#heading=h.lpfof2aylapb
    struct CompleteArgs
        : DurationArgs
    {
        //! Specifies the duration of the event in microseconds
        AZStd::chrono::microseconds m_dur;

        //! (Optional) Specifies the thread clock duration of the event in microseconds
        AZStd::optional<AZStd::chrono::microseconds> m_tdur;
    };

    //! Enums used to populate the scope key for instant events
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(InstantEventScope, char,
        (Global, 'g'),
        (Process, 'p'),
        (Thread, 't')
    );

    //! Structure which represents arguments associated with the complete instant events
    //! Instant events are used to record an event that has no duration associated with it
    //! https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview#heading=h.lenwiilchoxp
    struct InstantArgs
        : EventArgs
    {
        //! Id to associate with the event
        AZStd::optional<AZStd::string_view> m_id;
        //! Specifies the scope of the event
        //! Valid values are specified in the scope enum
        InstantEventScope m_scope{ InstantEventScope::Thread };
    };

    //! Structure which represents arguments associated with counter trace events
    //! Counter events should be used track values as they change over time
    //! https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview#heading=h.msg3086636uq
    struct CounterArgs
        : EventArgs
    {
        //! Id to associate with the event
        AZStd::optional<AZStd::string_view> m_id;
    };

    //! Structure for containing arguments for async start and end events, as well as async instant events
    //! Async events should be used to track asynchonous events, such as frames or asynchronous I/O
    //! https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview#heading=h.jh64i9l3vwa1
    struct AsyncArgs
        : EventArgs
    {
        //! Id to associate with the async event
        //! NOTE: ID is required in this case
        AZStd::string_view m_id;
        //! Scope string used to resolve conflicts when IDs are the same
        AZStd::optional<AZStd::string_view> m_scope;
    };

    //! IEventLogger API which supports recording event
    class IEventLogger
    {
    public:
        AZ_RTTI(IEventLogger, "{D39D09FA-DEA0-4874-BC45-4B310C3DD52E}");
        virtual ~IEventLogger() = default;

        //! Provides a qualified name for the Event Logger
        //! This is used to as part of the for the "/O3DE/Metrics/<Name>"
        //! to contain settings associated with any event logger with the name
        virtual void SetName([[maybe_unused]] AZStd::string_view name) {}

        //! Returns the qualified name for the Event Logger
        //! This can be used to query settings from event loggers with the name
        //! through queury the "/O3DE/Metrics/<Name>" object
        virtual AZStd::string_view GetName() const { return {}; }

        //! Provides a hook for implemented Event Loggers to flush recorded metrics
        //! to an associated stream(disk stream, network stream, etc...)
        virtual void Flush() = 0;

        //! Outcome which is populated with an error message should the event recording fail
        using ErrorString = AZStd::fixed_string<256>;
        using ResultOutcome = AZ::Outcome<void, ErrorString>;
        //! Records a begin duration event
        //! Uses the event header to populate the event fields
        virtual ResultOutcome RecordDurationEventBegin(const DurationArgs&) = 0;
        //! Records an end duration event
        //! Uses the event header to populate the event fields
        virtual ResultOutcome RecordDurationEventEnd(const DurationArgs&) = 0;

        //! Records a complete event(encapsulates a beging and end duration event)
        //! Uses the event header to populate the event fields
        virtual ResultOutcome RecordCompleteEvent(const CompleteArgs&) = 0;

        //! Records an instant event
        //! Uses the event header to populate the event fields
        virtual ResultOutcome RecordInstantEvent(const InstantArgs&) = 0;

        //! Records a Counter
        //! Uses the event header to populate the event fields
        virtual ResultOutcome RecordCounterEvent(const CounterArgs&) = 0;

        //! Records a start async event
        //! Uses the event header to populate the event fields
        virtual ResultOutcome RecordAsyncEventStart(const AsyncArgs&) = 0;
        //! Records an instant async event
        //! Uses the event header to populate the event fields
        virtual ResultOutcome RecordAsyncEventInstant(const AsyncArgs&) = 0;
        //! Records an end async event
        //! Uses the event header to populate the event fields
        virtual ResultOutcome RecordAsyncEventEnd(const AsyncArgs&) = 0;
    };
} // namespace AZ::Metrics

#include "IEventLogger.inl"
