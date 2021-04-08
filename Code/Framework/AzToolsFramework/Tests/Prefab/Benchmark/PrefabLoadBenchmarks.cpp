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
