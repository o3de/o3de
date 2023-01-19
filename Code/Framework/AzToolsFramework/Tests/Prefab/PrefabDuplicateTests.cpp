/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabDuplicateTest = PrefabTestFixture;

    TEST_F(PrefabDuplicateTest, DuplicateSingleEntitySucceeds)
    {
        const AZStd::string entityName = "EntityToDuplicate";
        AZ::EntityId entityToDuplicateId = CreateEditorEntityUnderRoot(entityName);

        // Add PrefabTestComponent to the entity and make the change to template.
        AZ::Entity* entityToDuplicate = AzToolsFramework::GetEntityById(entityToDuplicateId);
        entityToDuplicate->Deactivate();
        entityToDuplicate->AddComponent(aznew PrefabTestComponent());
        entityToDuplicate->Activate();
        m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(entityToDuplicateId, m_undoStack->GetTop());
        PropagateAllTemplateChanges();

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        EXPECT_TRUE(levelInstance.has_value());

        // Validate there is one entity before duplicating.
        EXPECT_EQ(levelInstance->get().GetEntityAliasCount(), 1);

        // Duplicate the entity.
        DuplicatePrefabResult result = m_prefabPublicInterface->DuplicateEntitiesInInstance({ entityToDuplicateId });
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate there are two entities with the same name and PrefabTestComponent.
        EXPECT_EQ(levelInstance->get().GetEntityAliasCount(), 2);
        levelInstance->get().GetConstEntities(
            [&](const AZ::Entity& entity)
            {
                EXPECT_EQ(entity.GetName(), entityName);
                EXPECT_TRUE(entity.FindComponent<PrefabTestComponent>());
                return true;
            });
    }

    TEST_F(PrefabDuplicateTest, DuplicateSingleInstanceSucceeds)
    {
        const AZStd::string prefabName = "PrefabToDuplicate";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path prefabFilepath = engineRootPath / prefabName;
        AZ::EntityId entityUnderPrefabId = CreateEditorEntityUnderRoot("Entity");
        AZ::EntityId containerId = CreateEditorPrefab(prefabFilepath, { entityUnderPrefabId });

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        EXPECT_TRUE(levelInstance.has_value());

        AZStd::vector<InstanceOptionalReference> nestedInstances;
        levelInstance->get().GetNestedInstances(
            [&nestedInstances](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedInstances.push_back(*(nestedInstance.get()));
            });

        // Validate there is one instance before duplicating.
        EXPECT_EQ(nestedInstances.size(), 1);

        // Duplicate the instance.
        DuplicatePrefabResult result = m_prefabPublicInterface->DuplicateEntitiesInInstance({ containerId });
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate there are two prefab instances with the same name.
        nestedInstances.clear();
        levelInstance->get().GetNestedInstances(
            [&nestedInstances](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedInstances.push_back(*(nestedInstance.get()));
            });
        EXPECT_EQ(nestedInstances.size(), 2);

        for (InstanceOptionalReference nestedInstance : nestedInstances)
        {
            EntityOptionalReference nestedContainerEntity = nestedInstance->get().GetContainerEntity();
            EXPECT_TRUE(nestedContainerEntity.has_value());
            EXPECT_EQ(nestedContainerEntity->get().GetName(), prefabName);
        }
    }

    TEST_F(PrefabDuplicateTest, DuplicateMultipleEntitiesAndFixesReferences)
    {
        const AZStd::string parentEntityName = "Parent Entity";
        const AZStd::string childEntityName = "Child Entity";

        AZ::EntityId parentEntityId = CreateEditorEntityUnderRoot(parentEntityName);
        AZ::EntityId childEntityId = CreateEditorEntity(childEntityName, parentEntityId);

        // Add PrefabTestComponent to the child entity and make the change to template.
        AZ::Entity* childEntity = AzToolsFramework::GetEntityById(childEntityId);
        childEntity->Deactivate();
        PrefabTestComponent* testComponentInChild = aznew PrefabTestComponent();
        testComponentInChild->m_entityIdProperty = parentEntityId; // set the entity id reference to parent
        childEntity->AddComponent(testComponentInChild);
        childEntity->Activate();
        m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(childEntityId, m_undoStack->GetTop());
        PropagateAllTemplateChanges();

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        EXPECT_TRUE(levelInstance.has_value());

        // Validate there are two entities before duplicating.
        EXPECT_EQ(levelInstance->get().GetEntityAliasCount(), 2);

        // Duplicate the parent entity.
        DuplicatePrefabResult result = m_prefabPublicInterface->DuplicateEntitiesInInstance({ parentEntityId });
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate there are four entities in total.
        EXPECT_EQ(levelInstance->get().GetEntityAliasCount(), 4);

        // Validate there are two parent entities.
        AzToolsFramework::EntityIdList parentEntityIds;
        levelInstance->get().GetConstEntities(
            [&](const AZ::Entity& entity)
            {
                if (entity.GetName() == parentEntityName)
                {
                    parentEntityIds.push_back(entity.GetId());
                }
                return true;
            });
        EXPECT_EQ(parentEntityIds.size(), 2);

        // Validate there are two child entities and they have correct parent reference in PrefabTestComponent.
        AzToolsFramework::EntityIdList childEntityIds;
        levelInstance->get().GetConstEntities(
            [&](const AZ::Entity& entity)
            {
                if (entity.GetName() == childEntityName)
                {
                    childEntityIds.push_back(entity.GetId());

                    PrefabTestComponent* testComponent = entity.FindComponent<PrefabTestComponent>();
                    if (testComponent)
                    {
                        auto it = AZStd::find(parentEntityIds.begin(), parentEntityIds.end(), testComponent->m_entityIdProperty);
                        EXPECT_NE(it, parentEntityIds.end());

                        // Erase when we find it so that the matches will be unique.
                        parentEntityIds.erase(it);
                    }
                }
                return true;
            });
        EXPECT_EQ(childEntityIds.size(), 2);

        // Verify we matched each of the parent entity ids.
        EXPECT_EQ(parentEntityIds.size(), 0);
    }
}
