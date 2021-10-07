/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabUpdateTemplateTest = PrefabTestFixture;

    /*
        The below tests use an example of car->axle->wheel templates to test that change propagation works correctly within templates.
        The car template will have axle templates nested under it and the axle template will have wheel templates nested under it.
        Because of the complexity that arises from multiple levels of prefab nesting, it's easier to write tests using an example scenario
        than use generic nesting terminology.
    */

    TEST_F(PrefabUpdateTemplateTest, UpdatePrefabTemplate_AddEntity_AllDependentTemplatesUpdated)
    {
        // Create a single entity wheel instance and create a template out of it.
        AZ::Entity* wheelEntity = CreateEntity("WheelEntity1");
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity }, {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);
        AZStd::vector<EntityAlias> wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();

        // Validate that the wheel template has the same entities(1) as the instance it was created from.
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 1);
        PrefabTestDomUtils::ValidatePrefabDomEntities(wheelTemplateEntityAliases, wheelTemplateDom);

        // Create an axle with 0 entities and 2 wheel instances.
        AZStd::unique_ptr<Instance> wheel1UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> wheel2UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> axleInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(wheel1UnderAxle), AZStd::move(wheel2UnderAxle)), AxlePrefabMockFilePath);
        const TemplateId axleTemplateId = axleInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);
        PrefabDom& axleTemplateDom = m_prefabSystemComponent->FindTemplateDom(axleTemplateId);

        // Create a car with 0 entities, 2 axle instances and 1 wheel instance.
        AZStd::unique_ptr<Instance> axle1UnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance> axle2UnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance> spareWheelUnderCar = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> carInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(axle1UnderCar), AZStd::move(axle2UnderCar), AZStd::move(spareWheelUnderCar)), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        const AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(wheelTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        // Add another entity to a wheel instance and use it to update the wheel template.
        wheelIsolatedInstance->AddEntity(*CreateEntity("WheelEntity2"));
        PrefabDom updatedWheelInstance;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*wheelIsolatedInstance, updatedWheelInstance));
        m_prefabSystemComponent->UpdatePrefabTemplate(wheelTemplateId, updatedWheelInstance);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // Validate that the wheel template has the same entities(2) as the updated instance.
        wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 2);
        PrefabTestDomUtils::ValidatePrefabDomEntities(wheelTemplateEntityAliases, wheelTemplateDom);

        // Validate that the wheels under axle are updated with 2 entities
        PrefabTestDomUtils::ValidatePrefabDomInstances(wheelInstanceAliasesUnderAxle, axleTemplateDom, wheelTemplateDom);

        // Validate that the wheels of axles under the car have 2 entities
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);

        // Validate that the wheel under the car has 2 entities
        PrefabTestDomUtils::ValidatePrefabDomInstances(wheelInstanceAliasesUnderCar, carTemplateDom, wheelTemplateDom);
    }
    
    TEST_F(PrefabUpdateTemplateTest, UpdatePrefabTemplate_AddInstance_AllDependentTemplatesUpdated)
    {
        // Create a single entity wheel instance and create a template out of it.
        AZ::Entity* wheelEntity = CreateEntity("WheelEntity1");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{wheelEntity});
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity },
            {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);

        // Create an axle with 0 entities and 1 wheel instance.
        AZStd::unique_ptr<Instance> wheel1UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> axleInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(wheel1UnderAxle)), AxlePrefabMockFilePath);
        const TemplateId axleTemplateId = axleInstance->GetTemplateId();
        PrefabDom& axleTemplateDom = m_prefabSystemComponent->FindTemplateDom(axleTemplateId);
        AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);

        // Validate that there is only 1 wheel instance under axle.
        ASSERT_EQ(wheelInstanceAliasesUnderAxle.size(), 1);

        // Create a car with 0 entities and 2 axle instances.
        AZStd::unique_ptr<Instance> axle1UnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance> axle2UnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance>  carInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(axle1UnderCar), AZStd::move(axle2UnderCar)), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        // Add another Wheel instance to Axle instance and use it to update the Axle template.
        AZStd::unique_ptr<Instance> wheel2UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        axleInstance->AddInstance(AZStd::move(wheel2UnderAxle));
        PrefabDom updatedAxleInstanceDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*axleInstance, updatedAxleInstanceDom));
        m_prefabSystemComponent->UpdatePrefabTemplate(axleTemplateId, updatedAxleInstanceDom);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // Validate that there are 2 wheel instances under axle
        wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);
        ASSERT_EQ(wheelInstanceAliasesUnderAxle.size(), 2);

        // Validate that the wheels under the axle have the same DOM as the wheel template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(wheelInstanceAliasesUnderAxle, axleTemplateDom, wheelTemplateDom, false);

        // Validate that the axles under the car have the same DOM as the axle template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);
    }

    TEST_F(PrefabUpdateTemplateTest, UpdatePrefabTemplate_AddComponent_AllDependentTemplatesUpdated)
    {
        // Create a single entity wheel instance and create a template out of it.
        AZ::Entity* wheelEntity = CreateEntity("WheelEntity1", false);
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity },
            {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);
        AZStd::vector<EntityAlias> wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();

        // Validate that the wheel template has the same entities(1) as the instance it was created from.
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 1);

        // Validate that the wheel entity doesn't have any components under it.
        EntityAlias entityAlias = wheelTemplateEntityAliases.front();
        PrefabDomValue* wheelEntityComponents =
            PrefabTestDomUtils::GetPrefabDomComponentsPath(entityAlias).Get(wheelTemplateDom);
        ASSERT_TRUE(wheelEntityComponents->IsArray() && wheelEntityComponents->Size() == 0);

        // Create an axle with 0 entities and 1 wheel instance.
        AZStd::unique_ptr<Instance> wheel1UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> axleInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(wheel1UnderAxle)), AxlePrefabMockFilePath);
        const TemplateId axleTemplateId = axleInstance->GetTemplateId();
        PrefabDom& axleTemplateDom = m_prefabSystemComponent->FindTemplateDom(axleTemplateId);
        const AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);

        // Create a car with 0 entities and 1 axle instance.
        AZStd::unique_ptr<Instance> axleUnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance>  carInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(axleUnderCar)), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        // Add a component to Wheel instance and use it to update the wheel template.
        PrefabTestComponent* prefabTestComponent = aznew PrefabTestComponent(true);
        wheelEntity->AddComponent(prefabTestComponent);
        auto expectedComponentId = prefabTestComponent->GetId();
        PrefabDom updatedWheelInstanceDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*wheelIsolatedInstance, updatedWheelInstanceDom));
        m_prefabSystemComponent->UpdatePrefabTemplate(wheelTemplateId, updatedWheelInstanceDom);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // Validate that the wheel entity does have a component under it.
        wheelEntityComponents = PrefabTestDomUtils::GetPrefabDomComponentsPath(entityAlias).Get(wheelTemplateDom);
        ASSERT_TRUE(wheelEntityComponents != nullptr && wheelEntityComponents->IsObject());
        EXPECT_EQ(wheelEntityComponents->MemberCount(), 1);

        // Extract the component id of the entity in wheel template and verify that it matches with the component id of the wheel instance.
        PrefabTestDomUtils::ValidateComponentsDomHasId(*wheelEntityComponents, expectedComponentId);

        // Validate that the wheels under the axle have the same DOM as the wheel template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(wheelInstanceAliasesUnderAxle, axleTemplateDom, wheelTemplateDom);

        // Validate that the axles under the car have the same DOM as the axle template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);
    }

    TEST_F(PrefabUpdateTemplateTest, UpdatePrefabTemplate_DetachEntity_AllDependentTemplatesUpdated)
    {
        // Create wheel instance with 2 entities and create a template out of it.
        AZ::Entity* wheelEntity1 = CreateEntity("WheelEntity1");
        AZ::Entity* wheelEntity2 = CreateEntity("WheelEntity2");
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity1, wheelEntity2 },
            {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);

        // Validate that the wheel template has the same entities(2) as the instance it was created from.
        AZStd::vector<EntityAlias> wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 2);

        // Create an axle with 0 entities and 1 wheel instance.
        AZStd::unique_ptr<Instance> wheel1UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> axleInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(wheel1UnderAxle)), AxlePrefabMockFilePath);
        const TemplateId axleTemplateId = axleInstance->GetTemplateId();
        PrefabDom& axleTemplateDom = m_prefabSystemComponent->FindTemplateDom(axleTemplateId);
        const AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);

        // Create a car with 0 entities and 1 axle instance.
        AZStd::unique_ptr<Instance> axleUnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance> carInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(axleUnderCar)), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        // Detach the first entity from the Wheel instance and use it to update the wheel template.
        AZStd::unique_ptr<AZ::Entity> detachedEntity = wheelIsolatedInstance->DetachEntity(wheelEntity1->GetId());
        ASSERT_TRUE(detachedEntity);
        PrefabDom updatedWheelInstanceDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*wheelIsolatedInstance, updatedWheelInstanceDom));
        m_prefabSystemComponent->UpdatePrefabTemplate(wheelTemplateId, updatedWheelInstanceDom);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // Validate that the wheel template only has 1 entity now.
        wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 1);
        PrefabTestDomUtils::ValidatePrefabDomEntities(wheelTemplateEntityAliases, wheelTemplateDom);

        // Validate that the wheels under the axle have the same DOM as the wheel template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(wheelInstanceAliasesUnderAxle, axleTemplateDom, wheelTemplateDom);

        // Validate that the axles under the car have the same DOM as the axle template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);
    }

    TEST_F(PrefabUpdateTemplateTest, UpdatePrefabTemplate_DetachNestedInstance_AllDependentTemplatesUpdated)
    {
        // Create a single entity wheel instance and create a template out of it.
        AZ::Entity* wheelEntity = CreateEntity("WheelEntity1");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{wheelEntity});
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity },
            {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);

        // Create an axle with 0 entities and 2 wheel instances.
        AZStd::unique_ptr<Instance> wheel1UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> wheel2UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> axleInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(wheel1UnderAxle), AZStd::move(wheel2UnderAxle) ),
            AxlePrefabMockFilePath);
        const TemplateId axleTemplateId = axleInstance->GetTemplateId();
        PrefabDom& axleTemplateDom = m_prefabSystemComponent->FindTemplateDom(axleTemplateId);
        AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);

        // Validate that there are 2 wheel instances under axle.
        ASSERT_EQ(wheelInstanceAliasesUnderAxle.size(), 2);

        // Create a car with 0 entities and 1 axle instance.
        AZStd::unique_ptr<Instance> axle1UnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance> carInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(axle1UnderCar)), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        // Detach second wheel instance from Axle instance and use it to update the Axle template.
        InstanceAlias aliasOfWheelInstanceToRetain = wheelInstanceAliasesUnderAxle.front();
        AZStd::unique_ptr<Instance> detachedInstance = axleInstance->DetachNestedInstance(wheelInstanceAliasesUnderAxle.back());
        ASSERT_TRUE(detachedInstance);
        m_prefabSystemComponent->RemoveLink(detachedInstance->GetLinkId());
        PrefabDom updatedAxleInstanceDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*axleInstance, updatedAxleInstanceDom));
        m_prefabSystemComponent->UpdatePrefabTemplate(axleTemplateId, updatedAxleInstanceDom);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // Validate that there is only 1 wheel instances under axle
        wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);
        ASSERT_EQ(wheelInstanceAliasesUnderAxle.size(), 1);
        EXPECT_EQ(wheelInstanceAliasesUnderAxle.front(), aliasOfWheelInstanceToRetain);

        // Validate that the wheels under the axle have the same DOM as the wheel template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(wheelInstanceAliasesUnderAxle, axleTemplateDom, wheelTemplateDom, false);

        // Validate that the axles under the car have the same DOM as the axle template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);
    }

    TEST_F(PrefabUpdateTemplateTest, UpdatePrefabTemplate_RemoveComponent_AllDependentTemplatesUpdated)
    {
        // Create a single entity wheel instance with a PrefabTestComponent and create a template out of it.
        AZ::Entity* wheelEntity = CreateEntity("WheelEntity1", false);
        PrefabTestComponent* prefabTestComponent = aznew PrefabTestComponent(true);
        wheelEntity->AddComponent(prefabTestComponent);
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity },
            {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);
        AZStd::vector<EntityAlias> wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();

        // Validate that the wheel template has the same entities(1) as the instance it was created from.
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 1);

        // Validate that the wheel entity has 1 component under it.
        AZStd::string entityAlias = wheelTemplateEntityAliases.front();
        PrefabDomValue* wheelEntityComponents =
            PrefabTestDomUtils::GetPrefabDomComponentsPath(entityAlias).Get(wheelTemplateDom);
        ASSERT_TRUE(wheelEntityComponents != nullptr && wheelEntityComponents->IsObject());
        EXPECT_EQ(wheelEntityComponents->MemberCount(), 1);

        // Extract the component id of the entity in wheel template and verify that it matches with the component id of the wheel instance.
        PrefabTestDomUtils::ValidateComponentsDomHasId(*wheelEntityComponents, prefabTestComponent->GetId());

        // Create an axle with 0 entities and 1 wheel instance.
        AZStd::unique_ptr<Instance> wheel1UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> axleInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(wheel1UnderAxle)), AxlePrefabMockFilePath);
        const TemplateId axleTemplateId = axleInstance->GetTemplateId();
        PrefabDom& axleTemplateDom = m_prefabSystemComponent->FindTemplateDom(axleTemplateId);
        const AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);

        // Create a car with 0 entities and 1 axle instance.
        AZStd::unique_ptr<Instance> axleUnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance> carInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(axleUnderCar)), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        // Remove the component from Wheel instance and use it to update the wheel template.
        wheelEntity->RemoveComponent(prefabTestComponent);
        PrefabDom updatedWheelInstanceDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*wheelIsolatedInstance, updatedWheelInstanceDom));
        m_prefabSystemComponent->UpdatePrefabTemplate(wheelTemplateId, updatedWheelInstanceDom);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // Validate that the wheel entity does not have a component under it.
        wheelEntityComponents = PrefabTestDomUtils::GetPrefabDomComponentsPath(entityAlias).Get(wheelTemplateDom);
        ASSERT_TRUE(wheelEntityComponents->IsArray() && wheelEntityComponents->Size() == 0);

        // Validate that the wheels under the axle have the same DOM as the wheel template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(wheelInstanceAliasesUnderAxle, axleTemplateDom, wheelTemplateDom);

        // Validate that the axles under the car have the same DOM as the axle template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);

        delete prefabTestComponent;
    }

    TEST_F(PrefabUpdateTemplateTest, UpdatePrefabTemplate_ChangeComponentProperty_AllDependentTemplatesUpdated)
    {
        // Create a single entity wheel instance with a PrefabTestComponent and create a template out of it.
        AZ::Entity* wheelEntity = CreateEntity("WheelEntity1", false);
        PrefabTestComponent* prefabTestComponent = aznew PrefabTestComponent(true);
        wheelEntity->AddComponent(prefabTestComponent);
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity },
            {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);
        AZStd::vector<EntityAlias> wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();

        // Validate that the wheel template has the same entities(1) as the instance it was created from.
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 1);

        // Validate that the wheel entity has 1 component under it.
        AZStd::string entityAlias = wheelTemplateEntityAliases.front();
        PrefabDomValue* wheelEntityComponents =
            PrefabTestDomUtils::GetPrefabDomComponentsPath(entityAlias).Get(wheelTemplateDom);
        ASSERT_TRUE(wheelEntityComponents != nullptr && wheelEntityComponents->IsObject());
        EXPECT_EQ(wheelEntityComponents->MemberCount(), 1);

        // Extract the component id of the entity in wheel template and verify that it matches with the component id of the wheel instance.
        PrefabTestDomUtils::ValidateComponentsDomHasId(*wheelEntityComponents, prefabTestComponent->GetId());

        // Create an axle with 0 entities and 1 wheel instance.
        AZStd::unique_ptr<Instance> wheel1UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        AZStd::unique_ptr<Instance> axleInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(wheel1UnderAxle)), AxlePrefabMockFilePath);
        const TemplateId axleTemplateId = axleInstance->GetTemplateId();
        PrefabDom& axleTemplateDom = m_prefabSystemComponent->FindTemplateDom(axleTemplateId);
        const AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);

        // Create a car with 0 entities and 1 axle instance.
        AZStd::unique_ptr<Instance> axleUnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance> carInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(axleUnderCar)), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        // Change the bool property of the component from Wheel instance and use it to update the wheel template.
        prefabTestComponent->m_boolProperty = false;
        PrefabDom updatedWheelInstanceDom;
        ASSERT_TRUE(PrefabDomUtils::StoreInstanceInPrefabDom(*wheelIsolatedInstance, updatedWheelInstanceDom));
        m_prefabSystemComponent->UpdatePrefabTemplate(wheelTemplateId, updatedWheelInstanceDom);
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // Validate that the BoolProperty of the prefabTestComponent in the wheel template DOM is set to false.
        wheelEntityComponents = PrefabTestDomUtils::GetPrefabDomComponentsPath(entityAlias).Get(wheelTemplateDom);
        ASSERT_TRUE(wheelEntityComponents != nullptr && wheelEntityComponents->IsObject());
        EXPECT_EQ(wheelEntityComponents->MemberCount(), 1);

        PrefabDomValueReference wheelEntityComponentBoolPropertyValue =
            PrefabDomUtils::FindPrefabDomValue(wheelEntityComponents->MemberBegin()->value, PrefabTestDomUtils::BoolPropertyName);
        ASSERT_TRUE(wheelEntityComponentBoolPropertyValue.has_value() && wheelEntityComponentBoolPropertyValue->get() == false);

        // Validate that the wheels under the axle have the same DOM as the wheel template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(wheelInstanceAliasesUnderAxle, axleTemplateDom, wheelTemplateDom);

        // Validate that the axles under the car have the same DOM as the axle template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);
    }
}
