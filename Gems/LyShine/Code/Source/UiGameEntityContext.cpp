/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiGameEntityContext.h"
#include <AzCore/Serialization/Utils.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/UiComponentTypes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
UiGameEntityContext::UiGameEntityContext(AZ::EntityId canvasEntityId)
    : m_canvasEntityId(canvasEntityId)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiGameEntityContext::~UiGameEntityContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiGameEntityContext::HandleLoadedRootSliceEntity(AZ::Entity* rootEntity, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable)
{
    AZ_Assert(m_entityOwnershipService->IsInitialized(), "The context has not been initialized.");

    bool rootEntityReloadSuccessful = false;
    AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootEntityReloadSuccessful, GetContextId(),
        &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::HandleRootEntityReloadedFromStream, rootEntity, remapIds, idRemapTable);
    if (!rootEntityReloadSuccessful)
    {
        return false;
    }

    AZ::SliceComponent::EntityList entities;
    m_entityOwnershipService->GetAllEntities(entities);

    AzFramework::SliceEntityOwnershipServiceRequestBus::Event(GetContextId(),
        &AzFramework::SliceEntityOwnershipServiceRequests::SetIsDynamic, true);

    InitializeEntities(entities);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiGameEntityContext::CreateUiEntity(const char* name)
{
    AZ::Entity* entity = CreateEntity(name);

    if (entity)
    {
        // we don't currently do anything extra here, UI entities are not automatically
        // Init'ed and Activate'd when they are created. We wait until the required components
        // are added before Init and Activate
    }

    return entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::AddUiEntity(AZ::Entity* entity)
{
    AZ_Assert(entity, "Supplied entity is invalid.");

    AddEntity(entity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::AddUiEntities(const AzFramework::EntityList& entities)
{
    for (AZ::Entity* entity : entities)
    {
        AZ_Assert(!AzFramework::EntityIdContextQueryBus::MultiHandler::BusIsConnectedId(entity->GetId()), "Entity already in context.");
        AzFramework::RootSliceAsset rootSliceAsset;
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSliceAsset, GetContextId(),
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootAsset);
        rootSliceAsset->GetComponent()->AddEntity(entity);
    }

    m_entityOwnershipService->HandleEntitiesAdded(entities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiGameEntityContext::CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityList& resultEntities)
{
    resultEntities.clear();

    AZ::SliceComponent::InstantiatedContainer sourceObjects(false);
    for (const AZ::EntityId& id : sourceEntities)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        if (entity)
        {
            sourceObjects.m_entities.push_back(entity);
        }
    }

    AZ::SliceComponent::EntityIdToEntityIdMap idMap;
    AZ::SliceComponent::InstantiatedContainer* clonedObjects =
        AZ::IdUtils::Remapper<AZ::EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(&sourceObjects, idMap);
    if (!clonedObjects)
    {
        AZ_Error("UiEntityContext", false, "Failed to clone source entities.");
        return false;
    }

    resultEntities = clonedObjects->m_entities;

    AddUiEntities(resultEntities);

    clonedObjects->m_deleteEntitiesOnDestruction = false;
    delete clonedObjects;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiGameEntityContext::DestroyUiEntity(AZ::EntityId entityId)
{
    return EntityContext::DestroyEntityById(entityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiGameEntityContext::DestroyEntity(AZ::Entity* entity)
{
    AZ_Assert(entity, "Invalid entity passed to DestroyEntity");
    AZ_Assert(m_entityOwnershipService->IsInitialized(), "The context has not been initialized.");

    AzFramework::EntityContextId owningContextId = AzFramework::EntityContextId::CreateNull();
    AzFramework::EntityIdContextQueryBus::EventResult(
        owningContextId, entity->GetId(), &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);
    AZ_Assert(owningContextId == m_contextId, "Entity does not belong to this context, and therefore can not be safely destroyed by this context.");

    if (owningContextId == m_contextId)
    {
        HandleEntitiesRemoved({ entity->GetId() });
        AzFramework::RootSliceAsset rootSliceAsset;
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSliceAsset, GetContextId(),
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootAsset);
        rootSliceAsset->GetComponent()->RemoveEntity(entity);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::InitUiContext()
{
    m_entityOwnershipService = AZStd::make_unique<AzFramework::SliceEntityOwnershipService>(GetContextId(), GetSerializeContext());
    InitContext();

    m_entityOwnershipService->InstantiateAllPrefabs();

    UiEntityContextRequestBus::Handler::BusConnect(GetContextId());
    UiGameEntityContextBus::Handler::BusConnect(GetContextId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::DestroyUiContext()
{
    UiEntityContextRequestBus::Handler::BusDisconnect();
    UiGameEntityContextBus::Handler::BusDisconnect();

    DestroyContext();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiGameEntityContext::SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    AzFramework::RootSliceAsset rootSliceAsset;
    AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSliceAsset, GetContextId(),
        &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootAsset);
    if (!rootSliceAsset)
    {
        return false;
    }

    AZ::Entity* rootSliceEntity = rootSliceAsset->GetEntity();
    return AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, rootSliceEntity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiGameEntityContext::SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    if (!canvasEntity)
    {
        return false;
    }

    return AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, canvasEntity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::OnContextEntitiesAdded(const AzFramework::EntityList& entities)
{
    EntityContext::OnContextEntitiesAdded(entities);

    InitializeEntities(entities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::InitializeEntities(const AzFramework::EntityList& entities)
{
    // UI entities are now automatically activated on creation

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::State::Constructed)
        {
            entity->Init();
        }
    }

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::State::Init)
        {
            entity->Activate();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool UiGameEntityContext::ValidateEntitiesAreValidForContext(const AzFramework::EntityList& entities)
{
    // All entities in a slice being instantiated in the UI editor should
    // have the UiElementComponent on them.
    for (AZ::Entity* entity : entities)
    {
        if (!entity->FindComponent(LyShine::UiElementComponentUuid))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiGameEntityContext::InstantiateDynamicSlice(
    const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, const AZ::Vector2& position, bool isViewportPosition,
    AZ::Entity* parent, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper)
{
    if (sliceAsset.GetId().IsValid())
    {
        AzFramework::SliceInstantiationTicket ticket;
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(ticket, GetContextId(),
            &AzFramework::SliceEntityOwnershipServiceRequests::InstantiateSlice, sliceAsset, customIdMapper, nullptr);
        if (ticket.IsValid())
        {
            auto it = m_instantiatingDynamicSlices.emplace(ticket, InstantiatingDynamicSlice(sliceAsset, position, isViewportPosition, parent));
            bool inserted = it.second;
            AZ_Error("UiEntityContext", inserted, "InstantiateDynamicSlice failed because the key already exists.");

            if (inserted)
            {
                AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);
                return ticket;
            }
        }
    }

    return AzFramework::SliceInstantiationTicket();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
{
    const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

    auto instantiatingIter = m_instantiatingDynamicSlices.find(ticket);
    if (instantiatingIter != m_instantiatingDynamicSlices.end())
    {
        const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

        // If the context was loaded from a stream and Ids were remapped, fix up entity Ids in that slice that
        // point to entities in the stream (i.e. level entities).
        AZ::SliceComponent::EntityIdToEntityIdMap loadedEntityIdMap;
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(loadedEntityIdMap, GetContextId(),
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetLoadedEntityIdMap);
        if (!loadedEntityIdMap.empty())
        {
            AZ::SliceComponent::InstantiatedContainer instanceEntities(false);
            instanceEntities.m_entities = entities;
            AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(&instanceEntities,
                [this](const AZ::EntityId& originalId, bool isEntityId, const AZStd::function<AZ::EntityId()>&) -> AZ::EntityId
                {
                    if (!isEntityId)
                    {
                        AZ::SliceComponent::EntityIdToEntityIdMap loadedEntityIdMap;
                        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(loadedEntityIdMap, GetContextId(),
                            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetLoadedEntityIdMap);
                        auto iter = loadedEntityIdMap.find(originalId);
                        if (iter != loadedEntityIdMap.end())
                        {
                            return iter->second;
                        }
                    }
                    return originalId;
                }, m_serializeContext, false);
        }

        UiGameEntityContextSliceInstantiationResultsBus::Event(
            ticket,
            &UiGameEntityContextSliceInstantiationResultsBus::Events::OnEntityContextSlicePreInstantiate,
            sliceAssetId,
            sliceAddress);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance)
{
    const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

    AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

    auto instantiatingIter = m_instantiatingDynamicSlices.find(ticket);
    if (instantiatingIter != m_instantiatingDynamicSlices.end())
    {
        InstantiatingDynamicSlice& instantiating = instantiatingIter->second;

        const AZ::SliceComponent::EntityList& entities = instance.GetInstance()->GetInstantiated()->m_entities;

        // It's possible that this dynamic slice only contains editor-only elements
        if (entities.empty())
        {
            return;
        }

        // Create a set of all the top-level entities.
        AZStd::unordered_set<AZ::Entity*> topLevelEntities;
        for (AZ::Entity* entity : entities)
        {
            topLevelEntities.insert(entity);
        }

        // remove anything from the topLevelEntities set that is referenced as the child of another element in the list
        for (AZ::Entity* entity : entities)
        {
            LyShine::EntityArray children;
            UiElementBus::EventResult(children, entity->GetId(), &UiElementBus::Events::GetChildElements);

            for (auto child : children)
            {
                topLevelEntities.erase(child);
            }
        }

        // This can be null is nothing is selected. That is OK, the usage of it below treats that as meaning
        // add as a child of the root element.
        AZ::Entity* parent = instantiating.m_parent;

        // Now topLevelElements contains all of the top-level elements in the set of newly instantiated entities
        // Copy the topLevelEntities set into a list
        LyShine::EntityArray entitiesToInit;
        for (auto entity : topLevelEntities)
        {
            entitiesToInit.push_back(entity);
        }

        // There must be at least one element
        AZ_Assert(entitiesToInit.size() >= 1, "There must be at least one top-level entity in a UI slice.");

        // Initialize the internal parent pointers and the canvas pointer in the elements
        // We do this before adding the elements, otherwise the GetUniqueChildName code in FixupCreatedEntities will
        // already see the new elements and think the names are not unique
        UiCanvasBus::Event(m_canvasEntityId, &UiCanvasBus::Events::FixupCreatedEntities, entitiesToInit, true, parent);

        // Add all of the top-level entities as children of the parent
        for (auto entity : topLevelEntities)
        {
            UiCanvasBus::Event(m_canvasEntityId, &UiCanvasBus::Events::AddElement, entity, parent, nullptr);
        }

        // Here we adjust the position of the instantiated entities. Depending on how the dynamic slice
        // was spawned we position it at a viewport position or a relative position.
        if (instantiating.m_isViewportPosition)
        {
            const AZ::Vector2& desiredViewportPosition = instantiating.m_position;

            AZ::Entity* rootElement = entitiesToInit[0];

            // Transform pivot position to canvas space
            AZ::Vector2 pivotPos;
            UiTransformBus::EventResult(pivotPos, rootElement->GetId(), &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);

            // Transform destination position to canvas space
            AZ::Matrix4x4 transformFromViewport;
            UiTransformBus::Event(rootElement->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);
            AZ::Vector3 destPos3 = transformFromViewport * AZ::Vector3(desiredViewportPosition.GetX(), desiredViewportPosition.GetY(), 0.0f);
            AZ::Vector2 destPos(destPos3.GetX(), destPos3.GetY());

            AZ::Vector2 offsetDelta = destPos - pivotPos;

            // Adjust offsets on all top level elements
            for (auto entity : entitiesToInit)
            {
                UiTransform2dInterface::Offsets offsets;
                UiTransform2dBus::EventResult(offsets, entity->GetId(), &UiTransform2dBus::Events::GetOffsets);
                UiTransform2dBus::Event(entity->GetId(), &UiTransform2dBus::Events::SetOffsets, offsets + offsetDelta);
            }
        }
        else if (!instantiating.m_position.IsZero())
        {
            AZ::Entity* rootElement = entitiesToInit[0];
            UiTransformBus::Event(rootElement->GetId(), &UiTransformBus::Events::MoveLocalPositionBy, instantiating.m_position);
        }

        // must erase this in case our instantiate calls trigger a slice spawn which would invalid this iterator.
        m_instantiatingDynamicSlices.erase(instantiatingIter);

        // This allows the UiSpawnerComponent to respond after the entities have been activated and fixed up
        UiGameEntityContextSliceInstantiationResultsBus::Event(
            ticket, &UiGameEntityContextSliceInstantiationResultsBus::Events::OnEntityContextSliceInstantiated, sliceAssetId, instance);

        UiGameEntityContextNotificationBus::Broadcast(
            &UiGameEntityContextNotificationBus::Events::OnSliceInstantiated, sliceAssetId, instance, ticket);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
{
    const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

    AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

    if (m_instantiatingDynamicSlices.erase(ticket) > 0)
    {
        UiGameEntityContextSliceInstantiationResultsBus::Event(
            ticket, &UiGameEntityContextSliceInstantiationResultsBus::Events::OnEntityContextSliceInstantiationFailed, sliceAssetId);
        UiGameEntityContextNotificationBus::Broadcast(
            &UiGameEntityContextNotificationBus::Events::OnSliceInstantiationFailed, sliceAssetId, ticket);
    }
}
