/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Benchmark
{
    class DomValueBenchmark : public Tests::DomBenchmarkFixture
    {
    };

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueGetType_UsingVariantIndex)(benchmark::State& state)
    {
        Value intValue(5);
        Value boolValue(true);
        Value objValue(Type::Object);
        Value nodeValue(Type::Node);
        Value arrValue(Type::Array);
        Value uintValue(5u);
        Value doubleValue(4.0);
        Value stringValue("foo", true);

        for ([[maybe_unused]] auto _ : state)
        {
            (intValue.GetType());
            (boolValue.GetType());
            (objValue.GetType());
            (nodeValue.GetType());
            (arrValue.GetType());
            (uintValue.GetType());
            (doubleValue.GetType());
            (stringValue.GetType());
        }

        state.SetItemsProcessed(8 * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, AzDomValueGetType_UsingVariantIndex);

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueGetType_UsingVariantVisit)(benchmark::State& state)
    {
        Value intValue(5);
        Value boolValue(true);
        Value objValue(Type::Object);
        Value nodeValue(Type::Node);
        Value arrValue(Type::Array);
        Value uintValue(5u);
        Value doubleValue(4.0);
        Value stringValue("foo", true);

        auto getTypeViaVisit = [](const Value& value)
        {
            return AZStd::visit(
                [](auto&& value) constexpr -> Type
                {
                    using CurrentType = AZStd::decay_t<decltype(value)>;
                    if constexpr (AZStd::is_same_v<CurrentType, AZStd::monostate>)
                    {
                        return Type::Null;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, AZ::s64>)
                    {
                        return Type::Int64;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, AZ::u64>)
                    {
                        return Type::Uint64;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, double>)
                    {
                        return Type::Double;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, bool>)
                    {
                        return Type::Bool;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, AZStd::string_view>)
                    {
                        return Type::String;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, Value::SharedStringType>)
                    {
                        return Type::String;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, Value::ShortStringType>)
                    {
                        return Type::String;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, ObjectPtr>)
                    {
                        return Type::Object;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, ArrayPtr>)
                    {
                        return Type::Array;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, NodePtr>)
                    {
                        return Type::Node;
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, Value::OpaqueStorageType>)
                    {
                        return Type::Opaque;
                    }
                    else
                    {
                        AZ_Assert(false, "AZ::Dom::Value::GetType: m_value has an unexpected type");
                    }
                },
                value.GetInternalValue());
        };

        for ([[maybe_unused]] auto _ : state)
        {
            (getTypeViaVisit(intValue));
            (getTypeViaVisit(boolValue));
            (getTypeViaVisit(objValue));
            (getTypeViaVisit(nodeValue));
            (getTypeViaVisit(arrValue));
            (getTypeViaVisit(uintValue));
            (getTypeViaVisit(doubleValue));
            (getTypeViaVisit(stringValue));
        }

        state.SetItemsProcessed(8 * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomValueBenchmark, AzDomValueGetType_UsingVariantVisit);

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueMakeComplexObject)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            TakeAndDiscardWithoutTimingDtor(GenerateDomBenchmarkPayload(state.range(0), state.range(1)), state);
        }

        state.SetItemsProcessed(state.range(0) * state.range(0) * state.iterations());
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomValueBenchmark, AzDomValueMakeComplexObject)

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueShallowCopy)(benchmark::State& state)
    {
        Value original = GenerateDomBenchmarkPayload(state.range(0), state.range(1));

        for ([[maybe_unused]] auto _ : state)
        {
            Value copy = original;
            benchmark::DoNotOptimize(copy);
        }

        state.SetItemsProcessed(state.iterations());
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_NS(DomValueBenchmark, AzDomValueShallowCopy)

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueCopyAndMutate)(benchmark::State& state)
    {
        Value original = GenerateDomBenchmarkPayload(state.range(0), state.range(1));

        for ([[maybe_unused]] auto _ : state)
        {
            Value copy = original;
            copy["entries"]["Key0"].ArrayPushBack(Value(42));
            TakeAndDiscardWithoutTimingDtor(AZStd::move(copy), state);
        }

        state.SetItemsProcessed(state.iterations());
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomValueBenchmark, AzDomValueCopyAndMutate)

    BENCHMARK_DEFINE_F(DomValueBenchmark, AzDomValueDeepCopy)(benchmark::State& state)
    {
        Value original = GenerateDomBenchmarkPayload(state.range(0), state.range(1));

        for ([[maybe_unused]] auto _ : state)
        {
            Value copy = Utils::DeepCopy(original);
            TakeAndDiscardWithoutTimingDtor(AZStd::move(copy), state);
        }

        state.SetItemsProcessed(state.iterations());
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomValueBenchmark, AzDomValueDeepCopy)

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

        for ([[maybe_unused]] auto _ : state)
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

        for ([[maybe_unused]] auto _ : state)
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

        for ([[maybe_unused]] auto _ : state)
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
