/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestFixture.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace UnitTest
{
    // Fixture for testing instance DOM generation based on the focused prefab via existing template DOMS
    class PrefabInstanceDomGeneratorTests : public PrefabTestFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();

            m_instanceDomGeneratorInterface = AZ::Interface<InstanceDomGeneratorInterface>::Get();
            EXPECT_TRUE(m_instanceDomGeneratorInterface);

            m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
            ASSERT_TRUE(m_prefabFocusPublicInterface);

            SetUpInstanceHierarchy();
        }

        void SetUpInstanceHierarchy()
        {
            // Level
            // | Car
            //   | Wheel
            //     | Tire

            const AZStd::string carPrefabName = "CarPrefab";
            const AZStd::string wheelPrefabName = "WheelPrefab";
            const AZStd::string tireEntityName = "Tire";

            AZ::IO::Path engineRootPath;
            m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            AZ::IO::Path carPrefabFilepath(engineRootPath);
            carPrefabFilepath.Append(carPrefabName);
            AZ::IO::Path wheelPrefabFilepath(engineRootPath);
            wheelPrefabFilepath.Append(wheelPrefabName);

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

            // Generate a patch that will alter the tire
            GenerateWorldXEntityPatch(m_tireAlias, m_overrideValueOnLevel, *m_wheelInstance, m_patch);

            // Apply the tire patch to the Root template (level)
            ApplyEntityPatchToTemplate(m_patch, m_tireAlias, *m_wheelInstance, *m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance());

            // Generate a patch that will alter the container entity of the Wheel
            GenerateWorldXEntityPatch("", m_overrideValueOnLevel, *m_wheelInstance, m_containerPatch);

            // Apply the container patch to the Root template (level)
            ApplyEntityPatchToTemplate(m_containerPatch, "", *m_wheelInstance, *m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance());

            // Update and apply the tire patch to the Wheel template
            UpdateWorldXEntityPatch(m_patch, m_overrideValueOnWheel);
            ApplyEntityPatchToTemplate(m_patch, m_tireAlias, *m_wheelInstance, *m_wheelInstance);

            // Update and apply the tire patch to the Car template
            UpdateWorldXEntityPatch(m_patch, m_overrideValueOnCar);
            ApplyEntityPatchToTemplate(m_patch, m_tireAlias, *m_wheelInstance, m_carInstance->get());
        }

        void TearDownEditorFixtureImpl() override
        {
            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        void GenerateWorldXEntityPatch(
            const EntityAlias& entityAlias,
            float updatedXValue,
            Instance& owningInstance,
            PrefabDom& patchOut)
        {
            EntityOptionalReference childEntity = entityAlias.empty() ? owningInstance.GetContainerEntity() : owningInstance.GetEntity(entityAlias);
            ASSERT_TRUE(childEntity.has_value());

            // Create document with before change snapshot
            PrefabDom entityDomBeforeUpdate;
            m_instanceToTemplateInterface->GenerateDomForEntity(entityDomBeforeUpdate, *childEntity);

            // Change the entity
            float prevXValue = 0.0f;
            AZ::TransformBus::EventResult(prevXValue, childEntity->get().GetId(), &AZ::TransformInterface::GetWorldX);
            AZ::TransformBus::Event(childEntity->get().GetId(), &AZ::TransformInterface::SetWorldX, updatedXValue);
            float curXValue = 0.0f;
            AZ::TransformBus::EventResult(curXValue, childEntity->get().GetId(), &AZ::TransformInterface::GetWorldX);
            EXPECT_EQ(curXValue, updatedXValue);

            // Create document with after change snapshot
            PrefabDom entityDomAfterUpdate;
            m_instanceToTemplateInterface->GenerateDomForEntity(entityDomAfterUpdate, *childEntity);

            // Generate patch
            m_instanceToTemplateInterface->GeneratePatch(patchOut, entityDomBeforeUpdate, entityDomAfterUpdate);

            // Revert the change to the entity
            AZ::TransformBus::Event(childEntity->get().GetId(), &AZ::TransformInterface::SetWorldX, prevXValue);
        }

        void UpdateWorldXEntityPatch(
            PrefabDom& patch,
            double newValue)
        {
            PrefabDomValue::MemberIterator patchEntryIterator = patch[0].FindMember("value");
            ASSERT_TRUE(patchEntryIterator != patch[0].MemberEnd());
            patchEntryIterator->value.SetDouble(newValue);
        }

        void ApplyEntityPatchToTemplate(
            PrefabDom& patch,
            const EntityAlias& entityAlias,
            const Instance& owningInstance,
            const Instance& targetInstance)
        {
            // We need to generate a patch prefix so that the path of the patches correctly refelects the hierarchy path
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

            auto instanceToAddPatches = m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
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

        void GenerateAndValidateInstanceDom(const Instance& instance, EntityAlias entityAlias, float expectedValue)
        {
            // Generate a prefab DOM for the provided instance
            PrefabDom generatedInstanceDom;
            m_instanceDomGeneratorInterface->GenerateInstanceDom(generatedInstanceDom, instance);

            // Create an instance from the generated prefab DOM for validation
            Instance instanceFromDom;
            ASSERT_TRUE(PrefabDomUtils::LoadInstanceFromPrefabDom(
                instanceFromDom, generatedInstanceDom, PrefabDomUtils::LoadFlags::UseSelectiveDeserialization));

            // Verify that the worldX value of the provided child entity is coming from the correct template
            EntityOptionalReference childEntity = instanceFromDom.GetEntity(entityAlias);
            childEntity->get().Init();
            childEntity->get().Activate(); // to connect to buses such as transform bus
            float curXValue = 0.0f;
            AZ::TransformBus::EventResult(curXValue, childEntity->get().GetId(), &AZ::TransformInterface::GetWorldX);
            EXPECT_EQ(curXValue, expectedValue);

            // Verify that the parent of the container entity is a valid entity
            EntityOptionalReference containerEntity = instanceFromDom.GetContainerEntity();
            containerEntity->get().Init();
            containerEntity->get().Activate(); // to connect to buses such as transform bus
            AZ::EntityId parentEntityId;
            AZ::TransformBus::EventResult(parentEntityId, containerEntity->get().GetId(), &AZ::TransformInterface::GetParentId);
            EXPECT_TRUE(parentEntityId.IsValid());
        }

        void GenerateAndValidateEntityDom(const AZ::Entity& entity, float expectedValue)
        {
            // Generate an entity DOM for the provided entity
            PrefabDom generatedEntityDom;
            m_instanceDomGeneratorInterface->GenerateEntityDom(generatedEntityDom, entity);
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

    protected:
        const float m_overrideValueOnLevel = 1.0f;
        const float m_overrideValueOnCar = 2.0f;
        const float m_overrideValueOnWheel = 3.0f;

        InstanceOptionalReference m_carInstance;
        InstanceOptionalReference m_wheelInstance;
        EntityAlias m_tireAlias;
        PrefabDom m_patch;
        PrefabDom m_containerPatch;

        InstanceDomGeneratorInterface* m_instanceDomGeneratorInterface = nullptr;
        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
    };

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateInstanceDom_DescendantOfFocusedOrRoot_Succeeds)
    {
        // Generate a prefab DOM for the Wheel instance while the Level is in focus
        GenerateAndValidateInstanceDom(m_wheelInstance->get(), m_tireAlias, m_overrideValueOnLevel);
    }

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateInstanceDom_Focused_Succeeds)
    {
        // Generate a prefab DOM for the Wheel instance while the Wheel instance is in focus
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_wheelInstance->get().GetContainerEntityId());

        GenerateAndValidateInstanceDom(m_wheelInstance->get(), m_tireAlias, m_overrideValueOnWheel);
    }

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateEntityDom_NotContainerEntity_Succeeds)
    {
        const AZ::Entity& tireEntity = m_wheelInstance->get().GetEntity(m_tireAlias)->get();

        // Focus is on Level by default
        GenerateAndValidateEntityDom(tireEntity, m_overrideValueOnLevel);

        // Change focus to Car
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_carInstance->get().GetContainerEntityId());
        GenerateAndValidateEntityDom(tireEntity, m_overrideValueOnCar);

        // Change focus to Wheel
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_wheelInstance->get().GetContainerEntityId());
        GenerateAndValidateEntityDom(tireEntity, m_overrideValueOnWheel);
    }

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateEntityDom_ContainerEntity_Succeeds)
    {
        const AZ::Entity& containerEntity = m_wheelInstance->get().GetContainerEntity()->get();

        // Change focus to Wheel, so the container entity DOM should come from the root
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_wheelInstance->get().GetContainerEntityId());
        GenerateAndValidateEntityDom(containerEntity, m_overrideValueOnLevel);
    }
} // namespace UnitTest
