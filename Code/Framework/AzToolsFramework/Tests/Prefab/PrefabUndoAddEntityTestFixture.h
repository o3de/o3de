/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabTestFixture.h>
#include <Prefab/Undo/PrefabUndoFocusedInstanceAddEntity.h>
#include <Prefab/Undo/PrefabUndoUnfocusedInstanceAddEntity.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;
    using namespace PrefabTestUtils;

    using CreateFocusedInstanceAddEntityUndoNodeResult =
        AZ::Outcome<PrefabUndoFocusedInstanceAddEntity, AZStd::string>;
    using CreateUnfocusedInstanceAddEntityUndoNodeResult =
        AZ::Outcome<PrefabUndoUnfocusedInstanceAddEntity, AZStd::string>;
    using InstanceList = AZStd::vector<AZStd::reference_wrapper<Instance>>;

    class PrefabUndoAddEntityTestFixture
        : public PrefabTestFixture
    {
    protected:
        using PrefabTestFixture::CreateEntity;

        EntityAlias CreateEntity(const AZStd::string& entityName,
            Instance& owningInstance);

        EntityAlias CreateEntity(const AZStd::string& entityName,
            Instance& owningInstance, const EntityAlias& parentEntityAlias);

        CreateFocusedInstanceAddEntityUndoNodeResult CreateFocusedInstanceAddEntityUndoNode(
            const EntityAlias& newEntityAlias,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName = "Undo Adding Entity");

        CreateFocusedInstanceAddEntityUndoNodeResult CreateFocusedInstanceAddEntityUndoNode(
            const EntityAlias& parentEntityAlias, const EntityAlias& newEntityAlias,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName = "Undo Adding Entity");

        CreateUnfocusedInstanceAddEntityUndoNodeResult CreateUnfocusedInstanceAddEntityUndoNode(
            const EntityAlias& newEntityAlias,
            Instance& owningInstance,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName = "Undo Adding Entity");

        CreateUnfocusedInstanceAddEntityUndoNodeResult CreateUnfocusedInstanceAddEntityUndoNode(
            const EntityAlias& parentEntityAlias, const EntityAlias& newEntityAlias,
            Instance& owningInstance,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName = "Undo Adding Entity");

        void ValidateNewEntityUnderInstance(
            InstanceList& instances,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityUnderInstance(
            Instance& instance,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityUnderParentEntity(
            InstanceList& instances,
            const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityUnderParentEntity(
            Instance& instance,
            const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderParentEntity(
            InstanceList& instances,
            const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderParentEntity(
            Instance& instance,
            const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderInstance(
            InstanceList& instances,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderInstance(
            Instance& instance,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

    private:
        EntityAlias CreateEntity(const AZStd::string& entityName,
            Instance& owningInstance, const AZ::Entity& parentEntity);

        CreateFocusedInstanceAddEntityUndoNodeResult CreateFocusedInstanceAddEntityUndoNode(
            const AZ::Entity& parentEntity, const AZ::Entity& newEntity,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName = "Undo Adding Entity");

        CreateUnfocusedInstanceAddEntityUndoNodeResult CreateUnfocusedInstanceAddEntityUndoNode(
            const AZ::Entity& parentEntity, const AZ::Entity& newEntity,
            Instance& owningInstance,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName = "Undo Adding Entity");

        void ValidateNewEntityUnderParentEntity(
            Instance& instance,
            const AZ::Entity& parentEntity,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderParentEntity(
            Instance& instance,
            const AZ::Entity& parentEntity,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

        AZ::Entity& GetEntityFromOwningInstance(const EntityAlias& entityAlias,
            Instance& owningInstance);
    };
}
