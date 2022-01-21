/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Benchmark
{
    class DomPatchBenchmark : public Tests::DomBenchmarkFixture
    {
    public:
        void TearDownHarness() override
        {
            m_before = {};
            m_after = {};
            Tests::DomBenchmarkFixture::TearDownHarness();
        }

        void SimpleReplace(benchmark::State& state, bool deepCopy, bool apply)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = deepCopy ? Utils::DeepCopy(m_before) : m_before;
            m_after["entries"]["Key0"] = Value("replacement string", true);

            RunBenchmarkInternal(state, apply);
        }

        void TopLevelReplace(benchmark::State& state, bool apply)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = Value(Type::Object);
            m_after["UnrelatedKey"] = Value(42);

            RunBenchmarkInternal(state, apply);
        }

        void KeyRemove(benchmark::State& state, bool deepCopy, bool apply)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = deepCopy ? Utils::DeepCopy(m_before) : m_before;
            m_after["entries"].RemoveMember("Key1");

            RunBenchmarkInternal(state, apply);
        }

        void ArrayAppend(benchmark::State& state, bool deepCopy, bool apply)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = deepCopy ? Utils::DeepCopy(m_before) : m_before;
            m_after["entries"]["Key2"].ArrayPushBack(Value(0));

            RunBenchmarkInternal(state, apply);
        }

        void ArrayPrepend(benchmark::State& state, bool deepCopy, bool apply)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = deepCopy ? Utils::DeepCopy(m_before) : m_before;
            auto& arr = m_after["entries"]["Key2"].GetMutableArray();
            arr.insert(arr.begin(), Value(42));

            RunBenchmarkInternal(state, apply);
        }

    private:
        void RunBenchmarkInternal(benchmark::State& state, bool apply)
        {
            if (apply)
            {
                auto patchInfo = GenerateHierarchicalDeltaPatch(m_before, m_after);
                for (auto _ : state)
                {
                    auto patchResult = patchInfo.m_forwardPatches.Apply(m_before);
                    benchmark::DoNotOptimize(patchResult);
                }
            }
            else
            {
                for (auto _ : state)
                {
                    auto patchInfo = GenerateHierarchicalDeltaPatch(m_before, m_after);
                    benchmark::DoNotOptimize(patchInfo);
                }
            }

            state.SetItemsProcessed(state.iterations());
        }

        Value m_before;
        Value m_after;
    };

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Generate_SimpleReplace_ShallowCopy)(benchmark::State& state)
    {
        SimpleReplace(state, false, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Generate_SimpleReplace_ShallowCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Generate_SimpleReplace_DeepCopy)(benchmark::State& state)
    {
        SimpleReplace(state, true, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Generate_SimpleReplace_DeepCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Generate_TopLevelReplace)(benchmark::State& state)
    {
        TopLevelReplace(state, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Generate_TopLevelReplace)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Generate_KeyRemove_ShallowCopy)(benchmark::State& state)
    {
        KeyRemove(state, false, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Generate_KeyRemove_ShallowCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Generate_KeyRemove_DeepCopy)(benchmark::State& state)
    {
        KeyRemove(state, true, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Generate_KeyRemove_DeepCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Generate_ArrayAppend_ShallowCopy)(benchmark::State& state)
    {
        ArrayAppend(state, false, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Generate_ArrayAppend_ShallowCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Generate_ArrayAppend_DeepCopy)(benchmark::State& state)
    {
        ArrayAppend(state, true, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Generate_ArrayAppend_DeepCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Generate_ArrayPrepend)(benchmark::State& state)
    {
        ArrayPrepend(state, true, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Generate_ArrayPrepend)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Apply_SimpleReplace)(benchmark::State& state)
    {
        SimpleReplace(state, true, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Apply_SimpleReplace)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Apply_TopLevelReplace)(benchmark::State& state)
    {
        TopLevelReplace(state, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Apply_TopLevelReplace)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Apply_KeyRemove)(benchmark::State& state)
    {
        KeyRemove(state, true, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Apply_KeyRemove)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Apply_ArrayAppend)(benchmark::State& state)
    {
        ArrayAppend(state, true, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Apply_ArrayAppend)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_Apply_ArrayPrepend)(benchmark::State& state)
    {
        ArrayPrepend(state, true, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_Apply_ArrayPrepend)
} // namespace AZ::Dom::Benchmark
