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

#include <Prefab/PrefabDomUtils.h>
#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

#include <AzCore/Component/ComponentApplicationBus.h>

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
        wheelEntity->Init();
        wheelEntity->Activate();
        AZStd::unique_ptr<Instance> wheelIsolatedInstance = m_prefabSystemComponent->CreatePrefab({ wheelEntity },
            {}, WheelPrefabMockFilePath);
        const TemplateId wheelTemplateId = wheelIsolatedInstance->GetTemplateId();
        PrefabDom& wheelTemplateDom = m_prefabSystemComponent->FindTemplateDom(wheelTemplateId);
        AZStd::vector<EntityAlias> wheelTemplateEntityAliases = wheelIsolatedInstance->GetEntityAliases();

        // Validate that the wheel template has the same entities(1) as the instance it was created from.
        ASSERT_EQ(wheelTemplateEntityAliases.size(), 1);

        // Validate that the wheel entity has 1 component under it.
        EntityAlias wheelEntityAlias = wheelTemplateEntityAliases.front();
        PrefabDomValue* wheelEntityComponents =
            PrefabTestDomUtils::GetPrefabDomComponentsPath(wheelEntityAlias).Get(wheelTemplateDom);
        ASSERT_TRUE(wheelEntityComponents != nullptr && wheelEntityComponents->IsArray());
        EXPECT_EQ(wheelEntityComponents->GetArray().Size(), 1);

        // Extract the component id of the entity in wheel template and verify that it matches with the component id of the wheel instance.
        PrefabDomValueReference wheelEntityComponentIdValue =
            PrefabDomUtils::FindPrefabDomValue(*wheelEntityComponents->Begin(), PrefabTestDomUtils::ComponentIdName);
        ASSERT_TRUE(wheelEntityComponentIdValue.has_value());
        EXPECT_EQ(prefabTestComponent->GetId(), wheelEntityComponentIdValue->get().GetUint64());

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
            MakeInstanceList( AZStd::move(axleUnderCar) ), CarPrefabMockFilePath);
        const TemplateId carTemplateId = carInstance->GetTemplateId();
        const AZStd::vector<InstanceAlias> axleInstanceAliasesUnderCar = carInstance->GetNestedInstanceAliases(axleTemplateId);
        PrefabDom& carTemplateDom = m_prefabSystemComponent->FindTemplateDom(carTemplateId);

        //activate the entity so we can access via transform bus
        axleInstance->InitializeNestedEntities();
        axleInstance->ActivateNestedEntities();

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
        AZ::ComponentApplicationBus::BroadcastResult(wheelEntityUnderAxle, &AZ::ComponentApplicationBus::Events::FindEntity, wheelEntityIdUnderAxle);

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

        // Validate that the prefabTestComponent in the wheel instance under axle doesn't have a BoolProperty.
        // Even though we changed the property to false, it won't be serialized out because it's a default value.
        PrefabDomValue* wheelInstanceDomUnderAxle =
            PrefabTestDomUtils::GetPrefabDomInstancePath(wheelInstanceAliasesUnderAxle.front()).Get(axleTemplateDom);
        wheelEntityComponents = PrefabTestDomUtils::GetPrefabDomComponentsPath(wheelEntityAlias).Get(*wheelInstanceDomUnderAxle);
        ASSERT_TRUE(wheelEntityComponents != nullptr);
        PrefabDomValueReference wheelEntityComponentBoolPropertyValue =
            PrefabDomUtils::FindPrefabDomValue(*wheelEntityComponents->Begin(), PrefabTestDomUtils::BoolPropertyName);
        ASSERT_FALSE(wheelEntityComponentBoolPropertyValue.has_value());

        // Validate that the axles under the car have the same DOM as the axle template.
        PrefabTestDomUtils::ValidatePrefabDomInstances(axleInstanceAliasesUnderCar, carTemplateDom, axleTemplateDom);
    }

}
