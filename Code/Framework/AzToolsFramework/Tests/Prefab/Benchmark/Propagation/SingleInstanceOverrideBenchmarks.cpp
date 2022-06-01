/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <Prefab/Benchmark/Propagation/SingleInstanceOverrideBenchmarks.h>

#define REGISTER_OVERRIDE_INSTANCE_BENCHMARK(BaseClass, Method)                                                                            \
    BENCHMARK_REGISTER_F(BaseClass, Method)                                                                                                \
        ->Args({ 1, 500  })                                                                                                                \
        ->Args({ 1, 5000 })                                                                                                                \
        ->Args({ 10, 100 })                                                                                                                \
        ->Args({ 10, 1000 })                                                                                                               \
        ->Args({ 50, 20 })                                                                                                                 \
        ->Args({ 50, 200 })                                                                                                                \
        ->ArgNames({ "DepthOfNesting", "EntitiesInEachPrefab" })                                                                           \
        ->Unit(benchmark::kMillisecond);

namespace Benchmark
{
    using namespace AzToolsFramework::Prefab;

    void SingleInstanceOverrideBenchmarks::SetupHarness(const benchmark::State& state)
    {
        BM_Prefab::SetupHarness(state);
        
        const unsigned int depthOfNesting = static_cast<unsigned int>(state.range(0));
        const unsigned int entitiesCountInEachPrefab = static_cast<unsigned int>(state.range(1));

        CreateFakePaths(depthOfNesting + 1); // The +1 is for the path of parent prefab
        const auto& parentTemplatePath = m_paths.back();
        const auto& templateToOverridePath = m_paths.front();

        // Create the prefab instance that will act as the leaf instance that we will be overriding.
        AZStd::vector<AZ::Entity*> entitiesInNestedPrefab = GetEntitiesToPutInNestedInstance(entitiesCountInEachPrefab);
        m_entityToModify = CreateEntity("Entity", AZ::EntityId());
        entitiesInNestedPrefab.emplace_back(m_entityToModify);
        AZStd::unique_ptr<Instance> nestedInstance = m_prefabSystemComponent->CreatePrefab(entitiesInNestedPrefab, {}, templateToOverridePath);
        m_instanceToModify = nestedInstance.get();

        for (unsigned int nestedInstanceCounter = 1; nestedInstanceCounter < depthOfNesting; nestedInstanceCounter++)
        {
            nestedInstance = m_prefabSystemComponent->CreatePrefab(
                GetEntitiesToPutInNestedInstance(entitiesCountInEachPrefab), MakeInstanceList(AZStd::move(nestedInstance)),
                m_paths[nestedInstanceCounter]);
        }

        m_instanceCreated = m_prefabSystemComponent->CreatePrefab(
            GetEntitiesToPutInNestedInstance(entitiesCountInEachPrefab), MakeInstanceList(AZStd::move(nestedInstance)), parentTemplatePath);

        // We need 2 prefab instances: One to make the original change to; And one to propagate that change to.
        m_instanceToUseForPropagation = m_prefabSystemComponent->InstantiatePrefab(m_instanceCreated->GetTemplateId());
    }

    void SingleInstanceOverrideBenchmarks::TeardownHarness(const benchmark::State& state)
    {
        m_instanceCreated.reset();
        m_instanceToUseForPropagation.reset();
        BM_Prefab::TeardownHarness(state);
    }

    AZStd::vector<AZ::Entity*> SingleInstanceOverrideBenchmarks::GetEntitiesToPutInNestedInstance(const unsigned int entityCount)
    {
        AZStd::vector<AZ::Entity*> entitiesInNestedInstance;
        entitiesInNestedInstance.reserve(entityCount);
        for (unsigned int entityCounter = 0; entityCounter < entityCount; entityCounter++)
        {
            entitiesInNestedInstance.emplace_back(CreateEntity("Entity"));
        }
        return AZStd::move(entitiesInNestedInstance);
    }

    BENCHMARK_DEFINE_F(SingleInstanceOverrideBenchmarks, PropagateOverrideComponent)(benchmark::State& state)
    {
        UpdateComponent(state);
    }
    REGISTER_OVERRIDE_INSTANCE_BENCHMARK(SingleInstanceOverrideBenchmarks, PropagateOverrideComponent);

    BENCHMARK_DEFINE_F(SingleInstanceOverrideBenchmarks, PropagateOverrideAddComponent)(benchmark::State& state)
    {
        AddComponent(state);
    }
    REGISTER_OVERRIDE_INSTANCE_BENCHMARK(SingleInstanceOverrideBenchmarks, PropagateOverrideAddComponent);

    BENCHMARK_DEFINE_F(SingleInstanceOverrideBenchmarks, PropagateOverrideRemoveComponent)(benchmark::State& state)
    {
        RemoveComponent(state);
    }
    REGISTER_OVERRIDE_INSTANCE_BENCHMARK(SingleInstanceOverrideBenchmarks, PropagateOverrideRemoveComponent);

    BENCHMARK_DEFINE_F(SingleInstanceOverrideBenchmarks, PropagateOverrideAddEntity)(benchmark::State& state)
    {
        AddEntity(state);
    }
    REGISTER_OVERRIDE_INSTANCE_BENCHMARK(SingleInstanceOverrideBenchmarks, PropagateOverrideAddEntity);

    BENCHMARK_DEFINE_F(SingleInstanceOverrideBenchmarks, PropagateOverrideRemoveEntity)(benchmark::State& state)
    {
        RemoveEntity(state);
    }
    REGISTER_OVERRIDE_INSTANCE_BENCHMARK(SingleInstanceOverrideBenchmarks, PropagateOverrideRemoveEntity);

} // namespace Benchmark
#endif
