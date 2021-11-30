/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabDomUtils.h>
#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace UnitTest
{
    using PrefabUpdateWithPatchesTest = PrefabTestFixture;

    /*
        The below tests use an example of car->axle->wheel templates to test that change propagation works correctly within templates.
        The car template will have axle templates nested under it and the axle template will have wheel templates nested under it.
        Because of the complexity that arises from multiple levels of prefab nesting, it's easier to write tests using an example scenario
        than use generic nesting terminology.
    */

    TEST_F(PrefabUpdateWithPatchesTest, ApplyPatchesToInstance_ComponentUpdated_PatchAppliedCorrectly)
    {
        // Create a single entity wheel instance with a PrefabTestComponent and create a template out of it.
        AZ::Entity* wheelEntity = CreateEntity("WheelEntity1", false);
        PrefabTestComponent* prefabTestComponent = aznew PrefabTestComponent(true);
        wheelEntity->AddComponent(prefabTestComponent);
        AZ::ComponentId prefabTestComponentId = prefabTestComponent->GetId();

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{wheelEntity});
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity },
            {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);
        AZStd::vector<EntityAlias> wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();

        // Validate that the wheel template has the same entities(1) as the instance it was created from.
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 1);

        // Validate that the wheel entity has 2 components. One of them is added through HandleEntitiesAdded() in EditorEntityContext.
        EntityAlias wheelEntityAlias = wheelTemplateEntityAliases.front();
        PrefabDomValue* wheelEntityComponents =
            PrefabTestDomUtils::GetPrefabDomComponentsPath(wheelEntityAlias).Get(wheelTemplateDom);
        ASSERT_TRUE(wheelEntityComponents != nullptr && wheelEntityComponents->IsObject());
        EXPECT_EQ(wheelEntityComponents->MemberCount(), 2);

        // Extract the component id of the entity in wheel template and verify that it matches with the component id of the wheel instance.
        PrefabTestDomUtils::ValidateComponentsDomHasId(*wheelEntityComponents, prefabTestComponentId);

        // Create an axle with 0 entities and 1 wheel instance.
        AZStd::unique_ptr<Instance> wheel1UnderAxle = m_prefabSystemComponent->InstantiatePrefab(wheelTemplateId);
        const AZStd::vector<EntityAlias> wheelEntityAliasesUnderAxle = wheel1UnderAxle->GetEntityAliases();
        AZStd::unique_ptr<Instance> axleInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(wheel1UnderAxle)), AxlePrefabMockFilePath);
        const TemplateId axleTemplateId = axleInstance->GetTemplateId();
        PrefabDom& axleTemplateDom = m_prefabSystemComponent->FindTemplateDom(axleTemplateId);
        const AZStd::vector<InstanceAlias> wheelInstanceAliasesUnderAxle = axleInstance->GetNestedInstanceAliases(wheelTemplateId);
        ASSERT_EQ(wheelInstanceAliasesUnderAxle.size(), 1);
        

        // Create a car with 0 entities and 1 axle instance.
        AZStd::unique_ptr<Instance> axleUnderCar = m_prefabSystemComponent->InstantiatePrefab(axleTemplateId);
        AZStd::unique_ptr<Instance> carInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(axleUnderCar)), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        InstanceOptionalReference nestedWheelInstanceRef = axleInstance->FindNestedInstance(wheelInstanceAliasesUnderAxle[0]);
        ASSERT_TRUE(nestedWheelInstanceRef);

        //get the entity id
        AZStd::vector<AZ::EntityId> entityIdVector;
        axleInstance->GetNestedEntityIds([&entityIdVector](AZ::EntityId entityId)
        {
            entityIdVector.push_back(entityId);
            return true;
        });

        ASSERT_EQ(entityIdVector.size(), 3);
        AZ::EntityId wheelEntityIdUnderAxle = nestedWheelInstanceRef->get().GetEntityId(wheelEntityAlias);

        // Retrieve the entity pointer from the component application bus.
        AZ::Entity* wheelEntityUnderAxle = nullptr;
        axleInstance->GetAllEntitiesInHierarchy([&wheelEntityUnderAxle, wheelEntityIdUnderAxle](AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetId() == wheelEntityIdUnderAxle)
                {
                    wheelEntityUnderAxle = entity.get();
                    return false;
                }
                else
                {
                    return true;
                }
            });
        ASSERT_NE(nullptr, wheelEntityUnderAxle);

        //create document with before change snapshot
        PrefabDom entityDomBefore;
        m_instanceToTemplateInterface->GenerateDomForEntity(entityDomBefore, *wheelEntityUnderAxle);

        PrefabTestComponent* axlewheelComponent = wheelEntityUnderAxle->FindComponent<PrefabTestComponent>();
        // Change the bool property of the component from Wheel instance and use it to update the wheel template.
        axlewheelComponent->m_boolProperty = false;

        //create document with after change snapshot
        PrefabDom entityDomAfter;
        m_instanceToTemplateInterface->GenerateDomForEntity(entityDomAfter, *wheelEntityUnderAxle);

        InstanceOptionalReference topMostInstanceInHierarchy = m_instanceToTemplateInterface->GetTopMostInstanceInHierarchy(wheelEntityIdUnderAxle);
        ASSERT_TRUE(topMostInstanceInHierarchy);
        PrefabDom patches;
        InstanceOptionalReference wheelInstanceUnderAxle = axleInstance->FindNestedInstance(wheelInstanceAliasesUnderAxle.front());
        m_instanceToTemplateInterface->GeneratePatchForLink(patches, entityDomBefore, entityDomAfter, wheelInstanceUnderAxle->get().GetLinkId());
        m_instanceToTemplateInterface->ApplyPatchesToInstance(wheelEntityIdUnderAxle, patches, topMostInstanceInHierarchy->get());
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // Validate that the prefabTestComponent in the wheel instance under axle doesn't have a BoolProperty.
        // Even though we changed the property to false, it won't be serialized out because it's a default value.
        PrefabDomValue* wheelInstanceDomUnderAxle =
            PrefabTestDomUtils::GetPrefabDomInstancePath(wheelInstanceAliasesUnderAxle.front()).Get(axleTemplateDom);
        wheelEntityComponents = PrefabTestDomUtils::GetPrefabDomComponentsPath(wheelEntityAlias).Get(*wheelInstanceDomUnderAxle);
        ASSERT_TRUE(wheelEntityComponents != nullptr);

        AZStd::string componentValueName = AZStd::string::format("Component_[%llu]", prefabTestComponentId);
        PrefabDomValueReference wheelEntityComponentValue = PrefabDomUtils::FindPrefabDomValue(*wheelEntityComponents, componentValueName.c_str());
        ASSERT_TRUE(wheelEntityComponentValue);

        PrefabDomValueReference wheelEntityComponentBoolPropertyValue =
            PrefabDomUtils::FindPrefabDomValue(wheelEntityComponentValue->get(), PrefabTestDomUtils::BoolPropertyName);

        ASSERT_TRUE(wheelEntityComponentBoolPropertyValue.has_value());
        ASSERT_TRUE(wheelEntityComponentBoolPropertyValue->get().IsBool());
        ASSERT_FALSE(wheelEntityComponentBoolPropertyValue->get().GetBool());

        // Validate that the axles under the car have the same DOM as the axle template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);
    }

}
