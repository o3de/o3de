/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(CARBONATED)

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Components/TransformComponent.h>

#include "PrefabSpawnerComponent.h"

#ifdef LMBR_CENTRAL_EDITOR
#include "EditorPrefabSpawnerComponent.h"
#endif
 
namespace LmbrCentral
{
    // BehaviorContext PrefabSpawnerComponentNotificationBus forwarder
    class BehaviorPrefabSpawnerComponentNotificationBusHandler : public PrefabSpawnerComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorPrefabSpawnerComponentNotificationBusHandler, "{B23AC232-BBAA-1286-BAD3-2387AVB324AB}", AZ::SystemAllocator,
            OnSpawnBegin, OnSpawnEnd, OnEntitySpawned, OnSpawnedPrefabDestroyed, OnEntitiesSpawned);

        void OnSpawnBegin(const AzFramework::EntitySpawnTicket& ticket) override
        {
            Call(FN_OnSpawnBegin, ticket);
        }

        void OnSpawnEnd(const AzFramework::EntitySpawnTicket& ticket) override
        {
            Call(FN_OnSpawnEnd, ticket);
        }

        void OnEntitySpawned(const AzFramework::EntitySpawnTicket& ticket, const AZ::EntityId& id) override
        {
            Call(FN_OnEntitySpawned, ticket, id);
        }

        void OnSpawnedPrefabDestroyed(const AzFramework::EntitySpawnTicket& ticket) override
        {
            Call(FN_OnSpawnedPrefabDestroyed, ticket);
        }

        //! Single event notification for an entire prefab spawn, providing a list of all resulting entity Ids.
        void OnEntitiesSpawned(const AzFramework::EntitySpawnTicket& ticket, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
        {
            Call(FN_OnEntitiesSpawned, ticket, spawnedEntities);
        }

    };

    //=========================================================================
    void PrefabSpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PrefabSpawnerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Prefab", &PrefabSpawnerComponent::m_prefabAsset)
                ->Field("SpawnOnActivate", &PrefabSpawnerComponent::m_spawnOnActivate)
                ->Field("DestroyOnDeactivate", &PrefabSpawnerComponent::m_destroyOnDeactivate)
                ;
        }
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<PrefabSpawnerComponentRequestBus>("PrefabSpawnerComponentRequestBus")
                ->Event("Spawn", &PrefabSpawnerComponentRequestBus::Events::Spawn)
                ->Event("SpawnRelative", &PrefabSpawnerComponentRequestBus::Events::SpawnRelative)
                ->Event("SpawnAbsolute", &PrefabSpawnerComponentRequestBus::Events::SpawnAbsolute)
                ->Event("DestroySpawnedPrefab", &PrefabSpawnerComponentRequestBus::Events::DestroySpawnedPrefab)
                ->Event("DestroyAllSpawnedPrefabs", &PrefabSpawnerComponentRequestBus::Events::DestroyAllSpawnedPrefabs)
                ->Event("GetCurrentlySpawnedPrefabs", &PrefabSpawnerComponentRequestBus::Events::GetCurrentlySpawnedPrefabs)
                ->Event("HasAnyCurrentlySpawnedPrefabs", &PrefabSpawnerComponentRequestBus::Events::HasAnyCurrentlySpawnedPrefabs)
                ->Event("GetCurrentEntitiesFromSpawnedPrefab", &PrefabSpawnerComponentRequestBus::Events::GetCurrentEntitiesFromSpawnedPrefab)
                ->Event("GetAllCurrentlySpawnedEntities", &PrefabSpawnerComponentRequestBus::Events::GetAllCurrentlySpawnedEntities)
                ->Event("SetSpawnablePrefab", &PrefabSpawnerComponentRequestBus::Events::SetSpawnablePrefabByAssetId)
                ->Event("IsReadyToSpawn", &PrefabSpawnerComponentRequestBus::Events::IsReadyToSpawn);

            behaviorContext->EBus<PrefabSpawnerComponentNotificationBus>("PrefabSpawnerComponentNotificationBus")
                ->Handler<BehaviorPrefabSpawnerComponentNotificationBusHandler>();

            behaviorContext->Constant("PrefabSpawnerComponentTypeId", BehaviorConstant(PrefabSpawnerComponentTypeId));

            behaviorContext
                ->Class<PrefabSpawnerConfig>()
                ->Property("prefabAsset", BehaviorValueProperty(&PrefabSpawnerConfig::m_prefabAsset))
                ->Property("spawnOnActivate", BehaviorValueProperty(&PrefabSpawnerConfig::m_spawnOnActivate))
                ->Property("destroyOnDeactivate", BehaviorValueProperty(&PrefabSpawnerConfig::m_destroyOnDeactivate));
        }
    }

    //=========================================================================
    void PrefabSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SpawnerService", 0xd2f1d7a3));
    }

    //=========================================================================
    void PrefabSpawnerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType&)
    {
    }

    //=========================================================================
    void PrefabSpawnerComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    //=========================================================================
    PrefabSpawnerComponent::PrefabSpawnerComponent()
    {
        // Prefab asset should load purely on-demand.
        m_prefabAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
    }

    //=========================================================================
    void PrefabSpawnerComponent::Activate()
    {
        PrefabSpawnerComponentRequestBus::Handler::BusConnect(GetEntityId());

        if (m_spawnOnActivate)
        {
            SpawnPrefabInternalRelative(m_prefabAsset, AZ::Transform::Identity());
        }
    }

    //=========================================================================
    void PrefabSpawnerComponent::Deactivate()
    {
        PrefabSpawnerComponentRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AZ::EntityBus::MultiHandler::BusDisconnect();

        if (m_destroyOnDeactivate)
        {
            DestroyAllSpawnedPrefabs();
        }

        m_activeTickets.clear();
        m_entityToTicketMap.clear();
        m_ticketToEntitiesMap.clear();
    }

    bool PrefabSpawnerComponent::ReadInConfig(const AZ::ComponentConfig* prefabPrefabSpawnerConfig)
    {
        if (auto config = azrtti_cast<const PrefabSpawnerConfig*>(prefabPrefabSpawnerConfig))
        {
            m_prefabAsset = config->m_prefabAsset;
            m_spawnOnActivate = config->m_spawnOnActivate;
            m_destroyOnDeactivate = config->m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    bool PrefabSpawnerComponent::WriteOutConfig(AZ::ComponentConfig* outPrefabSpawnerConfig) const
    {
        if (auto config = azrtti_cast<PrefabSpawnerConfig*>(outPrefabSpawnerConfig))
        {
            config->m_prefabAsset = m_prefabAsset;
            config->m_spawnOnActivate = m_spawnOnActivate;
            config->m_destroyOnDeactivate = m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    //=========================================================================
    void PrefabSpawnerComponent::SetSpawnablePrefab(const AZ::Data::Asset<AzFramework::Spawnable>& spawnablePrefabAsset)
    {
        m_prefabAsset = spawnablePrefabAsset;
    }

    //=========================================================================
    void PrefabSpawnerComponent::SetSpawnablePrefabByAssetId(AZ::Data::AssetId& assetId)
    {
        if (m_prefabAsset.GetId() == assetId)
        {
            return;
        }

        m_prefabAsset = AZ::Data::AssetManager::Instance().GetAsset(
            assetId, AZ::AzTypeInfo<AzFramework::SpawnableAsset>::Uuid(), m_prefabAsset.GetAutoLoadBehavior());
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusConnect(assetId);
    }

    //=========================================================================
    void PrefabSpawnerComponent::SetSpawnOnActivate(bool spawnOnActivate)
    {
        m_spawnOnActivate = spawnOnActivate;
    }

    //=========================================================================
    bool PrefabSpawnerComponent::GetSpawnOnActivate()
    {
        return m_spawnOnActivate;
    }

    //=========================================================================
    AzFramework::EntitySpawnTicket PrefabSpawnerComponent::Spawn()
    {
        return SpawnPrefabInternalRelative(m_prefabAsset, AZ::Transform::Identity());
    }

    //=========================================================================
    AzFramework::EntitySpawnTicket PrefabSpawnerComponent::SpawnRelative(const AZ::Transform& relative)
    {
        return SpawnPrefabInternalRelative(m_prefabAsset, relative);
    }
    
    //=========================================================================
    AzFramework::EntitySpawnTicket PrefabSpawnerComponent::SpawnAbsolute(const AZ::Transform& world)
    {
        return SpawnPrefabInternalAbsolute(m_prefabAsset, world);
    }

    //=========================================================================
    AzFramework::EntitySpawnTicket PrefabSpawnerComponent::SpawnPrefab(const AZ::Data::Asset<AzFramework::Spawnable>& prefab)
    {
        return SpawnPrefabInternalRelative(prefab, AZ::Transform::Identity());
    }

    //=========================================================================
    AzFramework::EntitySpawnTicket PrefabSpawnerComponent::SpawnPrefabRelative(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& relative)
    {
        return SpawnPrefabInternalRelative(prefab, relative);
    }
    
    //=========================================================================
    AzFramework::EntitySpawnTicket PrefabSpawnerComponent::SpawnPrefabAbsolute(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& world)
    {
        return SpawnPrefabInternalAbsolute(prefab, world);
    }

    //=========================================================================
    AzFramework::EntitySpawnTicket PrefabSpawnerComponent::SpawnPrefabInternalAbsolute(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& world)
    {
        AzFramework::EntitySpawnTicket ticket = AzFramework::EntitySpawnTicket(prefab);

        auto preSpawnCB = [ticket, this, world](
                              [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
                              [[maybe_unused]] AzFramework::SpawnableEntityContainerView view)
        {
            AZ::Entity* containerEntity = *view.begin();
            AzFramework::TransformComponent* entityTransform = containerEntity->FindComponent<AzFramework::TransformComponent>();

            if (entityTransform)
            {
                entityTransform->SetWorldTM(world);
            }

            PrefabSpawnerComponentNotificationBus::Event(GetEntityId(), &PrefabSpawnerComponentNotificationBus::Events::OnSpawnBegin, ticket);
        };

        auto spawnCompleteCB = [this, ticket](
                                   [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
                                   [[maybe_unused]] AzFramework::SpawnableConstEntityContainerView view)
        {
            OnPrefabInstantiated(ticket, view);
        };

        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_preInsertionCallback = AZStd::move(preSpawnCB);
        optionalArgs.m_completionCallback = AZStd::move(spawnCompleteCB);
        AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(ticket, AZStd::move(optionalArgs));

        if (ticket.IsValid())
        {
            m_activeTickets.emplace_back(ticket);
            m_ticketToEntitiesMap.emplace(ticket); // create entry for ticket, with no entities listed yet
        }
        return ticket;
    }

    //=========================================================================
    AzFramework::EntitySpawnTicket PrefabSpawnerComponent::SpawnPrefabInternalRelative(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& relative)
    {
        AZ::Transform transform = AZ::Transform::Identity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        transform *= relative;

        return SpawnPrefabInternalAbsolute(prefab, transform);
    }

    //=========================================================================
    void PrefabSpawnerComponent::DestroySpawnedPrefab(AzFramework::EntitySpawnTicket& prefabTicket)
    {
        auto foundTicketEntities = m_ticketToEntitiesMap.find(prefabTicket);
        if (foundTicketEntities != m_ticketToEntitiesMap.end())
        {
            AZStd::unordered_set<AZ::EntityId>& entitiesInPrefab = foundTicketEntities->second;

            for (AZ::EntityId entity : entitiesInPrefab)
            {
                AZ::EntityBus::MultiHandler::BusDisconnect(entity); // don't care anymore about events from this entity
                m_entityToTicketMap.erase(entity);
            }

            m_ticketToEntitiesMap.erase(foundTicketEntities);
            m_activeTickets.erase(AZStd::remove(m_activeTickets.begin(), m_activeTickets.end(), prefabTicket), m_activeTickets.end());

            // prefab destruction is queued, so queue the notification as well.
            AZ::EntityId entityId = GetEntityId();
            AZ::TickBus::QueueFunction(
                [entityId, prefabTicket]() // use copies, in case 'this' is destroyed
                {
                    PrefabSpawnerComponentNotificationBus::Event(entityId, &PrefabSpawnerComponentNotificationBus::Events::OnSpawnedPrefabDestroyed, prefabTicket);
                });
            AzFramework::SpawnableEntitiesInterface::Get()->DespawnAllEntities(prefabTicket);
        }
    }

    //=========================================================================
    void PrefabSpawnerComponent::DestroyAllSpawnedPrefabs()
    {
        AZStd::vector<AzFramework::EntitySpawnTicket> activeTickets = m_activeTickets; // iterate over a copy of the vector
        for (AzFramework::EntitySpawnTicket& ticket : activeTickets)
        {
            DestroySpawnedPrefab(ticket);
        }

        AZ_Assert(m_activeTickets.empty(), "PrefabSpawnerComponent::DestroyAllSpawnedPrefabs - tickets still listed");
        AZ_Assert(m_entityToTicketMap.empty(), "PrefabSpawnerComponent::DestroyAllSpawnedPrefabs - entities still listed");
        AZ_Assert(m_ticketToEntitiesMap.empty(), "PrefabSpawnerComponent::DestroyAllSpawnedPrefabs - ticket entities still listed");
    }

    //=========================================================================
    AZStd::vector<AzFramework::EntitySpawnTicket> PrefabSpawnerComponent::GetCurrentlySpawnedPrefabs()
    {
        return m_activeTickets;
    }

    //=========================================================================
    bool PrefabSpawnerComponent::HasAnyCurrentlySpawnedPrefabs()
    {
        return !m_activeTickets.empty();
    }

    //=========================================================================
    AZStd::vector<AZ::EntityId> PrefabSpawnerComponent::GetCurrentEntitiesFromSpawnedPrefab(const AzFramework::EntitySpawnTicket& ticket)
    {
        AZStd::vector<AZ::EntityId> entities;
        auto foundTicketEntities = m_ticketToEntitiesMap.find(ticket);
        if (foundTicketEntities != m_ticketToEntitiesMap.end())
        {
            const AZStd::unordered_set<AZ::EntityId>& ticketEntities = foundTicketEntities->second;

            AZ_Warning("PrefabSpawnerComponent", !ticketEntities.empty(), "PrefabSpawnerComponent::GetCurrentEntitiesFromSpawnedPrefab - Spawn has not completed, its entities are not available.");

            entities.reserve(ticketEntities.size());
            entities.insert(entities.end(), ticketEntities.begin(), ticketEntities.end());

            // Sort entities so that results are stable.
            AZStd::sort(entities.begin(), entities.end());
        }
        return entities;
    }

    //=========================================================================
    AZStd::vector<AZ::EntityId> PrefabSpawnerComponent::GetAllCurrentlySpawnedEntities()
    {
        AZStd::vector<AZ::EntityId> entities;
        entities.reserve(m_entityToTicketMap.size());

        // Return entities in the order their tickets spawned.
        // It's not a requirement, but it seems nice to do.
        for (const AzFramework::EntitySpawnTicket& ticket : m_activeTickets)
        {
            const AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];
            entities.insert(entities.end(), ticketEntities.begin(), ticketEntities.end());

            // Sort entities from a given ticket, so that results are stable.
            AZStd::sort(entities.end() - ticketEntities.size(), entities.end());
        }

        return entities;
    }

    //=========================================================================
    bool PrefabSpawnerComponent::IsReadyToSpawn()
    {
        return m_prefabAsset.IsReady();
    }

    void PrefabSpawnerComponent::OnPrefabInstantiated(
        AzFramework::EntitySpawnTicket ticket, AzFramework::SpawnableConstEntityContainerView view)
    {
        AZStd::vector<AZ::EntityId> entityIds;
        entityIds.reserve(view.size());

        AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];

        for (const AZ::Entity* currEntity : view)
        {
            AZ::EntityId currEntityId = currEntity->GetId();

            entityIds.emplace_back(currEntityId);

            // update internal prefab tracking data
            ticketEntities.emplace(currEntityId);
            m_entityToTicketMap.emplace(AZStd::make_pair(currEntityId, ticket));
            AZ::EntityBus::MultiHandler::BusConnect(currEntityId);

            PrefabSpawnerComponentNotificationBus::Event(
                GetEntityId(), &PrefabSpawnerComponentNotificationBus::Events::OnEntitySpawned, ticket, currEntityId);
        }

        PrefabSpawnerComponentNotificationBus::Event(GetEntityId(), &PrefabSpawnerComponentNotificationBus::Events::OnSpawnEnd, ticket);

        PrefabSpawnerComponentNotificationBus::Event(
            GetEntityId(), &PrefabSpawnerComponentNotificationBus::Events::OnEntitiesSpawned, ticket, entityIds);

        // If prefab had no entities, clean it up
        if (entityIds.empty())
        {
            DestroySpawnedPrefab(ticket);
        }
        m_prefabAsset = *ticket.GetSpawnable();
    }

    //=========================================================================
    void PrefabSpawnerComponent::OnEntityDestruction(const AZ::EntityId& entityId)
    {
        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);

        auto entityToTicketIter = m_entityToTicketMap.find(entityId);
        if (entityToTicketIter != m_entityToTicketMap.end())
        {
            AzFramework::EntitySpawnTicket ticket = entityToTicketIter->second;
            m_entityToTicketMap.erase(entityToTicketIter);

            AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];
            ticketEntities.erase(entityId);

            // If this was last entity in the spawn, clean it up
            if (ticketEntities.empty())
            {
                DestroySpawnedPrefab(ticket);
            }
        }
    }

} // namespace LmbrCentral

#endif
