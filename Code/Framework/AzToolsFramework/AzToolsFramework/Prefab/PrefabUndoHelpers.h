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
            //! @param parentEntityDomBeforeAddingEntity The DOM of the parent entity before adding a new entity under it.
            //! @param parentEntityDomAfterAddingEntity The DOM of the parent entity after adding a new entity under it.
            //! @param entityId The id of the entity to be added.
            //! @param parentEntityId The id of the parent entity under which the new entity is added.
            //! @param templateId The id of the prefab template under which the new entity DOM will live.
            //! @param undoBatch The undo batch node to register the add-entity undo nodes to. 
            void AddEntity(
                const PrefabDomValue& parentEntityDomBeforeAddingEntity,
                const PrefabDomValue& parentEntityDomAfterAddingEntity,
                const PrefabDomValue& newEntityDom,
                AZ::EntityId entityId,
                AZ::EntityId parentEntityId,
                TemplateId templateId,
                UndoSystem::URSequencePoint* undoBatch);
        }
    } // namespace Prefab
} // namespace AzToolsFramework
