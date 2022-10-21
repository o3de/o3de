/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabUndoAddEntityTestFixture.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    EntityAlias PrefabUndoAddEntityTestFixture::CreateEntity(const AZStd::string& entityName,
        Instance& owningInstance)
    {
        AZ::Entity& containerEntity = owningInstance.GetContainerEntity()->get();

        if (!containerEntity.GetTransform())
        {
            auto transform = aznew AzToolsFramework::Components::TransformComponent;
            containerEntity.AddComponent(transform);
            containerEntity.Init();
            containerEntity.Activate();
        }

        return CreateEntity(entityName, owningInstance, containerEntity);
    }

    EntityAlias PrefabUndoAddEntityTestFixture::CreateEntity(const AZStd::string& entityName,
        Instance& owningInstance, const EntityAlias& parentEntityAlias)
    {
        auto getParentEntityRefResult = owningInstance.GetEntity(parentEntityAlias);
        if (!getParentEntityRefResult.has_value())
        {
            return "";
        }

        AZ::Entity& parentEntity = getParentEntityRefResult->get();

        return CreateEntity(entityName, owningInstance, parentEntity);
    }

    EntityAlias PrefabUndoAddEntityTestFixture::CreateEntity(const AZStd::string& entityName,
        Instance& owningInstance, const AZ::Entity& parentEntity)
    {
        EntityAlias newEntityAlias = Instance::GenerateEntityAlias();

        AliasPath newAbsoluteEntityPath = owningInstance.GetAbsoluteInstanceAliasPath();
        newAbsoluteEntityPath.Append(newEntityAlias);

        AZ::EntityId newEntityId = InstanceEntityIdMapper::GenerateEntityIdForAliasPath(newAbsoluteEntityPath);
        AZ::Entity* newEntity = aznew AZ::Entity(newEntityId, entityName.c_str());

        owningInstance.AddEntity(*newEntity, newEntityAlias);

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{ newEntity });
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::FinalizeEditorEntity, newEntity);

        newEntity->Deactivate();
        auto transform = aznew AzToolsFramework::Components::TransformComponent;
        transform->SetParent(parentEntity.GetId());
        newEntity->AddComponent(transform);
        newEntity->Activate();

        return newEntityAlias;
    }

    CreatePrefabUndoAddEntityNodeResult PrefabUndoAddEntityTestFixture::CreatePrefabUndoAddEntityNode(
        const AZ::Entity& parentEntity, const AZ::Entity& newEntity, Instance& focusedInstance,
        const AZStd::string& undoAddEntityOperationName)
    {
        PrefabUndoAddEntity undoAddEntityNode(undoAddEntityOperationName);
        undoAddEntityNode.Capture(parentEntity, newEntity, focusedInstance);

        return AZ::Success(AZStd::move(undoAddEntityNode));
    }

    CreatePrefabUndoAddEntityNodeResult PrefabUndoAddEntityTestFixture::CreatePrefabUndoAddEntityNode(
        Instance& owningInstance, const EntityAlias& newEntityAlias, Instance& focusedInstance,
        const AZStd::string& undoAddEntityOperationName)
    {
        auto getNewEntityRefResult = owningInstance.GetEntity(newEntityAlias);
        if (!getNewEntityRefResult.has_value())
        {
            return AZ::Failure(AZStd::string("Can't find new entity in instance."));
        }

        AZ::Entity& newEntity = getNewEntityRefResult->get();

        return CreatePrefabUndoAddEntityNode(
            owningInstance.GetContainerEntity()->get(), newEntity,
            focusedInstance,
            undoAddEntityOperationName);
    }

    CreatePrefabUndoAddEntityNodeResult PrefabUndoAddEntityTestFixture::CreatePrefabUndoAddEntityNode(
        Instance& owningInstance,
        const EntityAlias& parentEntityAlias, const EntityAlias& newEntityAlias,
        Instance& focusedInstance,
        const AZStd::string& undoAddEntityOperationName)
    {
        auto getParentEntityRefResult = owningInstance.GetEntity(parentEntityAlias);
        if (!getParentEntityRefResult.has_value())
        {
            return AZ::Failure(AZStd::string("Can't find parent entity in instance."));
        }

        auto getNewEntityRefResult = owningInstance.GetEntity(newEntityAlias);
        if (!getNewEntityRefResult.has_value())
        {
            return AZ::Failure(AZStd::string("Can't find new entity in instance."));
        }

        AZ::Entity& parentEntity = getParentEntityRefResult->get();
        AZ::Entity& newEntity = getNewEntityRefResult->get();

        return CreatePrefabUndoAddEntityNode(
            parentEntity, newEntity,
            focusedInstance,
            undoAddEntityOperationName);
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityUnderInstance(
        AZStd::vector<AZStd::unique_ptr<Instance>>& instances,
        const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
        size_t expectedEntityCount)
    {
        for (auto& instancePtr : instances)
        {
            Instance& instance = *instancePtr;
            auto getContainerEntityRefResult = instance.GetContainerEntity();
            ASSERT_TRUE(getContainerEntityRefResult.has_value());
            auto& containerEntity = getContainerEntityRefResult->get();

            ValidateNewEntityUnderParentEntity(
                instance, containerEntity, newEntityAlias, newEntityName, expectedEntityCount);
        }
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityUnderParentEntity(
        AZStd::vector<AZStd::unique_ptr<Instance>>& instances,
        const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
        const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
        size_t expectedEntityCount)
    {
        for (auto& instancePtr : instances)
        {
            Instance& instance = *instancePtr;
            auto getParentEntityRefResult = instance.GetEntity(parentEntityAlias);
            ASSERT_TRUE(getParentEntityRefResult.has_value());

            AZ::Entity& parentEntity = getParentEntityRefResult->get();
            EXPECT_TRUE(parentEntity.GetName() == parentEntityName);

            ValidateNewEntityUnderParentEntity(
                instance, parentEntity, newEntityAlias, newEntityName, expectedEntityCount);
        }
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityUnderParentEntity(
        Instance& instance,
        const AZ::Entity& parentEntity,
        const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
        size_t expectedEntityCount)
    {
        auto getNewEntityRefResult = instance.GetEntity(newEntityAlias);
        ASSERT_TRUE(getNewEntityRefResult.has_value());

        AZ::Entity& newEntity = getNewEntityRefResult->get();
        EXPECT_TRUE(newEntity.GetName() == newEntityName);

        AZ::EntityId parentEntityId;
        AZ::TransformBus::EventResult(parentEntityId, newEntity.GetId(), &AZ::TransformBus::Events::GetParentId);
        EXPECT_TRUE(parentEntityId == parentEntity.GetId());

        AZStd::vector<AZ::EntityId> entitiesUnderParentEntity;
        AZ::TransformBus::EventResult(entitiesUnderParentEntity, parentEntity.GetId(), &AZ::TransformBus::Events::GetChildren);
        const bool newEntityFound = AZStd::find(entitiesUnderParentEntity.begin(), entitiesUnderParentEntity.end(),
            newEntity.GetId()) != entitiesUnderParentEntity.end();
        EXPECT_TRUE(newEntityFound);

        EXPECT_EQ(instance.GetEntityAliasCount(), expectedEntityCount);
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityNotUnderInstance(
        AZStd::vector<AZStd::unique_ptr<Instance>>& instances,
        const EntityAlias& newEntityAlias,
        size_t expectedEntityCount)
    {
        for (auto& instancePtr : instances)
        {
            Instance& instance = *instancePtr;
            auto getContainerEntityRefResult = instance.GetContainerEntity();
            ASSERT_TRUE(getContainerEntityRefResult.has_value());
            auto& containerEntity = getContainerEntityRefResult->get();

            ValidateNewEntityNotUnderParentEntity(instance, containerEntity, newEntityAlias, expectedEntityCount);
        }
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityNotUnderParentEntity(
        AZStd::vector<AZStd::unique_ptr<Instance>>& instances,
        const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
        const EntityAlias& newEntityAlias,
        size_t expectedEntityCount)
    {
        for (auto& instancePtr : instances)
        {
            Instance& instance = *instancePtr;
            auto getParentEntityRefResult = instance.GetEntity(parentEntityAlias);
            ASSERT_TRUE(getParentEntityRefResult.has_value());

            AZ::Entity& parentEntity = getParentEntityRefResult->get();
            EXPECT_TRUE(parentEntity.GetName() == parentEntityName);

            ValidateNewEntityNotUnderParentEntity(instance, parentEntity, newEntityAlias, expectedEntityCount);
        }
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityNotUnderParentEntity(
        Instance& instance,
        const AZ::Entity& parentEntity,
        const EntityAlias& newEntityAlias,
        size_t expectedEntityCount)
    {
        auto getNewEntityRefResult = instance.GetEntity(newEntityAlias);
        ASSERT_FALSE(getNewEntityRefResult.has_value());

        AZStd::vector<AZ::EntityId> entitiesUnderParentEntity;
        AZ::TransformBus::EventResult(entitiesUnderParentEntity, parentEntity.GetId(), &AZ::TransformBus::Events::GetChildren);
        for (AZ::EntityId childEntityId : entitiesUnderParentEntity)
        {
            auto getChildEntityAliasResult = instance.GetEntityAlias(childEntityId);
            ASSERT_TRUE(getChildEntityAliasResult.has_value());

            EntityAlias& childEntityAlias = getChildEntityAliasResult->get();
            EXPECT_TRUE(childEntityAlias != newEntityAlias);
        }

        EXPECT_EQ(instance.GetEntityAliasCount(), expectedEntityCount);
    }
}
