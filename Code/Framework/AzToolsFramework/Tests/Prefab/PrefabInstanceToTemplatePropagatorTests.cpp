/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        m_instanceToTemplateInterface->PatchEntityInTemplate(patch, entityId);

        //activate the entity so we can access via transform bus
        secondInstance->InitializeNestedEntities();
        secondInstance->ActivateNestedEntities();

        //get the entity id
        AZStd::vector<AZ::EntityId> entityIdVector;

        secondInstance->GetEntityIds([&entityIdVector](const AZ::EntityId& entityId)
        {
            entityIdVector.push_back(entityId);
            return true;
        });

        EXPECT_EQ(entityIdVector.size(), 1);
        AZStd::optional<AZ::EntityId> secondEntityId = entityIdVector[0];

        //verify template updated correctly
        //get the values from the transform on the entity
        float confirmXValue = 0.0f;
        AZ::TransformBus::EventResult(confirmXValue, entityId, &AZ::TransformInterface::GetWorldX);
        AZ::TransformBus::EventResult(confirmXValue, secondEntityId.value(), &AZ::TransformInterface::GetWorldX);

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

        //get the entity id
        AZStd::vector<AZ::EntityId> entityIdVector;

        secondInstance->GetEntityIds([&entityIdVector](const AZ::EntityId& entityId)
        {
            entityIdVector.push_back(entityId);
            return true;
        });

        EXPECT_EQ(entityIdVector.size(), 1);
    }

    TEST_F(PrefabInstanceToTemplateTests, PrefabUpdateTemplate_RemoveEntityFromInstance)
    {
        //create template with single entity
        const char* newEntityName = "New Entity";
        AZ::Entity* newEntity = CreateEntity(newEntityName, false);
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

        //get the entity id
        AZStd::vector<AZ::EntityId> entityIdVector;

        secondInstance->GetEntityIds([&entityIdVector](const AZ::EntityId& entityId)
        {
            entityIdVector.push_back(entityId);
            return true;
        });

        EXPECT_EQ(entityIdVector.size(), 0);
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
        InstanceOptionalConstReference addedInstanceRef { firstInstance->AddInstance(AZStd::move(addedInstance)) };
        const AzToolsFramework::Prefab::InstanceAlias addedAlias = addedInstanceRef->get().GetInstanceAlias();

        //create document with after change snapshot
        PrefabDom instanceDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateDomForInstance(instanceDomAfterUpdate, *firstInstance);

        //generate patch
        PrefabDom patch;
        m_instanceToTemplateInterface->GeneratePatch(patch, instanceDomBeforeUpdate, instanceDomAfterUpdate);

        //update template
        m_instanceToTemplateInterface->PatchTemplate(patch, templateId);

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

        //instantiate second instance for checking if propogation works
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

        EXPECT_EQ(secondInstance->FindNestedInstance(addedAlias), AZStd::nullopt);
    }
}
