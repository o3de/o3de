/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/JSON/RapidjsonAllocatorAdapter.h>
#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/ranges/repeat_view.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

namespace AZ::Metrics
{
    //! Used to enable TracePrintfs within the Json Event Logger class
    constexpr bool TraceLogging = false;

    constexpr AZStd::string_view NameKey = "name";
    constexpr AZStd::string_view CategoryKey = "cat";
    constexpr AZStd::string_view PhaseKey = "ph";
    constexpr AZStd::string_view IdKey = "id";
    constexpr AZStd::string_view TimestampKey = "ts";
    [[maybe_unused]] constexpr AZStd::string_view ThreadTimestampKey = "tts";
    constexpr AZStd::string_view ProcessIdKey = "pid";
    constexpr AZStd::string_view ThreadIdKey = "tid";

    // Format strings for the TraceEvent Format
    constexpr AZStd::string_view ArrayStart = "[";
    constexpr AZStd::string_view ArrayEnd = "]";
    constexpr AZStd::string_view Newline = "\n";
    constexpr AZStd::string_view CommaNewline = ",\n";

    constexpr int32_t IndentStep = 2;
    constexpr size_t StackAllocatorSize = 2048;

    using EventJsonWriter = rapidjson::Writer<AZ::IO::RapidJSONWriteStreamUnbuffered, rapidjson::UTF8<char>, rapidjson::UTF8<char>,
        AZ::Json::RapidjsonStackAllocator<StackAllocatorSize>>;

    static void AppendNewlineWithIndent(JsonTraceEventLogger::JsonEventString& eventString, int32_t indent, bool prependComma)
    {
        eventString += prependComma ? CommaNewline : Newline;
        eventString.append_range(AZStd::views::repeat(' ', indent));
    }

    // Lambdas can't exactly refer to themselves recursively as they are unnamed
    // Workarounds such as binding a std::function which is set to a lambda
    // at to lambda at a later point in time is possible
    // But std::function can't be constexpr
    struct VisitField
    {
        template<class T>
        constexpr void operator()(T&& fieldValue) const
        {
            using FieldType = AZStd::remove_cvref_t<T>;

            if constexpr (AZStd::same_as<FieldType, AZStd::string_view>)
            {
                // rapidjson doesn't correct check the size before deferning the pointer
                // So it will deference nullptr even when the size is 0
                AZStd::string_view value = !fieldValue.empty() ? fieldValue : "";
                AZ_Verify(m_jsonWriter.String(value.data(), static_cast<rapidjson::SizeType>(value.size())),
                    R"(Failed to write string with value "%.*s" to event string)", AZ_STRING_ARG(value));
            }
            else if constexpr (AZStd::same_as<FieldType, bool>)
            {
                AZ_Verify(m_jsonWriter.Bool(fieldValue), "Failed to write double with bool %s to event string",
                    fieldValue ? "true" : "false");
            }
            else if constexpr (AZStd::same_as<FieldType, AZ::s64>)
            {
                AZ_Verify(m_jsonWriter.Int64(fieldValue), "Failed to write int64_t with value %lld to event string", fieldValue);
            }
            else if constexpr (AZStd::same_as<FieldType, AZ::u64>)
            {
                AZ_Verify(m_jsonWriter.Uint64(fieldValue), "Failed to write int64_t with value %lld to event string", fieldValue);
            }
            else if constexpr (AZStd::same_as<FieldType, double>)
            {
                AZ_Verify(m_jsonWriter.Double(fieldValue), "Failed to write double with value %f to event string", fieldValue);
            }
            else if constexpr (AZStd::same_as<FieldType, EventArray>)
            {
                // Recursive call to visit the array fields
                auto VisitChildFields = [this](const EventValue& entryValue)
                {
                    AZStd::visit(*this, entryValue.m_value);
                };
                AZ_Verify(m_jsonWriter.StartArray(), "Failed to start a new array for event string");
                AZStd::ranges::for_each(fieldValue.GetArrayValues(), VisitChildFields);
                AZ_Verify(m_jsonWriter.EndArray(), "Failed to end array for event string");
            }
            else if constexpr (AZStd::same_as<FieldType, EventObject>)
            {
                // Recursive call to visit the object fields
                auto VisitChildFields = [this](const EventField& entryField)
                {
                    AZ_Verify(m_jsonWriter.Key(entryField.m_name.data(), static_cast<rapidjson::SizeType>(entryField.m_name.size())),
                        R"(Unable to add child field key "%.*s" to event string)");
                    AZStd::visit(*this, entryField.m_value.m_value);
                };

                AZ_Verify(m_jsonWriter.StartObject(), "Failed to start a new object for event string");
                AZStd::ranges::for_each(fieldValue.GetObjectFields(), VisitChildFields);
                AZ_Verify(m_jsonWriter.EndObject(), "Failed to end object for event string");
            }
        }
        EventJsonWriter& m_jsonWriter;
    };

    static void AppendEventField(EventJsonWriter& jsonWriter, const EventField& field)
    {
        VisitField fieldVisitor{ jsonWriter };

        AZStd::string_view fieldName = !field.m_name.empty() ? field.m_name : "";
        // Append the field name
        AZ_Verify(jsonWriter.Key(fieldName.data(), static_cast<rapidjson::SizeType>(fieldName.size())),
            R"(Unable to add key "%.*s" to trace event string)", AZ_STRING_ARG(fieldName));
        AZStd::visit(fieldVisitor, field.m_value.m_value);
    }

    JsonTraceEventLogger::JsonTraceEventLogger() = default;

    JsonTraceEventLogger::~JsonTraceEventLogger()
    {
        // Swaps any associated stream with a nullptr
        // and completes the json array by writing the trailing '] character
        ResetStream(nullptr);
    }

    JsonTraceEventLogger::JsonTraceEventLogger(JsonTraceLoggerEventConfig config)
        : JsonTraceEventLogger(nullptr, AZStd::move(config))
    {
    }

    JsonTraceEventLogger::JsonTraceEventLogger(AZStd::unique_ptr<AZ::IO::GenericStream> stream)
        : JsonTraceEventLogger(AZStd::move(stream), JsonTraceLoggerEventConfig{})
    {
    }

    JsonTraceEventLogger::JsonTraceEventLogger(AZStd::unique_ptr<AZ::IO::GenericStream> stream,
        JsonTraceLoggerEventConfig config)
        : m_stream(AZStd::move(stream))
        , m_name(AZStd::move(config.m_loggerName))
        , m_settingsRegistry{ config.m_settingsRegistry }
    {
        ResetSettingsHandler();
        if (m_stream != nullptr)
        {
            Start(*m_stream);
        }
    }


    // Static function which is used to initialize the active state of this JsonTraceEventLogger
    // based on the build configuration
    bool JsonTraceEventLogger::GetDefaultActiveState()
    {
#if !defined(AZ_RELEASE_BUILD)
        return true;
#else
        return false;
#endif
    }

    void JsonTraceEventLogger::SetName(AZStd::string_view name)
    {
        // Detect if the name has changed and reset
        // the active state and the settings handler if so
        const bool nameChanged = m_name != name;
        m_name = name;
        if (nameChanged)
        {
            // Reset the settings handler if the name has changed
            ResetSettingsHandler();
        }
    }

    AZStd::string_view JsonTraceEventLogger::GetName() const
    {
        return m_name;
    }

    void JsonTraceEventLogger::Flush()
    {
        // No-op - The data is written directly to the stream
    }

    auto JsonTraceEventLogger::RecordDurationEventBegin(const DurationArgs& durationArgs) -> ResultOutcome
    {
        if (!m_active)
        {
            // Event logger isn't active, return success
            return AZ::Success();
        }

        if (m_stream == nullptr)
        {
            return AZ::Failure(ErrorString("Logger has no output stream associated. The duration begin event cannot be recorded"));
        }

        // First make an EventDesc out of the arguments
        EventDesc eventDesc;
        eventDesc.SetName(durationArgs.m_name);
        eventDesc.SetCategory(durationArgs.m_cat);
        eventDesc.SetEventPhase(EventPhase::DurationBegin);
        eventDesc.SetProcessId(AZ::Platform::GetCurrentProcessId());
        eventDesc.SetThreadId(AZStd::this_thread::get_id());
        auto utcTimestamp = AZStd::chrono::utc_clock::now();
        eventDesc.SetTimestamp(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(utcTimestamp.time_since_epoch()));
        eventDesc.SetArgs(durationArgs.m_args);

        // The "id" field is optional for a duration event
        eventDesc.SetId(durationArgs.m_id);

        if (FlushRequest(eventDesc))
        {
            return AZ::Success();
        }

        return AZ::Failure(ErrorString("Logger has failed to flush duration begin event to stream"));
    }

    auto JsonTraceEventLogger::RecordDurationEventEnd(const DurationArgs& durationArgs) -> ResultOutcome
    {
        if (!m_active)
        {
            // Event logger isn't active, return success
            return AZ::Success();
        }

        if (m_stream == nullptr)
        {
            return AZ::Failure(ErrorString("Logger has no output stream associated. The duration end event cannot be recorded"));
        }

        // First make an EventDesc out of the arguments
        EventDesc eventDesc;
        eventDesc.SetName(durationArgs.m_name);
        eventDesc.SetCategory(durationArgs.m_cat);
        eventDesc.SetEventPhase(EventPhase::DurationEnd);
        eventDesc.SetProcessId(AZ::Platform::GetCurrentProcessId());
        eventDesc.SetThreadId(AZStd::this_thread::get_id());
        auto utcTimestamp = AZStd::chrono::utc_clock::now();
        eventDesc.SetTimestamp(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(utcTimestamp.time_since_epoch()));
        eventDesc.SetArgs(durationArgs.m_args);

        // The "id" field is optional for a duration event
        eventDesc.SetId(durationArgs.m_id);

        if (FlushRequest(eventDesc))
        {
            return AZ::Success();
        }

        return AZ::Failure(ErrorString("Logger has failed to flush duration end event to stream"));
    }

    auto JsonTraceEventLogger::RecordCompleteEvent(const CompleteArgs& completeArgs) -> ResultOutcome
    {
        if (!m_active)
        {
            return AZ::Success();
        }

        if (m_stream == nullptr)
        {
            return AZ::Failure(ErrorString("Logger has no output stream associated. The complete event cannot be recorded"));
        }

        // First make an EventDesc out of the arguments
        EventDesc eventDesc;
        eventDesc.SetName(completeArgs.m_name);
        eventDesc.SetCategory(completeArgs.m_cat);
        eventDesc.SetEventPhase(EventPhase::Complete);
        eventDesc.SetProcessId(AZ::Platform::GetCurrentProcessId());
        eventDesc.SetThreadId(AZStd::this_thread::get_id());
        auto utcTimestamp = AZStd::chrono::utc_clock::now();
        eventDesc.SetTimestamp(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(utcTimestamp.time_since_epoch()));
        eventDesc.SetArgs(completeArgs.m_args);

        // The "id" field is optional for a complete event
        eventDesc.SetId(completeArgs.m_id);

        // Add the extra complete event parameters for the duration and thread duration if set
        constexpr AZStd::string_view DurationKey = "dur";
        constexpr AZStd::string_view ThreadDurationKey = "tdur";
        constexpr size_t MaxExtraFieldCount = 8;
        AZStd::fixed_vector<EventField, MaxExtraFieldCount> extraParams;
        extraParams.emplace_back(DurationKey, EventValue{ AZStd::in_place_type<AZ::s64>, completeArgs.m_dur.count() });
        if (completeArgs.m_tdur)
        {
            extraParams.emplace_back(ThreadDurationKey, EventValue{ AZStd::in_place_type<AZ::s64>, completeArgs.m_tdur->count() });
        }

        eventDesc.SetExtraParams(extraParams);

        if (FlushRequest(eventDesc))
        {
            return AZ::Success();
        }

        return AZ::Failure(ErrorString("Logger has failed to flush complete event to stream"));
    }

    auto JsonTraceEventLogger::RecordInstantEvent(const InstantArgs& instantArgs) -> ResultOutcome
    {
        if (!m_active)
        {
            return AZ::Success();
        }

        if (m_stream == nullptr)
        {
            return AZ::Failure(ErrorString("Logger has no output stream associated. The instant event cannot be recorded"));
        }

        // First make an EventDesc out of the arguments
        EventDesc eventDesc;
        eventDesc.SetName(instantArgs.m_name);
        eventDesc.SetCategory(instantArgs.m_cat);
        eventDesc.SetEventPhase(EventPhase::Instant);
        eventDesc.SetProcessId(AZ::Platform::GetCurrentProcessId());
        eventDesc.SetThreadId(AZStd::this_thread::get_id());
        auto utcTimestamp = AZStd::chrono::utc_clock::now();
        eventDesc.SetTimestamp(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(utcTimestamp.time_since_epoch()));
        eventDesc.SetArgs(instantArgs.m_args);

        // The "id" field is optional for an instant event
        eventDesc.SetId(instantArgs.m_id);

        // Add the extra instant event parameters for the duration and thread duration if set
        constexpr AZStd::string_view ScopeKey = "s";
        constexpr size_t MaxExtraFieldCount = 8;
        AZStd::fixed_vector<EventField, MaxExtraFieldCount> extraParams;
        char scopeChar = static_cast<char>(instantArgs.m_scope);
        extraParams.emplace_back(ScopeKey, EventValue{ AZStd::in_place_type<AZStd::string_view>, &scopeChar, 1 });

        eventDesc.SetExtraParams(extraParams);

        if (FlushRequest(eventDesc))
        {
            return AZ::Success();
        }

        return AZ::Failure(ErrorString("Logger has failed to flush instant event to stream"));
    }

    auto JsonTraceEventLogger::RecordCounterEvent(const CounterArgs& counterArgs) -> ResultOutcome
    {
        if (!m_active)
        {
            return AZ::Success();
        }

        if (m_stream == nullptr)
        {
            return AZ::Failure(ErrorString("Logger has no output stream associated. The counter event cannot be recorded"));
        }

        // First make an EventDesc out of the arguments
        EventDesc eventDesc;
        eventDesc.SetName(counterArgs.m_name);
        eventDesc.SetCategory(counterArgs.m_cat);
        eventDesc.SetEventPhase(EventPhase::Counter);
        eventDesc.SetProcessId(AZ::Platform::GetCurrentProcessId());
        eventDesc.SetThreadId(AZStd::this_thread::get_id());
        auto utcTimestamp = AZStd::chrono::utc_clock::now();
        eventDesc.SetTimestamp(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(utcTimestamp.time_since_epoch()));
        eventDesc.SetArgs(counterArgs.m_args);

        // The "id" field is optional for a counter event
        eventDesc.SetId(counterArgs.m_id);

        if (FlushRequest(eventDesc))
        {
            return AZ::Success();
        }

        return AZ::Failure(ErrorString("Logger has failed to flush counter event to stream"));
    }

    auto JsonTraceEventLogger::RecordAsyncEventStart(const AsyncArgs& asyncArgs) -> ResultOutcome
    {
        if (!m_active)
        {
            return AZ::Success();
        }

        if (m_stream == nullptr)
        {
            return AZ::Failure(ErrorString("Logger has no output stream associated. The async start event cannot be recorded"));
        }

        // First make an EventDesc out of the arguments
        EventDesc eventDesc;
        eventDesc.SetName(asyncArgs.m_name);
        eventDesc.SetCategory(asyncArgs.m_cat);
        eventDesc.SetEventPhase(EventPhase::AsyncStart);
        eventDesc.SetProcessId(AZ::Platform::GetCurrentProcessId());
        eventDesc.SetThreadId(AZStd::this_thread::get_id());
        auto utcTimestamp = AZStd::chrono::utc_clock::now();
        eventDesc.SetTimestamp(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(utcTimestamp.time_since_epoch()));
        eventDesc.SetArgs(asyncArgs.m_args);

        // The "id" field is required for an async event
        eventDesc.SetId(asyncArgs.m_id);

        // Support the optional scope field for async events
        constexpr AZStd::string_view ScopeKey = "scope";
        constexpr size_t MaxExtraFieldCount = 8;
        AZStd::fixed_vector<EventField, MaxExtraFieldCount> extraParams;
        if (asyncArgs.m_scope)
        {
            extraParams.emplace_back(ScopeKey, EventValue{ AZStd::in_place_type<AZStd::string_view>, *asyncArgs.m_scope });
        }

        eventDesc.SetExtraParams(extraParams);

        if (FlushRequest(eventDesc))
        {
            return AZ::Success();
        }

        return AZ::Failure(ErrorString("Logger has failed to flush async start event to stream"));
    }

    auto JsonTraceEventLogger::RecordAsyncEventInstant(const AsyncArgs& asyncArgs) -> ResultOutcome
    {
        if (!m_active)
        {
            return AZ::Success();
        }

        if (m_stream == nullptr)
        {
            return AZ::Failure(ErrorString("Logger has no output stream associated. The async instant event cannot be recorded"));
        }

        // First make an EventDesc out of the arguments
        EventDesc eventDesc;
        eventDesc.SetName(asyncArgs.m_name);
        eventDesc.SetCategory(asyncArgs.m_cat);
        eventDesc.SetEventPhase(EventPhase::AsyncInstant);
        eventDesc.SetProcessId(AZ::Platform::GetCurrentProcessId());
        eventDesc.SetThreadId(AZStd::this_thread::get_id());
        auto utcTimestamp = AZStd::chrono::utc_clock::now();
        eventDesc.SetTimestamp(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(utcTimestamp.time_since_epoch()));
        eventDesc.SetArgs(asyncArgs.m_args);

        // The "id" field is required for an async event
        eventDesc.SetId(asyncArgs.m_id);

        // Support the optional scope field for async events
        constexpr AZStd::string_view ScopeKey = "scope";
        constexpr size_t MaxExtraFieldCount = 8;
        AZStd::fixed_vector<EventField, MaxExtraFieldCount> extraParams;
        if (asyncArgs.m_scope)
        {
            extraParams.emplace_back(ScopeKey, EventValue{ AZStd::in_place_type<AZStd::string_view>, *asyncArgs.m_scope });
        }

        eventDesc.SetExtraParams(extraParams);

        if (FlushRequest(eventDesc))
        {
            return AZ::Success();
        }

        return AZ::Failure(ErrorString("Logger has failed to flush async instant event to stream"));
    }

    auto JsonTraceEventLogger::RecordAsyncEventEnd(const AsyncArgs& asyncArgs) -> ResultOutcome
    {
        if (!m_active)
        {
            return AZ::Success();
        }

        if (m_stream == nullptr)
        {
            return AZ::Failure(ErrorString("Logger has no output stream associated. The async end event cannot be recorded"));
        }

        // First make an EventDesc out of the arguments
        EventDesc eventDesc;
        eventDesc.SetName(asyncArgs.m_name);
        eventDesc.SetCategory(asyncArgs.m_cat);
        eventDesc.SetEventPhase(EventPhase::AsyncEnd);
        eventDesc.SetProcessId(AZ::Platform::GetCurrentProcessId());
        eventDesc.SetThreadId(AZStd::this_thread::get_id());
        auto utcTimestamp = AZStd::chrono::utc_clock::now();
        eventDesc.SetTimestamp(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(utcTimestamp.time_since_epoch()));
        eventDesc.SetArgs(asyncArgs.m_args);

        // The "id" field is required for an async event
        eventDesc.SetId(asyncArgs.m_id);

        // Support the optional scope field for async events
        constexpr AZStd::string_view ScopeKey = "scope";
        constexpr size_t MaxExtraFieldCount = 8;
        AZStd::fixed_vector<EventField, MaxExtraFieldCount> extraParams;
        if (asyncArgs.m_scope)
        {
            extraParams.emplace_back(ScopeKey, EventValue{ AZStd::in_place_type<AZStd::string_view>, *asyncArgs.m_scope });
        }

        eventDesc.SetExtraParams(extraParams);

        if (FlushRequest(eventDesc))
        {
            return AZ::Success();
        }

        return AZ::Failure(ErrorString("Logger has failed to flush async end event to stream"));
    }

    void JsonTraceEventLogger::ResetStream(AZStd::unique_ptr<AZ::IO::GenericStream> stream)
    {
        // Complete and close any previous stream
        // Take the flushToStream mutex to safely swap the streams
        {
            AZStd::scoped_lock flushLock(m_flushToStreamMutex);
            AZStd::swap(stream, m_stream);
        }

        // Flush
        if (stream != nullptr)
        {
            Complete(*stream);
        }

        if (m_stream != nullptr)
        {
            Start(*m_stream);
        }
    }

    bool JsonTraceEventLogger::FlushRequest(const EventDesc& eventDesc)
    {
        JsonEventString eventString;

        AZ::Json::RapidjsonStackAllocator<StackAllocatorSize> stackAllocator;
        AZ::IO::ByteContainerStream byteStream(&eventString);
        AZ::IO::RapidJSONWriteStreamUnbuffered rapidJsonStream(byteStream);

        EventJsonWriter jsonWriter(rapidJsonStream, &stackAllocator);

        // Start the event object
        jsonWriter.StartObject();
        AppendEventField(jsonWriter, EventField{ NameKey, EventValue{AZStd::in_place_type<AZStd::string_view>, eventDesc.GetName()} });

        if (auto eventId = eventDesc.GetId(); eventId.has_value())
        {
            AppendEventField(jsonWriter, EventField{ IdKey, EventValue{AZStd::in_place_type<AZStd::string_view>, *eventId} });
        }

        AppendEventField(jsonWriter, EventField{ CategoryKey, EventValue{AZStd::in_place_type<AZStd::string_view>, eventDesc.GetCategory()} });

        const char phaseChar = static_cast<char>(eventDesc.GetEventPhase());
        AppendEventField(jsonWriter, EventField{ PhaseKey, EventValue{AZStd::string_view{ &phaseChar, 1} } });

        AppendEventField(jsonWriter, EventField{ TimestampKey, EventValue{AZStd::in_place_type<AZ::s64>, eventDesc.GetTimestamp().count()} });

        AppendEventField(jsonWriter, EventField{ ProcessIdKey, EventValue{AZStd::in_place_type<AZ::u64>, eventDesc.GetProcessId()} });

        // Since only little_endian platforms are supported reinterprets the thread id as a uintptr_t type
        uintptr_t numericThreadId{};
        *reinterpret_cast<AZStd::thread_id*>(&numericThreadId) = eventDesc.GetThreadId();
        AppendEventField(jsonWriter, EventField{ ThreadIdKey, EventValue{AZStd::in_place_type<AZ::u64>, numericThreadId} });

        AppendEventField(jsonWriter, EventField{ ArgsKey, EventValue{AZStd::in_place_type<EventObject>, eventDesc.GetArgs()} });

        // Append the extra parameters associated with the event to the event string
        for (const EventField& extraField : eventDesc.GetExtraParams())
        {
            AppendEventField(jsonWriter, extraField);
        }

        // End the event object
        jsonWriter.EndObject();

        [[maybe_unused]] AZ::IO::SizeType totalBytesWritten{};
        [[maybe_unused]] size_t currentEventIndex{};

        bool result{};
        {
            AZStd::scoped_lock flushLock(m_flushToStreamMutex);
            // Prepend a comma to the stream if an existing event was written previously
            {
                JsonEventString eventSeparatorString;
                bool expected{};
                AppendNewlineWithIndent(eventSeparatorString, IndentStep, !m_prependComma.compare_exchange_strong(expected, true));
                totalBytesWritten += m_stream->Write(eventSeparatorString.size(), eventSeparatorString.data());
            }

            AZ::IO::SizeType eventBytesWritten = m_stream->Write(eventString.size(), eventString.data());
            currentEventIndex = m_eventCount++;
            // Validate the event has written eventString characters
            result = eventBytesWritten == eventString.size();
            totalBytesWritten += eventBytesWritten;
        }

        if constexpr (TraceLogging)
        {
            AZ_TracePrintf("EventLogger", R"("Event Logger with name "%s", wrote event at index %zu with of %zu bytes and name %.*s)",
                m_name.c_str(), currentEventIndex, totalBytesWritten, AZ_STRING_ARG(eventDesc.GetName()));
        }

        return result;
    }

    bool JsonTraceEventLogger::Start(AZ::IO::GenericStream& stream)
    {
        return stream.Write(ArrayStart.size(), ArrayStart.data()) == ArrayStart.size();
    }

    bool JsonTraceEventLogger::Complete(AZ::IO::GenericStream& stream)
    {
        constexpr auto jsonCompleteString = []()
        {
            auto result = AZStd::fixed_string<2>(Newline);
            result += ArrayEnd;
            return result;
        }();

        return stream.Write(jsonCompleteString.size(), jsonCompleteString.data()) == jsonCompleteString.size();
    }

    void JsonTraceEventLogger::ResetSettingsHandler()
    {
        // Reset the active option back to default active state based on the build configuration
        // and then query it from the Settings Registry again
        m_active = GetDefaultActiveState();

        if (auto settingsRegistry = m_settingsRegistry != nullptr ? m_settingsRegistry : AZ::SettingsRegistry::Get();
            settingsRegistry != nullptr)
        {
            // Read the "/O3DE/Metrics/<Name>/Active" setting from the Settings Registry
            const AZStd::fixed_string<128> eventLoggerActiveSettingKey(SettingsKey(m_name + "/Active"));
            settingsRegistry->Get(m_active, eventLoggerActiveSettingKey);

            auto ActiveStateUpdateFunc = [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyArgs)
            {
                const AZStd::fixed_string<128> activeSettingKey(SettingsKey(m_name + "/Active"));
                if (AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(notifyArgs.m_jsonKeyPath, activeSettingKey))
                {
                    if (auto settingsRegistry = m_settingsRegistry != nullptr ? m_settingsRegistry : AZ::SettingsRegistry::Get();
                        settingsRegistry != nullptr)
                    {
                        // If the key has been deleted, then reset the active state to the default active state
                        if (settingsRegistry->GetType(activeSettingKey).m_type == AZ::SettingsRegistryInterface::Type::NoType)
                        {
                            m_active = GetDefaultActiveState();
                        }
                        else
                        {
                            settingsRegistry->Get(m_active, activeSettingKey);
                        }
                    }
                }
            };
            m_settingsHandler = settingsRegistry->RegisterNotifier(ActiveStateUpdateFunc);
        }
    }

} // namespace AZ::Metrics
