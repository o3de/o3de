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
            Value root(Type::ObjectType);

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
                Value entry(Type::ObjectType);
                entry.AddMember("string", createString(n));
                entry.AddMember("int", n);
                entry.AddMember("double", static_cast<double>(n) * 0.5);
                entry.AddMember("bool", n % 2 == 0);
                entry.AddMember("null", Value(Type::NullType));
                return entry;
            };

            auto createArray = [&]() -> Value
            {
                Value array(Type::ArrayType);
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
    };

    BENCHMARK_DEFINE_F(DomValueBenchmark, ValuePayloadGeneration)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            benchmark::DoNotOptimize(GenerateDomBenchmarkPayload(state.range(0), state.range(1)));
        }

        state.SetItemsProcessed(state.range(0) * state.range(0) * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, ValuePayloadGeneration)
        ->Args({ 10, 5 })
        ->Args({ 10, 500 })
        ->Args({ 100, 5 })
        ->Args({ 100, 500 })
        ->Unit(benchmark::kMillisecond);
} // namespace AZ::Dom::Benchmark
