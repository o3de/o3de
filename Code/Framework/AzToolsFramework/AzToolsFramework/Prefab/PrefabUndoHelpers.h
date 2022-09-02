/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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
            //! @param newEntityDom The DOM of the added entity.
            //! @param entityId The id of the added entity.
            //! @param templateId The id of the prefab template under which the new entity DOM will live.
            //! @param undoBatch The undo batch node to register the add-entity undo node to.
            void AddEntity(
                const PrefabDomValue& newEntityDom,
                AZ::EntityId entityId,
                TemplateId templateId,
                UndoSystem::URSequencePoint* undoBatch);

            //! Helper function for removing entities to a prefab template with undo-redo support.
            //! @param entityDomAndPathList The list of pairs of entity DOM before removal and its alias path in template.
            //! @param templateId The id of the prefab template under which the removed entity DOMs will live.
            //! @param undoBatch The undo batch node to register the remove-entities undo node to.
            void RemoveEntities(                
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
        }
    } // namespace Prefab
} // namespace AzToolsFramework
