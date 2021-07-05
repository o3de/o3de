/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"
#include "EntityStateCommand.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzFramework/Slice/SliceEntityBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
#include "PreemptiveUndoCache.h"

#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

// until we have some growing memory array:
#define STATIC_BUFFERSIZE (1024 * 64)

namespace AzToolsFramework
{
    namespace
    {
        void LoadEntity(void* pClassPtr, const AZ::Uuid& uuidValue, const AZ::SerializeContext* sc, AZ::Entity** destPtr)
        {
            // entity is loaded.
            *destPtr = sc->Cast<AZ::Entity*>(pClassPtr, uuidValue);
            AZ_Assert(*destPtr, "Could not cast object to entity!");
        }
    }

    EntityStateCommand::EntityStateCommand(UndoSystem::URCommandID ID, const char* friendlyName)
        : UndoSystem::URSequencePoint(friendlyName ? friendlyName : "Entity Change", ID)
    {
        m_entityID = AZ::EntityId(0);
        m_entityContextId = AzFramework::EntityContextId::CreateNull();
        m_entityState = AZ::Entity::State::Constructed;
        m_isSelected = false;
    }
    EntityStateCommand::~EntityStateCommand()
    {
    }

    void EntityStateCommand::Capture(AZ::Entity* pSourceEntity, bool captureUndo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_entityID = pSourceEntity->GetId();
        EBUS_EVENT_ID_RESULT(m_entityContextId, m_entityID, AzFramework::EntityIdContextQueryBus, GetOwningContextId);
        EBUS_EVENT_RESULT(m_isSelected, AzToolsFramework::ToolsApplicationRequests::Bus, IsSelected, m_entityID);

        AZ::SliceComponent::EntityRestoreInfo& sliceRestoreInfo = captureUndo ? m_undoSliceRestoreInfo : m_redoSliceRestoreInfo;
        sliceRestoreInfo = AZ::SliceComponent::EntityRestoreInfo();

        AZ_Assert(pSourceEntity, "Null entity for undo");
        AZ_Assert(PreemptiveUndoCache::Get(), "You need a pre-emptive undo cache instance to exist for this to work.");
        AZ_Assert((!captureUndo) || (m_undoState.empty()), "You can't capture undo more than once");
        AZ::SerializeContext* sc = nullptr;
        EBUS_EVENT_RESULT(sc, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(sc, "Serialization context not found!");

        m_entityState = pSourceEntity->GetState();

        // The entity is loose, so we capture it directly.
        if (captureUndo)
        {
            m_undoState = PreemptiveUndoCache::Get()->Retrieve(m_entityID);
            if (m_undoState.empty())
            {
                PreemptiveUndoCache::Get()->UpdateCache(m_entityID);
                m_undoState = PreemptiveUndoCache::Get()->Retrieve(m_entityID);
            }
            AZ_Assert(!m_undoState.empty(), "Invalid empty size for the undo state of an entity.");
        }
        else
        {
            m_redoState.clear();
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > ms(&m_redoState);
            AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&ms, *sc, AZ::ObjectStream::ST_BINARY);
            if (!objStream->WriteClass(pSourceEntity))
            {
                AZ_Assert(false, "Unable to serialize entity for undo/redo. ObjectStream::WriteClass() returned an error.");
            }
            if (!objStream->Finalize())
            {
                AZ_Assert(false, "Unable to serialize entity for undo/redo. ObjectStream::Finalize() returned an error.");
            }
        }

        // If slice-owned, extract the data we need to restore it.
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddr;
        AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddr, m_entityID,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
        if (sliceInstanceAddr.IsValid())
        {
            AZ::SliceComponent* rootSlice = nullptr;
            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(rootSlice,
                &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
            AZ_Assert(rootSlice, "Failed to retrieve editor root slice.");
            rootSlice->GetEntityRestoreInfo(m_entityID, sliceRestoreInfo);
        }
    }

    void EntityStateCommand::RestoreEntity(const AZ::u8* buffer, AZStd::size_t bufferSizeBytes, const AZ::SliceComponent::EntityRestoreInfo& sliceRestoreInfo) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ_Assert(buffer, "No data to undo!");
        AZ_Assert(bufferSizeBytes, "Undo data is empty.");

        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "Serialization context not found!");
        AZ::IO::MemoryStream memoryStream(reinterpret_cast<const char*>(buffer), bufferSizeBytes);

        // If restoring to a slice, keep a reference to the slice asset so it isn't released when the entity
        // is deleted, only to immediately reload upon restoring.
        AZ::Data::Asset<AZ::SliceAsset> asset;
        if (sliceRestoreInfo)
        {
            asset = AZ::Data::AssetManager::Instance().FindAsset(sliceRestoreInfo.m_assetId, AZ::Data::AssetLoadBehavior::Default);
        }

        // We have to delete the entity. If it's currently selected, make sure we re-select after re-creating.
        AzToolsFramework::EntityIdList selectedEntities;
        EBUS_EVENT_RESULT(selectedEntities, ToolsApplicationRequests::Bus, GetSelectedEntities);

        AZ::Entity* entity = nullptr;

        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_entityID);
        EntityStateCommandNotificationBus::Event(m_entityID, &EntityStateCommandNotificationBus::Events::PreRestore);

        bool isSliceMetadataEntity = false;
        SliceMetadataEntityContextRequestBus::BroadcastResult(isSliceMetadataEntity, &SliceMetadataEntityContextRequestBus::Events::IsSliceMetadataEntity, m_entityID);
        AZ::ObjectStream::InplaceLoadRootInfoCB inplaceLoadRootInfoCB;
        if (isSliceMetadataEntity)
        {
            // Instead of deleting the entity, dtor it which will remove and disconnect it entirely
            entity->~Entity();
            
            // placement new to default the entity
            entity = new(entity) AZ::Entity();

            // Set up a callback so we in-place load over this entity instead of creating it new
            inplaceLoadRootInfoCB = [entity](void** rootAddress, const AZ::SerializeContext::ClassData** /*classData*/, const AZ::Uuid& /*classId*/, AZ::SerializeContext* /*context*/)
            {
                *rootAddress = entity;
            };
        }
        else
        {
            // Normal entities will be deleted and loaded anew
            //EBUS_EVENT(AZ::ComponentApplicationBus, DeleteEntity, m_entityID);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_entityID);
        }

        bool success = AZ::ObjectStream::LoadBlocking(&memoryStream, *serializeContext,
            AZStd::bind(&LoadEntity, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, &entity),
            AZ::ObjectStream::FilterDescriptor(AZ::Data::AssetFilterNoAssetLoading),
            inplaceLoadRootInfoCB);
        (void)success;
        AZ_Assert(success, "Unable to serialize entity for undo/redo");
        AZ_Assert(entity, "Unable to create entity");

        if (entity)
        {
            if (sliceRestoreInfo)
            {
                AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
                    &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::RestoreSliceEntity,
                    entity, sliceRestoreInfo, AzToolsFramework::SliceEntityRestoreType::Deleted);
            }
            else
            {
                if (!m_entityContextId.IsNull() && !isSliceMetadataEntity)
                {
                    EBUS_EVENT_ID(m_entityContextId, AzFramework::EntityContextRequestBus, AddEntity, entity);
                }

                if (m_entityState == AZ::Entity::State::Init || m_entityState == AZ::Entity::State::Active)
                {
                    if (entity->GetState() == AZ::Entity::State::Constructed)
                    {
                        entity->Init();
                    }
                }
                if (m_entityState == AZ::Entity::State::Active)
                {
                    if (entity->GetState() == AZ::Entity::State::Init)
                    {
                        entity->Activate();
                    }
                }
            }

            PreemptiveUndoCache::Get()->UpdateCache(m_entityID);

            if (m_isSelected)
            {
                if (AZStd::find(selectedEntities.begin(), selectedEntities.end(), m_entityID) == selectedEntities.end())
                {
                    selectedEntities.push_back(m_entityID);
                }
            }

            // Signal that an Entity was created since ComponentApplicationBus::Events::DeleteEntity causes
            // EditorEntityContextNotification::OnEditorEntityDeleted to be fired, we need to have parity
            // However, we don't send the notification here if we are restoring a slice, because the RestoreSliceEntity
            // action gets queued so the Entity won't be in a good state yet until the restoration is complete.
            AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequestBus::Events::GetEditorEntityContextId);
            if (editorEntityContextId == m_entityContextId && !sliceRestoreInfo)
            {
                EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEditorEntityCreated, entity->GetId());
            }

            EntityStateCommandNotificationBus::Event(m_entityID, &EntityStateCommandNotificationBus::Events::PostRestore, entity);
        }

        EBUS_EVENT(ToolsApplicationRequests::Bus, SetSelectedEntities, selectedEntities);
    }

    void EntityStateCommand::Undo()
    {
        RestoreEntity(m_undoState.data(), m_undoState.size(), m_undoSliceRestoreInfo);
    }

    void EntityStateCommand::Redo()
    {
        RestoreEntity(m_redoState.data(), m_redoState.size(), m_redoSliceRestoreInfo);
    }

    EntityDeleteCommand::EntityDeleteCommand(UndoSystem::URCommandID ID)
        : EntityStateCommand(ID, "Delete Entity")
    {
    }

    void EntityDeleteCommand::Capture(AZ::Entity* pSourceEntity)
    {
        PreemptiveUndoCache::Get()->UpdateCache(pSourceEntity->GetId());
        EntityStateCommand::Capture(pSourceEntity, true);
    }

    void EntityDeleteCommand::Undo()
    {
        RestoreEntity(m_undoState.data(), m_undoState.size(), m_undoSliceRestoreInfo);
    }

    void EntityDeleteCommand::Redo()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        EBUS_EVENT(AZ::ComponentApplicationBus, DeleteEntity, m_entityID);
        PreemptiveUndoCache::Get()->PurgeCache(m_entityID);
    }

    EntityCreateCommand::EntityCreateCommand(UndoSystem::URCommandID ID)
        : EntityStateCommand(ID, "Create Entity")
    {
    }

    void EntityCreateCommand::Capture(AZ::Entity* pSourceEntity)
    {
        EntityStateCommand::Capture(pSourceEntity, false);
        m_isSelected = true;
    }

    void EntityCreateCommand::Undo()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        EBUS_EVENT(AZ::ComponentApplicationBus, DeleteEntity, m_entityID);
        PreemptiveUndoCache::Get()->PurgeCache(m_entityID);
    }

    void EntityCreateCommand::Redo()
    {
        RestoreEntity(m_redoState.data(), m_redoState.size(), m_redoSliceRestoreInfo);
    }
} // namespace AzToolsFramework
