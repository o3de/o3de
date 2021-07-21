/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <Prefab/Benchmark/PrefabBenchmarkFixture.h>

namespace Benchmark
{
    using BM_PrefabLoad = BM_Prefab;
    using namespace AzToolsFramework::Prefab;

    BENCHMARK_DEFINE_F(BM_PrefabLoad, LoadPrefab_Basic)(::benchmark::State& state)
    {
        const unsigned int numTemplates = state.range();
        CreateFakePaths(numTemplates);

        for (auto _ : state)
        {
            state.PauseTiming();

            SetUpMockValidatorForReadPrefab();

            m_prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();

            state.ResumeTiming();

            for (int templateCounter = 0; templateCounter < numTemplates; ++templateCounter)
            {
                m_prefabLoaderInterface->LoadTemplateFromFile(m_paths[templateCounter]);
            }

            state.PauseTiming();

            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numTemplates);
    }
    BENCHMARK_REGISTER_F(BM_PrefabLoad, LoadPrefab_Basic)
        ->RangeMultiplier(10)
        ->Range(100, 1000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();
}

#endif
