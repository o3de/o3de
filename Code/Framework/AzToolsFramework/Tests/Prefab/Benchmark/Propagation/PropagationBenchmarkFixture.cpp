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
#include <Prefab/Benchmark/Propagation/PropagationBenchmarkFixture.h>

namespace Benchmark
{
    void PropagationBenchmarkFixture::UpdateTemplate()
    {
        PrefabDom updatedPrefabDom;
        PrefabDomUtils::StoreInstanceInPrefabDom(*m_instanceCreated, updatedPrefabDom);
        PrefabDom& enclosingTemplatePrefabDom = m_prefabSystemComponent->FindTemplateDom(m_instanceCreated->GetTemplateId());
        enclosingTemplatePrefabDom.CopyFrom(updatedPrefabDom, enclosingTemplatePrefabDom.GetAllocator());
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(m_instanceCreated->GetTemplateId(), *m_instanceCreated);
    }

    void PropagationBenchmarkFixture::UpdateComponent(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            float worldX = 0.0f;
            AZ::TransformBus::EventResult(worldX, m_entityToModify->GetId(), &AZ::TransformInterface::GetWorldX);

            // Move the entity and update the template to capture this transform component change.
            AZ::TransformBus::Event(m_entityToModify->GetId(), &AZ::TransformInterface::SetWorldX, worldX + 1);
            UpdateTemplate();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        }
    }

    void PropagationBenchmarkFixture::AddComponent(benchmark::State& state)
    {
        m_entityToModify->Deactivate();
        for ([[maybe_unused]] auto _ : state)
        {
            // Add another component and update the template to capture this change.
            AZ::Component* inspectorComponent = m_entityToModify->CreateComponent<AzToolsFramework::Components::EditorInspectorComponent>();
            UpdateTemplate();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

            // Remove the second component added. This will make sure that when multiple iterations are done, we will always be going from
            // one components to two components.
            m_entityToModify->RemoveComponent(inspectorComponent);
            delete inspectorComponent;
            inspectorComponent = nullptr;
        }
    }

    void PropagationBenchmarkFixture::RemoveComponent(benchmark::State& state)
    {
        m_entityToModify->Deactivate();

        for ([[maybe_unused]] auto _ : state)
        {
            AZStd::span<AZ::Component* const> components = m_entityToModify->GetComponents();
            AZ::Component* transformComponent = components.front();
            m_entityToModify->RemoveComponent(transformComponent);
            delete transformComponent;
            transformComponent = nullptr;
            UpdateTemplate();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

            // Add the component back. This will make sure that when multiple iterations are done, we will always be going from
            // zero components to one component.
            m_entityToModify->CreateComponent(AZ::TransformComponentTypeId);
        }
    }

    void PropagationBenchmarkFixture::AddEntity(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            // Add an entity and update the template.
            AZStd::unique_ptr<AZ::Entity> newEntity = AZStd::make_unique<AZ::Entity>("Added Entity");
            AZ::EntityId newEntityId = newEntity->GetId();
            m_instanceToModify->AddEntity(AZStd::move(newEntity));
            UpdateTemplate();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

            // Remove the entityadded. This will make sure that when multiple iterations are done, we will always be going from
            // 'n' entities to 'n+1' entities.
            newEntity = m_instanceToModify->DetachEntity(newEntityId);
        }
    }

    void PropagationBenchmarkFixture::RemoveEntity(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            // Remove an entity and update the template.
            AZStd::unique_ptr<AZ::Entity> detachedEntity = m_instanceToModify->DetachEntity(m_entityToModify->GetId());
            UpdateTemplate();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

            // Add back the entity removed. This will make sure that when multiple iterations are done, we will always be going from
            // 'n' entities to 'n-1' entities.
            m_instanceToModify->AddEntity(AZStd::move(detachedEntity));
        }
    }

    void PropagationBenchmarkFixture::AddNestedInstance(benchmark::State& state, TemplateId nestedPrefabTemplateId)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            // Add a nested instance and update the template.
            AZStd::unique_ptr<Instance> nestedInstance = AZStd::make_unique<Instance>("Added nested instance");
            Instance& addedInstance =
                m_instanceToModify->AddInstance(AZStd::move(m_prefabSystemComponent->InstantiatePrefab(nestedPrefabTemplateId)));
            const InstanceAlias& instanceAlias = addedInstance.GetInstanceAlias();
            UpdateTemplate();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

            // Remove the nested prefab added. This will make sure that when multiple iterations are done, we will always be going from
            // 'n' nested prefabs to 'n+1' nested prefabs.
            nestedInstance = m_instanceToModify->DetachNestedInstance(instanceAlias);
        }
    }

    void PropagationBenchmarkFixture::RemoveNestedInstance(benchmark::State& state, TemplateId nestedPrefabTemplateId)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            AZStd::vector<InstanceAlias> nestedInstanceAliases = m_instanceCreated->GetNestedInstanceAliases(nestedPrefabTemplateId);
            AZStd::unique_ptr<Instance> detachedNestedInstance = m_instanceCreated->DetachNestedInstance(nestedInstanceAliases.back());
            UpdateTemplate();

            m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

            // Add back the nested instance removed. This will make sure that when multiple iterations are done, we will always be going
            // from 'n' nested instances to 'n-1' nested instances.
            m_instanceCreated->AddInstance(AZStd::move(detachedNestedInstance));
        }

        // After the last iteration, the template should be updated to avoid link deletion failures.
        UpdateTemplate();
    }
} // namespace Benchmark

#endif
