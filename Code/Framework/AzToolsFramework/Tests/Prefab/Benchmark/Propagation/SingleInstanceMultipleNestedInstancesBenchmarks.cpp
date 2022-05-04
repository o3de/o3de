/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <Prefab/Benchmark/Propagation/SingleInstanceMultipleNestedInstancesBenchmarks.h>

#define REGISTER_MULTIPLE_NESTED_INSTANCES_BENCHMARK(BaseClass, Method)                                                                    \
    BENCHMARK_REGISTER_F(BaseClass, Method)                                                                                                \
        ->Args({ 10, 100 })                                                                                                                \
        ->Args({ 100, 10 })                                                                                                                \
        ->Args({ 10, 1000 })                                                                                                               \
        ->Args({ 1000, 10 })                                                                                                               \
        ->Args({ 1, 10000 })                                                                                                               \
        ->Args({ 10000, 1 })                                                                                                               \
        ->ArgNames({ "NestedPrefabs", "EntitiesInEachNestedPrefab" })                                                                      \
        ->Unit(benchmark::kMillisecond)                                                                                                    \
        ->Complexity();

namespace Benchmark
{
    using namespace AzToolsFramework::Prefab;

    void SingleInstanceMultipleNestedInstancesBenchmarks::SetupHarness(const benchmark::State& state)
    {
        BM_Prefab::SetupHarness(state);
        CreateFakePaths(2);
        const auto& parentTemplatePath = m_paths.front();
        const auto& nestedTemplatePath = m_paths.back();
        const unsigned int nestedPrefabsCount = static_cast<unsigned int>(state.range(0));
        const unsigned int entitiesCountInNestedPrefab = static_cast<unsigned int>(state.range(1));

        AZStd::vector<AZ::Entity*> entitiesInParentInstance;

        AZStd::vector<AZStd::unique_ptr<Instance>> nestedInstances;
        nestedInstances.reserve(nestedPrefabsCount);

        AZStd::vector<AZ::Entity*> entitiesInNestedInstance;
        entitiesInNestedInstance.reserve(entitiesCountInNestedPrefab);
        for (unsigned int entityCounter = 0; entityCounter < entitiesCountInNestedPrefab; entityCounter++)
        {
            entitiesInNestedInstance.emplace_back(CreateEntity("Entity"));
        }
        AZStd::unique_ptr<Instance> nestedInstance =
            m_prefabSystemComponent->CreatePrefab(entitiesInNestedInstance, {}, nestedTemplatePath);
        m_nestedPrefabTemplateId = nestedInstance->GetTemplateId();
        nestedInstance.reset();

        for (unsigned int nestedInstanceCounter = 0; nestedInstanceCounter < nestedPrefabsCount; nestedInstanceCounter++)
        {
            nestedInstances.emplace_back(AZStd::move(m_prefabSystemComponent->InstantiatePrefab(m_nestedPrefabTemplateId)));
        }

        m_entityModify = CreateEntity("Entity", AZ::EntityId());
        entitiesInParentInstance.emplace_back(m_entityModify);

        m_instanceCreated =
            m_prefabSystemComponent->CreatePrefab(entitiesInParentInstance, AZStd::move(nestedInstances), parentTemplatePath);
        TemplateId templateToInstantiateId = m_instanceCreated->GetTemplateId();

        // We need 2 prefab instances: One to make the original change to; And one to propagate that change to.
        m_instanceToUseForPropagation = m_prefabSystemComponent->InstantiatePrefab(templateToInstantiateId);
    }

    void SingleInstanceMultipleNestedInstancesBenchmarks::TeardownHarness(const benchmark::State& state)
    {
        m_instanceCreated.reset();
        m_instanceToUseForPropagation.reset();
        BM_Prefab::TeardownHarness(state);
    }

    BENCHMARK_DEFINE_F(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateUpdateComponentChange)(benchmark::State& state)
    {
        UpdateComponent(state);
    }
    REGISTER_MULTIPLE_NESTED_INSTANCES_BENCHMARK(
        SingleInstanceMultipleNestedInstancesBenchmarks, PropagateUpdateComponentChange);
        

    BENCHMARK_DEFINE_F(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateAddComponentChange)(benchmark::State& state)
    {
        AddComponent(state);
    }
    REGISTER_MULTIPLE_NESTED_INSTANCES_BENCHMARK(
        SingleInstanceMultipleNestedInstancesBenchmarks, PropagateAddComponentChange);

    BENCHMARK_DEFINE_F(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateRemoveComponentChange)(benchmark::State& state)
    {
        RemoveComponent(state);
    }
    REGISTER_MULTIPLE_NESTED_INSTANCES_BENCHMARK(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateRemoveComponentChange);

    BENCHMARK_DEFINE_F(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateAddEntityChange)(benchmark::State& state)
    {
        AddEntity(state);
    }
    REGISTER_MULTIPLE_NESTED_INSTANCES_BENCHMARK(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateAddEntityChange);

    BENCHMARK_DEFINE_F(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateRemoveEntityChange)(benchmark::State& state)
    {
        RemoveEntity(state);
    }
    REGISTER_MULTIPLE_NESTED_INSTANCES_BENCHMARK(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateRemoveEntityChange);

    BENCHMARK_DEFINE_F(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateAddNestedPrefabChange)(benchmark::State& state)
    {
        AddNestedInstance(state, m_nestedPrefabTemplateId);
    }
    REGISTER_MULTIPLE_NESTED_INSTANCES_BENCHMARK(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateAddNestedPrefabChange);

    BENCHMARK_DEFINE_F(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateRemoveNestedPrefabChange)(benchmark::State& state)
    {
        RemoveNestedInstance(state, m_nestedPrefabTemplateId);
    }
    REGISTER_MULTIPLE_NESTED_INSTANCES_BENCHMARK(SingleInstanceMultipleNestedInstancesBenchmarks, PropagateRemoveNestedPrefabChange);

} // namespace Benchmark
#endif
