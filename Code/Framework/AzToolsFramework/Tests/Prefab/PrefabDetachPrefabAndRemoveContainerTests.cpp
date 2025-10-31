/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// this file is essentially a duplicate of PrefabDetachPrefabTests.cpp
// but it calls the API which deletes the container.
// This means that SOME, but not necessarily all, of the setup for each test is the same
// But the actual outcome is very different for each test.

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabDetachPrefabTests = PrefabTestFixture;

    TEST_F(PrefabDetachPrefabTests, DetachPrefabAndRemoveContainerEntityUnderLevelSucceeds)
    {
        // Level
        // | Car       (prefab)  <-- detach prefab
        //   | Tire
        //     | Belt

        // detaching removes the wrapper prefab object, so result is just
        // Level
        //   | Tire
        //     | Belt

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string tireEntityName = "Tire";
        const AZStd::string beltEntityName = "Belt";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;

        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        CreateEditorEntity(beltEntityName, tireEntityId);
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });

        InstanceAlias carInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), carPrefabName);

        // Detach the car prefab.
        PrefabOperationResult result = m_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(carContainerId);
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate there is no nested instance in the level prefab instance.
        ValidateNestedInstanceNotUnderInstance(GetRootContainerEntityId(), carInstanceAlias);

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        ASSERT_TRUE(levelInstance.has_value());

        // Validate there are two entities in the level prefab instance (Tire, Belt)
        EXPECT_EQ(levelInstance->get().GetEntityAliasCount(), 2);

        // Validate that the car entity (the prefab wrapper) does not exist.
        AZStd::string carEntityAliasAfterDetach = FindEntityAliasInInstance(GetRootContainerEntityId(), carPrefabName);
        EXPECT_STREQ(carEntityAliasAfterDetach.c_str(), "");

        // Validate that the tire's parent entity is the level container entity.
        AZStd::string tireEntityAliasAfterDetach = FindEntityAliasInInstance(GetRootContainerEntityId(), tireEntityName);
        AZ::EntityId tireEntityIdAfterDetach = levelInstance->get().GetEntityId(tireEntityAliasAfterDetach);
        EXPECT_TRUE(tireEntityIdAfterDetach.IsValid());

        AZ::EntityId parentEntityIdForTire;
        AZ::TransformBus::EventResult(parentEntityIdForTire, tireEntityIdAfterDetach, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(levelInstance->get().GetContainerEntityId(), parentEntityIdForTire);

        // Validate that the belt's parent entity is the tire.
        AZStd::string beltEntityAliasAfterDetach = FindEntityAliasInInstance(GetRootContainerEntityId(), beltEntityName);
        AZ::EntityId beltEntityIdAfterDetach = levelInstance->get().GetEntityId(beltEntityAliasAfterDetach);
        EXPECT_TRUE(beltEntityIdAfterDetach.IsValid());

        AZ::EntityId parentEntityIdForBelt;
        AZ::TransformBus::EventResult(parentEntityIdForBelt, beltEntityIdAfterDetach, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(tireEntityIdAfterDetach, parentEntityIdForBelt);
    }

    TEST_F(PrefabDetachPrefabTests, DetachPrefabAndRemoveContainerEntityUnderParentSucceeds)
    {
        // Level
        // | Garage
        //   | Car       (prefab)  <-- detach prefab
        //     | Tire

        // expected result
        // Level
        // | Garage
        //     | Tire (car is gone)

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string garageEntityName = "Garage";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;

        AZ::EntityId garageEntityId = CreateEditorEntityUnderRoot(garageEntityName);
        AZ::EntityId tireEntityId = CreateEditorEntity(tireEntityName, garageEntityId);
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });

        InstanceAlias carInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), carPrefabName);

        // Detach the car prefab.
        PrefabOperationResult result = m_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(carContainerId);
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate there is no nested instance in the level prefab instance.
        ValidateNestedInstanceNotUnderInstance(GetRootContainerEntityId(), carInstanceAlias);

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        ASSERT_TRUE(levelInstance.has_value());

        // Validate there are two entities in the level prefab instance (the car should be gone)
        EXPECT_EQ(levelInstance->get().GetEntityAliasCount(), 2);

        // Validate that the garage's parent entity is the level container entity.
        AZStd::string garageEntityAliasAfterDetach = FindEntityAliasInInstance(GetRootContainerEntityId(), garageEntityName);
        AZ::EntityId garageEntityIdAfterDetach = levelInstance->get().GetEntityId(garageEntityAliasAfterDetach);
        EXPECT_TRUE(garageEntityIdAfterDetach.IsValid());

        AZ::EntityId parentEntityIdForGarage;
        AZ::TransformBus::EventResult(parentEntityIdForGarage, garageEntityIdAfterDetach, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(levelInstance->get().GetContainerEntityId(), parentEntityIdForGarage);

        // Validate that the car is gone
        AZStd::string carEntityAliasAfterDetach = FindEntityAliasInInstance(GetRootContainerEntityId(), carPrefabName);
        AZ::EntityId carEntityIdAfterDetach = levelInstance->get().GetEntityId(carEntityAliasAfterDetach);
        EXPECT_FALSE(carEntityIdAfterDetach.IsValid());

        // Validate that the tire's parent entity is the garage.
        AZStd::string tireEntityAliasAfterDetach = FindEntityAliasInInstance(GetRootContainerEntityId(), tireEntityName);
        AZ::EntityId tireEntityIdAfterDetach = levelInstance->get().GetEntityId(tireEntityAliasAfterDetach);
        EXPECT_TRUE(tireEntityIdAfterDetach.IsValid());

        AZ::EntityId parentEntityIdForTire;
        AZ::TransformBus::EventResult(parentEntityIdForTire, tireEntityIdAfterDetach, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(garageEntityIdAfterDetach, parentEntityIdForTire);
    }

    TEST_F(PrefabDetachPrefabTests, DetachPrefabAndRemoveContainerEntityWithNestedPrefabSucceeds)
    {
        // Level
        // | Car       (prefab)  <-- detach prefab
        //   | Wheel   (prefab)
        //     | Tire

        // expected result
        // Level
        //   | Wheel   (prefab), car is gone
        //     | Tire


        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string wheelPrefabName = "WheelPrefab";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path wheelPrefabFilepath = engineRootPath / wheelPrefabName;

        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });
        EntityAlias tireEntityAlias = FindEntityAliasInInstance(wheelContainerId, tireEntityName);

        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelContainerId });
        InstanceAlias carInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), carPrefabName);

        // Detach the car prefab.
        PrefabOperationResult result = m_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(carContainerId);
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate there is no car instance in the level prefab instance.
        ValidateNestedInstanceNotUnderInstance(GetRootContainerEntityId(), carInstanceAlias);

        // Validate there is wheel instance in the level prefab instance.
        InstanceAlias wheelInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), wheelPrefabName);
        ValidateNestedInstanceUnderInstance(GetRootContainerEntityId(), wheelInstanceAlias);

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        ASSERT_TRUE(levelInstance.has_value());
        
        AZStd::vector<InstanceOptionalReference> nestedInstances;
        levelInstance->get().GetNestedInstances(
            [&nestedInstances](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedInstances.push_back(*(nestedInstance.get()));
            });

        EXPECT_EQ(nestedInstances.size(), 1) << "There should be only one nested instance in level after detaching.";
        EXPECT_TRUE(nestedInstances[0].has_value());

        // Validate that the car prefab is gone
        AZStd::string carEntityAliasAfterDetach = FindEntityAliasInInstance(GetRootContainerEntityId(), carPrefabName);
        AZ::EntityId carEntityIdAfterDetach = levelInstance->get().GetEntityId(carEntityAliasAfterDetach);
        EXPECT_FALSE(carEntityIdAfterDetach.IsValid());

        // Validate that the wheel's parent entity is the level.
        Instance& wheelInstanceAfterDetach = nestedInstances[0]->get();
        AZ::EntityId wheelContainerIdAfterDetach = wheelInstanceAfterDetach.GetContainerEntityId();
        EXPECT_TRUE(wheelContainerIdAfterDetach.IsValid());

        AZ::EntityId parentEntityIdForWheel;
        AZ::TransformBus::EventResult(parentEntityIdForWheel, wheelContainerIdAfterDetach, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(levelInstance->get().GetContainerEntityId(), parentEntityIdForWheel);

        // Validate that the tire's parent entity is the wheel.
        tireEntityId = wheelInstanceAfterDetach.GetEntityId(tireEntityAlias);
        EXPECT_TRUE(tireEntityId.IsValid());
        AZ::EntityId parentEntityIdForTire;
        AZ::TransformBus::EventResult(parentEntityIdForTire, tireEntityId, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(wheelContainerIdAfterDetach, parentEntityIdForTire);
    }

    TEST_F(PrefabDetachPrefabTests, DetachPrefabAndRemoveContainerEntityWithNestedPrefabUnderTopLevelEntitySucceeds)
    {
        // Level
        // | Car          (prefab)   <-- detach prefab
        //   | Wheels                <-- top level entity
        //     | Wheel    (prefab)
        //       | Tire

        // result (car is gone)
        // Level
        //   | Wheels                <-- top level entity
        //     | Wheel    (prefab)
        //       | Tire

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string wheelPrefabName = "WheelPrefab";

        const AZStd::string wheelsEntityName = "Wheels";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path wheelPrefabFilepath = engineRootPath / wheelPrefabName;

        // Create the wheels and tire entities.
        AZ::EntityId wheelsEntityId = CreateEditorEntityUnderRoot(wheelsEntityName);
        AZ::EntityId tireEntityId = CreateEditorEntity(tireEntityName, wheelsEntityId);

        // Create the wheel prefab.
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });

        EntityAlias tireEntityAlias = FindEntityAliasInInstance(wheelContainerId, tireEntityName);

        // Create the car prefab.
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelsEntityId });
        InstanceAlias carInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), carPrefabName);

        // Detach the car prefab.
        PrefabOperationResult result = m_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(carContainerId);
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate there is no car instance in the level prefab instance.
        ValidateNestedInstanceNotUnderInstance(GetRootContainerEntityId(), carInstanceAlias);

        // Validate there is wheels entity in the level prefab instance.
        EntityAlias wheelsEntityAlias = FindEntityAliasInInstance(GetRootContainerEntityId(), wheelsEntityName);
        ValidateEntityUnderInstance(GetRootContainerEntityId(), wheelsEntityAlias, wheelsEntityName);

        // Validate there is wheel instance in the level prefab instance.
        InstanceAlias wheelInstanceAlias = FindNestedInstanceAliasInInstance(GetRootContainerEntityId(), wheelPrefabName);
        ValidateNestedInstanceUnderInstance(GetRootContainerEntityId(), wheelInstanceAlias);

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        ASSERT_TRUE(levelInstance.has_value());

        AZStd::vector<InstanceOptionalReference> nestedInstances;
        levelInstance->get().GetNestedInstances(
            [&nestedInstances](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedInstances.push_back(*(nestedInstance.get()));
            });

        EXPECT_EQ(nestedInstances.size(), 1) << "There should be only one nested instance in level after detaching.";
        EXPECT_TRUE(nestedInstances[0].has_value());

        // Validate that the car is gone
        AZStd::string carEntityAliasAfterDetach = FindEntityAliasInInstance(GetRootContainerEntityId(), carPrefabName);
        AZ::EntityId carEntityIdAfterDetach = levelInstance->get().GetEntityId(carEntityAliasAfterDetach);
        EXPECT_FALSE(carEntityIdAfterDetach.IsValid());

        // Validate that the wheels' parent entity is the level instance.
        AZ::EntityId wheelsEntityIdAfterDetach = levelInstance->get().GetEntityId(wheelsEntityAlias);
        EXPECT_TRUE(wheelsEntityIdAfterDetach.IsValid());

        AZ::EntityId parentEntityIdForWheels;
        AZ::TransformBus::EventResult(parentEntityIdForWheels, wheelsEntityIdAfterDetach, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(levelInstance->get().GetContainerEntityId(), parentEntityIdForWheels);

        // Validate that the wheel prefab's container entity is the "wheels" entity.
        Instance& wheelInstanceAfterDetach = nestedInstances[0]->get();
        AZ::EntityId wheelContainerIdAfterDetach = wheelInstanceAfterDetach.GetContainerEntityId();
        EXPECT_TRUE(wheelContainerIdAfterDetach.IsValid());

        AZ::EntityId parentEntityIdForWheel;
        AZ::TransformBus::EventResult(parentEntityIdForWheel, wheelContainerIdAfterDetach, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(wheelsEntityIdAfterDetach, parentEntityIdForWheel);
    }

    TEST_F(PrefabDetachPrefabTests, DetachPrefabAndRemoveContainerEntityValidatesDetachedContainerEntityOrder)
    {
        // Validate the detached container entity's sort order in its parent.
        // The detached container entity should not be moved to the beginning or end of the child entity list.
        //
        // Level
        // | Station
        // | Car       (prefab)  <-- detach prefab
        //   | Tire
        // | House

        // result (car is gone)
        // The detached container entity should not be moved to the beginning or end of the child entity list.
        //
        // Level
        // | Station
        // | Tire
        // | House

        const AZStd::string carPrefabName = "CarPrefab";

        const AZStd::string tireEntityName = "Tire";
        const AZStd::string stationEntityName = "Station";
        const AZStd::string houseEntityName = "House";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;

        CreateEditorEntityUnderRoot(stationEntityName);
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });
        CreateEditorEntityUnderRoot(houseEntityName);

        // Validate child entity order before detaching the car prefab.
        AzToolsFramework::EntityOrderArray entityOrderArrayBeforeDetach =
            AzToolsFramework::GetEntityChildOrder(GetRootContainerEntityId());
        EXPECT_EQ(entityOrderArrayBeforeDetach.size(), 3);

        AZStd::string childEntityName;
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[0]);
        EXPECT_EQ(childEntityName, stationEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[1]);
        EXPECT_EQ(childEntityName, carPrefabName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[2]);
        EXPECT_EQ(childEntityName, houseEntityName);

        // Detach the car prefab.
        PrefabOperationResult result = m_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(carContainerId);
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate child entity order after detaching the car prefab.
        AzToolsFramework::EntityOrderArray entityOrderArrayAfterDetach =
            AzToolsFramework::GetEntityChildOrder(GetRootContainerEntityId());
        EXPECT_EQ(entityOrderArrayAfterDetach.size(), 3);

        childEntityName = "";
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[0]);
        EXPECT_EQ(childEntityName, stationEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[1]);
        EXPECT_EQ(childEntityName, tireEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[2]);
        EXPECT_EQ(childEntityName, houseEntityName);
    }

    TEST_F(PrefabDetachPrefabTests, DetachPrefabAndRemoveContainerEntityValidatesDetachedChildEntityOrder)
    {
        // Validate the sort order of top-level child entities.
        
        // Level
        //    Car (prefab)            child 0
        //        Engine
        //        Wheel (prefab)
        //           Tire
        //        Battery

        // expected result (car is gone)
        // Level
        //     Engine                 child 0
        //     Wheel (prefab)         child 1
        //        Tire
        //     Battery                child 2

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string wheelPrefabName = "WheelPrefab";

        const AZStd::string tireEntityName = "Tire";
        const AZStd::string engineEntityName = "Engine";
        const AZStd::string batteryEntityName = "Battery";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path wheelPrefabFilepath = engineRootPath / wheelPrefabName;

        AZ::EntityId engineEntityId = CreateEditorEntityUnderRoot(engineEntityName);
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });
        AZ::EntityId batteryEntityId = CreateEditorEntityUnderRoot(batteryEntityName);
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { engineEntityId, wheelContainerId, batteryEntityId });

        // Validate child entity order under car before detaching the car prefab.
        AzToolsFramework::EntityOrderArray entityOrderArrayBeforeDetach = AzToolsFramework::GetEntityChildOrder(carContainerId);
        EXPECT_EQ(entityOrderArrayBeforeDetach.size(), 3);

        AZStd::string childEntityName;
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[0]);
        EXPECT_EQ(childEntityName, engineEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[1]);
        EXPECT_EQ(childEntityName, wheelPrefabName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[2]);
        EXPECT_EQ(childEntityName, batteryEntityName);

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        ASSERT_TRUE(levelInstance.has_value());

        // before we detach, the level should contain 1 entity (the container entity) and 1 instance (of the car)
        int count = 0;
        levelInstance->get().GetEntityIds([&count](AZ::EntityId) { count++; return true; });
        EXPECT_EQ(count, 1); // expect 1 real entity, that is, the container for the car.

        count = 0;
        levelInstance->get().GetNestedInstances([&count](AZStd::unique_ptr<Instance>&) { count++; });
        EXPECT_EQ(count, 1); // expect 1 instance of another prefab (The the car)

        // Detach the car prefab.
        PrefabOperationResult result = m_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(carContainerId);
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // after we detach, the level should contain 3 entities (2 of them real entities, one of them an instance container)
        count = 0;
        levelInstance->get().GetEntityIds([&count](const AZ::EntityId&) { count++; return true; });
        EXPECT_EQ(count, 3); // expect 2 REAL entities.

        count = 0;
        levelInstance->get().GetNestedInstances([&count](AZStd::unique_ptr<Instance>&) { count++; });
        EXPECT_EQ(count, 1); // expect 1 instance of another prefab (the wheel)

        // Validate child entity order under the level after detaching the car prefab.  Should be engine, wheel, battery.
        AzToolsFramework::EntityOrderArray entityOrderArrayAfterDetach =
            AzToolsFramework::GetEntityChildOrder(levelInstance->get().GetContainerEntityId());
        EXPECT_EQ(entityOrderArrayAfterDetach.size(), 3);

        childEntityName = "";
        AZ::ComponentApplicationBus::BroadcastResult(childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[0]);
        EXPECT_EQ(childEntityName, engineEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[1]);
        EXPECT_EQ(childEntityName, wheelPrefabName);
        AZ::ComponentApplicationBus::BroadcastResult(childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[2]);
        EXPECT_EQ(childEntityName, batteryEntityName);
    }

    TEST_F(PrefabDetachPrefabTests, DetachPrefabAndRemoveContainerEntityValidatesTopLevelChildEntityOrder)
    {
        // Validate the sort order of child entities and prefabs that are under the top level entity.
        // 
        // Level
        // | Car          (prefab)   <-- detach prefab
        //   | Wheels                <-- top level entity
        //     | Red_Wheel
        //     | Wheel    (prefab)
        //       | Tire
        //     | Black_Wheel
        // 
        // expected result (car is gone)
        // Level
        // | Wheels                <-- top level entity
        //   | Red_Wheel
        //   | Wheel    (prefab)
        //     | Tire
        //   | Black_Wheel

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string wheelPrefabName = "WheelPrefab";

        const AZStd::string wheelsEntityName = "Wheels";
        const AZStd::string redWheelEntityName = "Red_Wheel";
        const AZStd::string blackWheelEntityName = "Black_Wheel";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path wheelPrefabFilepath = engineRootPath / wheelPrefabName;

        // Create the wheels, red wheel and tire entities.
        AZ::EntityId wheelsEntityId = CreateEditorEntityUnderRoot(wheelsEntityName);
        CreateEditorEntity(redWheelEntityName, wheelsEntityId);
        AZ::EntityId tireEntityId = CreateEditorEntity(tireEntityName, wheelsEntityId);

        // Create the wheel prefab.
        CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });

        // Create the black wheel entity.
        CreateEditorEntity(blackWheelEntityName, wheelsEntityId);

        // Create the car prefab.
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelsEntityId });

        InstanceOptionalReference levelInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        ASSERT_TRUE(levelInstance.has_value());

        // Validate child entity order under wheels before detaching the car prefab.
        EntityAlias wheelsEntityAlias = FindEntityAliasInInstance(carContainerId, wheelsEntityName);
        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        EXPECT_TRUE(carInstance.has_value());
        wheelsEntityId = carInstance->get().GetEntityId(wheelsEntityAlias);

        AzToolsFramework::EntityOrderArray entityOrderArrayBeforeDetach = AzToolsFramework::GetEntityChildOrder(wheelsEntityId);
        EXPECT_EQ(entityOrderArrayBeforeDetach.size(), 3);

        AZStd::string childEntityName;
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[0]);
        EXPECT_EQ(childEntityName, redWheelEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[1]);
        EXPECT_EQ(childEntityName, wheelPrefabName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayBeforeDetach[2]);
        EXPECT_EQ(childEntityName, blackWheelEntityName);

        // Detach the car prefab.
        PrefabOperationResult result = m_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(carContainerId);
        ASSERT_TRUE(result.IsSuccess());

        PropagateAllTemplateChanges();

        // Validate child entity order under wheels after detaching the car prefab.
        wheelsEntityAlias = FindEntityAliasInInstance(levelInstance->get().GetContainerEntityId(), wheelsEntityName);
        wheelsEntityId = levelInstance->get().GetEntityId(wheelsEntityAlias);

        AzToolsFramework::EntityOrderArray entityOrderArrayAfterDetach = AzToolsFramework::GetEntityChildOrder(wheelsEntityId);
        EXPECT_EQ(entityOrderArrayAfterDetach.size(), 3);

        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[0]);
        EXPECT_EQ(childEntityName, redWheelEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[1]);
        EXPECT_EQ(childEntityName, wheelPrefabName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArrayAfterDetach[2]);
        EXPECT_EQ(childEntityName, blackWheelEntityName);
    }
} // namespace UnitTest
