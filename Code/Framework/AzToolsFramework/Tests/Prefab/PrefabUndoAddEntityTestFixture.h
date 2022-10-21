/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabTestFixture.h>
#include <Prefab/Undo/PrefabUndoAddEntity.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;
    using namespace PrefabTestUtils;

    using CreatePrefabUndoAddEntityNodeResult = AZ::Outcome<PrefabUndoAddEntity, AZStd::string>;

    class PrefabUndoAddEntityTestFixture
        : public PrefabTestFixture
    {
    protected:
        using PrefabTestFixture::CreateEntity;

        EntityAlias CreateEntity(const AZStd::string& entityName,
            Instance& owningInstance);

        EntityAlias CreateEntity(const AZStd::string& entityName,
            Instance& owningInstance, const EntityAlias& parentEntityAlias);

        CreatePrefabUndoAddEntityNodeResult CreatePrefabUndoAddEntityNode(
            Instance& owningInstance,
            const EntityAlias& newEntityAlias,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName = "Undo Adding Entity");

        CreatePrefabUndoAddEntityNodeResult CreatePrefabUndoAddEntityNode(
            Instance& owningInstance,
            const EntityAlias& parentEntityAlias, const EntityAlias& newEntityAlias,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName = "Undo Adding Entity");

        void ValidateNewEntityUnderInstance(
            Instance& owningInstance,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityUnderParentEntity(
            Instance& owningInstance,
            const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderInstance(
            Instance& instance,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderParentEntity(
            Instance& instance,
            const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

    private:
        EntityAlias CreateEntity(const AZStd::string& entityName,
            Instance& owningInstance, const AZ::Entity& parentEntity);

        CreatePrefabUndoAddEntityNodeResult CreatePrefabUndoAddEntityNode(
            const AZ::Entity& parentEntity, const AZ::Entity& newEntity,
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
    };
}
