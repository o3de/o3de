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
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Benchmark
{
    class DomJsonBenchmark : public Tests::DomBenchmarkFixture
    {
    };

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
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomJsonBenchmark, AzDomDeserializeToRapidjsonInPlace)

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
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomJsonBenchmark, AzDomDeserializeToAzDomValueInPlace)

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
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomJsonBenchmark, AzDomDeserializeToRapidjson)

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
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomJsonBenchmark, AzDomDeserializeToAzDomValue)

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
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomJsonBenchmark, RapidjsonDeserializeToRapidjson)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, RapidjsonMakeComplexObject)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            TakeAndDiscardWithoutTimingDtor(GenerateDomJsonBenchmarkPayload(state.range(0), state.range(1)), state);
        }

        state.SetItemsProcessed(state.range(0) * state.range(0) * state.iterations());
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomJsonBenchmark, RapidjsonMakeComplexObject)

    BENCHMARK_DEFINE_F(DomJsonBenchmark, RapidjsonLookupMemberByString)(benchmark::State& state)
    {
        rapidjson::Document document(rapidjson::kObjectType);
        AZStd::vector<AZStd::string> keys;
        for (int64_t i = 0; i < state.range(0); ++i)
        {
            AZStd::string key(AZStd::string::format("key%" PRId64, i));
            keys.push_back(key);
            document.AddMember(
                rapidjson::Value(key.data(), static_cast<rapidjson::SizeType>(key.size()), document.GetAllocator()), rapidjson::Value(i),
                document.GetAllocator());
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

    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomJsonBenchmark, RapidjsonDeepCopy)

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
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomJsonBenchmark, RapidjsonCopyAndMutate)

} // namespace AZ::Dom::Benchmark

#endif // defined(HAVE_BENCHMARK)
