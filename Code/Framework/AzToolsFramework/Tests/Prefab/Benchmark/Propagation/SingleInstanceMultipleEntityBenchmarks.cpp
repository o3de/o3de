/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <Prefab/Benchmark/PrefabBenchmarkFixture.h>


namespace Benchmark
{
    using namespace AzToolsFramework::Prefab;

    //! This class captures benchmarks for propagating changes to a single prefab instance with multiple entities that are side-by-side.
    class SingleInstanceMultipleEntityBenchmarks : public Benchmark::BM_Prefab
    {
    protected:
        void SetupHarness(const benchmark::State& state) override
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

            m_entityModify= CreateEntity("Entity", AZ::EntityId());
            entities.emplace_back(m_entityModify);

            m_instanceCreated = m_prefabSystemComponent->CreatePrefab(entities, {}, templatePath);
            TemplateId templateToInstantiateId = m_instanceCreated->GetTemplateId();

            // We need 2 prefab instances: One to make the original change to; And one to propagate that change to.
            m_instanceToUseForPropagation = m_prefabSystemComponent->InstantiatePrefab(templateToInstantiateId);
        }

        void TeardownHarness(const benchmark::State& state) override
        {
            m_instanceCreated.reset();
            m_instanceToUseForPropagation.reset();
            BM_Prefab::TeardownHarness(state);
        }

    protected:
        void UpdateTemplate()
        {
            PrefabDom updatedPrefabDom;
            PrefabDomUtils::StoreInstanceInPrefabDom(*m_instanceCreated, updatedPrefabDom);
            PrefabDom& enclosingTemplatePrefabDom = m_prefabSystemComponent->FindTemplateDom(m_instanceCreated->GetTemplateId());
            enclosingTemplatePrefabDom.CopyFrom(updatedPrefabDom, enclosingTemplatePrefabDom.GetAllocator());
            m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(m_instanceCreated->GetTemplateId(), *m_instanceCreated);
        }

        AZ::Entity* m_entityModify;
        AZStd::unique_ptr<Instance> m_instanceCreated;
        AZStd::unique_ptr<Instance> m_instanceToUseForPropagation;
    };

    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateUpdateComponentChange)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            state.PauseTiming();
            float worldX = 0.0f;
            AZ::TransformBus::EventResult(worldX, m_entityModify->GetId(), &AZ::TransformInterface::GetWorldX);

            // Move the entity and update the template to capture this transform component change.
            AZ::TransformBus::Event(m_entityModify->GetId(), &AZ::TransformInterface::SetWorldX, worldX + 1);
            UpdateTemplate();
            state.ResumeTiming();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        }

        state.SetComplexityN(state.range());
    }

    BENCHMARK_REGISTER_F(SingleInstanceMultipleEntityBenchmarks, PropagateUpdateComponentChange)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();
    
    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateAddComponentChange)(benchmark::State& state)
    {
        m_entityModify->Deactivate();
        for (auto _ : state)
        {
            state.PauseTiming();
            ASSERT_EQ(m_entityModify->GetComponents().size(), 1);

            // Add another component and update the template to capture this change.
            AZ::Component* inspectorComponent = m_entityModify->CreateComponent<AzToolsFramework::Components::EditorInspectorComponent>();
            ASSERT_EQ(m_entityModify->GetComponents().size(), 2);
            UpdateTemplate();
            state.ResumeTiming();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
            state.PauseTiming();

            // Remove the second component added. This will make sure that when multiple iterations are done, we will always be going from
            // one components to two components.
            m_entityModify->RemoveComponent(inspectorComponent);
            ASSERT_EQ(m_entityModify->GetComponents().size(), 1);
            delete inspectorComponent;
            inspectorComponent = nullptr;
            UpdateTemplate();
        }

        state.SetComplexityN(state.range());
    }

    BENCHMARK_REGISTER_F(SingleInstanceMultipleEntityBenchmarks, PropagateAddComponentChange)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();
    
    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateRemoveComponentChange)(benchmark::State& state)
    {
        m_entityModify->Deactivate();

        for (auto _ : state)
        {
            state.PauseTiming();
            const AZStd::vector<AZ::Component*>& components = m_entityModify->GetComponents();
            ASSERT_EQ(m_entityModify->GetComponents().size(), 1);
            AZ::Component* transformComponent = components.front();
            m_entityModify->RemoveComponent(transformComponent);
            delete transformComponent;
            transformComponent = nullptr;
            ASSERT_EQ(m_entityModify->GetComponents().size(), 0);
            UpdateTemplate();
            state.ResumeTiming();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
            state.PauseTiming();

            // Add the component back. This will make sure that when multiple iterations are done, we will always be going from
            // zero components to one component.
            m_entityModify->CreateComponent(AZ::TransformComponentTypeId);
            ASSERT_EQ(m_entityModify->GetComponents().size(), 1);
            UpdateTemplate();
        }

        state.SetComplexityN(state.range());
    }

    BENCHMARK_REGISTER_F(SingleInstanceMultipleEntityBenchmarks, PropagateRemoveComponentChange)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();

    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateAddEntityChange)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            state.PauseTiming();

            // Add an entity and update the template.
            AZStd::unique_ptr<AZ::Entity> newEntity = AZStd::make_unique<AZ::Entity>("Added Entity");
            AZ::EntityId newEntityId = newEntity->GetId();
            ASSERT_TRUE(newEntity != nullptr);
            m_instanceCreated->AddEntity(AZStd::move(newEntity));
            UpdateTemplate();
            state.ResumeTiming();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
            state.PauseTiming();

            // Remove the entityadded. This will make sure that when multiple iterations are done, we will always be going from
            // 'n' entities to 'n+1' entities.
            ASSERT_TRUE(newEntity == nullptr);
            newEntity = m_instanceCreated->DetachEntity(newEntityId);
            ASSERT_TRUE(newEntity != nullptr);
            UpdateTemplate();
        }

        state.SetComplexityN(state.range());
    }

    BENCHMARK_REGISTER_F(SingleInstanceMultipleEntityBenchmarks, PropagateAddEntityChange)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();

    BENCHMARK_DEFINE_F(SingleInstanceMultipleEntityBenchmarks, PropagateDetachEntityChange)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            state.PauseTiming();

            // Add an entity and update the template.
            AZStd::unique_ptr<AZ::Entity> detachedEntity = m_instanceCreated->DetachEntity(m_entityModify->GetId());
            ASSERT_TRUE(detachedEntity != nullptr);
            UpdateTemplate();
            state.ResumeTiming();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
            state.PauseTiming();

            // Add back the entity removed. This will make sure that when multiple iterations are done, we will always be going from
            // 'n' entities to 'n-1' entities.
            m_instanceCreated->AddEntity(AZStd::move(detachedEntity));
            UpdateTemplate();
        }

        state.SetComplexityN(state.range());
    }

    BENCHMARK_REGISTER_F(SingleInstanceMultipleEntityBenchmarks, PropagateDetachEntityChange)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();
        
} // namespace Benchmark

#endif
