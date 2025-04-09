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
    using PrefabCreateTests = PrefabTestFixture;

    TEST_F(PrefabCreateTests, CreatePrefabFromEntitiesAndValidateChildEntityOrder)
    {
        // Level
        // | Car
        //   | Engine   (entity)
        //   | Wheel    (instance)
        //     | Tire
        //   | Battery  (entity)

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string wheelPrefabName = "WheelPrefab";
        const AZStd::string tireEntityName = "Tire";
        const AZStd::string engineEntityName = "Engine";
        const AZStd::string batteryEntityName = "Battery";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path wheelPrefabFilepath = engineRootPath / wheelPrefabName;

        // Create an Engine entity first
        AZ::EntityId engineEntityId = CreateEditorEntityUnderRoot(engineEntityName);
        // Create the Wheel instance after the Engine entity
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });
        // Create the Battery entity after the Wheel instance
        AZ::EntityId batteryEntityId = CreateEditorEntityUnderRoot(batteryEntityName);
        // Create a Car prefab from the entities and isntance
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { engineEntityId, batteryEntityId, wheelContainerId });
        auto carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        ASSERT_TRUE(carInstance.has_value());
        ASSERT_EQ(carInstance->get().GetContainerEntityId(), carContainerId);

        // Validate the child entity order of the car instance
        AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(carContainerId);
        EXPECT_EQ(entityOrderArray.size(), 3);
        AZStd::string entityName;
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArray[0]);
        EXPECT_EQ(entityName, engineEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArray[1]);
        EXPECT_EQ(entityName, wheelPrefabName);
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArray[2]);
        EXPECT_EQ(entityName, batteryEntityName);
    }
} // namespace UnitTest
