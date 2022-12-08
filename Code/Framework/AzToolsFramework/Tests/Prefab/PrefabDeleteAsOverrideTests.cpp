/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestFixture.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace UnitTest
{
    using PrefabDeleteAsOverrideTests = PrefabTestFixture;

    TEST_F(PrefabDeleteAsOverrideTests, DeleteEntitiesAndAllDescendantsInInstance_DeleteSingleEntitySucceeds)
    {
        // Level            <-- focused
        // | Car_1
        //   | Tire         <-- delete
        // | Car_2
        //   | Tire

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string tireEntityName = "Tire";
        const AZStd::string firstCarName = "Car_1";
        const AZStd::string secondCarName = "Car_2";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath(engineRootPath);
        carPrefabFilepath.Append(carPrefabName);

        // Create and rename the first car.
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId firstCarContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });
        EntityAlias tireEntityAlias = FindEntityAliasInInstance(firstCarContainerId, tireEntityName);
        RenameEntity(firstCarContainerId, firstCarName);

        // Create and rename the second car.
        AZ::EntityId secondCarContainerId = InstantiateEditorPrefab(carPrefabFilepath, GetRootContainerEntityId());
        RenameEntity(secondCarContainerId, secondCarName);

        // Delete the tire entity in the first car.
        // Note: Level root instance is focused by default.
        InstanceOptionalReference firstCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(firstCarContainerId);
        AZ::EntityId firstTireEntityId = firstCarInstance->get().GetEntityId(tireEntityAlias);

        InstanceOptionalReference secondCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(secondCarContainerId);
        AZ::EntityId secondTireEntityId = secondCarInstance->get().GetEntityId(tireEntityAlias);

        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ firstTireEntityId });
        ASSERT_TRUE(result.IsSuccess());

        // Validate that only the tire in the first car is deleted.
        AZ::Entity* firstTireEntity = AzToolsFramework::GetEntityById(firstTireEntityId);
        EXPECT_TRUE(firstTireEntity == nullptr);
        AZ::Entity* secondTireEntity = AzToolsFramework::GetEntityById(secondTireEntityId);
        EXPECT_TRUE(secondTireEntity != nullptr);

        ValidateEntityNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias);
        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
    }

    TEST_F(PrefabDeleteAsOverrideTests, DeleteEntitiesAndAllDescendantsInInstance_DeleteSinglePrefabSucceeds)
    {
        // Level            <-- focused
        // | Car_1
        //   | Wheel        <-- delete
        //     | Tire
        // | Car_2
        //   | Wheel
        //     | Tire

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string wheelPrefabName = "WheelPrefab";
        const AZStd::string tireEntityName = "Tire";
        const AZStd::string firstCarName = "Car_1";
        const AZStd::string secondCarName = "Car_2";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath(engineRootPath);
        carPrefabFilepath.Append(carPrefabName);
        AZ::IO::Path wheelPrefabFilepath(engineRootPath);
        wheelPrefabFilepath.Append(wheelPrefabName);

        // Create and rename the first car.
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });
        AZ::EntityId firstCarContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelContainerId });
        RenameEntity(firstCarContainerId, firstCarName);

        // Create and rename the second car.
        AZ::EntityId secondCarContainerId = InstantiateEditorPrefab(carPrefabFilepath, GetRootContainerEntityId());
        RenameEntity(secondCarContainerId, secondCarName);

        InstanceAlias wheelInstanceAlias = FindNestedInstanceAliasInInstance(firstCarContainerId, wheelPrefabName);
        ASSERT_FALSE(wheelInstanceAlias.empty());

        // Delete the wheel instance in the first car.
        // Note: Level root instance is focused by default.
        InstanceOptionalReference firstCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(firstCarContainerId);
        AZ::EntityId firstWheelContainerId;
        firstCarInstance->get().GetNestedInstances(
            [&firstWheelContainerId, wheelInstanceAlias](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                if (nestedInstance->GetInstanceAlias() == wheelInstanceAlias)
                {
                    firstWheelContainerId = nestedInstance->GetContainerEntityId();
                    return;
                }
            });
        ASSERT_TRUE(firstWheelContainerId.IsValid()) << "Cannot get wheel container entity id in the first car.";

        InstanceOptionalReference secondCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(secondCarContainerId);

        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ firstWheelContainerId });
        ASSERT_TRUE(result.IsSuccess());

        // Validate that only the wheel instance in the first car is deleted.
        AZ::Entity* firstWheelContainerEntity = AzToolsFramework::GetEntityById(firstWheelContainerId);
        EXPECT_TRUE(firstWheelContainerEntity == nullptr);

        ValidateNestedInstanceNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
        ValidateNestedInstanceUnderInstance(secondCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
    }

    TEST_F(PrefabDeleteAsOverrideTests, DeleteEntitiesAndAllDescendantsInInstance_DeletingEntityDeletesChildEntityToo)
    {
        // Level              <-- focused
        // | Car_1
        //   | Tire           <-- delete
        //     | ChildEntity
        // | Car_2
        //   | Tire
        //     | ChildEntity

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string tireEntityName = "Tire";
        const AZStd::string childEntityName = "ChildEntity";
        const AZStd::string firstCarName = "Car_1";
        const AZStd::string secondCarName = "Car_2";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath(engineRootPath);
        carPrefabFilepath.Append(carPrefabName);

        // Create and rename the first car.
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        CreateEditorEntity(childEntityName, tireEntityId);
        AZ::EntityId firstCarContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });
        EntityAlias tireEntityAlias = FindEntityAliasInInstance(firstCarContainerId, tireEntityName);
        EntityAlias childEntityAlias = FindEntityAliasInInstance(firstCarContainerId, childEntityName);
        RenameEntity(firstCarContainerId, firstCarName);

        // Create and rename the second car.
        AZ::EntityId secondCarContainerId = InstantiateEditorPrefab(carPrefabFilepath, GetRootContainerEntityId());
        RenameEntity(secondCarContainerId, secondCarName);

        // Delete the tire entity in the first car.
        // Note: Level root instance is focused by default.
        InstanceOptionalReference firstCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(firstCarContainerId);
        AZ::EntityId firstTireEntityId = firstCarInstance->get().GetEntityId(tireEntityAlias);
        AZ::EntityId firstChildEntityId = firstCarInstance->get().GetEntityId(childEntityAlias);

        InstanceOptionalReference secondCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(secondCarContainerId);
        AZ::EntityId secondTireEntityId = secondCarInstance->get().GetEntityId(tireEntityAlias);
        AZ::EntityId secondChildEntityId = secondCarInstance->get().GetEntityId(childEntityAlias);

        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ firstTireEntityId });
        ASSERT_TRUE(result.IsSuccess());

        // Validate that only the tire and its child entity in the first car are deleted.
        AZ::Entity* firstTireEntity = AzToolsFramework::GetEntityById(firstTireEntityId);
        EXPECT_TRUE(firstTireEntity == nullptr);
        AZ::Entity* firstChildEntity = AzToolsFramework::GetEntityById(firstChildEntityId);
        EXPECT_TRUE(firstChildEntity == nullptr);

        ValidateEntityNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias);
        ValidateEntityNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), childEntityAlias);

        AZ::Entity* secondTireEntity = AzToolsFramework::GetEntityById(secondTireEntityId);
        EXPECT_TRUE(secondTireEntity != nullptr);
        AZ::Entity* secondChildEntity = AzToolsFramework::GetEntityById(secondChildEntityId);
        EXPECT_TRUE(secondChildEntity != nullptr);

        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), childEntityAlias, childEntityName);
    }

    TEST_F(PrefabDeleteAsOverrideTests, DeleteEntitiesAndAllDescendantsInInstance_DeletingEntityDeletesChildPrefabToo)
    {
        // Level              <-- focused
        // | Car_1
        //   | Tire           <-- delete
        //     | ChildPrefab
        //       | ChildEntity
        // | Car_2
        //   | Tire
        //     | ChildPrefab
        //       | ChildEntity

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string tireEntityName = "Tire";
        const AZStd::string childPrefabName = "ChildPrefab";
        const AZStd::string childEntityName = "ChildEntity";
        const AZStd::string firstCarName = "Car_1";
        const AZStd::string secondCarName = "Car_2";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath(engineRootPath);
        carPrefabFilepath.Append(carPrefabName);
        AZ::IO::Path childPrefabFilepath(engineRootPath);
        childPrefabFilepath.Append(childPrefabName);

        // Create and rename the first car.
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId childEntityId = CreateEditorEntity(childEntityName, tireEntityId);
        CreateEditorPrefab(childPrefabFilepath, { childEntityId });
        AZ::EntityId firstCarContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });
        EntityAlias tireEntityAlias = FindEntityAliasInInstance(firstCarContainerId, tireEntityName);
        InstanceAlias childInstanceAlias = FindNestedInstanceAliasInInstance(firstCarContainerId, childPrefabName);
        RenameEntity(firstCarContainerId, firstCarName);

        // Create and rename the second car.
        AZ::EntityId secondCarContainerId = InstantiateEditorPrefab(carPrefabFilepath, GetRootContainerEntityId());
        RenameEntity(secondCarContainerId, secondCarName);

        // Delete the tire entity in the first car.
        // Note: Level root instance is focused by default.
        InstanceOptionalReference firstCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(firstCarContainerId);
        AZ::EntityId firstTireEntityId = firstCarInstance->get().GetEntityId(tireEntityAlias);
        AZ::EntityId firstChildContainerId;
        firstCarInstance->get().GetNestedInstances(
            [&firstChildContainerId, childInstanceAlias](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                if (nestedInstance->GetInstanceAlias() == childInstanceAlias)
                {
                    firstChildContainerId = nestedInstance->GetContainerEntityId();
                    return;
                }
            });
        ASSERT_TRUE(firstChildContainerId.IsValid()) << "Cannot get child container entity id in the first car.";

        InstanceOptionalReference secondCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(secondCarContainerId);
        AZ::EntityId secondTireEntityId = secondCarInstance->get().GetEntityId(tireEntityAlias);
        AZ::EntityId secondChildContainerId;
        secondCarInstance->get().GetNestedInstances(
            [&secondChildContainerId, childInstanceAlias](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                if (nestedInstance->GetInstanceAlias() == childInstanceAlias)
                {
                    secondChildContainerId = nestedInstance->GetContainerEntityId();
                    return;
                }
            });
        ASSERT_TRUE(secondChildContainerId.IsValid()) << "Cannot get child container entity id in the second car.";

        PrefabOperationResult result = m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance({ firstTireEntityId });
        ASSERT_TRUE(result.IsSuccess());

        // Validate that only the tire and its child prefab instance in the first car are deleted.
        AZ::Entity* firstTireEntity = AzToolsFramework::GetEntityById(firstTireEntityId);
        EXPECT_TRUE(firstTireEntity == nullptr);
        AZ::Entity* firstChildContainerEntity = AzToolsFramework::GetEntityById(firstChildContainerId);
        EXPECT_TRUE(firstChildContainerEntity == nullptr);

        ValidateEntityNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias);
        ValidateNestedInstanceNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), childInstanceAlias);

        AZ::Entity* secondTireEntity = AzToolsFramework::GetEntityById(secondTireEntityId);
        EXPECT_TRUE(secondTireEntity != nullptr);
        AZ::Entity* secondChildContainerEntity = AzToolsFramework::GetEntityById(secondChildContainerId);
        EXPECT_TRUE(secondChildContainerEntity != nullptr);

        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
        ValidateNestedInstanceUnderInstance(secondCarInstance->get().GetContainerEntityId(), childInstanceAlias);
    }
} // namespace UnitTest
