/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabDeleteTests = PrefabTestFixture;

    TEST_F(PrefabDeleteTests, DeleteEntitiesAndAllDescendantsInInstance_DeleteSingleEntitySucceeds)
    {
        const AZStd::string entityToDeleteName = "EntityToDelete";

        AZ::EntityId entityId = CreateEditorEntityUnderRoot(entityToDeleteName);
        EntityAlias entityAlias = FindEntityAliasInInstance(GetRootContainerEntityId(), entityToDeleteName);

        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ entityId });
        ASSERT_TRUE(result.IsSuccess());

        // Verify that entity can't be found after deletion.
        AZ::Entity* entityToDelete = AzToolsFramework::GetEntityById(entityId);
        EXPECT_TRUE(entityToDelete == nullptr);

        ValidateEntityNotUnderInstance(GetRootContainerEntityId(), entityAlias);
    }

    TEST_F(PrefabDeleteTests, DeleteEntitiesAndAllDescendantsInInstance_DeleteSinglePrefabSucceeds)
    {
        const AZStd::string entityName = "EntityUnderPrefab";
        const AZStd::string prefabName = "PrefabToDelete";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path prefabFilepath(engineRootPath);
        prefabFilepath.Append(prefabName);

        AZ::EntityId entityId = CreateEditorEntityUnderRoot(entityName);
        AZ::EntityId containerEntityId = CreateEditorPrefab(prefabFilepath, { entityId });
        InstanceAlias prefabInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), prefabName);

        // Verify that the prefab container entity and the entity within are deleted.
        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ containerEntityId });
        ASSERT_TRUE(result.IsSuccess());

        AZ::Entity* containerEntity = AzToolsFramework::GetEntityById(containerEntityId);
        EXPECT_TRUE(containerEntity == nullptr);

        ValidateNestedInstanceNotUnderInstance(GetRootContainerEntityId(), prefabInstanceAlias);
    }

    TEST_F(PrefabDeleteTests, DeleteEntitiesAndAllDescendantsInInstance_DeletingEntityDeletesChildEntityToo)
    {
        const AZStd::string parentEntityName = "ParentEntity";
        const AZStd::string childEntityName = "ChildEntity";

        AZ::EntityId parentEntityId = CreateEditorEntityUnderRoot(parentEntityName);
        AZ::EntityId childEntityId = CreateEditorEntity(childEntityName, parentEntityId);
        EntityAlias parentEntityAlias = FindEntityAliasInInstance(GetRootContainerEntityId(), parentEntityName);
        EntityAlias childEntityAlias = FindEntityAliasInInstance(GetRootContainerEntityId(), childEntityName);

        // Delete the parent entity.
        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ parentEntityId });
        ASSERT_TRUE(result.IsSuccess());

        // Verify that both the parent and its child entity are deleted.
        AZ::Entity* parentEntity = AzToolsFramework::GetEntityById(parentEntityId);
        EXPECT_TRUE(parentEntity == nullptr);
        AZ::Entity* childEntity = AzToolsFramework::GetEntityById(childEntityId);
        EXPECT_TRUE(childEntity == nullptr);

        ValidateEntityNotUnderInstance(GetRootContainerEntityId(), parentEntityAlias);
        ValidateEntityNotUnderInstance(GetRootContainerEntityId(), childEntityAlias);
    }

    TEST_F(PrefabDeleteTests, DeleteEntitiesAndAllDescendantsInInstance_DeletingEntityDeletesChildPrefabToo)
    {
        const AZStd::string parentEntityName = "ParentEntity";
        const AZStd::string childEntityName = "ChildEntity";
        const AZStd::string childPrefabName = "ChildPrefab";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path prefabFilepath(engineRootPath);
        prefabFilepath.Append(childPrefabName);

        AZ::EntityId parentEntityId = CreateEditorEntityUnderRoot(parentEntityName);
        EntityAlias parentEntityAlias = FindEntityAliasInInstance(GetRootContainerEntityId(), parentEntityName);

        AZ::EntityId childEntityId = CreateEditorEntity(childEntityName, parentEntityId);
        AZ::EntityId childContainerId = CreateEditorPrefab(prefabFilepath, { childEntityId });
        InstanceAlias childInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), childPrefabName);

        // Delete the parent entity.
        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ parentEntityId });
        ASSERT_TRUE(result.IsSuccess());

        // Validate that the parent and the prefab instance under it are deleted.
        AZ::Entity* parentEntity = AzToolsFramework::GetEntityById(parentEntityId);
        EXPECT_TRUE(parentEntity == nullptr);
        AZ::Entity* childContainerEntity = AzToolsFramework::GetEntityById(childContainerId);
        EXPECT_TRUE(childContainerEntity == nullptr);

        ValidateEntityNotUnderInstance(GetRootContainerEntityId(), parentEntityAlias);
        ValidateNestedInstanceNotUnderInstance(GetRootContainerEntityId(), childPrefabName);
    }

    TEST_F(PrefabDeleteTests, DeleteSelectedEntity_UndoRedoRestoresSelectionBeforeNextTick)
    {
        const AZ::EntityId entityId = CreateEditorEntityUnderRoot("SelectedEntity");

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{ entityId });

        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ entityId });
        ASSERT_TRUE(result.IsSuccess());
        ProcessDeferredUpdates();

        EXPECT_EQ(AzToolsFramework::GetEntityById(entityId), nullptr);

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);

        EXPECT_NE(AzToolsFramework::GetEntityById(entityId), nullptr);
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        ASSERT_EQ(selectedEntities.size(), 1);
        EXPECT_EQ(selectedEntities.front(), entityId);

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);

        selectedEntities.clear();
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        EXPECT_TRUE(selectedEntities.empty());

        ProcessDeferredUpdates();
        EXPECT_EQ(AzToolsFramework::GetEntityById(entityId), nullptr);
    }

    TEST_F(PrefabDeleteTests, DeleteOneOfTwoSelectedEntities_UndoRedoRestoresTheCompleteSelection)
    {
        const AZ::EntityId entityToDeleteId = CreateEditorEntityUnderRoot("EntityToDelete");
        const AZ::EntityId entityToKeepId = CreateEditorEntityUnderRoot("EntityToKeep");

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
            AzToolsFramework::EntityIdList{ entityToDeleteId, entityToKeepId });

        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ entityToDeleteId });
        ASSERT_TRUE(result.IsSuccess());
        ProcessDeferredUpdates();

        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        ASSERT_EQ(selectedEntities.size(), 1);
        EXPECT_EQ(selectedEntities.front(), entityToKeepId);

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);
        ProcessDeferredUpdates();

        EXPECT_NE(AzToolsFramework::GetEntityById(entityToDeleteId), nullptr);
        selectedEntities.clear();
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        ASSERT_EQ(selectedEntities.size(), 2);
        EXPECT_NE(AZStd::find(selectedEntities.begin(), selectedEntities.end(), entityToDeleteId), selectedEntities.end());
        EXPECT_NE(AZStd::find(selectedEntities.begin(), selectedEntities.end(), entityToKeepId), selectedEntities.end());

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);

        EXPECT_EQ(AzToolsFramework::GetEntityById(entityToDeleteId), nullptr);
        selectedEntities.clear();
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        ASSERT_EQ(selectedEntities.size(), 1);
        EXPECT_EQ(selectedEntities.front(), entityToKeepId);

        ProcessDeferredUpdates();
        EXPECT_EQ(AzToolsFramework::GetEntityById(entityToDeleteId), nullptr);
    }
} // namespace UnitTest
