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

#include <LyShine/Bus/UiGameEntityContextBus.h>
#include <LyShine/Bus/UiElementBus.h>

// BehaviorContext UiSpawnerNotificationBus forwarder
class BehaviorUiSpawnerNotificationBusHandler
    : public UiSpawnerNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiSpawnerNotificationBusHandler, "{95213AF9-F8F4-4D86-8C68-625F5AFE78FA}", AZ::SystemAllocator,
        OnSpawnBegin, OnEntitySpawned, OnEntitiesSpawned, OnTopLevelEntitiesSpawned, OnSpawnEnd, OnSpawnFailed);

    void OnSpawnBegin(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        Call(FN_OnSpawnBegin, ticket);
    }

    void OnEntitySpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZ::EntityId& id) override
    {
        Call(FN_OnEntitySpawned, ticket, id);
    }

    void OnEntitiesSpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
    {
        Call(FN_OnEntitiesSpawned, ticket, spawnedEntities);
    }

    void OnTopLevelEntitiesSpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
    {
        Call(FN_OnTopLevelEntitiesSpawned, ticket, spawnedEntities);
    }

    void OnSpawnEnd(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        Call(FN_OnSpawnEnd, ticket);
    }

    void OnSpawnFailed(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        Call(FN_OnSpawnFailed, ticket);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<UiSpawnerComponent, AZ::Component>()
            ->Version(1)
            ->Field("Slice", &UiSpawnerComponent::m_sliceAsset)
            ->Field("SpawnOnActivate", &UiSpawnerComponent::m_spawnOnActivate);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiSpawnerComponent>("UiSpawner",
                    "The spawner component provides dynamic UI slice spawning support.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Spawner.svg")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Spawner.svg")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiSpawnerComponent::m_sliceAsset, "Dynamic slice", "The slice to spawn");
            editInfo->DataElement(0, &UiSpawnerComponent::m_spawnOnActivate, "Spawn on activate",
                "Should the component spawn the selected slice upon activation?");
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
    // Slice asset should load purely on-demand.
    m_sliceAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Activate()
{
    UiSpawnerBus::Handler::BusConnect(GetEntityId());

    if (m_spawnOnActivate)
    {
        SpawnSliceInternal(m_sliceAsset, AZ::Vector2(0.0f, 0.0f), false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Deactivate()
{
    UiSpawnerBus::Handler::BusDisconnect();
    UiGameEntityContextSliceInstantiationResultsBus::MultiHandler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::Spawn()
{
    return SpawnSliceInternal(m_sliceAsset, AZ::Vector2(0.0f, 0.0f), false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnRelative(const AZ::Vector2& relative)
{
    return SpawnSliceInternal(m_sliceAsset, relative, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnViewport(const AZ::Vector2& pos)
{
    return SpawnSliceInternal(m_sliceAsset, pos, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice)
{
    return SpawnSliceInternal(slice, AZ::Vector2(0.0f, 0.0f), false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& relative)
{
    return SpawnSliceInternal(slice, relative, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnSliceViewport(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& pos)
{
    return SpawnSliceInternal(slice, pos, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::OnEntityContextSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/)
{
    const AzFramework::SliceInstantiationTicket ticket = (*UiGameEntityContextSliceInstantiationResultsBus::GetCurrentBusId());
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnSpawnBegin, ticket);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::OnEntityContextSliceInstantiated([[maybe_unused]] const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
{
    const AzFramework::SliceInstantiationTicket ticket = (*UiGameEntityContextSliceInstantiationResultsBus::GetCurrentBusId());

    // Stop listening for this ticket (since it's done). We can have have multiple tickets in flight.
    UiGameEntityContextSliceInstantiationResultsBus::MultiHandler::BusDisconnect(ticket);

    const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

    // first, send a notification of every individual entity that has been spawned (including top-level elements)
    AZStd::vector<AZ::EntityId> entityIds;
    entityIds.reserve(entities.size());
    for (AZ::Entity* currEntity : entities)
    {
        entityIds.push_back(currEntity->GetId());
        UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnEntitySpawned, ticket, currEntity->GetId());
    }

    // Then send one notification with all the entities spawned for this ticket
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnEntitiesSpawned, ticket, entityIds);

    // Then send notifications for all top level entities (there is usually only one). This will have been
    // included in the OnEntitySpawned and OnEntitiesSpawned but this is probably the most useful notification
    AZStd::vector<AZ::EntityId> topLevelEntityIds = GetTopLevelEntities(entities);
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnTopLevelEntitiesSpawned, ticket, topLevelEntityIds);

    // last, send an "OnSpawnEnd" to indicate the end of all notifications for this ticket
    UiSpawnerNotificationBus::Event(GetEntityId(), &UiSpawnerNotificationBus::Events::OnSpawnEnd, ticket);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::OnEntityContextSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
{
    UiGameEntityContextSliceInstantiationResultsBus::MultiHandler::BusDisconnect(*UiGameEntityContextSliceInstantiationResultsBus::GetCurrentBusId());

    AZStd::string assetPath;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, sliceAssetId);

    if (assetPath.empty())
    {
        assetPath = sliceAssetId.ToString<AZStd::string>();
    }

    AZ_Warning("UiSpawnerComponent", false, "Slice '%s' failed to instantiate. Check that it contains UI elements.", assetPath.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnSliceInternal(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& position, bool isViewportPosition)
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    AzFramework::EntityIdContextQueryBus::EventResult(
        contextId, GetEntityId(), &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

    AzFramework::SliceInstantiationTicket ticket;
    UiGameEntityContextBus::EventResult(
        ticket,
        contextId,
        &UiGameEntityContextBus::Events::InstantiateDynamicSlice,
        slice,
        position,
        isViewportPosition,
        GetEntity(),
        nullptr);

    UiGameEntityContextSliceInstantiationResultsBus::MultiHandler::BusConnect(ticket);

    return ticket;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::vector<AZ::EntityId> UiSpawnerComponent::GetTopLevelEntities(const AZ::SliceComponent::EntityList& entities)
{
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

    // Now topLevelElements contains all of the top-level elements in the set of newly instantiated entities
    // Copy the topLevelEntities set into a list
    AZStd::vector<AZ::EntityId> result;
    for (auto entity : topLevelEntities)
    {
        result.push_back(entity->GetId());
    }

    return result;
}
