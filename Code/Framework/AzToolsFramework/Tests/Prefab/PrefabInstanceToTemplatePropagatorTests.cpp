/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzFramework/Components/TransformComponent.h>

namespace UnitTest
{
    using PrefabInstanceToTemplateTests = PrefabTestFixture;

    TEST_F(PrefabInstanceToTemplateTests, PrefabUpdateTemplate_UpdateEntityOnInstance)
    {
        //create template with single entity
        const char* newEntityName = "New Entity";
        AZ::Entity* newEntity = CreateEntity(newEntityName, false);
        ASSERT_TRUE(newEntity);
        AZ::EntityId entityId = newEntity->GetId();

        //add a transform component for testing purposes
        newEntity->CreateComponent(AZ::EditorTransformComponentTypeId);
        newEntity->Init();
        newEntity->Activate();

        AZStd::unique_ptr<Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({ newEntity }, {}, "test/path");
        ASSERT_TRUE(firstInstance);

        EntityAliasOptionalReference newEntityAliasReference = firstInstance->GetEntityAlias(entityId);
        ASSERT_TRUE(newEntityAliasReference);

        EntityAlias newEntityAlias = newEntityAliasReference.value();

        //get template id
        TemplateId templateId = firstInstance->GetTemplateId();

        //instantiate second instance
        AZStd::unique_ptr<Instance> secondInstance = m_prefabSystemComponent->InstantiatePrefab(templateId);
        ASSERT_TRUE(secondInstance);

        //create document with before change snapshot
        PrefabDom entityDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateDomForEntity(entityDomBeforeUpdate, *newEntity);

        //update values on entity
        const float updatedXValue = 5.0f;
        AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetWorldX, updatedXValue);

        //create document with after change snapshot
        PrefabDom entityDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForEntity(entityDomAfterUpdate, *newEntity);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, entityDomBeforeUpdate, entityDomAfterUpdate);

        //update template
        ASSERT_TRUE(m_instanceToTemplateInterface->PatchEntityInTemplate(patch, entityId));
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        ValidateInstanceEntitiesActive(*secondInstance);

        //get the entity id
        AZ::EntityId secondEntityId = secondInstance->GetEntityId(newEntityAlias);
        ASSERT_TRUE(secondEntityId.IsValid());

        //verify template updated correctly
        //get the values from the transform on the entity
        float confirmXValue = 0.0f;
        AZ::TransformBus::EventResult(confirmXValue, entityId, &AZ::TransformInterface::GetWorldX);
        AZ::TransformBus::EventResult(confirmXValue, secondEntityId, &AZ::TransformInterface::GetWorldX);

        ASSERT_TRUE(confirmXValue == updatedXValue);
    }

    TEST_F(PrefabInstanceToTemplateTests, PrefabUpdateTemplate_AddEntityToInstance)
    {
        //create template with single entity
        const char* newEntityName = "New Entity";
        AZ::Entity* newEntity = CreateEntity(newEntityName, false);
        ASSERT_TRUE(newEntity);

        //create a first instance where the entity will be added
        AZStd::unique_ptr<Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({}, {}, "test/path");
        ASSERT_TRUE(firstInstance);

        //get template id
        TemplateId templateId = firstInstance->GetTemplateId();

        //instantiate second instance for checking if propogation works
        AZStd::unique_ptr<Instance> secondInstance = m_prefabSystemComponent->InstantiatePrefab(templateId);
        ASSERT_TRUE(secondInstance);

        //create document with before change snapshot
        PrefabDom instanceDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBeforeUpdate, *firstInstance);

        //add entity to instance
        firstInstance->AddEntity(*newEntity);

        //create document with after change snapshot
        PrefabDom instanceDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfterUpdate, *firstInstance);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, instanceDomBeforeUpdate, instanceDomAfterUpdate);

        //update template
        m_instanceToTemplateInterface->PatchTemplate(patch, templateId);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        //get the entity id
        AZStd::vector<AZ::EntityId> entityIdVector;

        secondInstance->GetEntityIds([&entityIdVector](AZ::EntityId entityId)
        {
            entityIdVector.push_back(entityId);
            return true;
        });

        // There should be 2 entities in the instance, the container entity and the one we added
        EXPECT_EQ(entityIdVector.size(), 2);
    }

    TEST_F(PrefabInstanceToTemplateTests, PrefabUpdateTemplate_RemoveEntityFromInstance)
    {
        //create template with single entity

        AZ::Entity* newEntity = CreateEntity("New Entity", false);
        ASSERT_TRUE(newEntity);
        AZ::EntityId entityId = newEntity->GetId();

        //create a first instance where the entity will be removed
        AZStd::unique_ptr<Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({ newEntity }, {}, "test/path");
        ASSERT_TRUE(firstInstance);

        //get template id
        TemplateId templateId = firstInstance->GetTemplateId();

        //instantiate second instance for checking if propogation works
        AZStd::unique_ptr<Instance> secondInstance = m_prefabSystemComponent->InstantiatePrefab(templateId);
        ASSERT_TRUE(secondInstance);

        //create document with before change snapshot
        PrefabDom instanceDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBeforeUpdate, *firstInstance);

        //remove entity from instance
        firstInstance->DetachEntity(entityId);

        //create document with after change snapshot
        PrefabDom instanceDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfterUpdate, *firstInstance);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, instanceDomBeforeUpdate, instanceDomAfterUpdate);

        //update template
        m_instanceToTemplateInterface->PatchTemplate(patch, templateId);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        //get the entity id
        AZStd::vector<AZ::EntityId> entityIdVector;

        secondInstance->GetEntityIds([&entityIdVector](AZ::EntityId entityId)
        {
            entityIdVector.push_back(entityId);
            return true;
        });

        // There should be 1 entity, the container and no others since we removed the other
        EXPECT_EQ(entityIdVector.size(), 1);
    }

    TEST_F(PrefabInstanceToTemplateTests, PrefabUpdateTemplate_AddInstanceToInstance)
    {
        //create a first instance where the instance will be added
        AZStd::unique_ptr<Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({}, {}, "test/path");
        ASSERT_TRUE(firstInstance);

        //get template id
        TemplateId templateId = firstInstance->GetTemplateId();

        //instantiate second instance for checking if propogation works
        AZStd::unique_ptr<Instance> secondInstance = m_prefabSystemComponent->InstantiatePrefab(templateId);
        ASSERT_TRUE(secondInstance);

        //create document with before change snapshot
        PrefabDom instanceDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBeforeUpdate, *firstInstance);

        //create new instance and get alias
        AZStd::unique_ptr<Instance> addedInstance = m_prefabSystemComponent->CreatePrefab({}, {}, "test/pathtest");

        //add instance to instance
        Instance& addedInstanceRef = firstInstance->AddInstance(AZStd::move(addedInstance));
        const AzToolsFramework::Prefab::InstanceAlias addedAlias = addedInstanceRef.GetInstanceAlias();

        //create document with after change snapshot
        PrefabDom instanceDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfterUpdate, *firstInstance);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, instanceDomBeforeUpdate, instanceDomAfterUpdate);

        //update template
        m_instanceToTemplateInterface->PatchTemplate(patch, templateId);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        EXPECT_NE(secondInstance->FindNestedInstance(addedAlias), AZStd::nullopt);
    }

    TEST_F(PrefabInstanceToTemplateTests, PrefabUpdateTemplate_RemoveInstanceFromInstance)
    {
        AZStd::unique_ptr<Instance> addedInstancePtr = m_prefabSystemComponent->CreatePrefab({}, {}, "test/pathtest");
        Instance& addedInstance = *addedInstancePtr;

        //create a first instance where the instance will be removed
        AZStd::unique_ptr<Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({}, MakeInstanceList( AZStd::move(addedInstancePtr) ), "test/path");
        ASSERT_TRUE(firstInstance);

        //get added instance alias
        const AzToolsFramework::Prefab::InstanceAlias addedAlias = addedInstance.GetInstanceAlias();

        //get template id
        TemplateId templateId = firstInstance->GetTemplateId();

        //instantiate second instance for checking if propagation works
        AZStd::unique_ptr<Instance> secondInstance = m_prefabSystemComponent->InstantiatePrefab(templateId);
        ASSERT_TRUE(secondInstance);

        //create document with before change snapshot
        PrefabDom instanceDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBeforeUpdate, *firstInstance);

        //remove instance from instance
        firstInstance->DetachNestedInstance(addedAlias);

        //create document with after change snapshot
        PrefabDom instanceDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfterUpdate, *firstInstance);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, instanceDomBeforeUpdate, instanceDomAfterUpdate);

        //update template
        m_instanceToTemplateInterface->PatchTemplate(patch, templateId);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        EXPECT_EQ(secondInstance->FindNestedInstance(addedAlias), AZStd::nullopt);
    }
}
