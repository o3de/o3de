/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomValue.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <cinttypes>

namespace AZ::Dom::Benchmark
{
    class DomValueBenchmark : public UnitTest::AllocatorsBenchmarkFixture
    {
    public:
        void SetUp(const ::benchmark::State& st) override
        {
            UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
            AZ::NameDictionary::Create();
            AZ::AllocatorInstance<ValueAllocator>::Create();
        }

        void SetUp(::benchmark::State& st) override
        {
            UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
            AZ::NameDictionary::Create();
            AZ::AllocatorInstance<ValueAllocator>::Create();
        }

        void TearDown(::benchmark::State& st) override
        {
            AZ::AllocatorInstance<ValueAllocator>::Destroy();
            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
        }

        void TearDown(const ::benchmark::State& st) override
        {
            AZ::AllocatorInstance<ValueAllocator>::Destroy();
            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
        }

        Value GenerateDomBenchmarkPayload(int64_t entryCount, int64_t stringTemplateLength)
        {
            Value root(Type::Object);

            AZStd::string entryTemplate;
            while (entryTemplate.size() < static_cast<size_t>(stringTemplateLength))
            {
                entryTemplate += "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor ";
            }
            entryTemplate.resize(stringTemplateLength);
            AZStd::string buffer;

            auto createString = [&](int n) -> Value
            {
                return Value(AZStd::string::format("#%i %s", n, entryTemplate.c_str()), true);
            };

            auto createEntry = [&](int n) -> Value
            {
                Value entry(Type::Object);
                entry.AddMember("string", createString(n));
                entry.AddMember("int", n);
                entry.AddMember("double", static_cast<double>(n) * 0.5);
                entry.AddMember("bool", n % 2 == 0);
                entry.AddMember("null", Value(Type::Null));
                return entry;
            };

            auto createArray = [&]() -> Value
            {
                Value array(Type::Array);
                for (int i = 0; i < entryCount; ++i)
                {
                    array.PushBack(createEntry(i));
                }
                return array;
            };

            auto createObject = [&]() -> Value
            {
                Value object;
                object.SetObject();
                for (int i = 0; i < entryCount; ++i)
                {
                    buffer = AZStd::string::format("Key%i", i);
                    object.AddMember(AZ::Name(buffer), createArray());
                }
                return object;
            };

            root["entries"] = createObject();

            return root;
        }

        template<class T>
        void TakeAndDiscardWithoutTimingDtor(T&& value, benchmark::State& state)
        {
            {
                T instance = AZStd::move(value);
                state.PauseTiming();
            }
            state.ResumeTiming();
        }
    };

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueMakeComplexObject)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            TakeAndDiscardWithoutTimingDtor(GenerateDomBenchmarkPayload(state.range(0), state.range(1)), state);
        }

        state.SetItemsProcessed(state.range(0) * state.range(0) * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, AzDomValueMakeComplexObject)
        ->Args({ 10, 5 })
        ->Args({ 10, 500 })
        ->Args({ 100, 5 })
        ->Args({ 100, 500 })
        ->Unit(benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueShallowCopy)(benchmark::State& state)
    {
        Value original = GenerateDomBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            Value copy = original;
            benchmark::DoNotOptimize(copy);
        }

        state.SetItemsProcessed(state.iterations());
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, AzDomValueShallowCopy)
        ->Args({ 10, 5 })
        ->Args({ 10, 500 })
        ->Args({ 100, 5 })
        ->Args({ 100, 500 })
        ->Unit(benchmark::kNanosecond);

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueCopyAndMutate)(benchmark::State& state)
    {
        Value original = GenerateDomBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            Value copy = original;
            copy["entries"]["Key0"].PushBack(42);
            TakeAndDiscardWithoutTimingDtor(AZStd::move(copy), state);
        }

        state.SetItemsProcessed(state.iterations());
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, AzDomValueCopyAndMutate)
        ->Args({ 10, 5 })
        ->Args({ 10, 500 })
        ->Args({ 100, 5 })
        ->Args({ 100, 500 })
        ->Unit(benchmark::kNanosecond);

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueDeepCopy)(benchmark::State& state)
    {
        Value original = GenerateDomBenchmarkPayload(state.range(0), state.range(1));

        for (auto _ : state)
        {
            Value copy = original.DeepCopy();
            TakeAndDiscardWithoutTimingDtor(AZStd::move(copy), state);
        }

        state.SetItemsProcessed(state.iterations());
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, AzDomValueDeepCopy)
        ->Args({ 10, 5 })
        ->Args({ 10, 500 })
        ->Args({ 100, 5 })
        ->Args({ 100, 500 })
        ->Unit(benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(DomValueBenchmark, LookupMemberByName)(benchmark::State& state)
    {
        Value value(Type::Object);
        AZStd::vector<AZ::Name> keys;
        for (int64_t i = 0; i < state.range(0); ++i)
        {
            AZ::Name key(AZStd::string::format("key%" PRId64, i));
            keys.push_back(key);
            value[key] = i;
        }

        for (auto _ : state)
        {
            for (const AZ::Name& key : keys)
            {
                benchmark::DoNotOptimize(value[key]);
            }
        }

        state.SetItemsProcessed(state.iterations() * state.range(0));
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, LookupMemberByName)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(DomValueBenchmark, LookupMemberByString)(benchmark::State& state)
    {
        Value value(Type::Object);
        AZStd::vector<AZStd::string> keys;
        for (int64_t i = 0; i < state.range(0); ++i)
        {
            AZStd::string key(AZStd::string::format("key%" PRId64, i));
            keys.push_back(key);
            value[key] = i;
        }

        for (auto _ : state)
        {
            for (const AZStd::string& key : keys)
            {
                benchmark::DoNotOptimize(value[key]);
            }
        }

        state.SetItemsProcessed(state.iterations() * state.range(0));
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, LookupMemberByString)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(DomValueBenchmark, LookupMemberByStringComparison)(benchmark::State& state)
    {
        Value value(Type::Object);
        AZStd::vector<AZStd::string> keys;
        for (int64_t i = 0; i < state.range(0); ++i)
        {
            AZStd::string key(AZStd::string::format("key%" PRId64, i));
            keys.push_back(key);
            value[key] = i;
        }

        for (auto _ : state)
        {
            for (const AZStd::string& key : keys)
            {
                const Object::ContainerType& object = value.GetObject();
                benchmark::DoNotOptimize(AZStd::find_if(
                    object.cbegin(), object.cend(),
                    [&key](const Object::EntryType& entry)
                    {
                        return key == entry.first.GetStringView();
                    }));
            }
        }

        state.SetItemsProcessed(state.iterations() * state.range(0));
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, LookupMemberByStringComparison)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMillisecond);

} // namespace AZ::Dom::Benchmark
