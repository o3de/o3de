/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <Prefab/Benchmark/Propagation/SingleInstanceMultipleEntityBenchmarks.h>

#define REGISTER_MULTIPLE_ENTITY_BENCHMARK(BaseClass, Method)                                                                              \
    BENCHMARK_REGISTER_F(BaseClass, Method)                                                                                                \
        ->RangeMultiplier(10)                                                                                                              \
        ->Range(100, 10000)                                                                                                                \
        ->Unit(benchmark::kMillisecond)                                                                                                    \
        ->Complexity();

namespace Benchmark
{
    using namespace AzToolsFramework::Prefab;

    void SingleInstanceMultipleEntityBenchmarks::SetupHarness(const benchmark::State& state)
    {
        BM_Prefab::SetupHarness(state);
        CreateFakePaths(1);
        const auto& templatePath = m_paths.front();
        const unsigned int numEntities = static_cast<unsigned int>(state.range());

        AZStd::vector<AZ::Entity*> entities;
        for (unsigned int i = 1; i < numEntities; i++)
        {
            entities.emplace_back(CreateEntity("Entity"));
        }

        m_entityModify = CreateEntity("Entity", AZ::EntityId());
        entities.emplace_back(m_entityModify);

        m_instanceCreated = m_prefabSystemComponent->CreatePrefab(entities, {}, templatePath);
        TemplateId templateToInstantiateId = m_instanceCreated->GetTemplateId();

        // We need 2 prefab instances: One to make the original change to; And one to propagate that change to.
        m_instanceToUseForPropagation = m_prefabSystemComponent->InstantiatePrefab(templateToInstantiateId);
    }

    void SingleInstanceMultipleEntityBenchmarks::TeardownHarness(const benchmark::State& state)
    {
        m_instanceCreated.reset();
        m_instanceToUseForPropagation.reset();
        BM_Prefab::TeardownHarness(state);
    }

    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateUpdateComponentChange)(benchmark::State& state)
    {
        UpdateComponent(state);
    }
    REGISTER_MULTIPLE_ENTITY_BENCHMARK(SingleInstanceMultipleEntityBenchmarks, PropagateUpdateComponentChange);
    
    
    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateAddComponentChange)(benchmark::State& state)
    {
        AddComponent(state);
    }
    REGISTER_MULTIPLE_ENTITY_BENCHMARK(SingleInstanceMultipleEntityBenchmarks, PropagateAddComponentChange);
    
    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateRemoveComponentChange)(benchmark::State& state)
    {
        RemoveComponent(state);
    }
    REGISTER_MULTIPLE_ENTITY_BENCHMARK(SingleInstanceMultipleEntityBenchmarks, PropagateRemoveComponentChange);

    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateAddEntityChange)(benchmark::State& state)
    {
        AddEntity(state);
    }
    REGISTER_MULTIPLE_ENTITY_BENCHMARK(SingleInstanceMultipleEntityBenchmarks, PropagateAddEntityChange);

    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateRemoveEntityChange)(benchmark::State& state)
    {
        RemoveEntity(state);
    }
    REGISTER_MULTIPLE_ENTITY_BENCHMARK(SingleInstanceMultipleEntityBenchmarks, PropagateRemoveEntityChange);
        
} // namespace Benchmark
#endif
