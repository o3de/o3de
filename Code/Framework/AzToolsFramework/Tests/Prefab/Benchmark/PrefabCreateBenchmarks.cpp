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
    using BM_PrefabCreate = BM_Prefab;
    using namespace AzToolsFramework::Prefab;

    BENCHMARK_DEFINE_F(BM_PrefabCreate, CreatePrefabs_SingleEntityEach)(::benchmark::State& state)
    {
        const unsigned int numEntities = static_cast<unsigned int>(state.range());
        const unsigned int numInstances = numEntities;

        CreateFakePaths(numInstances);

        for (auto _ : state)
        {
            state.PauseTiming();

            AZStd::vector<AZ::Entity*> entities;
            CreateEntities(numEntities, entities);

            AZStd::vector<AZStd::unique_ptr<Instance>> newInstances;

            state.ResumeTiming();

            for (unsigned int instanceCounter = 0; instanceCounter < numInstances; ++instanceCounter)
            {
                newInstances.push_back(m_prefabSystemComponent->CreatePrefab(
                    { entities[instanceCounter] },
                    {},
                    m_paths[instanceCounter]));
            }

            state.PauseTiming();

            newInstances.clear();
            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numInstances);
    }
    BENCHMARK_REGISTER_F(BM_PrefabCreate, CreatePrefabs_SingleEntityEach)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();

    BENCHMARK_DEFINE_F(BM_PrefabCreate, CreatePrefab_FromEntities)(::benchmark::State& state)
    {
        const unsigned int numEntities = static_cast<unsigned int>(state.range());

        for (auto _ : state)
        {
            state.PauseTiming();

            AZStd::vector<AZ::Entity*> entities;
            CreateEntities(numEntities, entities);

            state.ResumeTiming();

            AZStd::unique_ptr<Instance> instance = m_prefabSystemComponent->CreatePrefab(
                entities
                , {}
                , m_pathString);

            state.PauseTiming();

            instance.reset();

            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numEntities);
    }
    BENCHMARK_REGISTER_F(BM_PrefabCreate, CreatePrefab_FromEntities)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();

    BENCHMARK_DEFINE_F(BM_PrefabCreate, CreatePrefab_FromSingleDepthInstances)(::benchmark::State& state)
    {
        const unsigned int numInstancesToAdd = static_cast<unsigned int>(state.range());
        const unsigned int numEntities = numInstancesToAdd;

        // Create fake paths for all the nested instances
        // plus the instance receiving them
        CreateFakePaths(numInstancesToAdd + 1);

        for (auto _ : state)
        {
            state.PauseTiming();

            AZStd::vector<AZ::Entity*> entities;
            CreateEntities(numEntities, entities);

            AZStd::vector<AZStd::unique_ptr<Instance>> testInstances;
            testInstances.resize(numInstancesToAdd);

            for (unsigned int instanceCounter = 0; instanceCounter < numInstancesToAdd; ++instanceCounter)
            {
                testInstances[instanceCounter] = (m_prefabSystemComponent->CreatePrefab(
                    { entities[instanceCounter] }
                    , {}
                    , m_paths[instanceCounter]));
            }

            state.ResumeTiming();

            AZStd::unique_ptr<Instance> nestedInstance = m_prefabSystemComponent->CreatePrefab(
                {}
                , AZStd::move(testInstances)
                , m_paths.back());

            state.PauseTiming();

            nestedInstance.reset();

            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numInstancesToAdd);
    }
    BENCHMARK_REGISTER_F(BM_PrefabCreate, CreatePrefab_FromSingleDepthInstances)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();

    BENCHMARK_DEFINE_F(BM_PrefabCreate, CreatePrefab_FromLinearNestingOfInstances)(::benchmark::State& state)
    {
        const unsigned int numInstances = static_cast<unsigned int>(state.range());

        // Create fake paths for all the nested instances
        // plus the root instance
        CreateFakePaths(numInstances + 1);

        for (auto _ : state)
        {
            state.PauseTiming();

            AZStd::unique_ptr<Instance> nestedInstanceRoot = m_prefabSystemComponent->CreatePrefab(
                { CreateEntity("Entity1") },
                {},
                m_paths.back());

            state.ResumeTiming();

            for (unsigned int instanceCounter = 0; instanceCounter < numInstances; ++instanceCounter)
            {
                nestedInstanceRoot = m_prefabSystemComponent->CreatePrefab(
                    {},
                    MakeInstanceList(AZStd::move(nestedInstanceRoot)),
                    m_paths[instanceCounter]);
            }

            state.PauseTiming();

            nestedInstanceRoot.reset();

            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numInstances);
    }
}
#endif
