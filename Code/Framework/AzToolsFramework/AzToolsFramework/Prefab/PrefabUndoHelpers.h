/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Undo/PrefabUndo.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabUndoHelpers
        {
            void UpdatePrefabInstance(
                const Instance& instance, AZStd::string_view undoMessage, const PrefabDom& instanceDomBeforeUpdate,
                UndoSystem::URSequencePoint* undoBatch);
            LinkId CreateLink(
                TemplateId sourceTemplateId, TemplateId targetTemplateId, PrefabDom patch,
                const InstanceAlias& instanceAlias, UndoSystem::URSequencePoint* undoBatch);
            void RemoveLink(
                TemplateId sourceTemplateId, TemplateId targetTemplateId, const InstanceAlias& instanceAlias, LinkId linkId,
                PrefabDom linkPatches, UndoSystem::URSequencePoint* undoBatch);

            //! Helper function for adding an entity to a prefab template with undo-redo support.
            //! @param parentEntity The target parent entity of the newly added entity.
            //! @param newEntity The new entity to be added.
            //! @param owningInstance The owning prefab instance of both parentEntity and newEntity.
            //! @param focusedInstance The current focused prefab instance.
            //! @param undoBatch The undo batch node to register the add-entity undo node to.
            void AddEntity(
                const AZ::Entity& parentEntity,
                const AZ::Entity& newEntity,
                Instance& owningInstance,
                Instance& focusedInstance,
                UndoSystem::URSequencePoint* undoBatch);

            //! Helper function for removing entity DOMs to prefab template with undo-redo support.
            //! Note: It does not include updating relevant parent entities.
            //! @param entityDomAndPathList The list of pairs of entity DOM before removal and its alias path in template.
            //! @param templateId The id of the prefab template under which the removed entity DOMs will live.
            //! @param undoBatch The undo batch node to register the remove-entities undo node to.
            void RemoveEntityDoms(
                const AZStd::vector<AZStd::pair<const PrefabDomValue*, AZStd::string>>& entityDomAndPathList,
                TemplateId templateId,
                UndoSystem::URSequencePoint* undoBatch);

            //! Helper function for updating an entity to a prefab template with undo-redo support.
            //! @param entityDomBeforeUpdatingEntity The DOM of the entity before updating.
            //! @param entityDomAfterUpdatingEntity The DOM of the entity after updating.
            //! @param entityId The id of the entity.
            //! @param undoBatch The undo batch node to register the update-entity undo node to.
            void UpdateEntity(
                const PrefabDomValue& entityDomBeforeUpdatingEntity,
                const PrefabDomValue& entityDomAfterUpdatingEntity,
                AZ::EntityId entityId,
                UndoSystem::URSequencePoint* undoBatch);

            //! Helper function for deleting entities and update parents to prefab template with undo-redo support.
            //! Note: It includes updating relevant parent entities.
            //! @param entityAliasPathList The alias path list for entities that will be removed.
            //! @param parentEntityList The parent entity list that will be updated.
            //! @param focusedInstance The current focused prefab instance.
            //! @param undoBatch The undo batch node to register the removal undo nodes to.
            void DeleteEntities(
                const AZStd::vector<AZStd::string>& entityAliasPathList,
                const AZStd::vector<const AZ::Entity*> parentEntityList,
                Instance& focusedInstance,
                UndoSystem::URSequencePoint* undoBatch);

            //! Helper function for deleting entities and prefab instances as overrides to focused template with undo-redo support.
            //! @param entityAliasPathList The alias path list for entities that will be removed.
            //! @param instanceAliasPathList The alias path list for instances that will be removed.
            //! @param parentEntityList The parent entity list that will be updated.
            //! @param owningInstance The common owning prefab instance of all inputs.
            //! @param focusedInstance The current focused prefab instance.
            //! @param undoBatch The undo batch node to register the removal undo nodes to.
            void DeleteEntitiesAndPrefabsAsOverride(
                const AZStd::vector<AZStd::string>& entityAliasPathList,
                const AZStd::vector<AZStd::string>& instanceAliasPathList,
                const AZStd::vector<const AZ::Entity*> parentEntityList,
                Instance& owningInstance,
                Instance& focusedInstance,
                UndoSystem::URSequencePoint* undoBatch);
        }
    } // namespace Prefab
} // namespace AzToolsFramework
