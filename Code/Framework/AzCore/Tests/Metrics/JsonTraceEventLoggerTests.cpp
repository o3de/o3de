/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/ranges/filter_view.h>
#include <AzCore/std/ranges/zip_view.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class JsonTraceEventLoggerTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    };

    bool JsonStringContains(AZStd::string_view lhs, AZStd::string_view rhs)
    {
        auto TrimWhitespace = [](char element) -> bool
        {
            return !::isspace(element);
        };
        return AZStd::ranges::contains_subrange(lhs | AZStd::views::filter(TrimWhitespace),
            rhs | AZStd::views::filter(TrimWhitespace));
    }

    size_t JsonStringCountAll(AZStd::string_view sourceView, AZStd::string_view containsView)
    {
        auto TrimWhitespace = [](char element) -> bool
        {
            return !::isspace(element);
        };

        // Need persistent storage in this case to count all occurrences of JSON string inside of source string
        // Remove all whitespace from both strings
        AZStd::string sourceString(AZStd::from_range, sourceView | AZStd::views::filter(TrimWhitespace));
        AZStd::string containsString(AZStd::from_range, containsView | AZStd::views::filter(TrimWhitespace));

        size_t count{};
        AZStd::ranges::borrowed_subrange_t<decltype(sourceString)&> remainingRange(sourceString);
        for (;;)
        {
            auto foundRange = AZStd::ranges::search(remainingRange, containsString);
            if (foundRange.empty())
            {
                break;
            }

            // If the contains string has been found reduce the
            // remaining search range to be the end of the found string until the end of the source string
            remainingRange = { foundRange.end(), sourceString.end() };
            ++count;
        }

        return count;
    }

    TEST_F(JsonTraceEventLoggerTest, RecordDurationEvent_ProperJsonOutput_ToStream)
    {
        constexpr AZStd::string_view eventBeginMessage = "Hello world";

        AZStd::string metricsOutput;
        auto metricsStream = AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::string>>(&metricsOutput);
        AZ::Metrics::JsonTraceEventLogger googleTraceLogger(AZStd::move(metricsStream));

        // Provides storage for the arguments supplied to the "args" structure
        using EventFieldStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 8>;
        EventFieldStorage argContainer{ {"Field1", eventBeginMessage} };

        AZ::Metrics::DurationArgs testArgs;

        testArgs.m_name = "StringEvent";
        testArgs.m_cat = "Test";
        testArgs.m_args = argContainer;
        using ResultOutcome = AZ::Metrics::IEventLogger::ResultOutcome;
        ResultOutcome resultOutcome = googleTraceLogger.RecordDurationEventBegin(testArgs);
        EXPECT_TRUE(resultOutcome);

        // Update test args
        constexpr AZStd::string_view eventEndMessage = "Goodbye World";
        argContainer = EventFieldStorage{ { "Field1", eventEndMessage } };

        resultOutcome = googleTraceLogger.RecordDurationEventEnd(testArgs);
        EXPECT_TRUE(resultOutcome);
        // Flush and closes the event stream
        // This completes the json array
        googleTraceLogger.ResetStream(nullptr);

        EXPECT_TRUE(JsonStringContains(metricsOutput, R"("name": "StringEvent")"));
        EXPECT_TRUE(JsonStringContains(metricsOutput, R"("cat": "Test")"));
        EXPECT_TRUE(JsonStringContains(metricsOutput, R"("ph": "B")"));

        EXPECT_TRUE(JsonStringContains(metricsOutput, R"("name": "StringEvent")"));
        EXPECT_TRUE(JsonStringContains(metricsOutput, R"("cat": "Test")"));
        EXPECT_TRUE(JsonStringContains(metricsOutput, R"("ph": "B")"));

        // Validate that the output is a valid json doucment
        rapidjson::Document validateDoc;
        rapidjson::ParseResult parseResult = validateDoc.Parse(metricsOutput.c_str());
        EXPECT_TRUE(parseResult) << R"(JSON parse error ")" << rapidjson::GetParseError_En(parseResult.Code())
            << R"(" at offset (%u))";
    }

    TEST_F(JsonTraceEventLoggerTest, RecordAllEvents_StringsFromMultipleThreads_WrittenToStream)
    {
        constexpr AZStd::string_view eventString = "Hello world";
        constexpr AZ::s64 eventInt64 = -2;
        constexpr AZ::u64 eventUint64 = 0xFFFF'0000'FFFF'FFFF;
        constexpr bool eventBool = true;
        constexpr double eventDouble = 64.0;

        constexpr auto objectFieldNames = AZStd::to_array<AZStd::string_view>({"Field1", "Field2", "Field3", "Field4", "Field5"});
        using EventArgsType = AZ::Metrics::EventValue::ArgsVariant;
        const auto objectFieldValues = AZStd::to_array<EventArgsType>({ eventString, eventInt64, eventUint64, eventBool, eventDouble });

        // Provides storage for the arguments supplied to the "args" structure
        using EventArrayStorage = AZStd::fixed_vector<AZ::Metrics::EventValue, 8>;
        using EventObjectStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 8>;
        EventArrayStorage eventArray;
        EventObjectStorage eventObject;

        // Fill out the child array and child object fields
        for (auto [fieldName, fieldValue] : AZStd::views::zip(objectFieldNames, objectFieldValues))
        {
            auto AppendArgs = [name = fieldName, &eventArray, &eventObject](auto&& value)
            {
                eventArray.push_back(value);
                eventObject.emplace_back(name, value);
            };

            AZStd::visit(AppendArgs, fieldValue);
        }

        // Populate the "args" container to associate with each events "args" field
        EventObjectStorage argsContainer;
        argsContainer.emplace_back("string", eventString);
        argsContainer.emplace_back("int64_t", eventInt64);
        argsContainer.emplace_back("uint64_t", eventUint64);
        argsContainer.emplace_back("bool", eventBool);
        argsContainer.emplace_back("double", eventDouble);
        argsContainer.emplace_back("array", AZ::Metrics::EventArray(eventArray));
        argsContainer.emplace_back("object", AZ::Metrics::EventObject(eventObject));

        // Create an byte container stream that allows event logger output to be logged in-memory
        AZStd::string metricsOutput;
        auto metricsStream = AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::string>>(&metricsOutput);

        // Create the trace event logger that logs to the Google Trace Event format
        AZ::Metrics::JsonTraceEventLogger googleTraceLogger(AZStd::move(metricsStream));


        using ResultOutcome = AZ::Metrics::IEventLogger::ResultOutcome;

        // Defer logging until after all threads have been started
        AZStd::atomic_bool startLogging{};

        auto LogAllEvents = [&startLogging, &googleTraceLogger, &argsContainer](int threadIndex)
        {
            // Fake timestamp to use for complete event
            AZStd::chrono::utc_clock::time_point startThreadTime = AZStd::chrono::utc_clock::now();
            while (!startLogging)
            {
                AZStd::this_thread::yield();
            }

            // Convert the threadIndex to a string and use that as the "id" value
            AZStd::fixed_string<32> idString;
            AZStd::to_string(idString, threadIndex);

            ResultOutcome resultOutcome(AZStd::unexpect, AZ::Metrics::IEventLogger::ErrorString("Uninitialized"));
            {
                // Record Duration Begin and End Events
                AZ::Metrics::DurationArgs durationArgs;
                durationArgs.m_name = "Duration Event";
                durationArgs.m_cat = "Test";
                durationArgs.m_args = argsContainer;
                durationArgs.m_id = idString;
                resultOutcome = googleTraceLogger.RecordDurationEventBegin(durationArgs);
                EXPECT_TRUE(resultOutcome);

                resultOutcome = googleTraceLogger.RecordDurationEventEnd(durationArgs);
                EXPECT_TRUE(resultOutcome);
            }

            {
                // Record Complete Event
                auto duration = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::utc_clock::now() - startThreadTime);
                AZ::Metrics::CompleteArgs completeArgs;
                completeArgs.m_name = "Complete Event";
                completeArgs.m_cat = "Test";
                completeArgs.m_dur = duration;
                completeArgs.m_args = argsContainer;
                completeArgs.m_id = idString;
                resultOutcome = googleTraceLogger.RecordCompleteEvent(completeArgs);
                EXPECT_TRUE(resultOutcome);
            }

            {
                // Record Instant Event
                AZ::Metrics::InstantArgs instantArgs;
                instantArgs.m_name = "Instant Event";
                instantArgs.m_cat = "Test";
                instantArgs.m_args = argsContainer;
                instantArgs.m_id = idString;
                instantArgs.m_scope = AZ::Metrics::InstantEventScope::Thread;
                resultOutcome = googleTraceLogger.RecordInstantEvent(instantArgs);
                EXPECT_TRUE(resultOutcome);
            }

            {
                // Record Instant Event
                // Add an extra object field for a count by making a copy of the argsContainer
                auto extendedArgs = argsContainer;
                extendedArgs.emplace_back("frameTime", AZ::Metrics::EventValue{ AZStd::in_place_type<AZ::s64>, 16 + threadIndex });
                AZ::Metrics::CounterArgs counterArgs;
                counterArgs.m_name = "Counter Event";
                counterArgs.m_cat = "Test";
                counterArgs.m_args = extendedArgs;
                counterArgs.m_id = idString;
                resultOutcome = googleTraceLogger.RecordCounterEvent(counterArgs);
                EXPECT_TRUE(resultOutcome);
            }

            {
                // Record Async Start and End Events
                // Also records the Async Instant event
                constexpr AZStd::string_view asyncOuterEventName = "Async Event";

                AZ::Metrics::AsyncArgs asyncArgs;
                asyncArgs.m_name = asyncOuterEventName;
                asyncArgs.m_cat = "Test";
                asyncArgs.m_args = argsContainer;
                asyncArgs.m_id = idString;
                asyncArgs.m_scope = "Distinguishing Scope";
                resultOutcome = googleTraceLogger.RecordAsyncEventStart(asyncArgs);
                EXPECT_TRUE(resultOutcome);

                // Change the name of the event
                asyncArgs.m_name = "Async Instant Event";
                resultOutcome = googleTraceLogger.RecordAsyncEventInstant(asyncArgs);
                EXPECT_TRUE(resultOutcome);

                // "Async Event" is the logical name of the being and end event being recorded
                // So make sure the end event matches
                asyncArgs.m_name = asyncOuterEventName;
                resultOutcome = googleTraceLogger.RecordAsyncEventEnd(asyncArgs);
                EXPECT_TRUE(resultOutcome);
            }
        };

        // Spin up totalThreads for testing logging from multiple threads
        constexpr size_t totalThreads = 4;
        // Used for an id value for an event
        int32_t currentThreadIndex{};
        AZStd::array<AZStd::thread, totalThreads> threads;
        for (AZStd::thread& threadRef : threads)
        {
            threadRef = AZStd::thread(LogAllEvents, currentThreadIndex++);
        }

        // Start logging of events
        startLogging = true;

        // Join the threads. This should complete the event recording
        for (size_t threadIndex = 0; threadIndex < totalThreads; ++threadIndex)
        {
            threads[threadIndex].join();
        }

        // Flush and closes the event stream
        // This completes the json array
        googleTraceLogger.ResetStream(nullptr);

        // Count should be 32.
        // There are 8 variations of events (1. Duration Begin, 2. Duration End, 3. Complete, 4. Instant,
        // 5. Counter, 6. Async Start, 7. Async End, 8. Async Instant
        // Times 4 threads being run
        EXPECT_EQ(32, JsonStringCountAll(metricsOutput, R"("string": "Hello world")"));
        EXPECT_EQ(32, JsonStringCountAll(metricsOutput, R"("int64_t": -2)"));
        EXPECT_EQ(32, JsonStringCountAll(metricsOutput, R"("uint64_t": 18446462603027808255)"));
        EXPECT_EQ(32, JsonStringCountAll(metricsOutput, R"("bool": true)"));
        EXPECT_EQ(32, JsonStringCountAll(metricsOutput, R"("double": 64.0)"));
        EXPECT_EQ(32, JsonStringCountAll(metricsOutput, R"("array": [)"));
        EXPECT_EQ(32, JsonStringCountAll(metricsOutput, R"("object": {)"));


        // Validate that the output is a valid json doucment
        rapidjson::Document validateDoc;
        rapidjson::ParseResult parseResult = validateDoc.Parse(metricsOutput.c_str());
        EXPECT_TRUE(parseResult) << R"(JSON parse error ")" << rapidjson::GetParseError_En(parseResult.Code())
            << R"(" at offset (%u))";

        // Log the metrics output to stdout so it appears for visualizing purposes
        metricsOutput += '\n';
        AZ::Debug::Trace::Instance().RawOutput("metrics", metricsOutput.c_str());
    }
} // namespace UnitTest


#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    class JsonTraceEventLoggerBenchmarkFixture
        : public ::UnitTest::AllocatorsBenchmarkFixture
    {
    protected:
        void RecordMetricsToStream(AZStd::unique_ptr<AZ::IO::GenericStream>);

    protected:
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;
    };

    void JsonTraceEventLoggerBenchmarkFixture::RecordMetricsToStream(AZStd::unique_ptr<AZ::IO::GenericStream>)
    {
        constexpr AZStd::string_view eventString = "Hello world";
        constexpr AZ::s64 eventInt64 = -2;
        constexpr AZ::u64 eventUint64 = 0xFFFF'0000'FFFF'FFFF;
        constexpr bool eventBool = true;
        constexpr double eventDouble = 64.0;

        constexpr auto objectFieldNames = AZStd::to_array<AZStd::string_view>({ "Field1", "Field2", "Field3", "Field4", "Field5" });
        using EventArgsType = AZ::Metrics::EventValue::ArgsVariant;
        const auto objectFieldValues = AZStd::to_array<EventArgsType>({ eventString, eventInt64, eventUint64, eventBool, eventDouble });

        // Provides storage for the arguments supplied to the "args" structure
        using EventArrayStorage = AZStd::fixed_vector<AZ::Metrics::EventValue, 8>;
        using EventObjectStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 8>;
        EventArrayStorage eventArray;
        EventObjectStorage eventObject;

        // Fill out the child array and child object fields
        for (auto [fieldName, fieldValue] : AZStd::views::zip(objectFieldNames, objectFieldValues))
        {
            auto AppendArgs = [name = fieldName, &eventArray, &eventObject](auto&& value)
            {
                eventArray.push_back(value);
                eventObject.emplace_back(name, value);
            };

            AZStd::visit(AppendArgs, fieldValue);
        }

        // Populate the "args" container to associate with each events "args" field
        EventObjectStorage argsContainer;
        argsContainer.emplace_back("string", eventString);
        argsContainer.emplace_back("int64_t", eventInt64);
        argsContainer.emplace_back("uint64_t", eventUint64);
        argsContainer.emplace_back("bool", eventBool);
        argsContainer.emplace_back("double", eventDouble);
        argsContainer.emplace_back("array", AZ::Metrics::EventArray(eventArray));
        argsContainer.emplace_back("object", AZ::Metrics::EventObject(eventObject));

        // Create an byte container stream that allows event logger output to be logged in-memory
        AZStd::string metricsOutput;
        auto metricsStream = AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::string>>(&metricsOutput);

        // Create the trace event logger that logs to the Google Trace Event format
        AZ::Metrics::JsonTraceEventLogger googleTraceLogger(AZStd::move(metricsStream));

        auto LogAllEvents = [&googleTraceLogger, &argsContainer](int idValue)
        {
            // Fake timestamp to use for complete event
            AZStd::chrono::utc_clock::time_point startThreadTime = AZStd::chrono::utc_clock::now();
            // Convert the threadIndex to a string and use that as the "id" value
            AZStd::fixed_string<32> idString;
            AZStd::to_string(idString, idValue);

            using ResultOutcome = AZ::Metrics::IEventLogger::ResultOutcome;
            [[maybe_unused]] ResultOutcome resultOutcome(AZStd::unexpect, AZ::Metrics::IEventLogger::ErrorString("Uninitialized"));
            {
                // Record Duration Begin and End Events
                AZ::Metrics::DurationArgs durationArgs;
                durationArgs.m_name = "Duration Event";
                durationArgs.m_cat = "Test";
                durationArgs.m_args = argsContainer;
                durationArgs.m_id = idString;
                resultOutcome = googleTraceLogger.RecordDurationEventBegin(durationArgs);

                resultOutcome = googleTraceLogger.RecordDurationEventEnd(durationArgs);
            }

            {
                // Record Complete Event
                auto duration = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::utc_clock::now() - startThreadTime);
                AZ::Metrics::CompleteArgs completeArgs;
                completeArgs.m_name = "Complete Event";
                completeArgs.m_cat = "Test";
                completeArgs.m_dur = duration;
                completeArgs.m_args = argsContainer;
                completeArgs.m_id = idString;
                resultOutcome = googleTraceLogger.RecordCompleteEvent(completeArgs);
            }

            {
                // Record Instant Event
                AZ::Metrics::InstantArgs instantArgs;
                instantArgs.m_name = "Instant Event";
                instantArgs.m_cat = "Test";
                instantArgs.m_args = argsContainer;
                instantArgs.m_id = idString;
                instantArgs.m_scope = AZ::Metrics::InstantEventScope::Thread;
                resultOutcome = googleTraceLogger.RecordInstantEvent(instantArgs);
            }

            {
                // Record Complete Event
                // Add an extra object field for a count by making a copy of the argsContainer
                auto extendedArgs = argsContainer;
                extendedArgs.emplace_back("frameTime", AZ::Metrics::EventValue{ AZStd::in_place_type<AZ::s64>, 20 });
                AZ::Metrics::CounterArgs counterArgs;
                counterArgs.m_name = "Counter Event";
                counterArgs.m_cat = "Test";
                counterArgs.m_args = extendedArgs;
                counterArgs.m_id = idString;
                resultOutcome = googleTraceLogger.RecordCounterEvent(counterArgs);
            }

            {
                // Record Async Start and End Events
                // Also records the Async Instant event
                constexpr AZStd::string_view asyncOuterEventName = "Async Event";

                AZ::Metrics::AsyncArgs asyncArgs;
                asyncArgs.m_name = asyncOuterEventName;
                asyncArgs.m_cat = "Test";
                asyncArgs.m_args = argsContainer;
                asyncArgs.m_id = idString;
                asyncArgs.m_scope = "Distinguishing Scope";
                resultOutcome = googleTraceLogger.RecordAsyncEventStart(asyncArgs);

                // Change the name of the event
                asyncArgs.m_name = "Async Instant Event";
                resultOutcome = googleTraceLogger.RecordAsyncEventInstant(asyncArgs);

                // "Async Event" is the logical name of the being and end event being recorded
                // So make sure the end event matches
                asyncArgs.m_name = asyncOuterEventName;
                resultOutcome = googleTraceLogger.RecordAsyncEventEnd(asyncArgs);
            }
        };

        LogAllEvents(0);
        // Flush and closes the event stream
        // This completes the json array
        googleTraceLogger.ResetStream(nullptr);
    }

    BENCHMARK_F(JsonTraceEventLoggerBenchmarkFixture, BM_JsonTraceEventLogger_RecordEventsToInMemoryStream)(benchmark::State& state)
    {
        constexpr size_t eventOutputReserveSize = 4096;
        AZStd::string metricsPayload;
        // Reserve memory to prevent allocation of string data skewing results
        // for byte container stream
        metricsPayload.reserve(eventOutputReserveSize);
        for ([[maybe_unused]] auto _ : state)
        {
            state.PauseTiming();
            // Do not time the creation of the stream
            auto stream = AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::string>>(&metricsPayload);
            state.ResumeTiming();
            RecordMetricsToStream(AZStd::move(stream));
            metricsPayload.clear();
        }
    }

    BENCHMARK_F(JsonTraceEventLoggerBenchmarkFixture, BM_JsonTraceEventLogger_RecordEventsToTemporaryDirUnbufferedFileStream)(benchmark::State& state)
    {
        // Open the file and truncate the file
        constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite;
        const AZ::IO::FixedMaxPath metricsFilepath = m_tempDirectory.GetDirectoryAsFixedMaxPath() / "metrics.json";

        for ([[maybe_unused]] auto _ : state)
        {
            state.PauseTiming();
            auto stream = AZStd::make_unique<AZ::IO::SystemFileStream>(metricsFilepath.c_str(), openMode);
            state.ResumeTiming();
            RecordMetricsToStream(AZStd::move(stream));
        }
    }
} // Benchmark
#endif

