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
#include <AzToolsFramework/Prefab/Undo/PrefabUndoDelete.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoDeleteAsOverride.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoEntityOverrides.h>

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
                        aznew PrefabUndoAddEntityAsOverride("Undo Adding Entity As Override");
                    addEntityUndoState->SetParent(undoBatch);
                    addEntityUndoState->Capture(parentEntity, newEntity, owningInstance, focusedInstance);
                    addEntityUndoState->Redo();
                }
                
            }

            void AddEntityDoms(
                const AZStd::vector<const AZ::Entity*>& entityList,
                TemplateId templateId,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabUndoAddEntityDoms* undoState = aznew PrefabUndoAddEntityDoms("Undo Adding Entity DOMs");
                undoState->SetParent(undoBatch);
                undoState->Capture(entityList, templateId);
                undoState->Redo();
            }

            void RemoveEntityDoms(
                const AZStd::vector<AZStd::pair<const PrefabDomValue*, AZStd::string>>& entityDomAndPathList,
                TemplateId templateId,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabUndoRemoveEntityDoms* state = aznew PrefabUndoRemoveEntityDoms("Undo Removing Entity DOMs");
                state->SetParent(undoBatch);
                state->Capture(entityDomAndPathList, templateId);
                state->Redo();
            }

            void UpdateEntity(
                const PrefabDomValue& entityDomBeforeUpdatingEntity,
                const PrefabDomValue& entityDomAfterUpdatingEntity,
                AZ::EntityId entityId,
                UndoSystem::URSequencePoint* undoBatch,
                bool updateCache)
            {
                PrefabUndoEntityUpdate* state = aznew PrefabUndoEntityUpdate("Undo Updating Entity");
                state->SetParent(undoBatch);
                state->Capture(entityDomBeforeUpdatingEntity, entityDomAfterUpdatingEntity, entityId, updateCache);
                state->Redo();
            }

            void UpdateEntitiesAsOverrides(
                const AZStd::vector<const AZ::Entity*>& entityList,
                Instance& owningInstance,
                const Instance& focusedInstance,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabUndoEntityOverrides* state = aznew PrefabUndoEntityOverrides("Undo Updating Entity List As Override");
                state->SetParent(undoBatch);
                state->CaptureAndRedo(entityList, owningInstance, focusedInstance);
            }

            void DeleteEntities(
                const AZStd::vector<AZStd::string>& entityAliasPathList,
                const AZStd::vector<const AZ::Entity*> parentEntityList,
                Instance& focusedInstance,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabUndoDeleteEntity* deleteUndoState = aznew PrefabUndoDeleteEntity("Undo Deleting Entities");
                deleteUndoState->SetParent(undoBatch);
                deleteUndoState->Capture(entityAliasPathList, parentEntityList, focusedInstance);
                deleteUndoState->Redo();
            }

            void DeleteEntitiesAndPrefabsAsOverride(
                const AZStd::vector<AZStd::string>& entityAliasPathList,
                const AZStd::vector<AZStd::string>& instanceAliasPathList,
                const AZStd::vector<const AZ::Entity*> parentEntityList,
                Instance& owningInstance,
                const Instance& focusedInstance,
                UndoSystem::URSequencePoint* undoBatch)
            {
                PrefabUndoDeleteAsOverride* deleteUndoAsOverrideState =
                    aznew PrefabUndoDeleteAsOverride("Undo Deleting Entities and Prefab Instances As Override");
                deleteUndoAsOverrideState->SetParent(undoBatch);
                deleteUndoAsOverrideState->Capture(
                    entityAliasPathList, instanceAliasPathList, parentEntityList, owningInstance, focusedInstance);
                deleteUndoAsOverrideState->Redo();
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
