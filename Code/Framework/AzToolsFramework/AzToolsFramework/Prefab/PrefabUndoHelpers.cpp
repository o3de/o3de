/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabUndoHelpers.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoAddEntity.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoAddEntityAsOverride.h>

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
                const AZ::Entity& parentEntity,
                const AZ::Entity& newEntity,
                Instance& owningInstance,
                Instance& focusedInstance,
                UndoSystem::URSequencePoint* undoBatch)
            {
                if (&owningInstance == &focusedInstance)
                {
                    PrefabUndoAddEntity* addEntityUndoState =
                        aznew PrefabUndoAddEntity("Undo Adding Entity");
                    addEntityUndoState->SetParent(undoBatch);
                    addEntityUndoState->Capture(parentEntity, newEntity, focusedInstance);
                    addEntityUndoState->Redo();
                }
                else
                {
                    PrefabUndoAddEntityAsOverride* addEntityUndoState =
                        aznew PrefabUndoAddEntityAsOverride("Undo Adding Entity");
                    addEntityUndoState->SetParent(undoBatch);
                    addEntityUndoState->Capture(parentEntity, newEntity, owningInstance, focusedInstance);
                    addEntityUndoState->Redo();
                }
                
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
