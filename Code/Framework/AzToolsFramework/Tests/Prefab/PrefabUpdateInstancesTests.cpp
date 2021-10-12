/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabUpdateInstancesTest = PrefabTestFixture;

    TEST_F(PrefabUpdateInstancesTest, PrefabUpdateInstances_UpdateEntityName_UpdateSucceeds)
    {
        // Create a Template from an Instance owning a single entity.
        using namespace AzToolsFramework::Prefab;
        const char* newEntityName = "New Entity";
        AZ::Entity* newEntity = CreateEntity(newEntityName);
        AZStd::unique_ptr<Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({ newEntity }, {}, PrefabMockFilePath);
        ASSERT_TRUE(firstInstance);
        TemplateId newTemplateId = firstInstance->GetTemplateId();
        EXPECT_TRUE(newTemplateId != InvalidTemplateId);
        PrefabDom& templatePrefabDom = m_prefabSystemComponent->FindTemplateDom(newTemplateId);
        AZStd::vector<EntityAlias> entityAliases = firstInstance->GetEntityAliases();
        EXPECT_EQ(entityAliases.size(), 1);

        // Instantiate Instances and validate if all entities of each Template's Instance have the given entity names.
        const int numberOfInstances = 3;
        AZStd::vector<AZStd::unique_ptr<Instance>> instantiatedInstances;
        for (int i = 0; i < numberOfInstances; ++i)
        {
            instantiatedInstances.emplace_back(m_prefabSystemComponent->InstantiatePrefab(newTemplateId));
            ASSERT_TRUE(instantiatedInstances.back());
            EXPECT_EQ(instantiatedInstances.back()->GetTemplateId(), newTemplateId);
        }

        PrefabDomPath entityNamePath = PrefabTestDomUtils::GetPrefabDomEntityNamePath(entityAliases.front());
        const PrefabDomValue* entityNameValue =
            PrefabTestDomUtils::GetPrefabDomEntityName(templatePrefabDom, entityAliases.front());
        ASSERT_TRUE(entityNameValue != nullptr);
        PrefabTestDomUtils::ValidateInstances(newTemplateId, *entityNameValue, entityNamePath);

        // Update Template's PrefabDom with a new entity name.
        entityNamePath.Set(templatePrefabDom, "Updated Entity");

        // Update Template's Instances.
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(newTemplateId);
        const bool updateResult = m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        EXPECT_TRUE(updateResult);

        // Validate if all entities of each Template's Instance have the updated entity names.
        PrefabTestDomUtils::ValidateInstances(newTemplateId, *entityNameValue, entityNamePath);

    }

    TEST_F(PrefabUpdateInstancesTest, UpdatePrefabInstances_AddEntity_UpdateSucceeds)
    {
        // Create a Template from an Instance owning a single entity.
        using namespace AzToolsFramework::Prefab;
        AZ::Entity* entity1 = CreateEntity("Entity 1");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{entity1});
        AZStd::unique_ptr<Instance> newInstance = m_prefabSystemComponent->CreatePrefab({ entity1 }, {}, PrefabMockFilePath);
        TemplateId newTemplateId = newInstance->GetTemplateId();
        EXPECT_TRUE(newTemplateId != InvalidTemplateId);
        PrefabDom& newTemplateDom = m_prefabSystemComponent->FindTemplateDom(newTemplateId);
        AZStd::vector<EntityAlias> newTemplateEntityAliases = newInstance->GetEntityAliases();
        EXPECT_EQ(newTemplateEntityAliases.size(), 1);

        // Instantiate Instances and validate if all Instances have the entity.
        const int numberOfInstances = 3;
        AZStd::vector<AZStd::unique_ptr<Instance>> instantiatedInstances;
        for (int i = 0; i < numberOfInstances; ++i)
        {
            instantiatedInstances.emplace_back(m_prefabSystemComponent->InstantiatePrefab(newTemplateId));
            ASSERT_TRUE(instantiatedInstances.back());
            EXPECT_EQ(instantiatedInstances.back()->GetTemplateId(), newTemplateId);
        }

        PrefabTestDomUtils::ValidateEntitiesOfInstances(newTemplateId, newTemplateDom, newTemplateEntityAliases);

        // Add another entity to the Instance and use it to update the PrefabDom of Template.
        AZ::Entity* entity2 = CreateEntity("Entity 2");
        newInstance->AddEntity(*entity2);
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{entity2});
        newTemplateEntityAliases = newInstance->GetEntityAliases();
        EXPECT_EQ(newTemplateEntityAliases.size(), 2);

        PrefabDom updatedTemplateDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*newInstance, updatedTemplateDom));
        newTemplateDom.CopyFrom(updatedTemplateDom, newTemplateDom.GetAllocator());

        // Update Template's Instances and validate if all Instances have the new entity.
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(newTemplateId);
        const bool updateResult = m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        EXPECT_TRUE(updateResult);

        PrefabTestDomUtils::ValidateEntitiesOfInstances(newTemplateId, newTemplateDom, newTemplateEntityAliases);

    }

    TEST_F(PrefabUpdateInstancesTest, UpdatePrefabInstances_AddInstance_UpdateSucceeds)
    {
        // Create a Template with single entity.
        using namespace AzToolsFramework::Prefab;
        AZ::Entity* entity = CreateEntity("Entity");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{entity});
        AZStd::unique_ptr<Instance> newNestedInstance = m_prefabSystemComponent->CreatePrefab({ entity }, {}, NestedPrefabMockFilePath);
        TemplateId newNestedTemplateId = newNestedInstance->GetTemplateId();
        EXPECT_TRUE(newNestedTemplateId != InvalidTemplateId);
        EXPECT_EQ(newNestedInstance->GetEntityAliases().size(), 1);

        // Create an enclosing Template with 0 entities and 1 nested Instance.
        AZStd::unique_ptr<Instance> nestedInstance1 = m_prefabSystemComponent->InstantiatePrefab(newNestedTemplateId);
        AZStd::unique_ptr<Instance> newEnclosingInstance = m_prefabSystemComponent->CreatePrefab({}, MakeInstanceList(AZStd::move(nestedInstance1)), PrefabMockFilePath);
        TemplateId newEnclosingTemplateId = newEnclosingInstance->GetTemplateId();
        EXPECT_TRUE(newEnclosingTemplateId != InvalidTemplateId);
        PrefabDom& newEnclosingTemplateDom = m_prefabSystemComponent->FindTemplateDom(newEnclosingTemplateId);
        AZStd::vector<InstanceAlias> nestedInstanceAliases = newEnclosingInstance->GetNestedInstanceAliases(newNestedTemplateId);
        EXPECT_EQ(nestedInstanceAliases.size(), 1);

        // Instantiate enclosing Instances and validate if all enclosing Instances have the nested Instance.
        const int numberOfInstances = 3;
        AZStd::vector<AZStd::unique_ptr<Instance>> instantiatedInstances;
        for (int i = 0; i < numberOfInstances; ++i)
        {
            instantiatedInstances.emplace_back(m_prefabSystemComponent->InstantiatePrefab(newEnclosingTemplateId));
            ASSERT_TRUE(instantiatedInstances.back());
            EXPECT_EQ(instantiatedInstances.back()->GetTemplateId(), newEnclosingTemplateId);
        }

        PrefabTestDomUtils::ValidateNestedInstancesOfInstances(
            newEnclosingTemplateId, newEnclosingTemplateDom, nestedInstanceAliases);

        // Add another nested Instance to the enclosing Instance and use it to update the PrefabDom of Template.
        AZStd::unique_ptr<Instance> nestedInstance2 = m_prefabSystemComponent->InstantiatePrefab(newNestedTemplateId);
        newEnclosingInstance->AddInstance(AZStd::move(nestedInstance2));
       
        PrefabDom updatedTemplateDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*newEnclosingInstance, updatedTemplateDom));
        newEnclosingTemplateDom.CopyFrom(updatedTemplateDom, newEnclosingTemplateDom.GetAllocator());

        // Validate that there are 2 wheel Instances under the axle Instance
        nestedInstanceAliases = newEnclosingInstance->GetNestedInstanceAliases(newNestedTemplateId);
        EXPECT_EQ(nestedInstanceAliases.size(), 2);

        // Update axle Template's Instances and validate if all axle Instances have the new wheel Instance.
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(newEnclosingTemplateId);
        const bool updateResult = m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        EXPECT_TRUE(updateResult);

        PrefabTestDomUtils::ValidateNestedInstancesOfInstances(
            newEnclosingTemplateId, newEnclosingTemplateDom, nestedInstanceAliases);
    }

    TEST_F(PrefabUpdateInstancesTest, UpdatePrefabInstances_AddComponent_UpdateSucceeds)
    {
        // Create a Template from an Instance owning a single entity.
        AZ::Entity* entity = CreateEntity("Entity", false);
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{entity});
        AZStd::unique_ptr<Instance> newInstance = m_prefabSystemComponent->CreatePrefab({ entity }, {}, PrefabMockFilePath);
        TemplateId newTemplateId = newInstance->GetTemplateId();
        PrefabDom& newTemplateDom = m_prefabSystemComponent->FindTemplateDom(newTemplateId);
        AZStd::vector<EntityAlias> newTemplateEntityAliases = newInstance->GetEntityAliases();
        ASSERT_EQ(newTemplateEntityAliases.size(), 1);

        // Validate that the entity has 1 component under it. This is added through the scrubbing of entities in
        // EditorEntityContextComponent, which gets called through HandleEntitiesAdded during loading of entities in Prefab Instances.
        const PrefabDomValue* entityComponents =
            PrefabTestDomUtils::GetPrefabDomComponents(newTemplateDom, newTemplateEntityAliases.front());
        ASSERT_TRUE(entityComponents != nullptr && entityComponents->IsObject());
        EXPECT_EQ(entityComponents->MemberCount(), 1);

        // Instantiate Instances and validate if all Instances have the entity.
        const int numberOfInstances = 3;
        AZStd::vector<AZStd::unique_ptr<Instance>> instantiatedInstances;
        for (int i = 0; i < numberOfInstances; ++i)
        {
            instantiatedInstances.emplace_back(m_prefabSystemComponent->InstantiatePrefab(newTemplateId));
            ASSERT_TRUE(instantiatedInstances.back());
            EXPECT_EQ(instantiatedInstances.back()->GetTemplateId(), newTemplateId);
        }

        PrefabTestDomUtils::ValidateEntitiesOfInstances(newTemplateId, newTemplateDom, newTemplateEntityAliases);

        // Add a component to the Instance and use it to update the PrefabDom of Template.
        PrefabTestComponent* prefabTestComponent = aznew PrefabTestComponent(true);
        entity->Deactivate();
        entity->AddComponent(prefabTestComponent);
        PrefabDom updatedDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*newInstance, updatedDom));
        newTemplateDom.CopyFrom(updatedDom, newTemplateDom.GetAllocator());

        // Validate that the entity now has 2 components under it.
        entityComponents = PrefabTestDomUtils::GetPrefabDomComponents(newTemplateDom, newTemplateEntityAliases.front());
        ASSERT_TRUE(entityComponents != nullptr && entityComponents->IsObject());
        EXPECT_EQ(entityComponents->MemberCount(), 2);

        // Extract the component id of the entity in Template and verify that it matches with the component id of the Instance.
        PrefabTestDomUtils::ValidateComponentsDomHasId(*entityComponents, prefabTestComponent->GetId());

        // Update Template's Instances and validate if all Instances have the new component under their entities.
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(newTemplateId);
        const bool updateResult = m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        EXPECT_TRUE(updateResult);

        PrefabTestDomUtils::ValidateInstances(newTemplateId, *entityComponents,
            PrefabTestDomUtils::GetPrefabDomComponentsPath(newTemplateEntityAliases.front()));
    }

    TEST_F(PrefabUpdateInstancesTest, UpdatePrefabInstances_DetachEntity_UpdateSucceeds)
    {
        // Create a Template from an Instance owning 2 entities.
        using namespace AzToolsFramework::Prefab;
        AZ::Entity* entity1 = CreateEntity("Entity 1");
        AZ::Entity* entity2 = CreateEntity("Entity 2");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{entity1, entity2});
        AZStd::unique_ptr<Instance> newInstance = m_prefabSystemComponent->CreatePrefab(
            { entity1, entity2 },
            {},
            PrefabMockFilePath);
        TemplateId newTemplateId = newInstance->GetTemplateId();
        EXPECT_TRUE(newTemplateId != InvalidTemplateId);
        PrefabDom& newTemplateDom = m_prefabSystemComponent->FindTemplateDom(newTemplateId);
        AZStd::vector<EntityAlias> newTemplateEntityAliases = newInstance->GetEntityAliases();
        EXPECT_EQ(newTemplateEntityAliases.size(), 2);

        // Instantiate Instances and validate if all Instances have both entities.
        const int numberOfInstances = 3;
        AZStd::vector<AZStd::unique_ptr<Instance>> instantiatedInstances;
        for (int i = 0; i < numberOfInstances; ++i)
        {
            instantiatedInstances.emplace_back(m_prefabSystemComponent->InstantiatePrefab(newTemplateId));
            ASSERT_TRUE(instantiatedInstances.back());
            EXPECT_EQ(instantiatedInstances.back()->GetTemplateId(), newTemplateId);
        }

        PrefabTestDomUtils::ValidateEntitiesOfInstances(newTemplateId, newTemplateDom, newTemplateEntityAliases);

        // Remove an entity from the Instance and use the updated Instance to update the PrefabDom of Template.
        AZStd::unique_ptr<AZ::Entity> detachedEntity = newInstance->DetachEntity(entity1->GetId());
        ASSERT_TRUE(detachedEntity);
        EXPECT_EQ(detachedEntity->GetId(), entity1->GetId());
        newTemplateEntityAliases = newInstance->GetEntityAliases();
        EXPECT_EQ(newTemplateEntityAliases.size(), 1);

        PrefabDom updatedTemplateDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*newInstance, updatedTemplateDom));
        newTemplateDom.CopyFrom(updatedTemplateDom, newTemplateDom.GetAllocator());

        // Update Template's Instances and validate if all Instances have the remaining entity.
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(newTemplateId);
        const bool updateResult = m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        EXPECT_TRUE(updateResult);

        PrefabTestDomUtils::ValidateEntitiesOfInstances(newTemplateId, newTemplateDom, newTemplateEntityAliases);

    }

    TEST_F(PrefabUpdateInstancesTest, UpdatePrefabInstances_DetachNestedInstance_UpdateSucceeds)
    {
        // Create a Template with single entity.
        using namespace AzToolsFramework::Prefab;
        AZ::Entity* entity = CreateEntity("Entity");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{entity});
        AZStd::unique_ptr<Instance> newNestedInstance = m_prefabSystemComponent->CreatePrefab({ entity }, {}, NestedPrefabMockFilePath);
        TemplateId newNestedTemplateId = newNestedInstance->GetTemplateId();
        EXPECT_TRUE(newNestedTemplateId != InvalidTemplateId);
        EXPECT_EQ(newNestedInstance->GetEntityAliases().size(), 1);

        // Create an enclosing Template with 0 entities and 2 nested Instances.
        AZStd::unique_ptr<Instance> nestedInstance1 = m_prefabSystemComponent->InstantiatePrefab(newNestedTemplateId);
        AZStd::unique_ptr<Instance> nestedInstance2 = m_prefabSystemComponent->InstantiatePrefab(newNestedTemplateId);
        AZStd::unique_ptr<Instance> newEnclosingInstance = m_prefabSystemComponent->CreatePrefab(
            {},
            MakeInstanceList(AZStd::move(nestedInstance1), AZStd::move(nestedInstance2)),
            PrefabMockFilePath);
        TemplateId newEnclosingTemplateId = newEnclosingInstance->GetTemplateId();
        EXPECT_TRUE(newEnclosingTemplateId != InvalidTemplateId);
        PrefabDom& newEnclosingTemplateDom = m_prefabSystemComponent->FindTemplateDom(newEnclosingTemplateId);
        AZStd::vector<InstanceAlias> nestedInstanceAliases = newEnclosingInstance->GetNestedInstanceAliases(newNestedTemplateId);
        EXPECT_EQ(nestedInstanceAliases.size(), 2);

        // Instantiate enclosing Instances and validate if all enclosing Instances have both nested Instances.
        const int numberOfInstances = 3;
        AZStd::vector<AZStd::unique_ptr<Instance>> instantiatedInstances;
        for (int i = 0; i < numberOfInstances; ++i)
        {
            instantiatedInstances.emplace_back(m_prefabSystemComponent->InstantiatePrefab(newEnclosingTemplateId));
            ASSERT_TRUE(instantiatedInstances.back());
            EXPECT_EQ(instantiatedInstances.back()->GetTemplateId(), newEnclosingTemplateId);
        }

        PrefabTestDomUtils::ValidateNestedInstancesOfInstances(
            newEnclosingTemplateId, newEnclosingTemplateDom, nestedInstanceAliases);

        // Remove one nested Instance from the enclosing Instance
        // and use the updated enclosing Instance to update the PrefabDom of Template.
        AZStd::unique_ptr<Instance> detachedInstance = newEnclosingInstance->DetachNestedInstance(nestedInstanceAliases.front());
        ASSERT_TRUE(detachedInstance);
        m_prefabSystemComponent->RemoveLink(detachedInstance->GetLinkId());

        PrefabDom updatedTemplateDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*newEnclosingInstance, updatedTemplateDom));
        newEnclosingTemplateDom.CopyFrom(updatedTemplateDom, newEnclosingTemplateDom.GetAllocator());

        // Validate that there is only one nested Instances under the enclosing Instance.
        nestedInstanceAliases = newEnclosingInstance->GetNestedInstanceAliases(newNestedTemplateId);
        EXPECT_EQ(nestedInstanceAliases.size(), 1);

        // Update enclosing Template's Instances and validate if all enclosing Instances have the remaining nested Instances.
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(newEnclosingTemplateId);
        const bool updateResult = m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        EXPECT_TRUE(updateResult);

        PrefabTestDomUtils::ValidateNestedInstancesOfInstances(
            newEnclosingTemplateId, newEnclosingTemplateDom, nestedInstanceAliases);

    }

    TEST_F(PrefabUpdateInstancesTest, UpdatePrefabInstances_RemoveComponent_UpdateSucceeds)
    {
        // Create a Template from an Instance owning a single entity with a prefabTestComponent.
        AZ::Entity* entity = CreateEntity("Entity", false);
        PrefabTestComponent* prefabTestComponent = aznew PrefabTestComponent(true);
        entity->AddComponent(prefabTestComponent);
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{entity});
        AZStd::unique_ptr<Instance> newInstance = m_prefabSystemComponent->CreatePrefab({ entity }, {}, PrefabMockFilePath);
        TemplateId newTemplateId = newInstance->GetTemplateId();
        PrefabDom& newTemplateDom = m_prefabSystemComponent->FindTemplateDom(newTemplateId);
        AZStd::vector<EntityAlias> newTemplateEntityAliases = newInstance->GetEntityAliases();
        ASSERT_EQ(newTemplateEntityAliases.size(), 1);

        // Validate that the entity has 2 components under it. One of them is added through HandleEntitiesAdded() in EditorEntityContext.
        const PrefabDomValue* entityComponents =
            PrefabTestDomUtils::GetPrefabDomComponents(newTemplateDom, newTemplateEntityAliases.front());
        ASSERT_TRUE(entityComponents != nullptr && entityComponents->IsObject());
        EXPECT_EQ(entityComponents->MemberCount(), 2);

        // Extract the component id of the entity in the Template and verify that it matches with the component id of the entity's component.
        PrefabTestDomUtils::ValidateComponentsDomHasId(*entityComponents, prefabTestComponent->GetId());

        // Instantiate Instances and validate if all Instances have the entity.
        const int numberOfInstances = 3;
        AZStd::vector<AZStd::unique_ptr<Instance>> instantiatedInstances;
        for (int i = 0; i < numberOfInstances; ++i)
        {
            instantiatedInstances.emplace_back(m_prefabSystemComponent->InstantiatePrefab(newTemplateId));
            ASSERT_TRUE(instantiatedInstances.back());
            EXPECT_EQ(instantiatedInstances.back()->GetTemplateId(), newTemplateId);
        }

        PrefabTestDomUtils::ValidateInstances(
            newTemplateId, *entityComponents, PrefabTestDomUtils::GetPrefabDomComponentsPath(newTemplateEntityAliases.front()));

        // Remove a component from the Instance's entity and use the Instance to update the PrefabDom of Template.
        entity->Deactivate();
        entity->RemoveComponent(prefabTestComponent);
        delete prefabTestComponent;
        entity->Activate();
        PrefabDom updatedDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*newInstance, updatedDom));
        newTemplateDom.CopyFrom(updatedDom, newTemplateDom.GetAllocator());

        // Validate that the entity only has 1 component under it.
        entityComponents = PrefabTestDomUtils::GetPrefabDomComponents(newTemplateDom, newTemplateEntityAliases.front());
        ASSERT_TRUE(entityComponents != nullptr && entityComponents->IsObject());
        EXPECT_EQ(entityComponents->MemberCount(), 1);

        // Update Template's Instances and validate if all Instances have no component under their entities.
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(newTemplateId);
        const bool updateResult = m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        EXPECT_TRUE(updateResult);

        PrefabTestDomUtils::ValidateEntitiesOfInstances(newTemplateId, newTemplateDom, newTemplateEntityAliases);
    }

    TEST_F(PrefabUpdateInstancesTest, UpdatePrefabInstances_ChangeComponentProperty_UpdateSucceeds)
    {
        // Create a Template from an Instance owning a single entity with a PrefabTestComponent.
        AZ::Entity* entity = CreateEntity("Entity", false);
        PrefabTestComponent* prefabTestComponent = aznew PrefabTestComponent(true);
        entity->AddComponent(prefabTestComponent);
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{entity});
        AZStd::unique_ptr<Instance> newInstance = m_prefabSystemComponent->CreatePrefab({ entity }, {}, PrefabMockFilePath);
        TemplateId newTemplateId = newInstance->GetTemplateId();
        PrefabDom& newTemplateDom = m_prefabSystemComponent->FindTemplateDom(newTemplateId);
        AZStd::vector<EntityAlias> newTemplateEntityAliases = newInstance->GetEntityAliases();
        ASSERT_EQ(newTemplateEntityAliases.size(), 1);

        // Validate that the entity has 2 components under it. One of them is added through HandleEntitiesAdded() in EditorEntityContext.
        const PrefabDomValue* entityComponents =
            PrefabTestDomUtils::GetPrefabDomComponents(newTemplateDom, newTemplateEntityAliases.front());
        ASSERT_TRUE(entityComponents != nullptr && entityComponents->IsObject());
        EXPECT_EQ(entityComponents->MemberCount(), 2);

        // Extract the component id of the entity in the Template and verify that it matches with the component id of the entity's component.
        PrefabTestDomUtils::ValidateComponentsDomHasId(*entityComponents, prefabTestComponent->GetId());

        // Instantiate Instances and validate if all Instances have the entity.
        const int numberOfInstances = 3;
        AZStd::vector<AZStd::unique_ptr<Instance>> instantiatedInstances;
        for (int i = 0; i < numberOfInstances; ++i)
        {
            instantiatedInstances.emplace_back(m_prefabSystemComponent->InstantiatePrefab(newTemplateId));
            ASSERT_TRUE(instantiatedInstances.back());
            EXPECT_EQ(instantiatedInstances.back()->GetTemplateId(), newTemplateId);
        }

        PrefabDomPath entityComponentsPath = PrefabTestDomUtils::GetPrefabDomComponentsPath(newTemplateEntityAliases.front());
        PrefabTestDomUtils::ValidateInstances(
            newTemplateId, *entityComponents, entityComponentsPath);

        // Change the bool property of the component from the Instance and use the Instance to update the PrefabDom of Template.
        prefabTestComponent->m_boolProperty = false;
        PrefabDom updatedDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*newInstance, updatedDom));
        newTemplateDom.CopyFrom(updatedDom, newTemplateDom.GetAllocator());

        // Validate that the value of the BoolProperty of the prefabTestComponent in the Template's DOM has changed.
        entityComponents = PrefabTestDomUtils::GetPrefabDomComponents(newTemplateDom, newTemplateEntityAliases.front());
        ASSERT_TRUE(entityComponents != nullptr && entityComponents->IsObject());
        EXPECT_EQ(entityComponents->MemberCount(), 2);

        AZStd::string componentValueName = AZStd::string::format("Component_[%llu]", prefabTestComponent->GetId());
        PrefabDomValueConstReference wheelEntityComponentValue = PrefabDomUtils::FindPrefabDomValue(*entityComponents, componentValueName.c_str());
        ASSERT_TRUE(wheelEntityComponentValue);

        PrefabDomValueConstReference wheelEntityComponentBoolPropertyValue =
            PrefabDomUtils::FindPrefabDomValue(wheelEntityComponentValue->get(), PrefabTestDomUtils::BoolPropertyName);
        ASSERT_TRUE(wheelEntityComponentBoolPropertyValue.has_value() && wheelEntityComponentBoolPropertyValue->get() == false);

        // Update Template's Instances and validate if all Instances have no BoolProperty under their prefabTestComponents in entities.
        m_instanceUpdateExecutorInterface->AddTemplateInstancesToQueue(newTemplateId);
        const bool updateResult = m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();
        EXPECT_TRUE(updateResult);

        PrefabTestDomUtils::ValidateInstances(newTemplateId, *entityComponents, entityComponentsPath);
    }

}
