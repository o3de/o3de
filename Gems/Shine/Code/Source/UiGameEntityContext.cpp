/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiGameEntityContext.h"
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/IdUtils.h>
#include <Shine/Bus/UiCanvasBus.h>
#include <Shine/Bus/UiElementBus.h>
#include <Shine/Bus/UiTransformBus.h>
#include <Shine/Bus/UiTransform2dBus.h>
#include <Shine/UiComponentTypes.h>

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
bool UiGameEntityContext::HandleLoadedEntities(const AZStd::vector<AZ::Entity*>& entities, bool remapIds, AZStd::unordered_map<AZ::EntityId, AZ::EntityId>* idRemapTable)
{
    AZ_Assert(m_entityOwnershipService->IsInitialized(), "The context has not been initialized.");

    auto* simpleService = static_cast<UiSimpleEntityOwnershipService*>(m_entityOwnershipService.get());
    simpleService->LoadEntities(entities, remapIds, idRemapTable);

    AzFramework::EntityList allEntities;
    m_entityOwnershipService->GetAllEntities(allEntities);

    InitializeEntities(allEntities);

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
        m_entityOwnershipService->AddEntity(entity);
    }

    m_entityOwnershipService->HandleEntitiesAdded(entities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiGameEntityContext::CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityList& resultEntities)
{
    resultEntities.clear();

    AzFramework::EntityList sourceEntityList;
    for (const AZ::EntityId& id : sourceEntities)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        if (entity)
        {
            sourceEntityList.push_back(entity);
        }
    }

    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> idMap;
    AzFramework::EntityList* clonedEntities =
        AZ::IdUtils::Remapper<AZ::EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(&sourceEntityList, idMap);
    if (!clonedEntities)
    {
        AZ_Error("UiEntityContext", false, "Failed to clone source entities.");
        return false;
    }

    resultEntities = *clonedEntities;

    AddUiEntities(resultEntities);

    delete clonedEntities;

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
        m_entityOwnershipService->DestroyEntity(entity);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::InitUiContext()
{
    m_entityOwnershipService = AZStd::make_unique<UiSimpleEntityOwnershipService>(GetContextId(), GetSerializeContext());
    InitContext();

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
    AzFramework::EntityList allEntities;
    m_entityOwnershipService->GetAllEntities(allEntities);

    return AZ::Utils::SaveObjectToStream(stream, streamType, &allEntities);
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
    // All entities being loaded in the UI context should
    // have the UiElementComponent on them.
    for (AZ::Entity* entity : entities)
    {
        if (!entity->FindComponent(Shine::UiElementComponentUuid))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSpawnId UiGameEntityContext::SpawnSpawnable(
    const AZ::Data::Asset<AzFramework::Spawnable>& spawnableAsset, const AZ::Vector2& position, bool isViewportPosition,
    AZ::Entity* parent)
{
    if (!spawnableAsset.GetId().IsValid())
    {
        return InvalidUiSpawnId;
    }

    UiSpawnId spawnId = m_nextSpawnId++;
    m_pendingSpawns.emplace(spawnId, PendingSpawn(spawnableAsset, position, isViewportPosition, parent));

    // If the asset is already ready, process immediately
    if (spawnableAsset.IsReady())
    {
        ProcessSpawnableEntities(spawnId, *spawnableAsset.Get());
    }
    else
    {
        // Queue the asset for loading and wait for OnAssetReady
        AZ::Data::AssetBus::MultiHandler::BusConnect(spawnableAsset.GetId());
        AZ::Data::AssetManager::Instance().GetAsset(spawnableAsset.GetId(),
            azrtti_typeid<AzFramework::Spawnable>(), AZ::Data::AssetLoadBehavior::Default);
    }

    return spawnId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
{
    AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

    // Find the pending spawn for this asset
    for (auto it = m_pendingSpawns.begin(); it != m_pendingSpawns.end(); ++it)
    {
        if (it->second.m_asset.GetId() == asset.GetId())
        {
            UiSpawnId spawnId = it->first;
            auto* spawnable = asset.GetAs<AzFramework::Spawnable>();
            if (spawnable)
            {
                ProcessSpawnableEntities(spawnId, *spawnable);
            }
            else
            {
                AZ_Warning("UiGameEntityContext", false, "Asset loaded but is not a Spawnable.");
                UiGameEntityContextSpawnResultsBus::Event(spawnId, &UiGameEntityContextSpawnResultsBus::Events::OnEntityContextSpawnFailed);
                UiGameEntityContextNotificationBus::Broadcast(&UiGameEntityContextNotificationBus::Events::OnSpawnFailed, spawnId);
                m_pendingSpawns.erase(it);
            }
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
{
    AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

    for (auto it = m_pendingSpawns.begin(); it != m_pendingSpawns.end(); ++it)
    {
        if (it->second.m_asset.GetId() == asset.GetId())
        {
            UiSpawnId spawnId = it->first;
            AZ_Warning("UiGameEntityContext", false, "Spawnable asset failed to load.");
            UiGameEntityContextSpawnResultsBus::Event(spawnId, &UiGameEntityContextSpawnResultsBus::Events::OnEntityContextSpawnFailed);
            UiGameEntityContextNotificationBus::Broadcast(&UiGameEntityContextNotificationBus::Events::OnSpawnFailed, spawnId);
            m_pendingSpawns.erase(it);
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiGameEntityContext::ProcessSpawnableEntities(UiSpawnId spawnId, const AzFramework::Spawnable& spawnable)
{
    auto instantiatingIter = m_pendingSpawns.find(spawnId);
    if (instantiatingIter == m_pendingSpawns.end())
    {
        return;
    }

    PendingSpawn& pending = instantiatingIter->second;

    const auto& templateEntities = spawnable.GetEntities();
    if (templateEntities.empty())
    {
        m_pendingSpawns.erase(instantiatingIter);
        return;
    }

    // Clone entities from the spawnable template with new entity IDs
    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> idMap;
    AzFramework::EntityList clonedEntities;
    clonedEntities.reserve(templateEntities.size());

    for (const auto& templateEntity : templateEntities)
    {
        AZ::Entity* clonedEntity = AZ::IdUtils::Remapper<AZ::EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(
            templateEntity.get(), idMap, m_serializeContext);
        if (clonedEntity)
        {
            clonedEntities.push_back(clonedEntity);
        }
    }

    if (clonedEntities.empty())
    {
        AZ_Warning("UiGameEntityContext", false, "Failed to clone entities from spawnable.");
        UiGameEntityContextSpawnResultsBus::Event(spawnId, &UiGameEntityContextSpawnResultsBus::Events::OnEntityContextSpawnFailed);
        UiGameEntityContextNotificationBus::Broadcast(&UiGameEntityContextNotificationBus::Events::OnSpawnFailed, spawnId);
        m_pendingSpawns.erase(instantiatingIter);
        return;
    }

    // Validate that all entities are valid UI entities
    if (!ValidateEntitiesAreValidForContext(clonedEntities))
    {
        AZ_Warning("UiGameEntityContext", false, "Spawnable contains entities that are not valid UI elements.");
        for (AZ::Entity* entity : clonedEntities)
        {
            delete entity;
        }
        UiGameEntityContextSpawnResultsBus::Event(spawnId, &UiGameEntityContextSpawnResultsBus::Events::OnEntityContextSpawnFailed);
        UiGameEntityContextNotificationBus::Broadcast(&UiGameEntityContextNotificationBus::Events::OnSpawnFailed, spawnId);
        m_pendingSpawns.erase(instantiatingIter);
        return;
    }

    // Add entities to the entity context
    AddUiEntities(clonedEntities);

    // Create a set of all the top-level entities
    AZStd::unordered_set<AZ::Entity*> topLevelEntities;
    for (AZ::Entity* entity : clonedEntities)
    {
        topLevelEntities.insert(entity);
    }

    // Remove anything from the topLevelEntities set that is referenced as the child of another element
    for (AZ::Entity* entity : clonedEntities)
    {
        Shine::EntityArray children;
        UiElementBus::EventResult(children, entity->GetId(), &UiElementBus::Events::GetChildElements);

        for (auto child : children)
        {
            topLevelEntities.erase(child);
        }
    }

    AZ::Entity* parent = pending.m_parent;

    // Now topLevelElements contains all of the top-level elements in the set of newly cloned entities
    Shine::EntityArray entitiesToInit;
    for (auto entity : topLevelEntities)
    {
        entitiesToInit.push_back(entity);
    }

    // There must be at least one element
    AZ_Assert(entitiesToInit.size() >= 1, "There must be at least one top-level entity in a UI spawnable.");

    // Initialize the internal parent pointers and the canvas pointer in the elements
    UiCanvasBus::Event(m_canvasEntityId, &UiCanvasBus::Events::FixupCreatedEntities, entitiesToInit, true, parent);

    // Add all of the top-level entities as children of the parent
    for (auto entity : topLevelEntities)
    {
        UiCanvasBus::Event(m_canvasEntityId, &UiCanvasBus::Events::AddElement, entity, parent, nullptr);
    }

    // Position the instantiated entities
    if (pending.m_isViewportPosition)
    {
        const AZ::Vector2& desiredViewportPosition = pending.m_position;

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
    else if (!pending.m_position.IsZero())
    {
        AZ::Entity* rootElement = entitiesToInit[0];
        UiTransformBus::Event(rootElement->GetId(), &UiTransformBus::Events::MoveLocalPositionBy, pending.m_position);
    }

    // Collect all spawned entity IDs
    AZStd::vector<AZ::EntityId> spawnedEntityIds;
    spawnedEntityIds.reserve(clonedEntities.size());
    for (AZ::Entity* entity : clonedEntities)
    {
        spawnedEntityIds.push_back(entity->GetId());
    }

    // Must erase before firing events to avoid iterator invalidation
    m_pendingSpawns.erase(instantiatingIter);

    // Notify via per-spawn results bus (used by UiSpawnerComponent)
    UiGameEntityContextSpawnResultsBus::Event(
        spawnId, &UiGameEntityContextSpawnResultsBus::Events::OnEntityContextSpawnCompleted, spawnedEntityIds);

    // Notify via broadcast bus
    UiGameEntityContextNotificationBus::Broadcast(
        &UiGameEntityContextNotificationBus::Events::OnSpawnCompleted, spawnId, spawnedEntityIds);
}
