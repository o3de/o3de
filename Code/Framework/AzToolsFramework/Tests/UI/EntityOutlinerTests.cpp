/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerListModel.hxx>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationManager.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <Prefab/PrefabTestFixture.h>

#include <QAbstractItemModelTester>

namespace UnitTest
{
    // Test fixture for the entity outliner model that uses a QAbstractItemModelTester to validate the state of the model
    // when QAbstractItemModel signals fire. Tests will exit with a fatal error if an invalid state is detected.
    class EntityOutlinerTest : public PrefabTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();
            GetApplication()->RegisterComponentDescriptor(AzToolsFramework::EditorEntityContextComponent::CreateDescriptor());

            m_model = AZStd::make_unique<AzToolsFramework::EntityOutlinerListModel>();
            m_model->Initialize();
            m_modelTester =
                AZStd::make_unique<QAbstractItemModelTester>(m_model.get(), QAbstractItemModelTester::FailureReportingMode::Fatal);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_undoStack = nullptr;
            m_modelTester.reset();
            m_model.reset();
            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        // Creates an entity with a given name as one undoable operation
        // Parents to parentId, or the root prefab container entity if parentId is invalid
        AZ::EntityId CreateNamedEntity(AZStd::string name, AZ::EntityId parentId = AZ::EntityId())
        {
            // Normally, in invalid parent ID should automatically parent us to the root prefab, but currently in the unit test
            // environment entities aren't created with a default transform component, so CreateEntity won't correctly parent.
            // We get the actual target parent ID here, then create our missing transform component.
            if (!parentId.IsValid())
            {
                auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
                parentId = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntityId();
            }

            auto createResult = m_prefabPublicInterface->CreateEntity(parentId, AZ::Vector3());
            AZ_Assert(createResult.IsSuccess(), "Failed to create entity: %s", createResult.GetError().c_str());
            AZ::EntityId entityId = createResult.GetValue();

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            
            entity->SetName(name);

            // Update our undo cache entry to include the rename / reparent as one atomic operation.
            m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(entityId, m_undoStack->GetTop());

            ProcessDeferredUpdates();

            return entityId;
        }

        // Helper to visualize debug state
        void PrintModel()
        {
            AZStd::deque<AZStd::pair<QModelIndex, int>> indices;
            indices.push_back({ m_model->index(0, 0), 0 });
            while (!indices.empty())
            {
                auto [index, depth] = indices.front();
                indices.pop_front();

                QString indentString;
                for (int i = 0; i < depth; ++i)
                {
                    indentString += "  ";
                }
                qDebug() << (indentString + index.data(Qt::DisplayRole).toString()) << index.internalId();
                for (int i = 0; i < m_model->rowCount(index); ++i)
                {
                    indices.emplace_back(m_model->index(i, 0, index), depth + 1);
                }
            }
        };

        // Gets the index of the root prefab, i.e. the "New Level" container entity
        QModelIndex GetRootIndex() const
        {
            return m_model->index(0, 0);
        }

        // Kicks off any updates scheduled for the next tick
        void ProcessDeferredUpdates() override
        {
            // Force a prefab propagation for updates that are deferred to the next tick.
            PropagateAllTemplateChanges();

            // Ensure the model process its entity update queue
            m_model->ProcessEntityUpdates();
        }
        
        AZStd::unique_ptr<AzToolsFramework::EntityOutlinerListModel> m_model;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTester;
    };

    TEST_F(EntityOutlinerTest, TestCreateFlatHierarchyUndoAndRedoWorks)
    {
        constexpr size_t entityCount = 10;

        for (size_t i = 0; i < entityCount; ++i)
        {
            CreateNamedEntity(AZStd::string::format("Entity%zu", i));
            EXPECT_EQ(m_model->rowCount(GetRootIndex()), i + 1);
        }

        for (int i = entityCount; i > 0; --i)
        {
            Undo();
            EXPECT_EQ(m_model->rowCount(GetRootIndex()), i - 1);
        }

        for (size_t i = 0; i < entityCount; ++i)
        {
            Redo();
            EXPECT_EQ(m_model->rowCount(GetRootIndex()), i + 1);
        }
    }

    TEST_F(EntityOutlinerTest, TestCreateNestedHierarchyUndoAndRedoWorks)
    {
        constexpr size_t depth = 5;

        auto modelDepth = [this]() -> int
        {
            int depth = 0;
            QModelIndex index = GetRootIndex();
            while (m_model->rowCount(index) > 0)
            {
                ++depth;
                index = m_model->index(0, 0, index);
            }
            return depth;
        };

        AZ::EntityId parentId;
        for (int i = 0; i < depth; i++)
        {
            parentId = CreateNamedEntity(AZStd::string::format("EntityDepth%i", i), parentId);
            EXPECT_EQ(modelDepth(), i + 1);
        }

        for (int i = depth - 1; i >= 0; --i)
        {
            Undo();
            EXPECT_EQ(modelDepth(), i);
        }

        for (int i = 0; i < depth; ++i)
        {
            Redo();
            EXPECT_EQ(modelDepth(), i + 1);
        }
    }

    TEST_F(EntityOutlinerTest, TestReparentEntitiesSucceeds)
    {
        // Level     (prefab)   <-- focused
        // | Seat
        // | Driver_1
        // | Driver_2

        const AZStd::string seatEntityName = "Seat";
        const AZStd::string driverOneEntityName = "Driver_1";
        const AZStd::string driverTwoEntityName = "Driver_2";

        // Create the Seat and Driver entities.
        AZ::EntityId seatEntityId = CreateEditorEntityUnderRoot(seatEntityName);
        AZ::EntityId driverOneEntityId = CreateEditorEntityUnderRoot(driverOneEntityName);
        AZ::EntityId driverTwoEntityId = CreateEditorEntityUnderRoot(driverTwoEntityName);

        // Reparent the Driver_1 and Driver_2 entities under the Seat entity.
        auto appendForInvalid = AzToolsFramework::EntityOutlinerListModel::AppendEnd;
        bool isReparented = m_model->ReparentEntities(
            seatEntityId, { driverOneEntityId, driverTwoEntityId }, GetRootContainerEntityId(), appendForInvalid);
        EXPECT_TRUE(isReparented);

        // Validate that the parent entity of the Driver_1 and Driver_2 entities is the Seat entity.
        AZ::EntityId parentEntityIdForDriverOne;
        AZ::TransformBus::EventResult(parentEntityIdForDriverOne, driverOneEntityId, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(parentEntityIdForDriverOne, seatEntityId);
        AZ::EntityId parentEntityIdForDriverTwo;
        AZ::TransformBus::EventResult(parentEntityIdForDriverTwo, driverTwoEntityId, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(parentEntityIdForDriverTwo, seatEntityId);

        // Validate that the child entity order of the Seat entity is [Driver_1, Driver_2].
        AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(seatEntityId);
        EXPECT_EQ(entityOrderArray.size(), 2);

        AZStd::string childEntityName;
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArray[0]);
        EXPECT_EQ(childEntityName, driverOneEntityName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArray[1]);
        EXPECT_EQ(childEntityName, driverTwoEntityName);
    }

    TEST_F(EntityOutlinerTest, TestReparentPrefabsSucceeds)
    {
        // Level     (prefab)   <-- focused
        // | Garage
        // | Car     (prefab)
        //   | CarTire
        // | Bike    (prefab)
        //   | BikeTire
        
        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string bikePrefabName = "BikePrefab";

        const AZStd::string garageEntityName = "Garage";
        const AZStd::string carTireEntityName = "CarTire";
        const AZStd::string bikeTireEntityName = "BikeTire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path bikePrefabFilepath = engineRootPath / bikePrefabName;

        // Create the Garage, CarTire and BikeTire entities.
        AZ::EntityId garageEntityId = CreateEditorEntityUnderRoot(garageEntityName);
        AZ::EntityId carTireEntityId = CreateEditorEntityUnderRoot(carTireEntityName);
        AZ::EntityId bikeTireEntityId = CreateEditorEntityUnderRoot(bikeTireEntityName);

        // Create the Car and Bike prefabs.
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { carTireEntityId });
        AZ::EntityId bikeContainerId = CreateEditorPrefab(bikePrefabFilepath, { bikeTireEntityId });

        // Reparent the Car prefab under the Garage entity.
        auto appendForInvalid = AzToolsFramework::EntityOutlinerListModel::AppendBeginning; // test the opposite way of appending
        bool isCarReparented =
            m_model->ReparentEntities(garageEntityId, { carContainerId }, GetRootContainerEntityId(), appendForInvalid);
        EXPECT_TRUE(isCarReparented);

        // Reparent the Bike prefab under the Garage entity.
        bool isBikeReparented =
            m_model->ReparentEntities(garageEntityId, { bikeContainerId }, GetRootContainerEntityId(), appendForInvalid);
        EXPECT_TRUE(isBikeReparented);

        // Validate that the parent entity of the Car and Bike prefabs is the Garage entity.
        AZ::EntityId parentEntityIdForCar;
        AZ::TransformBus::EventResult(parentEntityIdForCar, carContainerId, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(parentEntityIdForCar, garageEntityId);
        AZ::EntityId parentEntityIdForBike;
        AZ::TransformBus::EventResult(parentEntityIdForBike, bikeContainerId, &AZ::TransformInterface::GetParentId);
        EXPECT_EQ(parentEntityIdForBike, garageEntityId);

        // Validate that the child entity order of the Garage entity is [Bike, Car], which is reversed due to the AppendBeginning flag.
        AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(garageEntityId);
        EXPECT_EQ(entityOrderArray.size(), 2);

        AZStd::string childEntityName;
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArray[0]);
        EXPECT_EQ(childEntityName, bikePrefabName);
        AZ::ComponentApplicationBus::BroadcastResult(
            childEntityName, &AZ::ComponentApplicationRequests::GetEntityName, entityOrderArray[1]);
        EXPECT_EQ(childEntityName, carPrefabName);
    }

    TEST_F(EntityOutlinerTest, TestReparentEntitiesThatDoNotBelongToSamePrefabFails)
    {
        // Level     (prefab)   <-- focused
        // | Car     (prefab)
        //   | Tire
        // | Driver

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string bikePrefabName = "BikePrefab";

        const AZStd::string tireEntityName = "Tire";
        const AZStd::string pedalEntityName = "Pedal";
        const AZStd::string driverEntityName = "Driver";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path bikePrefabFilepath = engineRootPath / bikePrefabName;

        // Create the Car prefab.
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });

        // Create the Driver entity.
        AZ::EntityId driverEntityId = CreateEditorEntityUnderRoot(driverEntityName);

        // Retrieve the Tire entity id.
        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        EXPECT_TRUE(carInstance.has_value());
        EntityAlias tireEntityAlias = FindEntityAliasInInstance(carContainerId, tireEntityName);
        EXPECT_FALSE(tireEntityAlias.empty());
        tireEntityId = carInstance->get().GetEntityId(tireEntityAlias);

        // Validate that the Tire and Driver entities cannot be reparented to Level.
        bool isReparented = m_model->ReparentEntities(GetRootContainerEntityId(), { tireEntityId, driverEntityId });
        EXPECT_FALSE(isReparented);
    }

    TEST_F(EntityOutlinerTest, TestReparentEntityToAnotherPrefabFails)
    {
        // Level     (prefab)   <-- focused
        // | Car     (prefab)
        //   | Tire
        // | Bike    (prefab)
        //   | Pedal
        // | Driver

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string bikePrefabName = "BikePrefab";

        const AZStd::string tireEntityName = "Tire";
        const AZStd::string pedalEntityName = "Pedal";
        const AZStd::string driverEntityName = "Driver";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path bikePrefabFilepath = engineRootPath / bikePrefabName;

        // Create the Car prefab.
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });

        // Create the Bike prefab.
        AZ::EntityId pedalEntityId = CreateEditorEntityUnderRoot(pedalEntityName);
        AZ::EntityId bikeContainerId = CreateEditorPrefab(bikePrefabFilepath, { pedalEntityId });

        // Create the Driver entity.
        AZ::EntityId driverEntityId = CreateEditorEntityUnderRoot(driverEntityName);

        // Retrieve the Tire entity id.
        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        EXPECT_TRUE(carInstance.has_value());
        EntityAlias tireEntityAlias = FindEntityAliasInInstance(carContainerId, tireEntityName);
        EXPECT_FALSE(tireEntityAlias.empty());
        tireEntityId = carInstance->get().GetEntityId(tireEntityAlias);

        // Retrieve the Pedal entity id.
        InstanceOptionalReference bikeInstance = m_instanceEntityMapperInterface->FindOwningInstance(bikeContainerId);
        EXPECT_TRUE(bikeInstance.has_value());
        EntityAlias pedalEntityAlias = FindEntityAliasInInstance(bikeContainerId, pedalEntityName);
        EXPECT_FALSE(pedalEntityAlias.empty());
        pedalEntityId = bikeInstance->get().GetEntityId(pedalEntityAlias);
        
        // Validate that the Driver entity cannot be reparented from the focused Level prefab to the unfocused Car prefab.
        bool isReparented = m_model->ReparentEntities(tireEntityId, { driverEntityId });
        EXPECT_FALSE(isReparented);

        // Validate that the Pedal entity cannot be reparented from the unfocused Bike prefab to the unfocused Car prefab.
        isReparented = m_model->ReparentEntities(tireEntityId, { pedalEntityId });
        EXPECT_FALSE(isReparented);

        // Validate that the Tire entity cannot be reparented from the unfocused Car prefab to the focused Level prefab.
        isReparented = m_model->ReparentEntities(driverEntityId, { tireEntityId });
        EXPECT_FALSE(isReparented);
    }

    TEST_F(EntityOutlinerTest, TestReparentPrefabToAnotherPrefabFails)
    {
        // Level     (prefab)   <-- focused
        // | Car     (prefab)
        //   | Wheel (prefab)
        //     | Tire
        //   | Trunk
        // | Bike    (prefab)
        //   | Pedal

        const AZStd::string carPrefabName = "CarPrefab";
        const AZStd::string wheelPrefabName = "WheelPrefab";
        const AZStd::string bikePrefabName = "BikePrefab";

        const AZStd::string tireEntityName = "Tire";
        const AZStd::string trunkEntityName = "Trunk";
        const AZStd::string pedalEntityName = "Pedal";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;
        AZ::IO::Path wheelPrefabFilepath = engineRootPath / wheelPrefabName;
        AZ::IO::Path bikePrefabFilepath = engineRootPath / bikePrefabName;

        // Create the Wheel prefab.
        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot(tireEntityName);
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });
        AZ::EntityId trunkEntityId = CreateEditorEntityUnderRoot(trunkEntityName);

        // Create the Car prefab.
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelContainerId, trunkEntityId });

        // Create the Bike prefab.
        AZ::EntityId pedalEntityId = CreateEditorEntityUnderRoot(pedalEntityName);
        AZ::EntityId bikeContainerId = CreateEditorPrefab(bikePrefabFilepath, { pedalEntityId });

        // Retrieve Trunk entity id.
        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        EXPECT_TRUE(carInstance.has_value());
        EntityAlias trunkEntityAlias = FindEntityAliasInInstance(carContainerId, trunkEntityName);
        EXPECT_FALSE(trunkEntityAlias.empty());
        trunkEntityId = carInstance->get().GetEntityId(trunkEntityAlias);

        // Retrieve the Wheel container entity id.
        EntityOptionalReference wheelContainerEntity;
        carInstance->get().GetNestedInstances(
            [&wheelContainerEntity](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                wheelContainerEntity = nestedInstance->GetContainerEntity();
            });
        EXPECT_TRUE(wheelContainerEntity.has_value());
        wheelContainerId = wheelContainerEntity->get().GetId();

        // Retrieve the Pedal entity id.
        InstanceOptionalReference bikeInstance = m_instanceEntityMapperInterface->FindOwningInstance(bikeContainerId);
        EXPECT_TRUE(bikeInstance.has_value());
        EntityAlias pedalEntityAlias = FindEntityAliasInInstance(bikeContainerId, pedalEntityName);
        EXPECT_FALSE(pedalEntityAlias.empty());
        pedalEntityId = bikeInstance->get().GetEntityId(pedalEntityAlias);

        // Validate that the Bike prefab cannot be reparented from the focused Level prefab to the unfocused Car prefab.
        bool isReparented = m_model->ReparentEntities(trunkEntityId, { bikeContainerId });
        EXPECT_FALSE(isReparented);

        // Validate that the Wheel prefab cannot be reparented from the unfocused Car prefab to the unfocused Bike prefab.
        isReparented = m_model->ReparentEntities(bikeContainerId, { wheelContainerId });
        EXPECT_FALSE(isReparented);

        // Validate that the Wheel prefab cannot be reparented from the unfocused Car prefab to the focused Level prefab.
        isReparented = m_model->ReparentEntities(GetRootContainerEntityId(), { wheelContainerId });
        EXPECT_FALSE(isReparented);
    }
} // namespace UnitTest
