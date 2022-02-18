/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Entity/EditorEntitySortComponent.h"
#include <UI/Outliner/EntityOutlinerSortFilterProxyModel.hxx>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
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
            m_proxyModel = AZStd::make_unique<AzToolsFramework::EntityOutlinerSortFilterProxyModel>();
            m_proxyModel->setSourceModel(m_model.get());
            m_proxyModel->sort(4, Qt::AscendingOrder);

            m_modelTester =
                AZStd::make_unique<QAbstractItemModelTester>(m_proxyModel.get(), QAbstractItemModelTester::FailureReportingMode::Fatal);
            
            // Create a new root prefab - the synthetic "NewLevel.prefab" that comes in by default isn't suitable for outliner tests
            // because it's created before the EditorEntityModel that our EntityOutlinerListModel subscribes to, and we want to
            // recreate it as part of the fixture regardless.
            CreateRootPrefab();

            auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
            auto rootEntity = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntity();
            rootEntity->get().Deactivate();
            rootEntity->get().CreateComponent<AzToolsFramework::Components::EditorEntitySortComponent>();
            rootEntity->get().Activate();

            AzToolsFramework::EditorEntityContextNotificationBus::Broadcast(
                &AzToolsFramework::EditorEntityContextNotification::SetForceAddEntitiesToBackFlag, true);
        }

        void TearDownEditorFixtureImpl() override
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Broadcast(
                &AzToolsFramework::EditorEntityContextNotification::SetForceAddEntitiesToBackFlag, false);

            m_undoStack = nullptr;
            m_modelTester.reset();
            m_model.reset();
            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        // Creates an entity with a given name as one undoable operation
        // Parents to parentId, or the root prefab container entity if parentId is invalid
        AZ::EntityId CreateNamedEntity(AZStd::string name, AZ::EntityId parentId = AZ::EntityId())
        {
            auto createResult = m_prefabPublicInterface->CreateEntity(parentId, AZ::Vector3());
            AZ_Assert(createResult.IsSuccess(), "Failed to create entity: %s", createResult.GetError().c_str());
            AZ::EntityId entityId = createResult.GetValue();

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

            entity->Deactivate();

            entity->SetName(name);

            // Normally, in invalid parent ID should automatically parent us to the root prefab, but currently in the unit test
            // environment entities aren't created with a default transform component, so CreateEntity won't correctly parent.
            // We get the actual target parent ID here, then create our missing transform component.
            if (!parentId.IsValid())
            {
                auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
                parentId = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntityId();
            }

            auto transform = aznew AzToolsFramework::Components::TransformComponent;
            entity->AddComponent(transform);
            transform->SetParent(parentId);

            entity->CreateComponent<AzToolsFramework::Components::EditorEntitySortComponent>();
            entity->Activate();

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

        // Gets the sorted index of an entity, i.e. same index that we see in EntityOutliner
        QModelIndex GetEntityIndex(AZ::EntityId entityId) const
        {
            return m_proxyModel->mapFromSource(m_model->GetIndexFromEntity(entityId));
        }

        // Kicks off any updates scheduled for the next tick
        void ProcessDeferredUpdates() override
        {
            // Force a prefab propagation for updates that are deferred to the next tick.
            PropagateAllTemplateChanges();

            // Ensure the model process its entity update queue
            m_model->ProcessEntityUpdates();
        }

        // Verifies that entities parented to parentId are in the expected order defined in entityMap
        void VerifyEntityOrder(AZStd::map<AZStd::string, size_t>& entityMap, AZ::EntityId parentId)
        {
            auto instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            auto instance = instanceEntityMapperInterface->FindOwningInstance(parentId);

            auto verifyEntityMapping = [this, &entityMap](const AZ::Entity& entity) -> bool
            {
                auto it = entityMap.find(entity.GetName());
                //verify that entity is present in cache
                EXPECT_TRUE(it != entityMap.end());
                auto entityIndex = GetEntityIndex(entity.GetId());
                EXPECT_EQ(it->second, entityIndex.row());
                return true;
            };
            instance->get().GetConstEntities(verifyEntityMapping);
        }
        
        AZStd::unique_ptr<AzToolsFramework::EntityOutlinerListModel> m_model;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTester;
        AZStd::unique_ptr<AzToolsFramework::EntityOutlinerSortFilterProxyModel> m_proxyModel;
    };

    TEST_F(EntityOutlinerTest, TestCreateFlatHierarchyUndoAndRedoWorks)
    {
        constexpr size_t entityCount = 10;
        AZStd::map<AZStd::string, size_t> entityMap;
        AZStd::vector<AZ::EntityId> entityIds;

        for (size_t i = 0; i < entityCount; ++i)
        {
            AZStd::string entityName = AZStd::string::format("Entity%zu", i);
            entityIds.push_back(CreateNamedEntity(entityName));
            EXPECT_EQ(m_model->rowCount(GetRootIndex()), i + 1);
            entityMap[entityName] = i;
        }
        
        auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        auto parentId = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntityId();

        // verify order is correct after creating new entities
        VerifyEntityOrder(entityMap, parentId);
        
        // undo creating entities
        for (int i = entityCount; i > 0; --i)
        {
            Undo();
            EXPECT_EQ(m_model->rowCount(GetRootIndex()), i - 1);
        }

        // redo creating entities
        for (size_t i = 0; i < entityCount; ++i)
        {
            Redo();
            EXPECT_EQ(m_model->rowCount(GetRootIndex()), i + 1);
        }

        // verify order is consistent after redo
        VerifyEntityOrder(entityMap, parentId);
    }

    // Disabled because deleting a non-container entity inside an instance triggers "Some of the patches were not successfully applied." error.
    TEST_F(EntityOutlinerTest, DISABLED_TestDeleteAndUndoWorks)
    {
        constexpr size_t entityCount = 10;
        AZStd::map<AZStd::string, size_t> entityMap;
        AZStd::vector<AZ::EntityId> entityIds;

        for (size_t i = 0; i < entityCount; ++i)
        {
            AZStd::string entityName = AZStd::string::format("Entity%zu", i);
            entityIds.push_back(CreateNamedEntity(entityName));
            EXPECT_EQ(m_model->rowCount(GetRootIndex()), i + 1);
            entityMap[entityName] = i;
        }

        auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        auto parentId = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntityId();

        // test deletion
        m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(entityIds);
        EXPECT_EQ(m_model->rowCount(GetRootIndex()), 0);

        // undo deletion
        Undo();
        EXPECT_EQ(m_model->rowCount(GetRootIndex()), entityCount);
        
        // verify order is consistent after undoing deletions
        VerifyEntityOrder(entityMap, parentId);
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

    // Disabled because entities inside the instantiated prefab are in incorrect order
    TEST_F(EntityOutlinerTest, DISABLED_TestCreateAndInstantiatePrefabWorks)
    {
        constexpr size_t entityCount = 10;
        AZStd::map<AZStd::string, size_t> entityMap;
        AZStd::vector<AZ::EntityId> entityIds;

        for (size_t i = 0; i < entityCount; ++i)
        {
            AZStd::string entityName = AZStd::string::format("Entity%zu", i);
            entityIds.push_back(CreateNamedEntity(entityName));
            EXPECT_EQ(m_model->rowCount(GetRootIndex()), i + 1);
            entityMap[entityName] = i;
        }
        
        auto createPrefabResult = m_prefabPublicInterface->CreatePrefabInMemory(entityIds, PrefabMockFilePath);
        EXPECT_TRUE(createPrefabResult.IsSuccess());

        auto containerId1 = createPrefabResult.GetValue();
        EXPECT_EQ(m_model->rowCount(GetEntityIndex(containerId1)), entityCount);
        // verify the entities are in correct order when re-parented to container entity during creation of prefab
        VerifyEntityOrder(entityMap, containerId1);

        auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        auto levelId = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntityId();
        auto instantiatePrefabResult = m_prefabPublicInterface->InstantiatePrefab(PrefabMockFilePath, levelId, AZ::Vector3::CreateZero());
        EXPECT_TRUE(instantiatePrefabResult.IsSuccess());

        auto containerId2 = instantiatePrefabResult.GetValue();
        EXPECT_EQ(m_model->rowCount(GetEntityIndex(containerId2)), entityCount);
        // verify entities are in correct order after instantiating a prefab (currently broken)
        VerifyEntityOrder(entityMap, containerId2);
    }
} // namespace UnitTest
