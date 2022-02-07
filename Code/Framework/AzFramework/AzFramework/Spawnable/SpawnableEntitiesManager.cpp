/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableEntitiesManager.h>

namespace AzFramework
{
    template<typename T>
    void SpawnableEntitiesManager::QueueRequest(EntitySpawnTicket& ticket, SpawnablePriority priority, T&& request)
    {
        request.m_ticket = &GetTicketPayload<Ticket>(ticket);
        Queue& queue = priority <= m_highPriorityThreshold ? m_highPriorityQueue : m_regularPriorityQueue;
        {
            AZStd::scoped_lock queueLock(queue.m_pendingRequestMutex);
            request.m_requestId = GetTicketPayload<Ticket>(ticket).m_nextRequestId++;
            queue.m_pendingRequest.push(AZStd::move(request));
        }
    }

    SpawnableEntitiesManager::SpawnableEntitiesManager()
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_defaultSerializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(
            m_defaultSerializeContext, "Failed to retrieve serialization context during construction of the Spawnable Entities Manager.");

        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            AZ::u64 value = aznumeric_caster(m_highPriorityThreshold);
            settingsRegistry->Get(value, "/O3DE/AzFramework/Spawnables/HighPriorityThreshold");
            m_highPriorityThreshold = aznumeric_cast<SpawnablePriority>(AZStd::clamp(value, 0llu, 255llu));
        }
    }

    void SpawnableEntitiesManager::SpawnAllEntities(EntitySpawnTicket& ticket, SpawnAllEntitiesOptionalArgs optionalArgs)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to SpawnAllEntities hasn't been initialized.");

        SpawnAllEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_serializeContext =
            optionalArgs.m_serializeContext == nullptr ? m_defaultSerializeContext : optionalArgs.m_serializeContext;
        queueEntry.m_completionCallback = AZStd::move(optionalArgs.m_completionCallback);
        queueEntry.m_preInsertionCallback = AZStd::move(optionalArgs.m_preInsertionCallback);
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::SpawnEntities(
        EntitySpawnTicket& ticket, AZStd::vector<size_t> entityIndices, SpawnEntitiesOptionalArgs optionalArgs)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to SpawnEntities hasn't been initialized.");

        SpawnEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_entityIndices = AZStd::move(entityIndices);
        queueEntry.m_serializeContext =
            optionalArgs.m_serializeContext == nullptr ? m_defaultSerializeContext : optionalArgs.m_serializeContext;
        queueEntry.m_completionCallback = AZStd::move(optionalArgs.m_completionCallback);
        queueEntry.m_preInsertionCallback = AZStd::move(optionalArgs.m_preInsertionCallback);
        queueEntry.m_referencePreviouslySpawnedEntities = optionalArgs.m_referencePreviouslySpawnedEntities;
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::DespawnAllEntities(EntitySpawnTicket& ticket, DespawnAllEntitiesOptionalArgs optionalArgs)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to DespawnAllEntities hasn't been initialized.");

        DespawnAllEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_completionCallback = AZStd::move(optionalArgs.m_completionCallback);
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::DespawnEntity(AZ::EntityId entityId, EntitySpawnTicket& ticket, DespawnEntityOptionalArgs optionalArgs)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to DespawnEntity hasn't been initialized.");

        DespawnEntityCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_entityId = entityId;
        queueEntry.m_completionCallback = AZStd::move(optionalArgs.m_completionCallback);
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::RetrieveEntitySpawnTicket(EntitySpawnTicket::Id entitySpawnTicketId, RetrieveEntitySpawnTicketCallback callback)
    {
        if (entitySpawnTicketId == 0)
        {
            AZ_Assert(false, "Ticket id provided to RetrieveEntitySpawnTicket is invalid.");
            return;
        }

        AZStd::scoped_lock lock(m_entitySpawnTicketMapMutex);
        auto entitySpawnTicketIterator = m_entitySpawnTicketMap.find(entitySpawnTicketId);
        if (entitySpawnTicketIterator == m_entitySpawnTicketMap.end())
        {
            AZ_Assert(false, "The EntitySpawnTicket corresponding to id '%lu' cannot be found", entitySpawnTicketId);
            return;
        }
        callback(entitySpawnTicketIterator->second);
    }

    void SpawnableEntitiesManager::ReloadSpawnable(
        EntitySpawnTicket& ticket, AZ::Data::Asset<Spawnable> spawnable, ReloadSpawnableOptionalArgs optionalArgs)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to ReloadSpawnable hasn't been initialized.");

        ReloadSpawnableCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_spawnable = AZStd::move(spawnable);
        queueEntry.m_serializeContext =
            optionalArgs.m_serializeContext == nullptr ? m_defaultSerializeContext : optionalArgs.m_serializeContext;
        queueEntry.m_completionCallback = AZStd::move(optionalArgs.m_completionCallback);
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::ListEntities(
        EntitySpawnTicket& ticket, ListEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs)
    {
        AZ_Assert(listCallback, "ListEntities called on spawnable entities without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to ListEntities hasn't been initialized.");

        ListEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_listCallback = AZStd::move(listCallback);
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::ListIndicesAndEntities(
        EntitySpawnTicket& ticket, ListIndicesEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs)
    {
        AZ_Assert(listCallback, "ListEntities called on spawnable entities without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to ListEntities hasn't been initialized.");

        ListIndicesEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_listCallback = AZStd::move(listCallback);
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::ClaimEntities(
        EntitySpawnTicket& ticket, ClaimEntitiesCallback listCallback, ClaimEntitiesOptionalArgs optionalArgs)
    {
        AZ_Assert(listCallback, "ClaimEntities called on spawnable entities without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to ClaimEntities hasn't been initialized.");

        ClaimEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_listCallback = AZStd::move(listCallback);
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::Barrier(EntitySpawnTicket& ticket, BarrierCallback completionCallback, BarrierOptionalArgs optionalArgs)
    {
        AZ_Assert(completionCallback, "Barrier on spawnable entities called without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to Barrier hasn't been initialized.");

        BarrierCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        QueueRequest(ticket, optionalArgs.m_priority, AZStd::move(queueEntry));
    }

    auto SpawnableEntitiesManager::ProcessQueue(CommandQueuePriority priority) -> CommandQueueStatus
    {
        CommandQueueStatus result = CommandQueueStatus::NoCommandsLeft;
        if ((priority & CommandQueuePriority::High) == CommandQueuePriority::High)
        {
            if (ProcessQueue(m_highPriorityQueue) == CommandQueueStatus::HasCommandsLeft)
            {
                result = CommandQueueStatus::HasCommandsLeft;
            }
        }
        if ((priority & CommandQueuePriority::Regular) == CommandQueuePriority::Regular)
        {
            if (ProcessQueue(m_regularPriorityQueue) == CommandQueueStatus::HasCommandsLeft)
            {
                result = CommandQueueStatus::HasCommandsLeft;
            }
        }
        return result;
    }

    auto SpawnableEntitiesManager::ProcessQueue(Queue& queue) -> CommandQueueStatus
    {
        // Process delayed requests first.
        // Only process the requests that are currently in this queue, not the ones that could be re-added if they still can't complete.
        size_t delayedSize = queue.m_delayed.size();
        for (size_t i = 0; i < delayedSize; ++i)
        {
            Requests& request = queue.m_delayed.front();
            bool result = AZStd::visit(
                [this](auto&& args) -> bool
                {
                    return ProcessRequest(args);
                },
                request);
            if (!result)
            {
                queue.m_delayed.emplace_back(AZStd::move(request));
            }
            queue.m_delayed.pop_front();
        }

        // Process newly added requests.
        while (true)
        {
            AZStd::queue<Requests> pendingRequestQueue;
            {
                AZStd::scoped_lock queueLock(queue.m_pendingRequestMutex);
                queue.m_pendingRequest.swap(pendingRequestQueue);
            }

            if (!pendingRequestQueue.empty())
            {
                while (!pendingRequestQueue.empty())
                {
                    Requests& request = pendingRequestQueue.front();
                    bool result = AZStd::visit(
                        [this](auto&& args) -> bool
                        {
                            return ProcessRequest(args);
                        },
                        request);
                    if (!result)
                    {
                        queue.m_delayed.emplace_back(AZStd::move(request));
                    }
                    pendingRequestQueue.pop();
                }
            }
            else
            {
                break;
            }
        };

        return queue.m_delayed.empty() ? CommandQueueStatus::NoCommandsLeft : CommandQueueStatus::HasCommandsLeft;
    }

    AZStd::pair<EntitySpawnTicket::Id, void*> SpawnableEntitiesManager::CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable)
    {
        static AZStd::atomic_uint32_t idCounter { 1 };

        auto result = aznew Ticket();
        result->m_spawnable = AZStd::move(spawnable);
        
        return AZStd::make_pair<EntitySpawnTicket::Id, void*>(idCounter++, result);
    }

    void SpawnableEntitiesManager::DestroyTicket(void* ticket)
    {
        DestroyTicketCommand queueEntry;
        queueEntry.m_ticket = reinterpret_cast<Ticket*>(ticket);
        {
            AZStd::scoped_lock queueLock(m_regularPriorityQueue.m_pendingRequestMutex);
            queueEntry.m_requestId = reinterpret_cast<Ticket*>(ticket)->m_nextRequestId++;
            m_regularPriorityQueue.m_pendingRequest.push(AZStd::move(queueEntry));
        }
    }

    AZ::Entity* SpawnableEntitiesManager::CloneSingleEntity(const AZ::Entity& entityTemplate,
        EntityIdMap& templateToCloneMap, AZ::SerializeContext& serializeContext)
    {
        // If the same ID gets remapped more than once, preserve the original remapping instead of overwriting it.
        constexpr bool allowDuplicateIds = false;

        return AZ::IdUtils::Remapper<AZ::EntityId, allowDuplicateIds>::CloneObjectAndGenerateNewIdsAndFixRefs(
                &entityTemplate, templateToCloneMap, &serializeContext);
    }

    void SpawnableEntitiesManager::InitializeEntityIdMappings(
        const Spawnable::EntityList& entities, EntityIdMap& idMap, AZStd::unordered_set<AZ::EntityId>& previouslySpawned)
    {
        // Make sure we don't have any previous data lingering around.
        idMap.clear();
        previouslySpawned.clear();

        idMap.reserve(entities.size());
        previouslySpawned.reserve(entities.size());

        for (auto& entity : entities)
        {
            idMap.emplace(entity->GetId(), AZ::Entity::MakeId());
        }
    }

    void SpawnableEntitiesManager::RefreshEntityIdMapping(
        const AZ::EntityId& entityId, EntityIdMap& idMap, AZStd::unordered_set<AZ::EntityId>& previouslySpawned)
    {
        if (previouslySpawned.contains(entityId))
        {
            // This entity has already been spawned at least once before, so we need to generate a new id for it and
            // preserve the new id to fix up any future entity references to this entity.
            idMap[entityId] = AZ::Entity::MakeId();
        }
        else
        {
            // This entity hasn't been spawned yet, so use the first id we've already generated for this entity and mark
            // it as spawned so we know not to reuse this id next time.
            previouslySpawned.emplace(entityId);
        }
    }


    bool SpawnableEntitiesManager::ProcessRequest(SpawnAllEntitiesCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        if (ticket.m_spawnable.IsReady() && request.m_requestId == ticket.m_currentRequestId)
        {
            AZStd::vector<AZ::Entity*>& spawnedEntities = ticket.m_spawnedEntities;
            AZStd::vector<size_t>& spawnedEntityIndices = ticket.m_spawnedEntityIndices;

            // Keep track how many entities there were in the array initially
            size_t spawnedEntitiesInitialCount = spawnedEntities.size();

            // These are 'template' entities we'll be cloning from
            const Spawnable::EntityList& entitiesToSpawn = ticket.m_spawnable->GetEntities();
            size_t entitiesToSpawnSize = entitiesToSpawn.size();

            // Reserve buffers
            spawnedEntities.reserve(spawnedEntities.size() + entitiesToSpawnSize);
            spawnedEntityIndices.reserve(spawnedEntityIndices.size() + entitiesToSpawnSize);

            // Pre-generate the full set of entity id to new entity id mappings, so that during the clone operation below,
            // any entity references that point to a not-yet-cloned entity will still get their ids remapped correctly.
            // We clear out and regenerate the set of IDs on every SpawnAllEntities call, because presumably every entity reference
            // in every entity we're about to instantiate is intended to point to an entity in our newly-instantiated batch, regardless
            // of spawn order.  If we didn't clear out the map, it would be possible for some entities here to have references to
            // previously-spawned entities from a previous SpawnEntities or SpawnAllEntities call.
            InitializeEntityIdMappings(entitiesToSpawn, ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

            for (size_t i = 0; i < entitiesToSpawnSize; ++i)
            {
                // If this entity has previously been spawned, give it a new id in the reference map
                RefreshEntityIdMapping(entitiesToSpawn[i].get()->GetId(), ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                AZ::Entity* clone = CloneSingleEntity(*entitiesToSpawn[i], ticket.m_entityIdReferenceMap, *request.m_serializeContext);
                AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");

                spawnedEntities.emplace_back(clone);
                spawnedEntityIndices.push_back(i);
            }

            // loadAll is true if every entity has been spawned only once
            ticket.m_loadAll = (spawnedEntities.size() == entitiesToSpawnSize);
            
            // Let other systems know about newly spawned entities for any pre-processing before adding to the scene/game context.
            if (request.m_preInsertionCallback)
            {
                request.m_preInsertionCallback(request.m_ticketId, SpawnableEntityContainerView(
                        ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
            }

            // Add to the game context, now the entities are active
            for (auto it = ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount; it != ticket.m_spawnedEntities.end(); ++it)
            {
                (*it)->SetSpawnTicketId(request.m_ticketId);
                GameEntityContextRequestBus::Broadcast(&GameEntityContextRequestBus::Events::AddGameEntity, *it);
            }

            // Let other systems know about newly spawned entities for any post-processing after adding to the scene/game context.
            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId, SpawnableConstEntityContainerView(
                        ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
            }

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(SpawnEntitiesCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        if (ticket.m_spawnable.IsReady() && request.m_requestId == ticket.m_currentRequestId)
        {
            AZStd::vector<AZ::Entity*>& spawnedEntities = ticket.m_spawnedEntities;
            AZStd::vector<size_t>& spawnedEntityIndices = ticket.m_spawnedEntityIndices;
            AZ_Assert(
                spawnedEntities.size() == spawnedEntityIndices.size(),
                "The indices for the spawned entities has gone out of sync with the entities.");

            // Keep track of how many entities there were in the array initially
            size_t spawnedEntitiesInitialCount = spawnedEntities.size();

            // These are 'template' entities we'll be cloning from
            const Spawnable::EntityList& entitiesToSpawn = ticket.m_spawnable->GetEntities();
            size_t entitiesToSpawnSize = request.m_entityIndices.size();

            if (ticket.m_entityIdReferenceMap.empty() || !request.m_referencePreviouslySpawnedEntities)
            {
                // This map keeps track of ids from template (spawnable) to clone (instance) allowing patch ups of fields referring
                // to entityIds outside of a given entity.
                // We pre-generate the full set of entity id to new entity id mappings, so that during the clone operation below,
                // any entity references that point to a not-yet-cloned entity will still get their ids remapped correctly.
                // By default, we only initialize this map once because it needs to persist across multiple SpawnEntities calls, so
                // that reference fixups work even when the entity being referenced is spawned in a different SpawnEntities
                // (or SpawnAllEntities) call.
                // However, the caller can also choose to reset the map by passing in "m_referencePreviouslySpawnedEntities = false".
                InitializeEntityIdMappings(entitiesToSpawn, ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);
            }

            spawnedEntities.reserve(spawnedEntities.size() + entitiesToSpawnSize);
            spawnedEntityIndices.reserve(spawnedEntityIndices.size() + entitiesToSpawnSize);

            for (size_t index : request.m_entityIndices)
            {
                if (index < entitiesToSpawn.size())
                {
                    // If this entity has previously been spawned, give it a new id in the reference map
                    RefreshEntityIdMapping(
                        entitiesToSpawn[index].get()->GetId(), ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                    AZ::Entity* clone =
                        CloneSingleEntity(*entitiesToSpawn[index], ticket.m_entityIdReferenceMap, *request.m_serializeContext);
                    AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");

                    spawnedEntities.push_back(clone);
                    spawnedEntityIndices.push_back(index);
                }
            }
            ticket.m_loadAll = false;

            // Let other systems know about newly spawned entities for any pre-processing before adding to the scene/game context.
            if (request.m_preInsertionCallback)
            {
                request.m_preInsertionCallback(request.m_ticketId, SpawnableEntityContainerView(
                        ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
            }

            // Add to the game context, now the entities are active
            for (auto it = ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount; it != ticket.m_spawnedEntities.end(); ++it)
            {
                (*it)->SetSpawnTicketId(request.m_ticketId);
                GameEntityContextRequestBus::Broadcast(&GameEntityContextRequestBus::Events::AddGameEntity, *it);
            }

            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId, SpawnableConstEntityContainerView(
                    ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
            }

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(DespawnAllEntitiesCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            for (AZ::Entity* entity : ticket.m_spawnedEntities)
            {
                if (entity != nullptr)
                {
                    // Setting it to 0 is needed to avoid the infite loop between GameEntityContext and SpawnableEntitiesManager.
                    entity->SetSpawnTicketId(0);
                    GameEntityContextRequestBus::Broadcast(
                        &GameEntityContextRequestBus::Events::DestroyGameEntity, entity->GetId());
                }
            }

            ticket.m_spawnedEntities.clear();
            ticket.m_spawnedEntityIndices.clear();

            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId);
            }

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(DespawnEntityCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            AZStd::vector<AZ::Entity*>& spawnedEntities = request.m_ticket->m_spawnedEntities;
            for (auto entityIterator = spawnedEntities.begin(); entityIterator != spawnedEntities.end(); ++entityIterator)
            {
                if (*entityIterator != nullptr && (*entityIterator)->GetId() == request.m_entityId)
                {
                    // Setting it to 0 is needed to avoid the infite loop between GameEntityContext and SpawnableEntitiesManager.
                    (*entityIterator)->SetSpawnTicketId(0);
                    GameEntityContextRequestBus::Broadcast(
                        &GameEntityContextRequestBus::Events::DestroyGameEntity, (*entityIterator)->GetId());
                    AZStd::iter_swap(entityIterator, spawnedEntities.rbegin());
                    spawnedEntities.pop_back();
                    break;
                }
            }

            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId);
            }

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ReloadSpawnableCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        AZ_Assert(ticket.m_spawnable.GetId() == request.m_spawnable.GetId(),
            "Spawnable is being reloaded, but the provided spawnable has a different asset id. "
            "This will likely result in unexpected entities being created.");
        if (ticket.m_spawnable.IsReady() && request.m_requestId == ticket.m_currentRequestId)
        {
            // Delete the original entities.
            for (AZ::Entity* entity : ticket.m_spawnedEntities)
            {
                if (entity != nullptr)
                {
                    // Setting it to 0 is needed to avoid the infite loop between GameEntityContext and SpawnableEntitiesManager.
                    entity->SetSpawnTicketId(0);
                    GameEntityContextRequestBus::Broadcast(
                        &GameEntityContextRequestBus::Events::DestroyGameEntity, entity->GetId());
                }
            }

            // Rebuild the list of entities.
            ticket.m_spawnedEntities.clear();
            const Spawnable::EntityList& entities = request.m_spawnable->GetEntities();

            // Pre-generate the full set of entity id to new entity id mappings, so that during the clone operation below,
            // any entity references that point to a not-yet-cloned entity will still get their ids remapped correctly.
            // This map is intentionally cleared out and regenerated here to ensure that we're starting fresh with mappings that
            // match the new set of template entities getting spawned.
            InitializeEntityIdMappings(entities, ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

            if (ticket.m_loadAll)
            {
                // The new spawnable may have a different number of entities and since the intent of the user was
                // to spawn every entity, simply start over.
                ticket.m_spawnedEntityIndices.clear();
                size_t entitiesToSpawnSize = entities.size();

                for (size_t i = 0; i < entitiesToSpawnSize; ++i)
                {
                    // If this entity has previously been spawned, give it a new id in the reference map
                    RefreshEntityIdMapping(entities[i].get()->GetId(), ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                    AZ::Entity* clone = CloneSingleEntity(*entities[i], ticket.m_entityIdReferenceMap, *request.m_serializeContext);
                    AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");

                    ticket.m_spawnedEntities.push_back(clone);
                    ticket.m_spawnedEntityIndices.push_back(i);
                }
            }
            else
            {
                size_t entitiesSize = entities.size();

                for (size_t index : ticket.m_spawnedEntityIndices)
                {
                    // It's possible for the new spawnable to have a different number of entities, so guard against this.
                    // It's also possible that the entities have moved within the spawnable to a new index. This can't be
                    // detected and will result in the incorrect entities being spawned.
                    if (index < entitiesSize)
                    {
                        // If this entity has previously been spawned, give it a new id in the reference map
                        RefreshEntityIdMapping(entities[index].get()->GetId(), ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                        AZ::Entity* clone = CloneSingleEntity(*entities[index], ticket.m_entityIdReferenceMap, *request.m_serializeContext);
                        AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");
                        ticket.m_spawnedEntities.push_back(clone);
                    }
                }
            }
            ticket.m_spawnable = AZStd::move(request.m_spawnable);

            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId, SpawnableConstEntityContainerView(
                    ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));
            }

            ticket.m_currentRequestId++;

            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ListEntitiesCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            request.m_listCallback(request.m_ticketId, SpawnableConstEntityContainerView(
                ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));
            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ListIndicesEntitiesCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            AZ_Assert(
                ticket.m_spawnedEntities.size() == ticket.m_spawnedEntityIndices.size(),
                "Entities and indices on spawnable ticket have gone out of sync.");
            request.m_listCallback(request.m_ticketId, SpawnableConstIndexEntityContainerView(
                    ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntityIndices.begin(), ticket.m_spawnedEntities.size()));
            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ClaimEntitiesCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            request.m_listCallback(request.m_ticketId, SpawnableEntityContainerView(
                ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));

            ticket.m_spawnedEntities.clear();
            ticket.m_spawnedEntityIndices.clear();

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(BarrierCommand& request)
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId);
            }

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(DestroyTicketCommand& request)
    {
        if (request.m_requestId == request.m_ticket->m_currentRequestId)
        {
            for (AZ::Entity* entity : request.m_ticket->m_spawnedEntities)
            {
                if (entity != nullptr)
                {
                    // Setting it to 0 is needed to avoid the infite loop between GameEntityContext and SpawnableEntitiesManager.
                    entity->SetSpawnTicketId(0);
                    GameEntityContextRequestBus::Broadcast(
                        &GameEntityContextRequestBus::Events::DestroyGameEntity, entity->GetId());
                }
            }
            delete request.m_ticket;

            return true;
        }
        else
        {
            return false;
        }
    }
} // namespace AzFramework
