/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabDeleteTest = PrefabTestFixture;

    TEST_F(PrefabDeleteTest, DeleteEntitiesAndAllDescendantsInInstance_DeleteSingleEntitySucceeds)
    {
        PrefabEntityResult createEntityResult = m_prefabPublicInterface->CreateEntity(AZ::EntityId(), AZ::Vector3());

        // Verify that a valid entity is created.
        AZ::EntityId testEntityId = createEntityResult.GetValue();
        ASSERT_TRUE(testEntityId.IsValid());
        AZ::Entity* testEntity = AzToolsFramework::GetEntityById(testEntityId);
        ASSERT_TRUE(testEntity != nullptr);

        m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(AzToolsFramework::EntityIdList{ testEntityId });

        // Verify that entity can't be found after deletion.
        testEntity = AzToolsFramework::GetEntityById(testEntityId);
        EXPECT_TRUE(testEntity == nullptr);
    }

    TEST_F(PrefabDeleteTest, DeleteEntitiesAndAllDescendantsInInstance_DeleteSinglePrefabSucceeds)
    {
        // Verify that a valid entity is created.
        AZ::EntityId createdEntityId = CreateEditorEntityUnderRoot("EntityUnderRootPrefab");
        ASSERT_TRUE(createdEntityId.IsValid());
        AZ::Entity* createdEntity = AzToolsFramework::GetEntityById(createdEntityId);
        ASSERT_TRUE(createdEntity != nullptr);

        // Rather than hardcode a path, use a path from settings registry since that will work on all platforms.
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath path;
        registry->Get(path.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        CreatePrefabResult createPrefabResult =
            m_prefabPublicInterface->CreatePrefabInMemory(AzToolsFramework::EntityIdList{ createdEntityId }, path);

        AZ::EntityId createdPrefabContainerId = createPrefabResult.GetValue();
        ASSERT_TRUE(createdPrefabContainerId.IsValid());
        AZ::Entity* prefabContainerEntity = AzToolsFramework::GetEntityById(createdPrefabContainerId);
        ASSERT_TRUE(prefabContainerEntity != nullptr);

        // Verify that the prefab container entity and the entity within are deleted.
        m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(AzToolsFramework::EntityIdList{ createdPrefabContainerId });
        prefabContainerEntity = AzToolsFramework::GetEntityById(createdPrefabContainerId);
        EXPECT_TRUE(prefabContainerEntity == nullptr);
        createdEntity = AzToolsFramework::GetEntityById(createdEntityId);
        EXPECT_TRUE(createdEntity == nullptr);
    }

    TEST_F(PrefabDeleteTest, DeleteEntitiesAndAllDescendantsInInstance_DeletingEntityDeletesChildEntityToo)
    {
        PrefabEntityResult parentEntityCreationResult = m_prefabPublicInterface->CreateEntity(AZ::EntityId(), AZ::Vector3());

        // Verify that valid parent entity is created.
        AZ::EntityId parentEntityId = parentEntityCreationResult.GetValue();
        ASSERT_TRUE(parentEntityId.IsValid());
        AZ::Entity* parentEntity = AzToolsFramework::GetEntityById(parentEntityId);
        ASSERT_TRUE(parentEntity != nullptr);

        // Verify that valid child entity is created.
        PrefabEntityResult childEntityCreationResult = m_prefabPublicInterface->CreateEntity(parentEntityId, AZ::Vector3());
        AZ::EntityId childEntityId = childEntityCreationResult.GetValue();
        ASSERT_TRUE(childEntityId.IsValid());
        AZ::Entity* childEntity = AzToolsFramework::GetEntityById(childEntityId);
        ASSERT_TRUE(childEntity != nullptr);

        // Parent the child entity under the parent entity.
        AZ::TransformBus::Event(childEntityId, &AZ::TransformBus::Events::SetParent, parentEntityId);

        // Delete parent entity and its children.
        m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(AzToolsFramework::EntityIdList{ parentEntityId });

        // Verify that both the parent and child entities are deleted.
        parentEntity = AzToolsFramework::GetEntityById(parentEntityId);
        EXPECT_TRUE(parentEntity == nullptr);
        childEntity = AzToolsFramework::GetEntityById(childEntityId);
        EXPECT_TRUE(childEntity == nullptr);
    }

    TEST_F(PrefabDeleteTest, DeleteEntitiesAndAllDescendantsInInstance_DeletingEntityDeletesChildPrefabToo)
    {
        // Verify that a valid entity is created that will be put in a prefab later.
        AZ::EntityId entityToBePutUnderPrefabId = CreateEditorEntityUnderRoot("EntityToBePutUnderPrefab");
        ASSERT_TRUE(entityToBePutUnderPrefabId.IsValid());
        AZ::Entity* entityToBePutUnderPrefab = AzToolsFramework::GetEntityById(entityToBePutUnderPrefabId);
        ASSERT_TRUE(entityToBePutUnderPrefab != nullptr);

        // Verify that a valid parent entity is created.
        PrefabEntityResult parentEntityCreationResult = m_prefabPublicInterface->CreateEntity(AZ::EntityId(), AZ::Vector3());
        AZ::EntityId parentEntityId = parentEntityCreationResult.GetValue();
        ASSERT_TRUE(parentEntityId.IsValid());
        AZ::Entity* parentEntity = AzToolsFramework::GetEntityById(parentEntityId);
        ASSERT_TRUE(parentEntity != nullptr);

        // Rather than hardcode a path, use a path from settings registry since that will work on all platforms.
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath path;
        registry->Get(path.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        CreatePrefabResult createPrefabResult =
            m_prefabPublicInterface->CreatePrefabInMemory(AzToolsFramework::EntityIdList{ entityToBePutUnderPrefabId }, path);

        // Verify that a valid prefab container entity is created.
        AZ::EntityId createdPrefabContainerId = createPrefabResult.GetValue();
        ASSERT_TRUE(createdPrefabContainerId.IsValid());
        AZ::Entity* prefabContainerEntity = AzToolsFramework::GetEntityById(createdPrefabContainerId);
        ASSERT_TRUE(prefabContainerEntity != nullptr);

        // Parent the prefab under the parent entity.
        AZ::TransformBus::Event(createdPrefabContainerId, &AZ::TransformBus::Events::SetParent, parentEntityId);

        // Delete the parent entity.
        m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(AzToolsFramework::EntityIdList{ parentEntityId });

        // Validate that the parent and the prefab under it and the entity inside the prefab are all deleted.
        parentEntity = AzToolsFramework::GetEntityById(parentEntityId);
        ASSERT_TRUE(parentEntity == nullptr);
        entityToBePutUnderPrefab = AzToolsFramework::GetEntityById(entityToBePutUnderPrefabId);
        ASSERT_TRUE(entityToBePutUnderPrefab == nullptr);
        prefabContainerEntity = AzToolsFramework::GetEntityById(createdPrefabContainerId);
        EXPECT_TRUE(prefabContainerEntity == nullptr);
    }
} // namespace UnitTest
