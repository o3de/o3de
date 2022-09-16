/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabUndo.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabUndoHelpers
        {
            void UpdatePrefabInstance(
                const Instance& instance, AZStd::string_view undoMessage, const PrefabDom& instanceDomBeforeUpdate,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabDom instanceDomAfterUpdate;
                PrefabDomUtils::StoreInstanceInPrefabDom(instance, instanceDomAfterUpdate);

                PrefabUndoInstance* state = aznew Prefab::PrefabUndoInstance(undoMessage);
                state->Capture(instanceDomBeforeUpdate, instanceDomAfterUpdate, instance.GetTemplateId());
                state->SetParent(undoBatch);
                state->Redo();
            }

            LinkId CreateLink(
                TemplateId sourceTemplateId, TemplateId targetTemplateId, PrefabDom patch,
                const InstanceAlias& instanceAlias, UndoSystem::URSequencePoint* undoBatch)
            {
                auto linkAddUndo = aznew PrefabUndoInstanceLink("Create Link");
                linkAddUndo->Capture(targetTemplateId, sourceTemplateId, instanceAlias, AZStd::move(patch), InvalidLinkId);
                linkAddUndo->SetParent(undoBatch);
                linkAddUndo->Redo();

                return linkAddUndo->GetLinkId();
            }

            void RemoveLink(
                TemplateId sourceTemplateId, TemplateId targetTemplateId, const InstanceAlias& instanceAlias, LinkId linkId,
                PrefabDom linkPatches, UndoSystem::URSequencePoint* undoBatch)
            {
                auto linkRemoveUndo = aznew PrefabUndoInstanceLink("Remove Link");
                linkRemoveUndo->Capture(targetTemplateId, sourceTemplateId, instanceAlias, AZStd::move(linkPatches), linkId);
                linkRemoveUndo->SetParent(undoBatch);
                linkRemoveUndo->Redo();
            }

            void AddEntity(
                const PrefabDomValue& newEntityDom,
                AZ::EntityId entityId,
                TemplateId templateId,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabUndoAddEntity* addEntityUndoState = aznew PrefabUndoAddEntity("Undo Adding Entity");
                addEntityUndoState->SetParent(undoBatch);
                addEntityUndoState->Capture(newEntityDom, entityId, templateId);
                addEntityUndoState->Redo();
            }

            void RemoveEntities(
                const AZStd::vector<AZStd::pair<const PrefabDomValue*, AZStd::string>>& entityDomAndPathList,
                TemplateId templateId,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabUndoRemoveEntities* removeEntitiesUndoState = aznew PrefabUndoRemoveEntities("Undo Removing Entities");
                removeEntitiesUndoState->SetParent(undoBatch);
                removeEntitiesUndoState->Capture(entityDomAndPathList, templateId);
                removeEntitiesUndoState->Redo();
            }

            void UpdateEntity(
                const PrefabDomValue& entityDomBeforeUpdatingEntity,
                const PrefabDomValue& entityDomAfterUpdatingEntity,
                AZ::EntityId entityId,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabUndoEntityUpdate* state = aznew PrefabUndoEntityUpdate("Undo Updating Entity");
                state->SetParent(undoBatch);
                state->Capture(entityDomBeforeUpdatingEntity, entityDomAfterUpdatingEntity, entityId);
                state->Redo();
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
