/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <Prefab/Benchmark/PrefabBenchmarkFixture.h>

namespace Benchmark
{
    using BM_PrefabInstantiate = BM_Prefab;
    using namespace AzToolsFramework::Prefab;

    BENCHMARK_DEFINE_F(BM_PrefabInstantiate, InstantiatePrefab_SingleEntityInstance)(::benchmark::State& state)
    {
        const unsigned int numInstances = static_cast<unsigned int>(state.range());

        AZStd::unique_ptr<Instance> firstInstance = m_prefabSystemComponent->CreatePrefab(
            { CreateEntity("Entity1") },
            {},
            m_pathString);

        TemplateId templateToInstantiateId = firstInstance->GetTemplateId();
        for (auto _ : state)
        {
            state.PauseTiming();

            AZStd::vector<AZStd::unique_ptr<Instance>> newInstances;
            newInstances.resize(numInstances);

            state.ResumeTiming();

            for (unsigned int instanceCounter = 0; instanceCounter < numInstances; ++instanceCounter)
            {
                newInstances[instanceCounter] = m_prefabSystemComponent->InstantiatePrefab(templateToInstantiateId);
            }
        }

        state.SetComplexityN(numInstances);
    }
    BENCHMARK_REGISTER_F(BM_PrefabInstantiate, InstantiatePrefab_SingleEntityInstance)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();
}

#endif
