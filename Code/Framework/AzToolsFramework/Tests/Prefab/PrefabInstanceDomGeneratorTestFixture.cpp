/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabInstanceDomGeneratorTestFixture.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace UnitTest
{
    void PrefabInstanceDomGeneratorTestFixture::SetUpEditorFixtureImpl()
    {
        PrefabTestFixture::SetUpEditorFixtureImpl();

        m_instanceDomGeneratorInterface = AZ::Interface<InstanceDomGeneratorInterface>::Get();
        ASSERT_TRUE(m_instanceDomGeneratorInterface);

        m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
        ASSERT_TRUE(m_prefabFocusPublicInterface);

        SetUpInstanceHierarchy();
    }

    void PrefabInstanceDomGeneratorTestFixture::SetUpInstanceHierarchy()
    {
        // Level        <-- override WorldX of Tire and WorldX of Wheel container
        // | Car        <-- override WorldX of Tire
        //   | Wheel
        //     | Tire

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string wheelPrefabName = "WheelPrefab";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path wheelPrefabFilepath = engineRootPath / wheelPrefabName;

        // Create the car hierarchy
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });

        m_wheelInstance = m_instanceEntityMapperInterface->FindOwningInstance(wheelContainerId);
        ASSERT_TRUE(m_wheelInstance.has_value());
        // Save the tire alias for further testing
        AZStd::vector<EntityAlias> entityAliases = m_wheelInstance->get().GetEntityAliases();
        ASSERT_TRUE(!entityAliases.empty());
        m_tireAlias = entityAliases[0];

        // Save the Wheel instance alias before the Car prefab is created
        InstanceAlias wheelInstanceAlias = m_wheelInstance->get().GetInstanceAlias();

        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelContainerId });
        m_carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        ASSERT_TRUE(m_carInstance.has_value());

        // Reassign the Wheel instance now that it's been recreated by propagation
        m_wheelInstance = m_carInstance->get().FindNestedInstance(wheelInstanceAlias);
        ASSERT_TRUE(m_wheelInstance.has_value());

        // Activate the container entity of the Wheel instance
        m_wheelInstance->get().ActivateContainerEntity();

        InitializePrefabTemplates();
    }

    void PrefabInstanceDomGeneratorTestFixture::InitializePrefabTemplates()
    {
        const Instance& rootInstance = *m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();

        // Generate a patch that will alter the Tire
        PrefabDom entityPatch;
        GenerateWorldXEntityPatch(m_tireAlias, m_entityOverrideValueOnLevel, *m_wheelInstance, entityPatch);

        // Apply the Tire patch to the Root template (level)
        ApplyEntityPatchToTemplate(entityPatch, m_tireAlias, *m_wheelInstance, rootInstance);

        // Update the Tire patch and apply it to the Car template
        UpdateWorldXEntityPatch(entityPatch, m_entityOverrideValueOnCar);
        ApplyEntityPatchToTemplate(entityPatch, m_tireAlias, *m_wheelInstance, m_carInstance->get());

        // Update the Tire patch and apply it to the Wheel template
        UpdateWorldXEntityPatch(entityPatch, m_entityValueOnWheel);
        ApplyEntityPatchToTemplate(entityPatch, m_tireAlias, *m_wheelInstance, *m_wheelInstance);

        // Generate a patch that will alter the Wheel's container entity
        PrefabDom containerPatch;
        GenerateWorldXEntityPatch("", m_wheelContainerOverrideValueOnLevel, *m_wheelInstance, containerPatch);

        // Apply the Wheel container patch to the Root template (level)
        ApplyEntityPatchToTemplate(containerPatch, "", *m_wheelInstance, rootInstance);
    }

    void PrefabInstanceDomGeneratorTestFixture::GenerateWorldXEntityPatch(
        const EntityAlias& entityAlias,
        float updatedXValue,
        Instance& owningInstance,
        PrefabDom& patchOut)
    {
        EntityOptionalReference childEntity = entityAlias.empty() ? owningInstance.GetContainerEntity() : owningInstance.GetEntity(entityAlias);
        ASSERT_TRUE(childEntity.has_value());

        // Create document with before change snapshot
        PrefabDom entityDomBeforeUpdate;
        m_instanceToTemplateInterface->GenerateEntityDomBySerializing(entityDomBeforeUpdate, *childEntity);

        // Change the entity
        float prevXValue = 0.0f;
        AZ::TransformBus::EventResult(prevXValue, childEntity->get().GetId(), &AZ::TransformInterface::GetWorldX);
        AZ::TransformBus::Event(childEntity->get().GetId(), &AZ::TransformInterface::SetWorldX, updatedXValue);
        float curXValue = 0.0f;
        AZ::TransformBus::EventResult(curXValue, childEntity->get().GetId(), &AZ::TransformInterface::GetWorldX);
        EXPECT_EQ(curXValue, updatedXValue);

        // Create document with after change snapshot
        PrefabDom entityDomAfterUpdate;
        m_instanceToTemplateInterface->GenerateEntityDomBySerializing(entityDomAfterUpdate, *childEntity);

        // Generate patch
        m_instanceToTemplateInterface->GeneratePatch(patchOut, entityDomBeforeUpdate, entityDomAfterUpdate);

        // Revert the change to the entity
        AZ::TransformBus::Event(childEntity->get().GetId(), &AZ::TransformInterface::SetWorldX, prevXValue);
    }

    void PrefabInstanceDomGeneratorTestFixture::UpdateWorldXEntityPatch(
        PrefabDom& patch,
        double newValue)
    {
        PrefabDomValue::MemberIterator patchEntryIterator = patch[0].FindMember("value");
        ASSERT_TRUE(patchEntryIterator != patch[0].MemberEnd());
        patchEntryIterator->value.SetDouble(newValue);
    }

    void PrefabInstanceDomGeneratorTestFixture::ApplyEntityPatchToTemplate(
        PrefabDom& patch,
        const EntityAlias& entityAlias,
        const Instance& owningInstance,
        const Instance& targetInstance)
    {
        // We need to generate a patch prefix so that the path of the patches correctly reflects the hierarchy path
        // from the entity to the instance where the patch needs to be added.
        AZStd::string patchPrefix;
        if (entityAlias.empty())
        {
            patchPrefix.insert(0, "/ContainerEntity");
        }
        else
        {
            patchPrefix.insert(0, "/Entities/" + entityAlias);
        }

        const Instance* curInstance = &owningInstance;
        while (curInstance != &targetInstance)
        {
            patchPrefix.insert(0, "/Instances/" + curInstance->GetInstanceAlias());
            InstanceOptionalConstReference parentInstance = curInstance->GetParentInstance();
            ASSERT_TRUE(parentInstance.has_value());
            curInstance = &parentInstance->get();
        }

        PrefabDomValueReference patchPathReference = PrefabDomUtils::FindPrefabDomValue(patch[0], "path");
        ASSERT_TRUE(patchPathReference.has_value());
        AZStd::string patchPathStringBefore = patchPathReference->get().GetString();
        AZStd::string patchPathString = patchPathStringBefore;
        patchPathString.insert(0, patchPrefix);
        patchPathReference->get().SetString(patchPathString.c_str(), patch.GetAllocator());

        // Apply the patch
        TemplateId targetTemplateId = targetInstance.GetTemplateId();
        PrefabDom& targetTemplateDomReference = m_prefabSystemComponent->FindTemplateDom(targetTemplateId);
        AZ::JsonSerializationResult::ResultCode result =
            PrefabDomUtils::ApplyPatches(targetTemplateDomReference, targetTemplateDomReference.GetAllocator(), patch);
        ASSERT_TRUE(result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success);

        // Revert back to previous path
        patchPathReference->get().SetString(patchPathStringBefore.c_str(), patch.GetAllocator());
    }

    void PrefabInstanceDomGeneratorTestFixture::GenerateAndValidateInstanceDom(
        const Instance& instance, EntityAlias entityAlias, float expectedValue)
    {
        // Gets a copy of an instance DOM for the provided instance
        PrefabDom instanceDomFromTemplate;
        m_instanceDomGeneratorInterface->GetInstanceDomFromTemplate(instanceDomFromTemplate, instance);

        // Create an instance from the generated prefab DOM for validation
        Instance instanceFromDom;
        ASSERT_TRUE(PrefabDomUtils::LoadInstanceFromPrefabDom(
            instanceFromDom, instanceDomFromTemplate, PrefabDomUtils::LoadFlags::UseSelectiveDeserialization));

        // Verify that the worldX value of the provided child entity is coming from the correct template
        EntityOptionalReference childEntity = FindEntityInInstanceHierarchy(instanceFromDom, entityAlias);
        ASSERT_TRUE(childEntity.has_value());
        childEntity->get().Init();
        childEntity->get().Activate(); // to connect to buses such as transform bus
        float curXValue = 0.0f;
        AZ::TransformBus::EventResult(curXValue, childEntity->get().GetId(), &AZ::TransformInterface::GetWorldX);
        EXPECT_EQ(curXValue, expectedValue);

        // Verify that the parent of the container entity is a valid entity
        EntityOptionalReference containerEntity = instanceFromDom.GetContainerEntity();
        ASSERT_TRUE(containerEntity.has_value());
        containerEntity->get().Init();
        containerEntity->get().Activate(); // to connect to buses such as transform bus
        AZ::EntityId parentEntityId;
        AZ::TransformBus::EventResult(parentEntityId, containerEntity->get().GetId(), &AZ::TransformInterface::GetParentId);
        EXPECT_TRUE(parentEntityId.IsValid());
    }

    EntityOptionalReference PrefabInstanceDomGeneratorTestFixture::FindEntityInInstanceHierarchy(
        Instance& instance,
        EntityAlias entityAlias)
    {
        AZStd::queue<InstanceOptionalReference> instanceQueue;
        instanceQueue.push(instance);
        EntityOptionalReference foundEntity;
        while (!instanceQueue.empty())
        {
            InstanceOptionalReference currentInstance = instanceQueue.front();
            instanceQueue.pop();

            if (currentInstance.has_value())
            {
                foundEntity = currentInstance->get().GetEntity(entityAlias);
                if (foundEntity.has_value())
                {
                    break;
                }

                currentInstance->get().GetNestedInstances(
                    [&](AZStd::unique_ptr<Instance>& nestedInstance)
                    {
                        instanceQueue.push(*nestedInstance.get());
                    });
            }
        }

        return foundEntity;
    }

    void PrefabInstanceDomGeneratorTestFixture::GenerateAndValidateEntityDom(const AZ::Entity& entity, float expectedValue)
    {
        // Generate an entity DOM for the provided entity
        PrefabDom generatedEntityDom;
        m_instanceDomGeneratorInterface->GetEntityDomFromTemplate(generatedEntityDom, entity);
        EXPECT_TRUE(generatedEntityDom.IsObject());

        // Create an entity from the generated entity DOM for validation
        AZ::Entity entityFromDom;
        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(entityFromDom, generatedEntityDom);
        ASSERT_TRUE(result.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted);

        // Verify that the worldX value is coming from the correct template
        entityFromDom.Init();
        entityFromDom.Activate(); // to connect to buses such as transform bus
        float curXValue = 0.0f;
        AZ::TransformBus::EventResult(curXValue, entityFromDom.GetId(), &AZ::TransformInterface::GetWorldX);
        EXPECT_EQ(curXValue, expectedValue);
    }
} // namespace UnitTest
