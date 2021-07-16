/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "BaseSliceCommand.h"
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>

namespace AzToolsFramework
{
    BaseSliceCommand::BaseSliceCommand(const AZStd::string& friendlyName)
        : UndoSystem::URSequencePoint(friendlyName)
        , m_changed(true)
    {
    }

    bool BaseSliceCommand::CaptureRestoreInfoForUndo(const AZ::EntityId& entityId)
    {
        AZ::SliceComponent* editorRootSlice = nullptr;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(editorRootSlice,
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);

        AZ::SliceComponent::EntityRestoreInfo restoreInfo;
        AZ::SliceComponent::SliceInstanceAddress owningSlice = editorRootSlice->FindSlice(entityId);

        // If no owning slice then store with an empty restore info
        // We'll only need to detach the entity on undo
        if (!owningSlice.IsValid())
        {
            m_entityUndoRestoreInfoArray.emplace_back(entityId, restoreInfo);

            return true;
        }

        // Gather restore info so that on undo we can return this entity
        // to its original owning slice instance
        if (editorRootSlice->GetEntityRestoreInfo(entityId, restoreInfo))
        {
            m_entityUndoRestoreInfoArray.emplace_back(entityId, restoreInfo);
        }
        else
        {
            AZ_Error("CreateSliceCommand::Capture",
                false,
                "Failed to capture RestoreInfo for entity with Id: %s owned by sliceInstance with asset Id: %s. Unable to capture undo/redo state",
                entityId.ToString().c_str(),
                owningSlice.GetReference()->GetSliceAsset().GetId().ToString<AZStd::string>().c_str());

            return false;
        }

        return true;
    }

    void BaseSliceCommand::RestoreEntities(AZ::SliceComponent::EntityRestoreInfoList& entitiesToRestore,
        bool clearRestoreList /*=false*/,
        SliceEntityRestoreType restoreType /*=SliceEntityRestoreType::Detached*/)
    {
        AZ::SliceComponent* editorRootSlice = nullptr;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(editorRootSlice,
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);

        if (!editorRootSlice)
        {
            AZ_Assert(false,
                "BaseSliceCommand::RestoreEntities: GetEditorRootSlice returned nullptr. Unable to proceed.");

            return;
        }

        AzToolsFramework::EntityIdList detachedEntities;
        for (const auto& restoreInfoPair : entitiesToRestore)
        {
            const AZ::EntityId& entityId = restoreInfoPair.first;
            const AZ::SliceComponent::EntityRestoreInfo& restoreInfo = restoreInfoPair.second;

            AZ::Entity* entity = editorRootSlice->FindEntity(entityId);

            if (!entity)
            {
                AZ_Error("BaseSliceCommand::RestoreEntities",
                    false,
                    "Unable to find entity with Id: %s in the EditorEntityContext. Cannot complete operation and restore entity",
                    entityId.ToString().c_str());

                return;
            }

            // Detach the entity from whatever slice instance owned it before and place it directly in the root slice
            editorRootSlice->RemoveEntity(entity, false);
            editorRootSlice->AddEntity(entity);

            if (restoreInfo)
            {
                SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
                    &SliceEditorEntityOwnershipServiceRequestBus::Events::RestoreSliceEntity, entity, restoreInfo, restoreType);
            }
            else
            {
                // An invalid restoreInfo implies no slice to restore to
                // Detaching the entity is all that's needed
                detachedEntities.emplace_back(entity->GetId());
            }
        }

        // RestoreSliceEntity will update slice info for the entities passed in. Only need to handle entities that weren't restored that way.
        SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
            &SliceEditorEntityOwnershipServiceNotificationBus::Events::OnEditorEntitiesSliceOwnershipChanged, detachedEntities);

        if (clearRestoreList)
        {
            entitiesToRestore.clear();
        }
    }
}
