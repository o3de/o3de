/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/DomValue.h>
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
            AZ::AllocatorInstance<AZ::Dom::ValueAllocator>::Create();
        }

        void SetUp(::benchmark::State& st) override
        {
            UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
            AZ::NameDictionary::Create();
            AZ::AllocatorInstance<AZ::Dom::ValueAllocator>::Create();
        }

        void TearDown(::benchmark::State& st) override
        {
            AZ::AllocatorInstance<AZ::Dom::ValueAllocator>::Destroy();
            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
        }

        void TearDown(const ::benchmark::State& st) override
        {
            AZ::AllocatorInstance<AZ::Dom::ValueAllocator>::Destroy();
            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
        }

        rapidjson::Document GenerateDomJsonBenchmarkDocument(int64_t entryCount, int64_t stringTemplateLength)
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

            return document;
        }

        AZStd::string GenerateDomJsonBenchmarkPayload(int64_t entryCount, int64_t stringTemplateLength)
        {
            rapidjson::Document document = GenerateDomJsonBenchmarkDocument(entryCount, stringTemplateLength);

            AZStd::string serializedJson;
            auto result = AZ::JsonSerializationUtils::WriteJsonString(document, serializedJson);
            AZ_Assert(result.IsSuccess(), "Failed to serialize generated JSON");
            return serializedJson;
        }

        template <class T>
        void TakeAndDiscardWithoutTimingDtor(T&& value, benchmark::State& state)
        {
            {
                T instance = AZStd::move(value);
                state.PauseTiming();
            }
            state.ResumeTiming();
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

    BENCHMARK_DEFINE_F(DomJsonBenchmark, AzDomDeserializeToRapidjsonInPlace)(benchmark::State& state)
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

            TakeAndDiscardWithoutTimingDtor(result.TakeValue(), state);
        }

        state.SetBytesProcessed(serializedPayload.size() * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, AzDomDeserializeToRapidjsonInPlace)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, AzDomDeserializeToAzDomValueInPlace)(benchmark::State& state)
    {
        AZ::Dom::JsonBackend backend;
        AZStd::string serializedPayload = GenerateDomJsonBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            state.PauseTiming();
            AZStd::string payloadCopy = serializedPayload;
            state.ResumeTiming();

            auto result = AZ::Dom::Utils::WriteToValue(
                [&](AZ::Dom::Visitor& visitor)
                {
                    return AZ::Dom::Utils::ReadFromStringInPlace(backend, payloadCopy, visitor);
                });

            TakeAndDiscardWithoutTimingDtor(result.TakeValue(), state);
        }

        state.SetBytesProcessed(serializedPayload.size() * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, AzDomDeserializeToAzDomValueInPlace)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, AzDomDeserializeToRapidjson)(benchmark::State& state)
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

            TakeAndDiscardWithoutTimingDtor(result.TakeValue(), state);
        }

        state.SetBytesProcessed(serializedPayload.size() * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, AzDomDeserializeToRapidjson)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, AzDomDeserializeToAzDomValue)(benchmark::State& state)
    {
        AZ::Dom::JsonBackend backend;
        AZStd::string serializedPayload = GenerateDomJsonBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            auto result = AZ::Dom::Utils::WriteToValue(
                [&](AZ::Dom::Visitor& visitor)
                {
                    return AZ::Dom::Utils::ReadFromString(backend, serializedPayload, AZ::Dom::Lifetime::Temporary, visitor);
                });

            TakeAndDiscardWithoutTimingDtor(result.TakeValue(), state);
        }

        state.SetBytesProcessed(serializedPayload.size() * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, AzDomDeserializeToAzDomValue)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, RapidjsonDeserializeToRapidjson)(benchmark::State& state)
    {
        AZ::Dom::JsonBackend backend;
        AZStd::string serializedPayload = GenerateDomJsonBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            auto result = AZ::JsonSerializationUtils::ReadJsonString(serializedPayload);

            TakeAndDiscardWithoutTimingDtor(result.TakeValue(), state);
        }

        state.SetBytesProcessed(serializedPayload.size() * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, RapidjsonDeserializeToRapidjson)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, RapidjsonMakeComplexObject)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            TakeAndDiscardWithoutTimingDtor(GenerateDomJsonBenchmarkPayload(state.range(0), state.range(1)), state);
        }

        state.SetItemsProcessed(state.range(0) * state.range(0) * state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, RapidjsonMakeComplexObject)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, RapidjsonLookupMemberByString)(benchmark::State& state)
    {
        rapidjson::Document document(rapidjson::kObjectType);
        AZStd::vector<AZStd::string> keys;
        for (int64_t i = 0; i < state.range(0); ++i)
        {
            AZStd::string key(AZStd::string::format("key%" PRId64, i));
            keys.push_back(key);
            document.AddMember(rapidjson::Value(key.data(), static_cast<rapidjson::SizeType>(key.size()), document.GetAllocator()), rapidjson::Value(i), document.GetAllocator());
        }

        for (auto _ : state)
        {
            for (const AZStd::string& key : keys)
            {
                benchmark::DoNotOptimize(document.FindMember(key.data()));
            }
        }

        state.SetItemsProcessed(state.iterations() * state.range(0));
    }
    BENCHMARK_REGISTER_F(DomJsonBenchmark, RapidjsonLookupMemberByString)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(DomJsonBenchmark, RapidjsonDeepCopy)(benchmark::State& state)
    {
        rapidjson::Document original = GenerateDomJsonBenchmarkDocument(state.range(0), state.range(1));

        for (auto _ : state)
        {
            rapidjson::Document copy;
            copy.CopyFrom(original, copy.GetAllocator(), true);
            TakeAndDiscardWithoutTimingDtor(AZStd::move(copy), state);
        }

        state.SetItemsProcessed(state.iterations());
    }

    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, RapidjsonDeepCopy)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, RapidjsonCopyAndMutate)(benchmark::State& state)
    {
        rapidjson::Document original = GenerateDomJsonBenchmarkDocument(state.range(0), state.range(1));

        for (auto _ : state)
        {
            rapidjson::Document copy;
            copy.CopyFrom(original, copy.GetAllocator(), true);
            copy["entries"]["Key0"].PushBack(42, copy.GetAllocator());
            TakeAndDiscardWithoutTimingDtor(AZStd::move(copy), state);
        }

        state.SetItemsProcessed(state.iterations());
    }
    BENCHMARK_REGISTER_JSON(DomJsonBenchmark, RapidjsonCopyAndMutate)

#undef BENCHMARK_REGISTER_JSON
} // namespace Benchmark

#endif // defined(HAVE_BENCHMARK)
