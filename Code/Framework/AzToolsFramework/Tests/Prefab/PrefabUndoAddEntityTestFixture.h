/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabTestFixture.h>

#include <AzToolsFramework/Prefab/Undo/PrefabUndoAddEntity.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoAddEntityAsOverride.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;
    using namespace PrefabTestUtils;

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

        PrefabUndoAddEntity CreatePrefabUndoAddEntityNode(
            const EntityAlias& newEntityAlias,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName,
            const EntityAlias& parentEntityAlias = "");

        PrefabUndoAddEntityAsOverride CreatePrefabUndoAddEntityAsOverrideNode(
            const EntityAlias& newEntityAlias,
            Instance& owningInstance,
            Instance& focusedInstance,
            const AZStd::string& undoAddEntityOperationName,
            const EntityAlias& parentEntityAlias = "");

        void ValidateNewEntityUnderInstance(
            Instance& instance,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityUnderParentEntity(
            Instance& instance,
            const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
            const EntityAlias& newEntityAlias, const AZStd::string& newEntityName,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderParentEntity(
            Instance& instance,
            const EntityAlias& parentEntityAlias, const AZStd::string& parentEntityName,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

        void ValidateNewEntityNotUnderInstance(
            Instance& instance,
            const EntityAlias& newEntityAlias,
            size_t expectedEntityCount);

    private:
        EntityAlias CreateEntity(const AZStd::string& entityName,
            Instance& owningInstance, const AZ::Entity& parentEntity);

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
