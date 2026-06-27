/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiSpawnerComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <Shine/Bus/UiGameEntityContextBus.h>
#include <Shine/Bus/UiElementBus.h>

// BehaviorContext UiSpawnerNotificationBus forwarder
class BehaviorUiSpawnerNotificationBusHandler
    : public UiSpawnerNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiSpawnerNotificationBusHandler, "{95213AF9-F8F4-4D86-8C68-625F5AFE78FA}", AZ::SystemAllocator,
        OnSpawnBegin, OnEntitySpawned, OnEntitiesSpawned, OnTopLevelEntitiesSpawned, OnSpawnEnd, OnSpawnFailed);

    void OnSpawnBegin(const AzFramework::EntitySpawnTicket::Id& ticketId) override
    {
        Call(FN_OnSpawnBegin, ticketId);
    }

    void OnEntitySpawned(const AzFramework::EntitySpawnTicket::Id& ticketId, const AZ::EntityId& id) override
    {
        Call(FN_OnEntitySpawned, ticketId, id);
    }

    void OnEntitiesSpawned(const AzFramework::EntitySpawnTicket::Id& ticketId, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
    {
        Call(FN_OnEntitiesSpawned, ticketId, spawnedEntities);
    }

    void OnTopLevelEntitiesSpawned(const AzFramework::EntitySpawnTicket::Id& ticketId, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
    {
        Call(FN_OnTopLevelEntitiesSpawned, ticketId, spawnedEntities);
    }

    void OnSpawnEnd(const AzFramework::EntitySpawnTicket::Id& ticketId) override
    {
        Call(FN_OnSpawnEnd, ticketId);
    }

    void OnSpawnFailed(const AzFramework::EntitySpawnTicket::Id& ticketId) override
    {
        Call(FN_OnSpawnFailed, ticketId);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<UiSpawnerComponent, AZ::Component>()
            ->Version(2)
            ->Field("SpawnableAsset", &UiSpawnerComponent::m_spawnableAsset)
            ->Field("SpawnOnActivate", &UiSpawnerComponent::m_spawnOnActivate);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiSpawnerComponent>("UiSpawner",
                    "The spawner component provides dynamic UI spawnable spawning support.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Spawner.svg")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Spawner.svg")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiSpawnerComponent::m_spawnableAsset, "Spawnable", "The spawnable to spawn");
            editInfo->DataElement(0, &UiSpawnerComponent::m_spawnOnActivate, "Spawn on activate",
                "Should the component spawn the selected spawnable upon activation?");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiSpawnerBus>("UiSpawnerBus")
            ->Event("Spawn", &UiSpawnerBus::Events::Spawn)
            ->Event("SpawnRelative", &UiSpawnerBus::Events::SpawnRelative)
            ->Event("SpawnAbsolute", &UiSpawnerBus::Events::SpawnViewport)
        ;

        behaviorContext->EBus<UiSpawnerNotificationBus>("UiSpawnerNotificationBus")
            ->Handler<BehaviorUiSpawnerNotificationBusHandler>()
        ;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC_CE("SpawnerService"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
{
    dependent.push_back(AZ_CRC_CE("TransformService"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSpawnerComponent::UiSpawnerComponent()
{
    // Spawnable asset should load purely on-demand.
    m_spawnableAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Activate()
{
    UiSpawnerBus::Handler::BusConnect(GetEntityId());

    if (m_spawnOnActivate)
    {
        SpawnSpawnableInternal(m_spawnableAsset, AZ::Vector2(0.0f, 0.0f), false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Deactivate()
{
    UiSpawnerBus::Handler::BusDisconnect();
    UiGameEntityContextSpawnResultsBus::MultiHandler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::EntitySpawnTicket::Id UiSpawnerComponent::Spawn()
{
    return SpawnSpawnableInternal(m_spawnableAsset, AZ::Vector2(0.0f, 0.0f), false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::EntitySpawnTicket::Id UiSpawnerComponent::SpawnRelative(const AZ::Vector2& relative)
{
    return SpawnSpawnableInternal(m_spawnableAsset, relative, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::EntitySpawnTicket::Id UiSpawnerComponent::SpawnViewport(const AZ::Vector2& pos)
{
    return SpawnSpawnableInternal(m_spawnableAsset, pos, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::EntitySpawnTicket::Id UiSpawnerComponent::SpawnSpawnable(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable)
{
    return SpawnSpawnableInternal(spawnable, AZ::Vector2(0.0f, 0.0f), false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::EntitySpawnTicket::Id UiSpawnerComponent::SpawnSpawnableRelative(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable, const AZ::Vector2& relative)
{
    return SpawnSpawnableInternal(spawnable, relative, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::EntitySpawnTicket::Id UiSpawnerComponent::SpawnSpawnableViewport(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable, const AZ::Vector2& pos)
{
    return SpawnSpawnableInternal(spawnable, pos, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::OnEntityContextSpawnCompleted(const AZStd::vector<AZ::EntityId>& spawnedEntities)
{
    const UiSpawnId spawnId = (*UiGameEntityContextSpawnResultsBus::GetCurrentBusId());

    // Find the ticket ID for this spawn
    auto it = m_activeSpawns.find(spawnId);
    if (it == m_activeSpawns.end())
    {
        return;
    }

    AzFramework::EntitySpawnTicket::Id ticketId = it->second;

    // Stop listening for this spawn (since it's done)
    UiGameEntityContextSpawnResultsBus::MultiHandler::BusDisconnect(spawnId);
    m_activeSpawns.erase(it);

    // Notify: OnSpawnBegin
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnSpawnBegin, ticketId);

    // Notify per-entity
    for (const AZ::EntityId& entityId : spawnedEntities)
    {
        UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnEntitySpawned, ticketId, entityId);
    }

    // Notify all entities at once
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnEntitiesSpawned, ticketId, spawnedEntities);

    // Notify top-level entities
    AZStd::vector<AZ::EntityId> topLevelEntityIds = GetTopLevelEntities(spawnedEntities);
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnTopLevelEntitiesSpawned, ticketId, topLevelEntityIds);

    // Notify spawn end
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnSpawnEnd, ticketId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::OnEntityContextSpawnFailed()
{
    const UiSpawnId spawnId = (*UiGameEntityContextSpawnResultsBus::GetCurrentBusId());

    auto it = m_activeSpawns.find(spawnId);
    if (it == m_activeSpawns.end())
    {
        return;
    }

    AzFramework::EntitySpawnTicket::Id ticketId = it->second;

    UiGameEntityContextSpawnResultsBus::MultiHandler::BusDisconnect(spawnId);
    m_activeSpawns.erase(it);

    AZ_Warning("UiSpawnerComponent", false, "Spawnable failed to instantiate. Check that it contains UI elements.");
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnSpawnFailed, ticketId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::EntitySpawnTicket::Id UiSpawnerComponent::SpawnSpawnableInternal(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable, const AZ::Vector2& position, bool isViewportPosition)
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    AzFramework::EntityIdContextQueryBus::EventResult(
        contextId, GetEntityId(), &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

    UiSpawnId spawnId = InvalidUiSpawnId;
    UiGameEntityContextBus::EventResult(
        spawnId,
        contextId,
        &UiGameEntityContextBus::Events::SpawnSpawnable,
        spawnable,
        position,
        isViewportPosition,
        GetEntity());

    if (spawnId != InvalidUiSpawnId)
    {
        AzFramework::EntitySpawnTicket::Id ticketId = m_nextTicketId++;
        m_activeSpawns[spawnId] = ticketId;
        UiGameEntityContextSpawnResultsBus::MultiHandler::BusConnect(spawnId);
        return ticketId;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::vector<AZ::EntityId> UiSpawnerComponent::GetTopLevelEntities(const AZStd::vector<AZ::EntityId>& entityIds)
{
    // Create a set of all the entities
    AZStd::unordered_set<AZ::EntityId> topLevelEntities;
    for (const AZ::EntityId& entityId : entityIds)
    {
        topLevelEntities.insert(entityId);
    }

    // Remove anything from the set that is referenced as the child of another element in the list
    for (const AZ::EntityId& entityId : entityIds)
    {
        Shine::EntityArray children;
        UiElementBus::EventResult(children, entityId, &UiElementBus::Events::GetChildElements);

        for (auto child : children)
        {
            topLevelEntities.erase(child->GetId());
        }
    }

    AZStd::vector<AZ::EntityId> result;
    result.reserve(topLevelEntities.size());
    for (const AZ::EntityId& entityId : topLevelEntities)
    {
        result.push_back(entityId);
    }

    return result;
}
