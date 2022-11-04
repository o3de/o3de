/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndo.h>

#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabUndoTests = PrefabTestFixture;

    TEST_F(PrefabUndoTests, PrefabUndoEntityUpdate)
    {
        //create template with single entity
        AZ::Entity* newEntity = CreateEntity("New Entity", false);
        ASSERT_TRUE(newEntity);
        AZ::EntityId entityId = newEntity->GetId();

        //add a transform component for testing purposes
        newEntity->CreateComponent(AZ::EditorTransformComponentTypeId);
        newEntity->Init();
        newEntity->Activate();

        AZStd::unique_ptr<Instance> testInstance = m_prefabSystemComponent->CreatePrefab({ newEntity }, {}, "test/path");
        ASSERT_TRUE(testInstance);

        //get template id
        TemplateId templateId = testInstance->GetTemplateId();

        //create document with before change snapshot
        PrefabDom entityDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateDomForEntity(entityDomBeforeUpdate, *newEntity);

        float checkXValue = 0.0f;
        AZ::TransformBus::EventResult(checkXValue, entityId, &AZ::TransformInterface::GetWorldX);
        ASSERT_TRUE(0.0f == checkXValue);

        //update values on entity
        const float updatedXValue = 5.0f;
        AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetWorldX, updatedXValue);

        AZ::TransformBus::EventResult(checkXValue, entityId, &AZ::TransformInterface::GetWorldX);
        ASSERT_TRUE(updatedXValue == checkXValue);

        //create document with after change snapshot
        PrefabDom entityDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForEntity(entityDomAfterUpdate, *newEntity);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, entityDomBeforeUpdate, entityDomAfterUpdate);

        //create undo node
        PrefabUndoEntityUpdate instanceEntityUndo("Entity Update Undo Node");
        instanceEntityUndo.Capture(entityDomBeforeUpdate, entityDomAfterUpdate, entityId);

        EntityAliasOptionalReference entityAliasRef = testInstance->GetEntityAlias(entityId);
        ASSERT_TRUE(entityAliasRef);

        EntityAlias entityAlias = entityAliasRef.value();

        //update template
        ASSERT_TRUE(m_instanceToTemplateInterface->PatchEntityInTemplate(patch, entityId));

        //undo change
        instanceEntityUndo.Undo();
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // verify template updated correctly
        //instantiate second instance for checking if propagation works
        AZStd::unique_ptr<Instance> secondInstance = m_prefabSystemComponent->InstantiatePrefab(
            templateId, AZStd::nullopt,
            [](const AzToolsFramework::EntityList& entities)
            {
                AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                    &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, entities);
            });

        ASSERT_TRUE(secondInstance);

        ValidateInstanceEntitiesActive(*secondInstance);

        //get the values from the transform on the entity
        AZ::EntityId secondNewEntity = secondInstance->GetEntityId(entityAlias);

        const float confirmXValue = 0.0f;
        float value = 10.0f;
        AZ::TransformBus::EventResult(value, secondNewEntity, &AZ::TransformInterface::GetWorldX);

        ASSERT_TRUE(confirmXValue == value);
    }

    TEST_F(PrefabUndoTests, PrefabUndoInstanceUpdate_AddEntity)
    {
        //create single entity
        AZ::Entity* newEntity = CreateEntity("New Entity", false);
        ASSERT_TRUE(newEntity);

        //create a first instance where the entity will be added
        AZStd::unique_ptr<Instance> testInstance = m_prefabSystemComponent->CreatePrefab({}, {}, "test/path");
        ASSERT_TRUE(testInstance);

        //get template id
        TemplateId templateId = testInstance->GetTemplateId();

        //create document with before change snapshot
        PrefabDom instanceDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBeforeUpdate, *testInstance);

        //add entity to instance
        testInstance->AddEntity(*newEntity);

        //create document with after change snapshot
        PrefabDom instanceDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfterUpdate, *testInstance);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, instanceDomBeforeUpdate, instanceDomAfterUpdate);

        //create undo node
        PrefabUndoInstance instanceEntityAddUndo("Entity Update Undo Node");
        instanceEntityAddUndo.Capture(instanceDomBeforeUpdate, instanceDomAfterUpdate, templateId);

        //update template
        m_instanceToTemplateInterface->PatchTemplate(patch, templateId);

        //undo change
        instanceEntityAddUndo.Undo();
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        //get the entity id
        AZStd::vector<AZ::EntityId> entityIdVector;

        testInstance->GetEntityIds([&entityIdVector](const AZ::EntityId& entityId)
        {
            entityIdVector.push_back(entityId);
            return true;
        });

        // Entity count minimum is 1 due to container entity
        EXPECT_EQ(entityIdVector.size(), 1);
    }

    TEST_F(PrefabUndoTests, PrefabUndoInstanceUpdate_RemoveEntity)
    {
        //create single entity
        AZ::Entity* newEntity = CreateEntity("New Entity", false);
        ASSERT_TRUE(newEntity);
        AZ::EntityId entityId = newEntity->GetId();

        //create a first instance where the entity will be added
        AZStd::unique_ptr<Instance> testInstance = m_prefabSystemComponent->CreatePrefab({newEntity}, {}, "test/path");
        ASSERT_TRUE(testInstance);

        //get template id
        TemplateId templateId = testInstance->GetTemplateId();

        //create document with before change snapshot
        PrefabDom instanceDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomBeforeUpdate, *testInstance);

        //detach entity from instance
        testInstance->DetachEntity(entityId);

        //create document with after change snapshot
        PrefabDom instanceDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfterUpdate, *testInstance);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, instanceDomBeforeUpdate, instanceDomAfterUpdate);

        //create undo node
        PrefabUndoInstance instanceEntityRemoveUndo("Entity Update Undo Node");
        instanceEntityRemoveUndo.Capture(instanceDomBeforeUpdate, instanceDomAfterUpdate, templateId);

        //update template
        m_instanceToTemplateInterface->PatchTemplate(patch, templateId);

        //undo change
        instanceEntityRemoveUndo.Undo();
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        //get the entity id
        AZStd::vector<AZ::EntityId> entityIdVector;

        testInstance->GetEntityIds([&entityIdVector](const AZ::EntityId& entityId)
        {
            entityIdVector.push_back(entityId);
            return true;
        });

        // Entity count is container entity + our entity restored via the undo
        EXPECT_EQ(entityIdVector.size(), 2);
    }
}
