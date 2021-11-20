/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <Prefab/Benchmark/PrefabBenchmarkFixture.h>

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace Benchmark
{
    using BM_PrefabUpdateInstances = BM_Prefab;
    using namespace AzToolsFramework::Prefab;

    BENCHMARK_DEFINE_F(BM_PrefabUpdateInstances, UpdateInstances_SingeEntityInstances)(::benchmark::State& state)
    {
        const unsigned int numInstances = static_cast<unsigned int>(state.range());

        CreateFakePaths(2);
        const auto& nestedTemplatePath = m_paths.front();
        const auto& enclosingTemplatePath = m_paths.back();

        for (auto _ : state)
        {
            state.PauseTiming();

            AZ::Entity* entity = CreateEntity("Entity");
            AZStd::unique_ptr<Instance> nestedInstance = m_prefabSystemComponent->CreatePrefab(
                { entity },
                {},
                nestedTemplatePath);

            AZStd::unique_ptr<Instance> enclosingInstance = m_prefabSystemComponent->CreatePrefab(
                {},
                MakeInstanceList(AZStd::move(nestedInstance)),
                enclosingTemplatePath);

            TemplateId templateToInstantiateId = enclosingInstance->GetTemplateId();
            {
                AZStd::vector<AZStd::unique_ptr<Instance>> newInstances;
                newInstances.resize(numInstances);
                for (unsigned int instanceCounter = 0; instanceCounter < numInstances; ++instanceCounter)
                {
                    newInstances[instanceCounter] = m_prefabSystemComponent->InstantiatePrefab(templateToInstantiateId);
                }

                entity->SetName("Updated Entity");

                PrefabDom updatedPrefabDom;
                PrefabDomUtils::StoreInstanceInPrefabDom(*enclosingInstance, updatedPrefabDom);
                PrefabDom& enclosingTemplatePrefabDom = m_prefabSystemComponent->FindTemplateDom(templateToInstantiateId);
                enclosingTemplatePrefabDom.CopyFrom(updatedPrefabDom, enclosingTemplatePrefabDom.GetAllocator());

                state.ResumeTiming();

                m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(templateToInstantiateId);
                m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

                state.PauseTiming();
            }

            enclosingInstance.reset();

            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numInstances);
    }
    BENCHMARK_REGISTER_F(BM_PrefabUpdateInstances, UpdateInstances_SingeEntityInstances)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();

    BENCHMARK_DEFINE_F(BM_PrefabUpdateInstances, UpdateInstances_SingleLinearNestingOfInstances)(::benchmark::State& state)
    {
        const unsigned int maxDepth = static_cast<unsigned int>(state.range());
        CreateFakePaths(maxDepth);

        const unsigned int numInstances = maxDepth;

        for (auto _ : state)
        {
            state.PauseTiming();

            AZ::Entity* entity = CreateEntity("Entity");
            AZStd::unique_ptr<Instance> currentInstanceRoot = m_prefabSystemComponent->CreatePrefab(
                { entity },
                {},
                m_paths.back());

            for (unsigned int currentDepth = 1; currentDepth < maxDepth; ++currentDepth)
            {
                currentInstanceRoot = m_prefabSystemComponent->CreatePrefab(
                    {},
                    MakeInstanceList(AZStd::move(currentInstanceRoot)),
                    m_paths[currentDepth - 1]);
            }

            entity->SetName("Updated Entity");

            PrefabDom updatedPrefabDom;
            PrefabDomUtils::StoreInstanceInPrefabDom(*currentInstanceRoot, updatedPrefabDom);

            const TemplateId rootTemplateId = currentInstanceRoot->GetTemplateId();
            PrefabDom& rootTemplatePrefabDom = m_prefabSystemComponent->FindTemplateDom(rootTemplateId);
            rootTemplatePrefabDom.CopyFrom(updatedPrefabDom, rootTemplatePrefabDom.GetAllocator());

            state.ResumeTiming();

            m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(rootTemplateId);
            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

            state.PauseTiming();

            currentInstanceRoot.reset();

            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numInstances);
    }

    BENCHMARK_DEFINE_F(BM_PrefabUpdateInstances, UpdateInstances_MultipleLinearNestingOfInstances)(::benchmark::State& state)
    {
        const unsigned int numRootInstances = static_cast<unsigned int>(state.range());
        const unsigned int maxDepth = static_cast<unsigned int>(state.range());
        CreateFakePaths(maxDepth);

        const unsigned int numInstances = numRootInstances * maxDepth;

        for (auto _ : state)
        {
            state.PauseTiming();

            AZ::Entity* entity = CreateEntity("Entity");
            AZStd::unique_ptr<Instance> currentInstanceRoot = m_prefabSystemComponent->CreatePrefab(
                { entity },
                {},
                m_paths.back());

            for (unsigned int currentDepth = 0; currentDepth < maxDepth - 1; ++currentDepth)
            {
                currentInstanceRoot = m_prefabSystemComponent->CreatePrefab(
                    {},
                    MakeInstanceList(AZStd::move(currentInstanceRoot)),
                    m_paths[currentDepth]);
            }

            const TemplateId rootTemplateId = currentInstanceRoot->GetTemplateId();
            {
                AZStd::vector<AZStd::unique_ptr<Instance>> newInstances;
                newInstances.resize(numRootInstances - 1);
                for (unsigned int instanceCounter = 0; instanceCounter < numRootInstances - 1; ++instanceCounter)
                {
                    newInstances[instanceCounter] = m_prefabSystemComponent->InstantiatePrefab(rootTemplateId);
                }

                entity->SetName("Updated Entity");

                PrefabDom updatedPrefabDom;
                PrefabDomUtils::StoreInstanceInPrefabDom(*currentInstanceRoot, updatedPrefabDom);

                PrefabDom& rootTemplatePrefabDom = m_prefabSystemComponent->FindTemplateDom(rootTemplateId);
                rootTemplatePrefabDom.CopyFrom(updatedPrefabDom, rootTemplatePrefabDom.GetAllocator());

                state.ResumeTiming();

                m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(rootTemplateId);
                m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

                state.PauseTiming();
            }

            currentInstanceRoot.reset();

            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numInstances);
    }

    BENCHMARK_DEFINE_F(BM_PrefabUpdateInstances, UpdateInstances_BinaryTreeNestedInstanceHierarchy)(::benchmark::State& state)
    {
        const unsigned int maxDepth = static_cast<unsigned int>(state.range());
        CreateFakePaths(maxDepth);

        const unsigned int numInstances =  (1 << maxDepth) - 1;

        for (auto _ : state)
        {
            state.PauseTiming();

            AZ::Entity* entity = CreateEntity("Entity");
            AZStd::unique_ptr<Instance> currentInstanceRoot = m_prefabSystemComponent->CreatePrefab(
                { entity },
                {},
                m_paths.back());

            for (unsigned int currentDepth = 0; currentDepth < maxDepth - 1; ++currentDepth)
            {
                AZStd::unique_ptr<Instance> extraNestedInstance =
                    m_prefabSystemComponent->InstantiatePrefab(currentInstanceRoot->GetTemplateId());

                currentInstanceRoot = m_prefabSystemComponent->CreatePrefab(
                    {},
                    MakeInstanceList(AZStd::move(currentInstanceRoot), AZStd::move(extraNestedInstance)),
                    m_paths[currentDepth]);
            }

            entity->SetName("Updated Entity");

            PrefabDom updatedPrefabDom;
            PrefabDomUtils::StoreInstanceInPrefabDom(*currentInstanceRoot, updatedPrefabDom);

            const TemplateId rootTemplateId = currentInstanceRoot->GetTemplateId();
            PrefabDom& rootTemplatePrefabDom = m_prefabSystemComponent->FindTemplateDom(rootTemplateId);
            rootTemplatePrefabDom.CopyFrom(updatedPrefabDom, rootTemplatePrefabDom.GetAllocator());

            state.ResumeTiming();

            m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(rootTemplateId);
            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

            state.PauseTiming();

            currentInstanceRoot.reset();

            ResetPrefabSystem();

            state.ResumeTiming();
        }

        state.SetComplexityN(numInstances);
    }
    BENCHMARK_REGISTER_F(BM_PrefabUpdateInstances, UpdateInstances_BinaryTreeNestedInstanceHierarchy)
        ->DenseRange(8, 12, 2)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();
}

#endif
