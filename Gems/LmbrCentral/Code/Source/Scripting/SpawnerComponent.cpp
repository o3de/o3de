/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipServiceBus.h>

#include "SpawnerComponent.h"

#ifdef LMBR_CENTRAL_EDITOR
#include "EditorSpawnerComponent.h"
#endif
 
namespace LmbrCentral
{
    // BehaviorContext SpawnerComponentNotificationBus forwarder
    class BehaviorSpawnerComponentNotificationBusHandler : public SpawnerComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSpawnerComponentNotificationBusHandler, "{AC202871-2522-48A6-9B62-5FDAABB302CD}", AZ::SystemAllocator,
            OnSpawnBegin, OnSpawnEnd, OnEntitySpawned, OnSpawnedSliceDestroyed, OnEntitiesSpawned);

        void OnSpawnBegin(const AzFramework::SliceInstantiationTicket& ticket) override
        {
            Call(FN_OnSpawnBegin, ticket);
        }

        void OnSpawnEnd(const AzFramework::SliceInstantiationTicket& ticket) override
        {
            Call(FN_OnSpawnEnd, ticket);
        }

        void OnEntitySpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZ::EntityId& id) override
        {
            Call(FN_OnEntitySpawned, ticket, id);
        }

        void OnSpawnedSliceDestroyed(const AzFramework::SliceInstantiationTicket& ticket) override
        {
            Call(FN_OnSpawnedSliceDestroyed, ticket);
        }

        //! Single event notification for an entire slice spawn, providing a list of all resulting entity Ids.
        void OnEntitiesSpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
        {
            Call(FN_OnEntitiesSpawned, ticket, spawnedEntities);
        }

    };

    // Convert any instances of the old SampleComponent data into the appropriate
    // modern editor-component or game-component.
    bool ConvertLegacySpawnerComponent(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classNode)
    {
#ifdef LMBR_CENTRAL_EDITOR
        // To determine whether we want an editor or runtime component, we check
        // if the old component was contained within GenericComponentWrapper::m_template.
        bool isEditorComponent = (classNode.GetName() == AZ::Crc32("m_template"));
#endif

        // Get Component::m_id from the base class.
        AZ::u64 componentId = 0;
        if (auto baseClassNode = classNode.FindSubElement(AZ::Crc32("BaseClass1")))
        {
            baseClassNode->GetChildData(AZ::Crc32("Id"), componentId);
        }

        // Get data values.
        SpawnerConfig config;

        classNode.GetChildData(AZ::Crc32("Slice"), config.m_sliceAsset);
        classNode.GetChildData(AZ::Crc32("SpawnOnActivate"), config.m_spawnOnActivate);
        classNode.GetChildData(AZ::Crc32("DestroyOnDeactivate"), config.m_destroyOnDeactivate);

        // Convert this node into the appropriate component-type.
        // Note that converting the node will clear all child data nodes.
#ifdef LMBR_CENTRAL_EDITOR
        if (isEditorComponent)
        {
            classNode.Convert(serializeContext, azrtti_typeid<EditorSpawnerComponent>());

            // Create a temporary editor-component and write its contents to this node
            EditorSpawnerComponent component;
            component.SetId(componentId);
            component.SetConfiguration(config);

            classNode.SetData(serializeContext, component);
        }
        else
#endif // LMBR_CENTRAL_EDITOR
        {
            classNode.Convert(serializeContext, azrtti_typeid<SpawnerComponent>());

            // Create a temporary game-component and write its contents to this classNode
            SpawnerComponent component;
            component.SetId(componentId);
            component.SetConfiguration(config);

            classNode.SetData(serializeContext, component);
        }

        return true;
    }

    //=========================================================================
    void SpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate("SpawnerComponent", DeprecatedSpawnerComponentTypeId, ConvertLegacySpawnerComponent);

            serializeContext->Class<SpawnerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Slice", &SpawnerComponent::m_sliceAsset)
                ->Field("SpawnOnActivate", &SpawnerComponent::m_spawnOnActivate)
                ->Field("DestroyOnDeactivate", &SpawnerComponent::m_destroyOnDeactivate)
                ;
        }
    }

    //=========================================================================
    void SpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SpawnerService"));
    }

    //=========================================================================
    void SpawnerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType&)
    {
    }

    //=========================================================================
    void SpawnerComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("TransformService"));
    }

    //=========================================================================
    SpawnerComponent::SpawnerComponent()
    {
        // Slice asset should load purely on-demand.
        m_sliceAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
    }

    //=========================================================================
    void SpawnerComponent::Activate()
    {
        SpawnerComponentRequestBus::Handler::BusConnect(GetEntityId());

        if (m_spawnOnActivate)
        {
            SpawnSliceInternalRelative(m_sliceAsset, AZ::Transform::Identity());
        }
    }

    //=========================================================================
    void SpawnerComponent::Deactivate()
    {
        SpawnerComponentRequestBus::Handler::BusDisconnect();
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect();
        AZ::EntityBus::MultiHandler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();

        if (m_destroyOnDeactivate)
        {
            DestroyAllSpawnedSlices();
        }

        m_activeTickets.clear();
        m_entityToTicketMap.clear();
        m_ticketToEntitiesMap.clear();
    }

    bool SpawnerComponent::ReadInConfig(const AZ::ComponentConfig* spawnerConfig)
    {
        if (auto config = azrtti_cast<const SpawnerConfig*>(spawnerConfig))
        {
            m_sliceAsset = config->m_sliceAsset;
            m_spawnOnActivate = config->m_spawnOnActivate;
            m_destroyOnDeactivate = config->m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    bool SpawnerComponent::WriteOutConfig(AZ::ComponentConfig* outSpawnerConfig) const
    {
        if (auto config = azrtti_cast<SpawnerConfig*>(outSpawnerConfig))
        {
            config->m_sliceAsset = m_sliceAsset;
            config->m_spawnOnActivate = m_spawnOnActivate;
            config->m_destroyOnDeactivate = m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    //=========================================================================
    void SpawnerComponent::SetDynamicSlice(const AZ::Data::Asset<AZ::DynamicSliceAsset>& dynamicSliceAsset)
    {
        m_sliceAsset = dynamicSliceAsset;
    }

    //=========================================================================
    void SpawnerComponent::SetDynamicSliceByAssetId(AZ::Data::AssetId& assetId)
    {
        if (m_sliceAsset.GetId() == assetId)
        {
            return;
        }

        m_sliceAsset = AZ::Data::AssetManager::Instance().GetAsset(
            assetId, AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid(), m_sliceAsset.GetAutoLoadBehavior());
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusConnect(assetId);
    }

    //=========================================================================
    void SpawnerComponent::SetSpawnOnActivate(bool spawnOnActivate)
    {
        m_spawnOnActivate = spawnOnActivate;
    }

    //=========================================================================
    bool SpawnerComponent::GetSpawnOnActivate()
    {
        return m_spawnOnActivate;
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::Spawn()
    {
        return SpawnSliceInternalRelative(m_sliceAsset, AZ::Transform::Identity());
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnRelative(const AZ::Transform& relative)
    {
        return SpawnSliceInternalRelative(m_sliceAsset, relative);
    }
    
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnAbsolute(const AZ::Transform& world)
    {
        return SpawnSliceInternalAbsolute(m_sliceAsset, world);
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice)
    {
        return SpawnSliceInternalRelative(slice, AZ::Transform::Identity());
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative)
    {
        return SpawnSliceInternalRelative(slice, relative);
    }
    
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world)
    {
        return SpawnSliceInternalAbsolute(slice, world);
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceInternalAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world)
    {
        AzFramework::SliceInstantiationTicket ticket;
        AzFramework::SliceGameEntityOwnershipServiceRequestBus::BroadcastResult(ticket,
            &AzFramework::SliceGameEntityOwnershipServiceRequests::InstantiateDynamicSlice, slice, world, nullptr);
        if (ticket.IsValid())
        {
            m_activeTickets.emplace_back(ticket);
            m_ticketToEntitiesMap.emplace(ticket); // create entry for ticket, with no entities listed yet

            AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);
        }
        return ticket;
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceInternalRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative)
    {
        AZ::Transform transform = AZ::Transform::Identity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        transform *= relative;

        return SpawnSliceInternalAbsolute(slice, transform);
    }

    //=========================================================================
    void SpawnerComponent::DestroySpawnedSlice(const AzFramework::SliceInstantiationTicket& sliceTicket)
    {
        auto foundTicketEntities = m_ticketToEntitiesMap.find(sliceTicket);
        if (foundTicketEntities != m_ticketToEntitiesMap.end())
        {
            AZStd::unordered_set<AZ::EntityId>& entitiesInSlice = foundTicketEntities->second;

            AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(sliceTicket); // don't care anymore about events from this ticket

            if (entitiesInSlice.empty())
            {
                AzFramework::SliceGameEntityOwnershipServiceRequestBus::Broadcast(
                    &AzFramework::SliceGameEntityOwnershipServiceRequestBus::Events::CancelDynamicSliceInstantiation, sliceTicket);
            }
            else
            {
                for (AZ::EntityId entity : entitiesInSlice)
                {
                    AZ::EntityBus::MultiHandler::BusDisconnect(entity); // don't care anymore about events from this entity
                    m_entityToTicketMap.erase(entity);
                }

                AzFramework::SliceGameEntityOwnershipServiceRequestBus::Broadcast(
                    &AzFramework::SliceGameEntityOwnershipServiceRequests::DestroyDynamicSliceByEntity, *entitiesInSlice.begin());
            }

            m_ticketToEntitiesMap.erase(foundTicketEntities);
            m_activeTickets.erase(AZStd::remove(m_activeTickets.begin(), m_activeTickets.end(), sliceTicket), m_activeTickets.end());

            // slice destruction is queued, so queue the notification as well.
            AZ::EntityId entityId = GetEntityId();
            AZ::TickBus::QueueFunction(
                [entityId, sliceTicket]() // use copies, in case 'this' is destroyed
                {
                    SpawnerComponentNotificationBus::Event(entityId, &SpawnerComponentNotificationBus::Events::OnSpawnedSliceDestroyed, sliceTicket);
                });
        }
    }

    //=========================================================================
    void SpawnerComponent::DestroyAllSpawnedSlices()
    {
        AZStd::vector<AzFramework::SliceInstantiationTicket> activeTickets = m_activeTickets; // iterate over a copy of the vector
        for (AzFramework::SliceInstantiationTicket& ticket : activeTickets)
        {
            DestroySpawnedSlice(ticket);
        }

        AZ_Assert(m_activeTickets.empty(), "SpawnerComponent::DestroyAllSpawnedSlices - tickets still listed");
        AZ_Assert(m_entityToTicketMap.empty(), "SpawnerComponent::DestroyAllSpawnedSlices - entities still listed");
        AZ_Assert(m_ticketToEntitiesMap.empty(), "SpawnerComponent::DestroyAllSpawnedSlices - ticket entities still listed");
    }

    //=========================================================================
    AZStd::vector<AzFramework::SliceInstantiationTicket> SpawnerComponent::GetCurrentlySpawnedSlices()
    {
        return m_activeTickets;
    }

    //=========================================================================
    bool SpawnerComponent::HasAnyCurrentlySpawnedSlices()
    {
        return !m_activeTickets.empty();
    }

    //=========================================================================
    AZStd::vector<AZ::EntityId> SpawnerComponent::GetCurrentEntitiesFromSpawnedSlice(const AzFramework::SliceInstantiationTicket& ticket)
    {
        AZStd::vector<AZ::EntityId> entities;
        auto foundTicketEntities = m_ticketToEntitiesMap.find(ticket);
        if (foundTicketEntities != m_ticketToEntitiesMap.end())
        {
            const AZStd::unordered_set<AZ::EntityId>& ticketEntities = foundTicketEntities->second;

            AZ_Warning("SpawnerComponent", !ticketEntities.empty(), "SpawnerComponent::GetCurrentEntitiesFromSpawnedSlice - Spawn has not completed, its entities are not available.");

            entities.reserve(ticketEntities.size());
            entities.insert(entities.end(), ticketEntities.begin(), ticketEntities.end());

            // Sort entities so that results are stable.
            AZStd::sort(entities.begin(), entities.end());
        }
        return entities;
    }

    //=========================================================================
    AZStd::vector<AZ::EntityId> SpawnerComponent::GetAllCurrentlySpawnedEntities()
    {
        AZStd::vector<AZ::EntityId> entities;
        entities.reserve(m_entityToTicketMap.size());

        // Return entities in the order their tickets spawned.
        // It's not a requirement, but it seems nice to do.
        for (const AzFramework::SliceInstantiationTicket& ticket : m_activeTickets)
        {
            const AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];
            entities.insert(entities.end(), ticketEntities.begin(), ticketEntities.end());

            // Sort entities from a given ticket, so that results are stable.
            AZStd::sort(entities.end() - ticketEntities.size(), entities.end());
        }

        return entities;
    }

    //=========================================================================
    bool SpawnerComponent::IsReadyToSpawn()
    {
        return m_sliceAsset.IsReady();
    }

    //=========================================================================
    void SpawnerComponent::OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, [[maybe_unused]] const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const AzFramework::SliceInstantiationTicket ticket = (*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());

        SpawnerComponentNotificationBus::Event(GetEntityId(), &SpawnerComponentNotificationBus::Events::OnSpawnBegin, ticket);
    }

    //=========================================================================
    void SpawnerComponent::OnSliceInstantiated([[maybe_unused]] const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const AzFramework::SliceInstantiationTicket ticket = (*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());

        // Stop listening for this ticket (since it's done). We can have have multiple tickets in flight.
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

        AZStd::vector<AZ::EntityId> entityIds;
        entityIds.reserve(entities.size());

        AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];

        for (AZ::Entity* currEntity : entities)
        {
            AZ::EntityId currEntityId = currEntity->GetId();

            entityIds.emplace_back(currEntityId);

            // update internal slice tracking data
            ticketEntities.emplace(currEntityId);
            m_entityToTicketMap.emplace(AZStd::make_pair(currEntityId, ticket));
            AZ::EntityBus::MultiHandler::BusConnect(currEntityId);

            SpawnerComponentNotificationBus::Event(
                GetEntityId(), &SpawnerComponentNotificationBus::Events::OnEntitySpawned, ticket, currEntityId);
        }

        SpawnerComponentNotificationBus::Event(GetEntityId(), &SpawnerComponentNotificationBus::Events::OnSpawnEnd, ticket);

        SpawnerComponentNotificationBus::Event(
            GetEntityId(), &SpawnerComponentNotificationBus::Events::OnEntitiesSpawned, ticket, entityIds);

        // If slice had no entities, clean it up
        if (entities.empty())
        {
            DestroySpawnedSlice(ticket);
        }
    }

    //=========================================================================
    void SpawnerComponent::OnSliceInstantiationFailedOrCanceled(const AZ::Data::AssetId& sliceAssetId, bool canceled)
    {
        const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        // clean it up
        DestroySpawnedSlice(ticket);

        if (!canceled)
        {
            // error msg
            if (sliceAssetId == m_sliceAsset.GetId())
            {
                AZ_Error("SpawnerComponent", false, "Slice %s failed to instantiate", m_sliceAsset.ToString<AZStd::string>().c_str());
            }
            else
            {
                AZ_Error("SpawnerComponent", false, "Slice [id:'%s'] failed to instantiate", sliceAssetId.ToString<AZStd::string>().c_str());
            }
        }
    }

    //=========================================================================
    void SpawnerComponent::OnEntityDestruction(const AZ::EntityId& entityId)
    {
        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);

        auto entityToTicketIter = m_entityToTicketMap.find(entityId);
        if (entityToTicketIter != m_entityToTicketMap.end())
        {
            AzFramework::SliceInstantiationTicket ticket = entityToTicketIter->second;
            m_entityToTicketMap.erase(entityToTicketIter);

            AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];
            ticketEntities.erase(entityId);

            // If this was last entity in the spawn, clean it up
            if (ticketEntities.empty())
            {
                DestroySpawnedSlice(ticket);
            }
        }
    }


    void SpawnerComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();

        m_sliceAsset = asset;  
    }

} // namespace LmbrCentral
