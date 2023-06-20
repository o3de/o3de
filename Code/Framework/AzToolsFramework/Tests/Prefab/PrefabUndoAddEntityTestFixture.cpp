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
            const bool activated = containerEntity.GetState() == AZ::Entity::State::Active;
            if (activated)
            {
                containerEntity.Deactivate();
            }

            auto transform = aznew AzToolsFramework::Components::TransformComponent;
            containerEntity.AddComponent(transform);
            if (!activated)
            {
                containerEntity.Init();
            }
            containerEntity.Activate();
        }

        return CreateEntity(entityName, owningInstance, containerEntity);
    }

    EntityAlias PrefabUndoAddEntityTestFixture::CreateEntity(const AZStd::string& entityName,
        Instance& owningInstance, const EntityAlias& parentEntityAlias)
    {
        AZ::Entity& parentEntity = GetEntityFromOwningInstance(parentEntityAlias, owningInstance);
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
        newEntity->Init();
        newEntity->Activate();
        AddRequiredEditorComponents({ newEntity->GetId() });

        owningInstance.AddEntity(*newEntity, newEntityAlias);

        AZ::TransformBus::Event(newEntityId, &AZ::TransformBus::Events::SetParent, parentEntity.GetId());

        return newEntityAlias;
    }

    PrefabUndoAddEntity PrefabUndoAddEntityTestFixture::CreatePrefabUndoAddEntityNode(
        const EntityAlias& newEntityAlias,
        Instance& focusedInstance,
        const AZStd::string& undoAddEntityOperationName,
        const EntityAlias& parentEntityAlias)
    {
        AZ::Entity& parentEntity = parentEntityAlias.empty()?
            focusedInstance.GetContainerEntity()->get() : 
            GetEntityFromOwningInstance(parentEntityAlias, focusedInstance);
        AZ::Entity& newEntity = GetEntityFromOwningInstance(newEntityAlias, focusedInstance);

        PrefabUndoAddEntity undoAddEntityNode(undoAddEntityOperationName);
        undoAddEntityNode.Capture(parentEntity, newEntity, focusedInstance);
        return AZStd::move(undoAddEntityNode);
    }

    PrefabUndoAddEntityAsOverride PrefabUndoAddEntityTestFixture::CreatePrefabUndoAddEntityAsOverrideNode(
        const EntityAlias& newEntityAlias,
        Instance& owningInstance,
        Instance& focusedInstance,
        const AZStd::string& undoAddEntityOperationName,
        const EntityAlias& parentEntityAlias)
    {
        AZ::Entity& parentEntity = parentEntityAlias.empty() ?
            owningInstance.GetContainerEntity()->get() :
            GetEntityFromOwningInstance(parentEntityAlias, owningInstance);
        AZ::Entity& newEntity = GetEntityFromOwningInstance(newEntityAlias, owningInstance);

        PrefabUndoAddEntityAsOverride undoAddEntityNode(undoAddEntityOperationName);
        undoAddEntityNode.Capture(parentEntity, newEntity, owningInstance, focusedInstance);
        return AZStd::move(undoAddEntityNode);
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityUnderInstance(
        Instance& instance,
        const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
        size_t expectedEntityCount)
    {
        auto getContainerEntityRefResult = instance.GetContainerEntity();
        ASSERT_TRUE(getContainerEntityRefResult.has_value());
        auto& containerEntity = getContainerEntityRefResult->get();

        ValidateNewEntityUnderParentEntity(instance,
            containerEntity,
            newEntityAlias, newEntityName,
            expectedEntityCount);
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityUnderParentEntity(
        Instance& instance,
        const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
        const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
        size_t expectedEntityCount)
    {
        auto getParentEntityRefResult = instance.GetEntity(parentEntityAlias);
        ASSERT_TRUE(getParentEntityRefResult.has_value());

        AZ::Entity& parentEntity = getParentEntityRefResult->get();
        EXPECT_TRUE(parentEntity.GetName() == parentEntityName);

        ValidateNewEntityUnderParentEntity(
            instance, parentEntity, newEntityAlias, newEntityName, expectedEntityCount);
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
        Instance& instance,
        const EntityAlias& newEntityAlias,
        size_t expectedEntityCount)
    {
        auto getContainerEntityRefResult = instance.GetContainerEntity();
        ASSERT_TRUE(getContainerEntityRefResult.has_value());
        auto& containerEntity = getContainerEntityRefResult->get();

        ValidateNewEntityNotUnderParentEntity(instance, containerEntity, newEntityAlias, expectedEntityCount);
    }

    void PrefabUndoAddEntityTestFixture::ValidateNewEntityNotUnderParentEntity(
        Instance& instance,
        const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
        const EntityAlias& newEntityAlias,
        size_t expectedEntityCount)
    {
        auto getParentEntityRefResult = instance.GetEntity(parentEntityAlias);
        ASSERT_TRUE(getParentEntityRefResult.has_value());

        AZ::Entity& parentEntity = getParentEntityRefResult->get();
        EXPECT_TRUE(parentEntity.GetName() == parentEntityName);

        ValidateNewEntityNotUnderParentEntity(instance,
            parentEntity, newEntityAlias,
            expectedEntityCount);
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

    AZ::Entity& PrefabUndoAddEntityTestFixture::GetEntityFromOwningInstance(const EntityAlias& entityAlias,
        Instance& owningInstance)
    {
        auto getEntityRefResult = owningInstance.GetEntity(entityAlias);
        AZ_Assert(getEntityRefResult.has_value(), "Entity not found in instance.");
        return getEntityRefResult->get();
    }
}
