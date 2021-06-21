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

#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

namespace Benchmark
{
    using BM_SpawnableCreate = BM_Prefab;
    using namespace AzToolsFramework::Prefab;

    BENCHMARK_DEFINE_F(BM_SpawnableCreate, CreateSpawnable_SingleEntityInstance)(::benchmark::State& state)
    {
        const unsigned int numSpawnables = state.range();

        AZStd::unique_ptr<Instance> instance(m_prefabSystemComponent->CreatePrefab(
            { CreateEntity("Entity1") },
            {},
            m_pathString));


        auto& prefabDom = m_prefabSystemComponent->FindTemplateDom(instance->GetTemplateId());
        for (auto _ : state)
        {
            // Create a vector to store spawnables so that they don't get destroyed immediately after construction.
            AZStd::vector<AZStd::unique_ptr<AzFramework::Spawnable>> spawnables;
            spawnables.reserve(numSpawnables);
            
            for (int spwanableCounter = 0; spwanableCounter < numSpawnables; ++spwanableCounter)
            {
                AZStd::unique_ptr<AzFramework::Spawnable> spawnable = AZStd::make_unique<AzFramework::Spawnable>();
                AzToolsFramework::Prefab::SpawnableUtils::CreateSpawnable(*spawnable, prefabDom);
                spawnables.push_back(AZStd::move(spawnable));
            }
        }

        state.SetComplexityN(numSpawnables);
    }
    BENCHMARK_REGISTER_F(BM_SpawnableCreate, CreateSpawnable_SingleEntityInstance)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();
}

#endif

