/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptSystemBus.h>

#include <AzFramework/Entity/GameEntityContextBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Commands/DetachSubSliceInstanceCommand.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/Commands/SliceDetachEntityCommand.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipService.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzToolsFramework/Slice/SliceDataFlagsCommand.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace AzToolsFramework
{
    namespace Internal
    {
        struct SliceEditorEntityOwnershipServiceNotificationBusHandler final
            : public SliceEditorEntityOwnershipServiceNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
            AZ_EBUS_BEHAVIOR_BINDER(SliceEditorEntityOwnershipServiceNotificationBusHandler, "{159C07A6-BCB6-432E-BEBB-6AABF6C76989}", AZ::SystemAllocator,
                OnSliceInstantiated, OnSliceInstantiationFailed);

            void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& ticket) override
            {
                Call(FN_OnSliceInstantiated, sliceAssetId, sliceAddress, ticket);
            }

            void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& ticket) override
            {
                Call(FN_OnSliceInstantiationFailed, sliceAssetId, ticket);
            }
        };
    }

    SliceEditorEntityOwnershipService::SliceEditorEntityOwnershipService(const AzFramework::EntityContextId& entityContextId,
        AZ::SerializeContext* serializeContext)
        : SliceEntityOwnershipService(entityContextId, serializeContext)
    {
        SliceEditorEntityOwnershipServiceRequestBus::Handler::BusConnect();
    }

    SliceEditorEntityOwnershipService::~SliceEditorEntityOwnershipService()
    {
        SliceEditorEntityOwnershipServiceRequestBus::Handler::BusDisconnect();
    }

    void SliceEditorEntityOwnershipService::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SliceEditorEntityOwnershipServiceNotificationBus>("SliceEditorEntityOwnershipServiceNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor")
                ->Handler<Internal::SliceEditorEntityOwnershipServiceNotificationBusHandler>()
                ->Event("OnSliceInstantiated", &SliceEditorEntityOwnershipServiceNotifications::OnSliceInstantiated)
                ->Event("OnSliceInstantiationFailed", &SliceEditorEntityOwnershipServiceNotifications::OnSliceInstantiationFailed)
                ;
        }
    }

    AzFramework::SliceInstantiationTicket SliceEditorEntityOwnershipService::InstantiateEditorSlice(
        const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, const AZ::Transform& worldTransform)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (sliceAsset.GetId().IsValid())
        {
            m_instantiatingSlices.push_back(AZStd::make_pair(sliceAsset, worldTransform));

            const AzFramework::SliceInstantiationTicket ticket = InstantiateSlice(sliceAsset);
            if (ticket.IsValid())
            {
                AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);
            }

            return ticket;
        }

        return AzFramework::SliceInstantiationTicket();
    }

    void SliceEditorEntityOwnershipService::OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        // Start an undo that will wrap the entire slice instantiation event (unable to do this at a higher level since this is queued up by AzFramework and there's no undo concept at that level)
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "Slice Instantiation");

        // Find the next ticket corresponding to this asset.
        // Given the desired world root, position entities in the instance.
        for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
        {
            if (instantiatingIter->first.GetId() == sliceAssetId)
            {
                const AZ::Transform& worldTransform = instantiatingIter->second;

                const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

                for (AZ::Entity* entity : entities)
                {
                    auto* transformComponent = entity->FindComponent<Components::TransformComponent>();
                    if (transformComponent)
                    {
                        // Non-root entities will be positioned relative to their parents.
                        if (!transformComponent->GetParentId().IsValid())
                        {
                            // Note: Root slice entity always has translation at origin, so this maintains scale & rotation.
                            transformComponent->SetWorldTM(worldTransform * transformComponent->GetWorldTM());
                        }
                    }
                }

                break;
            }
        }
    }

    void SliceEditorEntityOwnershipService::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        // Make a copy of sliceAddress because a handler of EditorEntityContextNotification::OnSliceInstantiated (CCryEditDoc::OnSliceInstantiated)
        // can end up destroying the slice instance which invalidates the values stored in sliceAddress for the following handlers. 
        // If CCryEditDoc::OnSliceInstantiated destroys the slice instance, it will set SliceInstanceAddress::m_instance and SliceInstanceAddress::m_reference 
        // to nullptr making the sliceAddressCopy invalid for the following handlers.
        AZ::SliceComponent::SliceInstanceAddress sliceAddressCopy = sliceAddress;

        // Close out the next ticket corresponding to this asset.
        for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::OnSliceInstantiated:CloseTicket");
            if (instantiatingIter->first.GetId() == sliceAssetId)
            {
                const AZ::SliceComponent::EntityList& entities = sliceAddressCopy.GetInstance()->GetInstantiated()->m_entities;

                // Select slice roots when found, otherwise default to selecting all entities in slice
                EntityIdSet setOfEntityIds;
                for (const AZ::Entity* const entity : entities)
                {
                    setOfEntityIds.insert(entity->GetId());
                }

                UpdateSelectedEntitiesInHierarchy(setOfEntityIds);

                // Create a slice instantiation undo command.
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::OnSliceInstantiated:CloseTicket:CreateInstantiateUndo");
                    ScopedUndoBatch undoBatch("Instantiate Slice");
                    for (AZ::Entity* entity : entities)
                    {

                        EntityCreateCommand* command = aznew EntityCreateCommand(
                            reinterpret_cast<UndoSystem::URCommandID>(sliceAddressCopy.GetInstance()));
                        command->Capture(entity);
                        command->SetParent(undoBatch.GetUndoBatch());
                    }
                }

                SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
                    &SliceEditorEntityOwnershipServiceNotifications::OnSliceInstantiated, sliceAssetId, sliceAddressCopy, ticket);

                m_instantiatingSlices.erase(instantiatingIter);

                break;
            }
        }

        // End the slice instantiation undo batch started in OnSlicePreInstantiate
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::EndUndoBatch);
    }

    void SliceEditorEntityOwnershipService::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
        {
            if (instantiatingIter->first.GetId() == sliceAssetId)
            {
                SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
                    &SliceEditorEntityOwnershipServiceNotifications::OnSliceInstantiationFailed, sliceAssetId, ticket);

                m_instantiatingSlices.erase(instantiatingIter);
                break;
            }
        }
    }

    AZ::SliceComponent::SliceInstanceAddress SliceEditorEntityOwnershipService::CloneEditorSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress sourceInstance, AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (sourceInstance.IsValid())
        {
            AZ::SliceComponent::SliceInstanceAddress address = CloneSliceInstance(sourceInstance, sourceToCloneEntityIdMap);
            return address;
        }
        else
        {
            AZ_Error("EditorEntityContext", false, "Invalid slice source instance. Unable to clone.");
        }

        return AZ::SliceComponent::SliceInstanceAddress();
    }

    AZ::SliceComponent::SliceInstanceAddress SliceEditorEntityOwnershipService::CloneSubSliceInstance(
        const AZ::SliceComponent::SliceInstanceAddress& sourceSliceInstanceAddress,
        const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
        const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
        AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap)
    {
        AZ::SliceComponent::SliceInstanceAddress subsliceInstanceCloneAddress =
            GetRootSlice()->CloneAndAddSubSliceInstance(sourceSliceInstanceAddress.GetInstance(),
            sourceSubSliceInstanceAncestry, sourceSubSliceInstanceAddress, out_sourceToCloneEntityIdMap);

        if (!subsliceInstanceCloneAddress.IsValid())
        {
            AZ_Assert(false, "Clone sub-slice instance failed.");
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        return subsliceInstanceCloneAddress;
    }

    AZ::SliceComponent::SliceInstanceAddress SliceEditorEntityOwnershipService::PromoteEditorEntitiesIntoSlice(const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, const AZ::SliceComponent::EntityIdToEntityIdMap& liveToAssetMap)
    {
        AZ::SliceComponent* editorRootSlice = GetRootSlice();

        if (!sliceAsset.GetId().IsValid())
        {
            AZ_Error("SliceEditorEntityOwnershipService::PromoteEditorEntitiesIntoSlice", false, "SliceAsset with Invalid Asset ID passed in. Unable to promote entities into slice");
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        if (!editorRootSlice)
        {
            AZ_Assert(editorRootSlice, "SliceEditorEntityOwnershipService::PromoteEditorEntitiesIntoSlice: Editor root slice not found! Unable to promote entities into slice");
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        AzFramework::SliceInstantiationTicket ticket = GenerateSliceInstantiationTicket();

        AzFramework::SliceInstantiationResultBus::Event(ticket, &AzFramework::SliceInstantiationResultBus::Events::OnSlicePreInstantiate,
            sliceAsset.GetId(), AZ::SliceComponent::SliceInstanceAddress());

        AZ::SliceComponent::SliceInstanceAddress instanceAddress = editorRootSlice->
            AddSliceUsingExistingEntities(sliceAsset, liveToAssetMap);

        if (!instanceAddress.IsValid())
        {
            AzFramework::SliceInstantiationResultBus::Event(ticket,
                &AzFramework::SliceInstantiationResultBus::Events::OnSliceInstantiationFailed, sliceAsset.GetId());
            AzFramework::SliceInstantiationResultBus::Event(ticket,
                &AzFramework::SliceInstantiationResultBus::Events::OnSliceInstantiationFailedOrCanceled, sliceAsset.GetId(), false);
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        AzFramework::SliceInstantiationResultBus::Event(ticket, &AzFramework::SliceInstantiationResultBus::Events::OnSliceInstantiated,
            sliceAsset.GetId(), instanceAddress);

        SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
            &SliceEditorEntityOwnershipServiceNotificationBus::Events::OnSliceInstantiated,
            sliceAsset.GetId(), instanceAddress, ticket);

        const EntityList& instanceEntities = instanceAddress.GetInstance()->GetInstantiated()->m_entities;

        EntityIdSet instantiatedIdSet;
        AzToolsFramework::EntityIdList instantiatedIds;
        for (const AZ::Entity* instanceEntity : instanceEntities)
        {
            instantiatedIdSet.emplace(instanceEntity->GetId());
            instantiatedIds.emplace_back(instanceEntity->GetId());
        }

        UpdateSelectedEntitiesInHierarchy(instantiatedIdSet);

        SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
            &SliceEditorEntityOwnershipServiceNotificationBus::Events::OnEditorEntitiesPromotedToSlicedEntities, instantiatedIds);

        return instanceAddress;
    }

    void SliceEditorEntityOwnershipService::UpdateSelectedEntitiesInHierarchy(const EntityIdSet& entityIdSet)
    {
        // Use a lambda to call multiple requests via one Broadcast
        ToolsApplicationRequestBus::Broadcast([&entityIdSet](ToolsApplicationRequests* toolsApplicationRequest)
        {
            EntityIdList selectedEntities;
            AZ::EntityId commonRoot;
            if (toolsApplicationRequest->FindCommonRoot(entityIdSet, commonRoot, &selectedEntities) || selectedEntities.empty())
            {
                selectedEntities.insert(selectedEntities.end(), entityIdSet.begin(), entityIdSet.end());
            }

            toolsApplicationRequest->SetSelectedEntities(selectedEntities);
        });
    }

    void SliceEditorEntityOwnershipService::DetachSliceEntities(const AzToolsFramework::EntityIdList& entities)
    {
        const char* undoMsg = entities.size() == 1 ? "Detach Entity from Slice" : "Detach Entities from Slice";
        DetachFromSlice(entities, undoMsg);
    }

    void SliceEditorEntityOwnershipService::DetachSliceInstances(const AZ::SliceComponent::SliceInstanceAddressSet& instances)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const char* undoMsg = instances.size() == 1 ? "Detach Instance from Slice" : "Detach Instances from Slice";

        // Get all entities in the input instance(s)
        AzToolsFramework::EntityIdList entities;
        for (const AZ::SliceComponent::SliceInstanceAddress& sliceInstanceAddress : instances)

        {
            if (!sliceInstanceAddress.IsValid())
            {
                continue;
            }

            const AZ::SliceComponent::InstantiatedContainer* instantiated = sliceInstanceAddress.GetInstance()->GetInstantiated();
            if (instantiated)
            {
                for (const AZ::Entity* entityInSlice : instantiated->m_entities)
                {
                    entities.push_back(entityInSlice->GetId());
                }
            }
        }

        DetachFromSlice(entities, undoMsg);
    }

    void SliceEditorEntityOwnershipService::DetachSubsliceInstances(const AZ::SliceComponent::SliceInstanceEntityIdRemapList& subsliceRootList)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (subsliceRootList.empty())
        {
            return;
        }

        const char* undoMsg = subsliceRootList.size() == 1 ? "Detach Subslice Instance from owning Slice Instance" : "Detach Subslice Instances from owning Slice Instance";

        ScopedUndoBatch undoBatch(undoMsg);

        AzToolsFramework::DetachSubsliceInstanceCommand* detachCommand = aznew AzToolsFramework::DetachSubsliceInstanceCommand(subsliceRootList, undoMsg);
        detachCommand->SetParent(undoBatch.GetUndoBatch());

        // Running Redo on the DetachSubsliceInstanceCommand will perform the Detach command
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::RunRedoSeparately, undoBatch.GetUndoBatch());
    }

    void SliceEditorEntityOwnershipService::DetachFromSlice(const AzToolsFramework::EntityIdList& entities, const char* undoMessage)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (entities.empty())
        {
            return;
        }

        ScopedUndoBatch undoBatch(undoMessage);

        AzToolsFramework::SliceDetachEntityCommand* detachCommand = aznew AzToolsFramework::SliceDetachEntityCommand(entities, undoMessage);
        detachCommand->SetParent(undoBatch.GetUndoBatch());

        // Running Redo on the SliceDetachEntityCommand will perform the Detach command
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::RunRedoSeparately, undoBatch.GetUndoBatch());
    }

    void SliceEditorEntityOwnershipService::RestoreSliceEntity(AZ::Entity* entity,
        const AZ::SliceComponent::EntityRestoreInfo& info, SliceEntityRestoreType restoreType)
    {
        AZ_Error("EditorEntityContext", info.m_assetId.IsValid(), "Invalid asset Id for entity restore.");

        // If asset isn't loaded when this request is made, we need to queue the load and process the request
        // when the asset is ready. Otherwise we'll immediately process the request when OnAssetReady is invoked
        // by the AssetBus connection policy.

        // Hold a reference to the slice asset so the underlying AssetData won't be ref-counted away when re-adding slice instance.
        AZ::Data::Asset<AZ::Data::AssetData> asset =
            AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(info.m_assetId, AZ::Data::AssetLoadBehavior::Default);

        SliceEntityRestoreRequest request = { entity, info, asset, restoreType };

        // Remove all previous queued states of the same entity, only restore to the most recent state.
        AZ::EntityId entityId = entity->GetId();
        m_queuedSliceEntityRestores.erase(
            AZStd::remove_if(m_queuedSliceEntityRestores.begin(), m_queuedSliceEntityRestores.end(),
                [entityId](const SliceEntityRestoreRequest& request) { return request.m_entity->GetId() == entityId; }),
            m_queuedSliceEntityRestores.end());

        m_queuedSliceEntityRestores.emplace_back(request);

        AZ::Data::AssetBus::MultiHandler::BusConnect(asset.GetId());
    }

    void SliceEditorEntityOwnershipService::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

        AzFramework::EntityList deletedEntitiesRestored;
        AzFramework::EntityIdList detachedEntitiesRestored;

        AZ::SliceComponent* rootSlice = GetRootSlice();
        AZ_Assert(rootSlice, "Failed to retrieve editor root slice.");

        for (auto iter = m_queuedSliceEntityRestores.begin(); iter != m_queuedSliceEntityRestores.end(); )
        {
            SliceEntityRestoreRequest& request = *iter;
            if (asset.GetId() == request.m_asset.GetId())
            {
                AZ::SliceComponent::SliceInstanceAddress address =
                    rootSlice->RestoreEntity(request.m_entity, request.m_restoreInfo,
                        request.m_restoreType == SliceEntityRestoreType::Added);

                if (address.IsValid())
                {
                    switch (request.m_restoreType)
                    {
                    case SliceEntityRestoreType::Deleted:
                    {
                        deletedEntitiesRestored.push_back(request.m_entity);
                    } break;

                    case SliceEntityRestoreType::Added:
                    {
                        // Fall through and perform same operations as Detached
                    }
                    case SliceEntityRestoreType::Detached:
                    {
                        detachedEntitiesRestored.push_back(request.m_entity->GetId());
                        rootSlice->RemoveLooseEntity(request.m_entity->GetId());
                    } break;

                    default:
                    {
                        AZ_Error("EditorEntityContext", false, "Invalid slice entity restore type (%d). Entity: \"%s\" [%llu]",
                            request.m_restoreType, request.m_entity->GetName().c_str(), request.m_entity->GetId());
                    } break;
                    }
                }
                else
                {
                    AZ_Error("EditorEntityContext", false, "Failed to restore entity \"%s\" [%llu]",
                        request.m_entity->GetName().c_str(), request.m_entity->GetId());

                    if (request.m_restoreType == SliceEntityRestoreType::Deleted)
                    {
                        delete request.m_entity;
                    }
                }

                iter = m_queuedSliceEntityRestores.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        HandleEntitiesAdded(deletedEntitiesRestored);

        // Broadcast the created entity notification now after the restore is complete.
        for (auto deletedEntity : deletedEntitiesRestored)
        {
            EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEditorEntityCreated, deletedEntity->GetId());
        }

        if (!detachedEntitiesRestored.empty())
        {
            SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
                &SliceEditorEntityOwnershipServiceNotifications::OnEditorEntitiesSliceOwnershipChanged, detachedEntitiesRestored);
        }

        // Pass on to base EntityOwnershipService.
        SliceEntityOwnershipService::OnAssetReady(asset);
    }

    //=========================================================================
    // OnAssetReloaded - Root slice (or its dependents) has been reloaded.
    //=========================================================================
    void SliceEditorEntityOwnershipService::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        EntityIdList selectedEntities;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

        SliceEntityOwnershipService::OnAssetReloaded(asset);

        // Ensure selection set is preserved after applying the new level slice.
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selectedEntities);
    }

    void SliceEditorEntityOwnershipService::ResetEntitiesToSliceDefaults(EntityIdList entities)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        ScopedUndoBatch undoBatch("Resetting entities to slice defaults.");

        PreemptiveUndoCache* preemptiveUndoCache = nullptr;
        ToolsApplicationRequests::Bus::BroadcastResult(preemptiveUndoCache, &ToolsApplicationRequests::GetUndoCache);

        EntityIdList selectedEntities;
        ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

        SelectionCommand* selCommand = aznew SelectionCommand(
            selectedEntities, "Reset Entity to Slice Defaults");
        selCommand->SetParent(undoBatch.GetUndoBatch());

        SelectionCommand* newSelCommand = aznew SelectionCommand(
            selectedEntities, "Reset Entity to Slice Defaults");

        for (AZ::EntityId id : entities)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceAddress = GetOwningSlice(id);

            AZ::SliceComponent::SliceReference* sliceReference = sliceAddress.GetReference();
            AZ::SliceComponent::SliceInstance* sliceInstance = sliceAddress.GetInstance();

            AZ::EntityId sliceRootEntityId;
            ToolsApplicationRequestBus::BroadcastResult(sliceRootEntityId,
                &ToolsApplicationRequestBus::Events::GetRootEntityIdOfSliceInstance, sliceAddress);

            bool isSliceRootEntity = (id == sliceRootEntityId);

            if (sliceReference)
            {
                // Clear any data flags for entity
                auto clearDataFlagsCommand = aznew ClearSliceDataFlagsBelowAddressCommand(id, AZ::DataPatch::AddressType(), "Clear data flags");
                clearDataFlagsCommand->SetParent(undoBatch.GetUndoBatch());

                AZ::Entity* oldEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(oldEntity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
                AZ_Assert(oldEntity, "Couldn't find the entity we were looking for!");
                if (!oldEntity)
                {
                    continue;
                }

                // Clone the entity from the slice source (clean)
                auto sourceEntityIterator = sliceInstance->GetEntityIdToBaseMap().find(id);
                AZ_Assert(sourceEntityIterator != sliceInstance->GetEntityIdToBaseMap().end(), "Attempting to clone an invalid instance entity id for this slice instance!");
                if (sourceEntityIterator != sliceInstance->GetEntityIdToBaseMap().end())
                {
                    AZ::SliceComponent* dependentSlice = sliceReference->GetSliceAsset().Get()->GetComponent();
                    AZ::Entity* sourceEntity = dependentSlice->FindEntity(sourceEntityIterator->second);

                    AZ_Assert(sourceEntity, "Couldn't find source entity from sourceEntityId in slice reference!");
                    if (sourceEntity)
                    {
                        AZ::Entity* entityClone = dependentSlice->GetSerializeContext()->CloneObject(sourceEntity);
                        if (entityClone)
                        {
                            AZ::IdUtils::Remapper<AZ::EntityId>::ReplaceIdsAndIdRefs(
                                entityClone,
                                [sliceInstance](const AZ::EntityId& originalId, bool /*isEntityId*/, const AZStd::function<AZ::EntityId()>& /*idGenerator*/) -> AZ::EntityId
                            {
                                auto findIt = sliceInstance->GetEntityIdMap().find(originalId);
                                if (findIt == sliceInstance->GetEntityIdMap().end())
                                {
                                    return originalId; // entityId is not being remapped
                                }
                                else
                                {
                                    return findIt->second; // return the remapped id
                                }
                            }, dependentSlice->GetSerializeContext());

                            // Only restore world position and rotation for the slice root entity.
                            if (isSliceRootEntity)
                            {
                                // Get the transform component on the cloned entity.  We cannot use the bus since it isn't activated.
                                Components::TransformComponent* transformComponent = entityClone->FindComponent<Components::TransformComponent>();
                                AZ_Assert(transformComponent, "Entity doesn't have a transform component!");
                                if (transformComponent)
                                {
                                    AZ::Vector3 oldEntityWorldPosition;
                                    AZ::TransformBus::EventResult(oldEntityWorldPosition, id, &AZ::TransformBus::Events::GetWorldTranslation);
                                    transformComponent->SetWorldTranslation(oldEntityWorldPosition);

                                    AZ::Quaternion oldEntityRotation;
                                    AZ::TransformBus::EventResult(oldEntityRotation, id, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
                                    transformComponent->SetWorldRotationQuaternion(oldEntityRotation);

                                    // Ensure the existing hierarchy is maintained
                                    AZ::EntityId oldParentEntityId;
                                    AZ::TransformBus::EventResult(oldParentEntityId, id, &AZ::TransformBus::Events::GetParentId);
                                    transformComponent->SetParent(oldParentEntityId);
                                }
                            }

                            // Create a state command and capture both the undo and redo data
                            EntityStateCommand* stateCommand = aznew EntityStateCommand(
                                static_cast<UndoSystem::URCommandID>(id));
                            stateCommand->Capture(oldEntity, true);
                            stateCommand->Capture(entityClone, false);
                            stateCommand->SetParent(undoBatch.GetUndoBatch());

                            // Delete our temporary entity clone
                            delete entityClone;
                        }
                    }
                }
            }
        }

        newSelCommand->SetParent(undoBatch.GetUndoBatch());

        // Run the redo in order to do the initial swap of entity data
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::RunRedoSeparately, undoBatch.GetUndoBatch());

        // Make sure to set selection to newly cloned entities
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::SetSelectedEntities, selectedEntities);
    }

    bool SliceEditorEntityOwnershipService::SaveToStreamForEditor(AZ::IO::GenericStream& stream, const EntityList& entitiesInLayers,
        AZ::SliceComponent::SliceReferenceToInstancePtrs& instancesInLayers)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ_Assert(stream.IsOpen(), "Invalid target stream.");
        AzFramework::RootSliceAsset rootSliceAsset = GetRootAsset();
        AZ_Assert(rootSliceAsset && rootSliceAsset->GetComponent(), "The entity ownership service is not initialized.");

        // Grab the entire entity list to make sure saving layers doesn't cause entity order to shuffle.
        EntityList entities = rootSliceAsset->GetComponent()->GetNewEntities();

        if (!instancesInLayers.empty())
        {
            rootSliceAsset->GetComponent()->RemoveAndCacheInstances(instancesInLayers);
        }

        if (!entitiesInLayers.empty())
        {
            // These entities were already saved out to individual layer files, so do not save them to the level file.
            rootSliceAsset->GetComponent()->EraseEntities(entitiesInLayers);
        }

        bool saveResult = AZ::Utils::SaveObjectToStream<AZ::Entity>(stream,
            SliceUtilities::GetSliceStreamFormat(), GetRootAsset()->GetEntity());

        // Restore the level slice's entities.
        if (!entities.empty())
        {
            rootSliceAsset->GetComponent()->ReplaceEntities(entities);
        }

        if (!instancesInLayers.empty())
        {
            rootSliceAsset->GetComponent()->RestoreCachedInstances();
        }

        return saveResult;
    }

    bool SliceEditorEntityOwnershipService::SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::SliceComponent::EntityList sourceEntities;
        GetRootSlice()->GetEntities(sourceEntities);

        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> tempEntities;
        SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
            &SliceEditorEntityOwnershipServiceNotifications::OnSaveStreamForGameBegin, stream, streamType, tempEntities);

        sourceEntities.insert(sourceEntities.end(), tempEntities.begin(), tempEntities.end());
        // Add the root slice metadata entity so that we export any level components
        sourceEntities.push_back(GetRootSlice()->GetMetadataEntity());

        // Create a source slice from our editor components.
        AZ::Entity* sourceSliceEntity = aznew AZ::Entity();
        AZ::SliceComponent* sourceSliceData = sourceSliceEntity->CreateComponent<AZ::SliceComponent>();
        AZ::Data::Asset<AZ::SliceAsset> sourceSliceAsset(aznew AZ::SliceAsset(), AZ::Data::AssetLoadBehavior::Default);
        sourceSliceAsset.Get()->SetData(sourceSliceEntity, sourceSliceData);

        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            // This is a sanity check that the editor source entity was created and validated before starting export to game target entity
            // Note - this assert should *not* live directly in PreExportEntity implementation, because of the case where we are modifying
            // system entities we do not want system components active, ticking tick buses, running simulations, etc.
            AZ_Assert(sourceEntity->GetState() == AZ::Entity::State::Active, "Source entity must be active.");

            sourceSliceData->AddEntity(sourceEntity);
        }

        // Emulate client flags.
        AZ::PlatformTagSet platformTags = { AZ_CRC("renderer", 0xf199a19c) };

        // Compile the source slice into the runtime slice (with runtime components).
        WorldEditorOnlyEntityHandler worldEditorOnlyEntityHandler;
        EditorOnlyEntityHandlers handlers =
        {
            &worldEditorOnlyEntityHandler
        };
        AZ::SerializeContext* serializeContext = GetSerializeContext();
        SliceCompilationResult sliceCompilationResult = CompileEditorSlice(sourceSliceAsset, platformTags, *serializeContext, handlers);

        // This is a sanity check that the editor source entity was created and validated and didn't suddenly
        // get out of an active state during export.
        // Note - this assert should *not* live directly in PostExportEntity implementation, because of the case where we are
        // modifying system entities we do not want system components active, ticking tick buses, running simulations, etc.
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            AZ_UNUSED(sourceEntity);
            AZ_Assert(sourceEntity->GetState() == AZ::Entity::State::Active, "Source entity must be active.");
        }

        // Reclaim entities from the temporary source asset. We now have ownership of the entities.
        sourceSliceData->RemoveAllEntities(false);


        if (!sliceCompilationResult)
        {
            AZ_Error("Save Runtime Stream", false, "Failed to export entities for runtime:\n%s",
                sliceCompilationResult.GetError().c_str());
            SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
                &SliceEditorEntityOwnershipServiceNotifications::OnSaveStreamForGameFailure, sliceCompilationResult.GetError());
            return false;
        }

        // Export runtime slice representing the level, which is a completely flat list of entities.
        AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset = sliceCompilationResult.GetValue();
        bool saveResult = AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, exportSliceAsset.Get()->GetEntity());
        if (saveResult)
        {
            SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
                &SliceEditorEntityOwnershipServiceNotifications::OnSaveStreamForGameSuccess, stream);
        }
        else
        {
            SliceEditorEntityOwnershipServiceNotificationBus::Broadcast(
                &SliceEditorEntityOwnershipServiceNotifications::OnSaveStreamForGameFailure,
                AZStd::string::format("Unable to save slice asset %s to stream",
                exportSliceAsset.GetId().ToString<AZStd::string>().data()));
        }

        return saveResult;
    }

    void SliceEditorEntityOwnershipService::StartPlayInEditor(EntityIdToEntityIdMap& m_editorToRuntimeIdMap,
        EntityIdToEntityIdMap& m_runtimeToEditorIdMap)
    {
        // Save the editor context to a memory stream.
        AZStd::vector<char> entityBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > stream(&entityBuffer);
        const bool isRuntimeStreamValid = SaveToStreamForGame(stream, AZ::DataStream::ST_BINARY);

        if (!isRuntimeStreamValid)
        {
            AZ_Error("EditorEntityContext", false,
                "Failed to create runtime entity context for play-in-editor mode. Entities will not be created.");
        }

        // Deactivate the editor context.
        AZ::SliceComponent* rootSlice = GetRootSlice();
        if (rootSlice)
        {
            AZ::SliceComponent::EntityList entities;
            rootSlice->GetEntities(entities);

            AZ::Entity* rootMetaDataEntity = rootSlice->GetMetadataEntity();
            if (rootMetaDataEntity)
            {
                entities.push_back(rootMetaDataEntity);
            }

            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    entity->Deactivate();
                }
            }
        }

        m_runtimeToEditorIdMap.clear();

        if (isRuntimeStreamValid)
        {
            // Load the exported stream into the game context.
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequests::LoadFromStream, stream, true);

            // Retrieve Id map from game entity context (editor->runtime).
            AzFramework::EntityContextId gameContextId = AzFramework::EntityContextId::CreateNull();
            AzFramework::GameEntityContextRequestBus::BroadcastResult(
                gameContextId, &AzFramework::GameEntityContextRequests::GetGameEntityContextId);
            AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(
                m_editorToRuntimeIdMap, gameContextId, &AzFramework::SliceEntityOwnershipServiceRequests::GetLoadedEntityIdMap);

            // Generate reverse lookup (runtime->editor).
            for (const auto& idMapping : m_editorToRuntimeIdMap)
            {
                m_runtimeToEditorIdMap.insert(AZStd::make_pair(idMapping.second, idMapping.first));
            }
        }
    }

    void SliceEditorEntityOwnershipService::StopPlayInEditor(EntityIdToEntityIdMap& m_editorToRuntimeIdMap,
        EntityIdToEntityIdMap& m_runtimeToEditorIdMap)
    {
        m_editorToRuntimeIdMap.clear();
        m_runtimeToEditorIdMap.clear();

        // Reset the runtime context.
        AzFramework::GameEntityContextRequestBus::Broadcast(
            &AzFramework::GameEntityContextRequests::ResetGameContext);

        // Do a full lua GC.
        AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::GarbageCollect);

        // Re-activate the editor context.
        AZ::SliceComponent* rootSlice = GetRootSlice();
        if (rootSlice)
        {
            AZ::SliceComponent::EntityList entities;
            rootSlice->GetEntities(entities);

            AZ::Entity* rootMetaDataEntity = rootSlice->GetMetadataEntity();
            if (rootMetaDataEntity)
            {
                entities.push_back(rootMetaDataEntity);
            }

            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    entity->Init();
                }

                if (entity->GetState() == AZ::Entity::State::Init)
                {
                    entity->Activate();
                }
            }
            // we need to check for locked layers and re-apply the lock to their children.
            // This needs to be in a separate loop as we need to be sure all the entities are activated.
            for (AZ::Entity* entity : entities)
            {
                AZ::EntityId entityId = entity->GetId();
                bool locked = false;
                EditorLockComponentRequestBus::EventResult(locked, entityId, &EditorLockComponentRequests::GetLocked);
                if (locked)
                {
                    bool isLayer = false;
                    Layers::EditorLayerComponentRequestBus::EventResult(
                        isLayer, entityId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                    if (isLayer)
                    {
                        SetEntityLockState(entityId, true);
                    }
                }
            }
        }
    }

    void SliceEditorEntityOwnershipService::HandleNewMetadataEntitiesCreated(AZ::SliceComponent& slice)
    {
        AZ::SliceComponent::EntityList metadataEntityIds;
        slice.GetAllMetadataEntities(metadataEntityIds);

        const auto& slices = slice.GetSlices();

        // Add the metadata entity from the root slice to the appropriate context
        // For now, the instance address of the root slice is <nullptr,nullptr>
        SliceMetadataEntityContextRequestBus::Broadcast(&SliceMetadataEntityContextRequestBus::Events::AddMetadataEntityToContext, AZ::SliceComponent::SliceInstanceAddress(), *slice.GetMetadataEntity());

        // Add the metadata entities from every slice instance streamed in as well. We need to grab them directly
        // from their slices so we have the appropriate instance address for each one.
        for (auto& subSlice : slices)
        {
            for (const auto& instance : subSlice.GetInstances())
            {
                if (auto instantiated = instance.GetInstantiated())
                {
                    for (auto* metadataEntity : instantiated->m_metadataEntities)
                    {
                        SliceMetadataEntityContextRequestBus::Broadcast(&SliceMetadataEntityContextRequestBus::Events::AddMetadataEntityToContext, AZ::SliceComponent::SliceInstanceAddress(const_cast<AZ::SliceComponent::SliceReference*>(&subSlice), const_cast<AZ::SliceComponent::SliceInstance*>(&instance)), *metadataEntity);
                    }
                }
            }
        }
    }

    void SliceEditorEntityOwnershipService::AssociateToRootMetadataEntity(const EntityList& entities)
    {
        for (const auto* entity : entities)
        {
            AZ::SliceComponent* rootSliceComponent = GetRootSlice();
            auto sliceAddress = rootSliceComponent->FindSlice(entity->GetId());
            if (!sliceAddress.IsValid())
            {
                AZ::SliceMetadataInfoManipulationBus::Event(rootSliceComponent->GetMetadataEntity()->GetId(), &AZ::SliceMetadataInfoManipulationBus::Events::AddAssociatedEntity, entity->GetId());
            }
        }
    }

    bool SliceEditorEntityOwnershipService::LoadFromStreamWithLayers(AZ::IO::GenericStream& stream, QString levelPakFile)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::ObjectStream::FilterDescriptor filterDesc = AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterSourceSlicesOnly);

        AZ::Entity* newRootEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(stream, GetSerializeContext(), filterDesc);

        AZ::SliceComponent* newRootSlice = newRootEntity->FindComponent<AZ::SliceComponent>();
        EntityList entities = newRootSlice->GetNewEntities();
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs layerSliceInstances;

        // Collect all of the layers by names, so duplicates can be found.
        // Layer name collision is handled here instead of in the layer component because none of these entities are connected to buses yet.
        using EntityAndLayerComponent = AZStd::pair<AZ::Entity*, Layers::EditorLayerComponent*>;
        AZStd::unordered_map<AZStd::string, AZStd::vector<EntityAndLayerComponent>> layerNameToLayerComponents;

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;
        for (AZ::Entity* entity : entities)
        {
            // The entities just loaded from the layer aren't going to be connected to any buses yet,
            // check for the layer component directly.
            Layers::EditorLayerComponent* layerComponent = entity->FindComponent<Layers::EditorLayerComponent>();

            if (layerComponent)
            {
                layerNameToLayerComponents[layerComponent->GetCachedLayerBaseFileName()].push_back(EntityAndLayerComponent(entity, layerComponent));
            }

            AZStd::unordered_map<AZ::EntityId, AZ::Entity*>::iterator existingEntity = uniqueEntities.find(entity->GetId());
            if (existingEntity != uniqueEntities.end())
            {
                AZ_Warning(
                    "Editor",
                    false,
                    "Multiple entities that exist in your level with ID %s and name %s were found. The last entity found has been loaded.",
                    entity->GetId().ToString().c_str(),
                    entity->GetName().c_str());
                AZ::Entity* duplicateEntity = existingEntity->second;
                existingEntity->second = entity;
                delete duplicateEntity;
            }
            else
            {
                uniqueEntities[entity->GetId()] = entity;
            }
        }

        for (auto& layersIterator : layerNameToLayerComponents)
        {
            if (layersIterator.second.size() > 1)
            {
                AZ_Error("Layers", false, "There is more than one layer with the name %s at the same hierarchy level. Rename these layers and try again.", layersIterator.first.c_str());
            }
            else
            {
                // Only load layers that have no collisions with other layer names.
                Layers::EditorLayerComponent* layerComponent(layersIterator.second[0].second);
                Layers::LayerResult layerReadResult =
                    layerComponent->ReadLayer(levelPakFile, layerSliceInstances, uniqueEntities);

                layerReadResult.MessageResult();
            }
        }

        EntityList uniqueEntityList;
        for (const auto& uniqueEntity : uniqueEntities)
        {
            uniqueEntityList.push_back(uniqueEntity.second);
        }

        // Add the layer entities before anything's been activated.
        // This allows the layer system to function with minimal changes to level loading flow in the editor,
        // no existing systems need to know about the layer system.
        AZStd::unordered_set<const AZ::SliceComponent::SliceInstance*> instances;
        newRootSlice->AddSliceInstances(layerSliceInstances, instances);
        newRootSlice->ReplaceEntities(uniqueEntityList);

        // Clean up layer data after adding the information to the slice, so it isn't lost.
        for (auto& layersIterator : layerNameToLayerComponents)
        {
            if (layersIterator.second.size() == 1)
            {
                // Only load layers that have no collisions with other layer names.
                Layers::EditorLayerComponent* layerComponent(layersIterator.second[0].second);
                layerComponent->CleanupLoadedLayer();
            }
        }

        // make sure that PRE_NOTIFY assets get their notify before we activate, so that we can preserve the order of 
        // (load asset) -> (notify) -> (init) -> (activate)
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // for other kinds of instantiations, like slice instantiations, because they use the queued slice instantiation mechanism, they will always
        // be instantiated after their asset is already ready.

        return HandleRootEntityReloadedFromStream(newRootEntity, false, nullptr);
    }

    AZ::SliceComponent* SliceEditorEntityOwnershipService::GetEditorRootSlice()
    {
        return GetRootSlice();
    }
}
