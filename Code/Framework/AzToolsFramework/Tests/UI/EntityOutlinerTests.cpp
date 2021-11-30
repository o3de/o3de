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

            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                m_undoStack, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetUndoStack);
            AZ_Assert(m_undoStack, "Failed to look up undo stack from tools application");

            // Create a new root prefab - the synthetic "NewLevel.prefab" that comes in by default isn't suitable for outliner tests
            // because it's created before the EditorEntityModel that our EntityOutlinerListModel subscribes to, and we want to
            // recreate it as part of the fixture regardless.
            auto entityOwnershipService = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
            entityOwnershipService->CreateNewLevelPrefab("UnitTestRoot.prefab", "");
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
            transform->SetParent(parentId);
            entity->AddComponent(transform);

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

        // Kicks off any updates scheduled for the next tick
        void ProcessDeferredUpdates()
        {
            // Force a prefab propagation for updates that are deferred to the next tick.
            m_prefabSystemComponent->OnSystemTick();

            // Ensure the model process its entity update queue
            m_model->ProcessEntityUpdates();
        }

        // Performs an undo operation and ensures the tick-scheduled updates happen
        void Undo()
        {
            m_undoStack->Undo();
            ProcessDeferredUpdates();
        }

        // Performs a redo operation and ensures the tick-scheduled updates happen
        void Redo()
        {
            m_undoStack->Redo();
            ProcessDeferredUpdates();
        }

        AZStd::unique_ptr<AzToolsFramework::EntityOutlinerListModel> m_model;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTester;
        AzToolsFramework::UndoSystem::UndoStack* m_undoStack = nullptr;
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
} // namespace UnitTest
