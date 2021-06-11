/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or , if provided, by the license below or the license accompanying this file.Do not
*remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        const unsigned int numInstances = state.range();

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

            for (int instanceCounter = 0; instanceCounter < numInstances; ++instanceCounter)
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
