/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace Benchmark
{
    class DomJsonBenchmark : public UnitTest::AllocatorsBenchmarkFixture
    {
    public:
        void SetUp(const ::benchmark::State& st) override
        {
            UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
            AZ::NameDictionary::Create();
        }

        void SetUp(::benchmark::State& st) override
        {
            UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
            AZ::NameDictionary::Create();
        }

        void TearDown(::benchmark::State& st) override
        {
            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
        }

        void TearDown(const ::benchmark::State& st) override
        {
            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
        }

        AZStd::string GenerateDomJsonBenchmarkPayload(int64_t entryCount, int64_t stringTemplateLength)
        {
            rapidjson::Document document;
            document.SetObject();

            AZStd::string entryTemplate;
            while (entryTemplate.size() < static_cast<size_t>(stringTemplateLength))
            {
                entryTemplate += "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor ";
            }
            entryTemplate.resize(stringTemplateLength);
            AZStd::string buffer;

            auto createString = [&](int n) -> rapidjson::Value
            {
                buffer = AZStd::string::format("#%i %s", n, entryTemplate.c_str());
                return rapidjson::Value(buffer.data(), static_cast<rapidjson::SizeType>(buffer.size()), document.GetAllocator());
            };

            auto createEntry = [&](int n) -> rapidjson::Value
            {
                rapidjson::Value entry(rapidjson::kObjectType);
                entry.AddMember("string", createString(n), document.GetAllocator());
                entry.AddMember("int", rapidjson::Value(n), document.GetAllocator());
                entry.AddMember("double", rapidjson::Value(static_cast<double>(n) * 0.5), document.GetAllocator());
                entry.AddMember("bool", rapidjson::Value(n % 2 == 0), document.GetAllocator());
                entry.AddMember("null", rapidjson::Value(rapidjson::kNullType), document.GetAllocator());
                return entry;
            };

            auto createArray = [&]() -> rapidjson::Value
            {
                rapidjson::Value array;
                array.SetArray();
                for (int i = 0; i < entryCount; ++i)
                {
                    array.PushBack(createEntry(i), document.GetAllocator());
                }
                return array;
            };

            auto createObject = [&]() -> rapidjson::Value
            {
                rapidjson::Value object;
                object.SetObject();
                for (int i = 0; i < entryCount; ++i)
                {
                    buffer = AZStd::string::format("Key%i", i);
                    rapidjson::Value key;
                    key.SetString(buffer.data(), static_cast<rapidjson::SizeType>(buffer.length()), document.GetAllocator());
                    object.AddMember(key.Move(), createArray(), document.GetAllocator());
                }
                return object;
            };

            document.SetObject();
            document.AddMember("entries", createObject(), document.GetAllocator());

            AZStd::string serializedJson;
            auto result = AZ::JsonSerializationUtils::WriteJsonString(document, serializedJson);
            AZ_Assert(result.IsSuccess(), "Failed to serialize generated JSON");
            return serializedJson;
        }
    };

// Helper macro for registering JSON benchmarks
#define BENCHMARK_REGISTER_JSON(BaseClass, Method)                                                                                         \
    BENCHMARK_REGISTER_F(BaseClass, Method)                                                                                                \
        ->Args({ 10, 5 })                                                                                                                  \
        ->Args({ 10, 500 })                                                                                                                \
        ->Args({ 100, 5 })                                                                                                                 \
        ->Args({ 100, 500 })                                                                                                               \
        ->Unit(benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(DomJsonBenchmark, DomDeserializeToDocumentInPlace)(benchmark::State& state)
    {
        AZ::Dom::JsonBackend backend;
        AZStd::string serializedPayload = GenerateDomJsonBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            state.PauseTiming();
            AZStd::string payloadCopy = serializedPayload;
            state.ResumeTiming();

            auto result = AZ::Dom::Json::WriteToRapidJsonDocument(
                [&](AZ::Dom::Visitor& visitor)
                {
                    return AZ::Dom::Utils::ReadFromStringInPlace(backend, payloadCopy, visitor);
                });

            benchmark::DoNotOptimize(result.GetValue());
        }

        state.SetBytesProcessed(serializedPayload.size() * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, DomDeserializeToDocumentInPlace)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, DomDeserializeToDocument)(benchmark::State& state)
    {
        AZ::Dom::JsonBackend backend;
        AZStd::string serializedPayload = GenerateDomJsonBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            auto result = AZ::Dom::Json::WriteToRapidJsonDocument(
                [&](AZ::Dom::Visitor& visitor)
                {
                    return AZ::Dom::Utils::ReadFromString(backend, serializedPayload, AZ::Dom::Lifetime::Temporary, visitor);
                });

            benchmark::DoNotOptimize(result.GetValue());
        }

        state.SetBytesProcessed(serializedPayload.size() * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, DomDeserializeToDocument)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, JsonUtilsDeserializeToDocument)(benchmark::State& state)
    {
        AZ::Dom::JsonBackend backend;
        AZStd::string serializedPayload = GenerateDomJsonBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            auto result = AZ::JsonSerializationUtils::ReadJsonString(serializedPayload);

            benchmark::DoNotOptimize(result.GetValue());
        }

        state.SetBytesProcessed(serializedPayload.size() * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, JsonUtilsDeserializeToDocument)

#undef BENCHMARK_REGISTER_JSON
} // namespace Benchmark

#endif // defined(HAVE_BENCHMARK)
