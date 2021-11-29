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

    TEST_F(PrefabDeleteTest, DeleteEntitiesInInstance_DeleteSingleEntitySucceeds)
    {
        PrefabEntityResult createEntityResult = m_prefabPublicInterface->CreateEntity(AZ::EntityId(), AZ::Vector3());
        PropagateAllTemplateChanges();
        AZ::EntityId createdEntityId = createEntityResult.GetValue();
        ASSERT_TRUE(createdEntityId.IsValid());

        AZ::Entity* testEntity = AzToolsFramework::GetEntityById(createdEntityId);
        EXPECT_TRUE(testEntity != nullptr);
        m_prefabPublicInterface->DeleteEntitiesInInstance(AzToolsFramework::EntityIdList{ createdEntityId });
        PropagateAllTemplateChanges();
        testEntity = AzToolsFramework::GetEntityById(createdEntityId);
        EXPECT_TRUE(testEntity == nullptr);
    }

    TEST_F(PrefabDeleteTest, DeleteEntitiesInInstance_DeleteSinglePrefabSucceeds)
    {
        PrefabEntityResult createEntityResult = m_prefabPublicInterface->CreateEntity(AZ::EntityId(), AZ::Vector3());
        PropagateAllTemplateChanges();
        AZ::EntityId createdEntityId = createEntityResult.GetValue();
        ASSERT_TRUE(createdEntityId.IsValid());

        //AZ::IO::FixedMaxPath path = "/F:\\EngineRoot/";
        //path.MakePreferred();
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath path;
        registry->Get(path.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        CreatePrefabResult createPrefabResult = m_prefabPublicInterface->CreatePrefabInMemory(
            AzToolsFramework::EntityIdList{ createdEntityId }, path);
        PropagateAllTemplateChanges();
        AZ::EntityId createdPrefabContainerId = createPrefabResult.GetValue();
        ASSERT_TRUE(createdPrefabContainerId.IsValid());

        AZ::Entity* testEntity = AzToolsFramework::GetEntityById(createdPrefabContainerId);
        EXPECT_TRUE(testEntity != nullptr);
        m_prefabPublicInterface->DeleteEntitiesInInstance(AzToolsFramework::EntityIdList{ createdEntityId });
        PropagateAllTemplateChanges();
        testEntity = AzToolsFramework::GetEntityById(createdEntityId);
        EXPECT_TRUE(testEntity == nullptr);
    }
} // namespace UnitTest
