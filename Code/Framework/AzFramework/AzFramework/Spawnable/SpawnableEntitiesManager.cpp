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
            auto& payload = GetTicketPayload<Ticket>(ticket);
            request.m_requestId = payload.m_nextRequestId++;
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

    SpawnableEntitiesManager::~SpawnableEntitiesManager()
    {
        AZ_Assert(m_totalTickets == 0, "Shutting down the Spawnable Entities Manager while there are still active Spawnable Tickets.");
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
        EntitySpawnTicket& ticket, AZStd::vector<uint32_t> entityIndices, SpawnEntitiesOptionalArgs optionalArgs)
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

    void SpawnableEntitiesManager::RetrieveTicket(
        EntitySpawnTicket::Id ticketId, RetrieveEntitySpawnTicketCallback callback, RetrieveTicketOptionalArgs optionalArgs)
    {
        if (ticketId == 0)
        {
            AZ_Assert(false, "Ticket id provided to RetrieveEntitySpawnTicket is invalid.");
            return;
        }

        RetrieveTicketCommand queueEntry;
        queueEntry.m_ticketId = ticketId;
        queueEntry.m_callback = AZStd::move(callback);

        Queue& queue = optionalArgs.m_priority <= m_highPriorityThreshold ? m_highPriorityQueue : m_regularPriorityQueue;
        {
            AZStd::scoped_lock queueLock(queue.m_pendingRequestMutex);
            queue.m_pendingRequest.push(AZStd::move(queueEntry));
        }
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

    void SpawnableEntitiesManager::UpdateEntityAliasTypes(
        EntitySpawnTicket& ticket,
        AZStd::vector<EntityAliasTypeChange> updatedAliases,
        UpdateEntityAliasTypesOptionalArgs optionalArgs)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to ReloadSpawnable hasn't been initialized.");

        UpdateEntityAliasTypesCommand queueEntry;
        queueEntry.m_entityAliases = AZStd::move(updatedAliases);
        queueEntry.m_ticketId = ticket.GetId();
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

    void SpawnableEntitiesManager::LoadBarrier(
        EntitySpawnTicket& ticket, BarrierCallback completionCallback, LoadBarrierOptionalArgs optionalArgs)
    {
        AZ_Assert(completionCallback, "Load barrier on spawnable entities called without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to LoadBarrier hasn't been initialized.");

        LoadBarrierCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        queueEntry.m_checkAliasSpawnables = optionalArgs.m_checkAliasSpawnables;
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
            CommandResult result = AZStd::visit(
                [this](auto&& args) -> CommandResult
                {
                    return ProcessRequest(args);
                },
                request);
            if (result == CommandResult::Requeue)
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
                    CommandResult result = AZStd::visit(
                        [this](auto&& args) -> CommandResult
                        {
                            return ProcessRequest(args);
                        },
                        request);
                    if (result == CommandResult::Requeue)
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

    void* SpawnableEntitiesManager::CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable)
    {
        static AZStd::atomic_uint32_t idCounter { 1 };

        auto result = aznew Ticket();
        result->m_spawnable = AZStd::move(spawnable);
        result->m_ticketId = idCounter++;

        m_totalTickets++;
        m_ticketsPendingRegistration++;
        AZ_Assert(
            m_ticketsPendingRegistration <= m_totalTickets,
            "There are less total entity spawn tickets than there are tickets pending registration in the SpawnableEntitiesManager.");

        RegisterTicketCommand queueEntry;
        queueEntry.m_ticket = result;
        {
            AZStd::scoped_lock queueLock(m_highPriorityQueue.m_pendingRequestMutex);
            queueEntry.m_requestId = result->m_nextRequestId++;
            m_highPriorityQueue.m_pendingRequest.push(AZStd::move(queueEntry));
        }
        
        return result;
    }

    void SpawnableEntitiesManager::IncrementTicketReference(void* ticket)
    {
        reinterpret_cast<Ticket*>(ticket)->m_referenceCount++;
    }

    void SpawnableEntitiesManager::DecrementTicketReference(void* ticket)
    {
        auto ticketInstance = reinterpret_cast<Ticket*>(ticket);
        if (ticketInstance->m_referenceCount-- == 1)
        {
            DestroyTicketCommand queueEntry;
            queueEntry.m_ticket = ticketInstance;
            {
                AZStd::scoped_lock queueLock(m_regularPriorityQueue.m_pendingRequestMutex);
                queueEntry.m_requestId = ticketInstance->m_nextRequestId++;
                m_regularPriorityQueue.m_pendingRequest.push(AZStd::move(queueEntry));
            }
        }
    }

    EntitySpawnTicket::Id SpawnableEntitiesManager::GetTicketId(void* ticket)
    {
        return reinterpret_cast<Ticket*>(ticket)->m_ticketId;
    }

    const AZ::Data::Asset<Spawnable>& SpawnableEntitiesManager::GetSpawnableOnTicket(void* ticket)
    {
        return reinterpret_cast<Ticket*>(ticket)->m_spawnable;
    }

    AZ::Entity* SpawnableEntitiesManager::CloneSingleEntity(const AZ::Entity& entityPrototype,
        EntityIdMap& prototypeToCloneMap, AZ::SerializeContext& serializeContext)
    {
        // If the same ID gets remapped more than once, preserve the original remapping instead of overwriting it.
        constexpr bool allowDuplicateIds = false;

        return AZ::IdUtils::Remapper<AZ::EntityId, allowDuplicateIds>::CloneObjectAndGenerateNewIdsAndFixRefs(
            &entityPrototype, prototypeToCloneMap, &serializeContext);
    }

    AZ::Entity* SpawnableEntitiesManager::CloneSingleAliasedEntity(
        const AZ::Entity& entityPrototype,
        const Spawnable::EntityAlias& alias,
        EntityIdMap& prototypeToCloneMap,
        AZ::Entity* previouslySpawnedEntity,
        AZ::SerializeContext& serializeContext)
    {
        AZ::Entity* clone = nullptr;
        switch (alias.m_aliasType)
        {
        case Spawnable::EntityAliasType::Original:
            // Behave as the original version.
            clone = CloneSingleEntity(entityPrototype, prototypeToCloneMap, serializeContext);
            AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");
            return clone;
        case Spawnable::EntityAliasType::Disable:
            // Do nothing.
            return nullptr;
        case Spawnable::EntityAliasType::Replace:
            clone = CloneSingleEntity(*(alias.m_spawnable->GetEntities()[alias.m_targetIndex]), prototypeToCloneMap, serializeContext);
            AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");
            return clone;
        case Spawnable::EntityAliasType::Additional:
            // The asset handler will have sorted and inserted a Spawnable::EntityAliasType::Original, so the just
            // spawn the additional entity.
            clone = CloneSingleEntity(*(alias.m_spawnable->GetEntities()[alias.m_targetIndex]), prototypeToCloneMap, serializeContext);
            AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");
            return clone;
        case Spawnable::EntityAliasType::Merge:
            AZ_Assert(previouslySpawnedEntity != nullptr, "Merging components but there's no entity to add to yet.");
            AppendComponents(
                *previouslySpawnedEntity, alias.m_spawnable->GetEntities()[alias.m_targetIndex]->GetComponents(), prototypeToCloneMap,
                serializeContext);
            return nullptr;
        default:
            AZ_Assert(false, "Unsupported spawnable entity alias type: %i", alias.m_aliasType);
            return nullptr;
        }
    }

    void SpawnableEntitiesManager::AppendComponents(
        AZ::Entity& target,
        const AZ::Entity::ComponentArrayType& componentPrototypes,
        EntityIdMap& prototypeToCloneMap,
        AZ::SerializeContext& serializeContext)
    {
        // Only components are added and entities are looked up so no duplicate entity ids should be encountered.
        constexpr bool allowDuplicateIds = false;

        for (const AZ::Component* component : componentPrototypes)
        {
            AZ::Component* clone = AZ::IdUtils::Remapper<AZ::EntityId, allowDuplicateIds>::CloneObjectAndGenerateNewIdsAndFixRefs(
                component, prototypeToCloneMap, &serializeContext);
            AZ_Assert(clone, "Unable to clone component for entity '%s' (%zu).", target.GetName().c_str(), target.GetId());
            [[maybe_unused]] bool result = target.AddComponent(clone);
            AZ_Assert(result, "Unable to add cloned component to entity '%s' (%zu).", target.GetName().c_str(), target.GetId());
        }
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

    auto SpawnableEntitiesManager::ProcessRequest(SpawnAllEntitiesCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (ticket.m_spawnable.IsReady() && request.m_requestId == ticket.m_currentRequestId)
        {
            if (Spawnable::EntityAliasConstVisitor aliases = ticket.m_spawnable->TryGetAliasesConst();
                aliases.IsValid() && aliases.AreAllSpawnablesReady())
            {
                AZStd::vector<AZ::Entity*>& spawnedEntities = ticket.m_spawnedEntities;
                AZStd::vector<uint32_t>& spawnedEntityIndices = ticket.m_spawnedEntityIndices;

                // Keep track how many entities there were in the array initially
                size_t spawnedEntitiesInitialCount = spawnedEntities.size();

                // These are 'prototype' entities we'll be cloning from
                const Spawnable::EntityList& entitiesToSpawn = ticket.m_spawnable->GetEntities();
                uint32_t entitiesToSpawnSize = aznumeric_caster(entitiesToSpawn.size());

                // Reserve buffers
                spawnedEntities.reserve(spawnedEntities.size() + entitiesToSpawnSize);
                spawnedEntityIndices.reserve(spawnedEntityIndices.size() + entitiesToSpawnSize);

                // Pre-generate the full set of entity-id-to-new-entity-id mappings, so that during the clone operation below,
                // any entity references that point to a not-yet-cloned entity will still get their ids remapped correctly.
                // We clear out and regenerate the set of IDs on every SpawnAllEntities call, because presumably every entity reference
                // in every entity we're about to instantiate is intended to point to an entity in our newly-instantiated batch, regardless
                // of spawn order.  If we didn't clear out the map, it would be possible for some entities here to have references to
                // previously-spawned entities from a previous SpawnEntities or SpawnAllEntities call.
                InitializeEntityIdMappings(entitiesToSpawn, ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                auto aliasIt = aliases.begin();
                auto aliasEnd = aliases.end();
                if (aliasIt == aliasEnd)
                {
                    for (uint32_t i = 0; i < entitiesToSpawnSize; ++i)
                    {
                        // If this entity has previously been spawned, give it a new id in the reference map
                        RefreshEntityIdMapping(
                            entitiesToSpawn[i].get()->GetId(), ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                        spawnedEntities.emplace_back(
                            CloneSingleEntity(*entitiesToSpawn[i], ticket.m_entityIdReferenceMap, *request.m_serializeContext));
                        spawnedEntityIndices.push_back(i);
                    }
                }
                else
                {
                    for (uint32_t i = 0; i < entitiesToSpawnSize; ++i)
                    {
                        // If this entity has previously been spawned, give it a new id in the reference map
                        RefreshEntityIdMapping(
                            entitiesToSpawn[i].get()->GetId(), ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                        if (aliasIt == aliasEnd || aliasIt->m_sourceIndex != i)
                        {
                            spawnedEntities.emplace_back(
                                CloneSingleEntity(*entitiesToSpawn[i], ticket.m_entityIdReferenceMap, *request.m_serializeContext));
                            spawnedEntityIndices.push_back(i);
                        }
                        else
                        {
                            // The list of entities has already been sorted and optimized (See SpawnableEntitiesAliasList:Optimize) so can
                            // be safely executed in order without risking an invalid state.
                            AZ::Entity* previousEntity = nullptr;
                            do
                            {
                                AZ::Entity* clone = CloneSingleAliasedEntity(
                                    *entitiesToSpawn[i], *aliasIt, ticket.m_entityIdReferenceMap, previousEntity,
                                    *request.m_serializeContext);
                                previousEntity = clone;
                                if (clone)
                                {
                                    spawnedEntities.emplace_back(clone);
                                    spawnedEntityIndices.push_back(i);
                                }
                                ++aliasIt;
                            } while (aliasIt != aliasEnd && aliasIt->m_sourceIndex == i);
                        }
                    }
                }

                // There were no initial entities then the ticket now holds exactly all entities. If there were already entities then
                // a new set are not added so it no longer holds exactly the number of entities.
                ticket.m_loadAll = spawnedEntitiesInitialCount == 0;

                auto newEntitiesBegin = ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount;
                auto newEntitiesEnd = ticket.m_spawnedEntities.end();
                // Let other systems know about newly spawned entities for any pre-processing before adding to the scene/game context.
                if (request.m_preInsertionCallback)
                {
                    request.m_preInsertionCallback(request.m_ticketId, SpawnableEntityContainerView(newEntitiesBegin, newEntitiesEnd));
                }

                // Add to the game context, now the entities are active
                for (auto it = newEntitiesBegin; it != newEntitiesEnd; ++it)
                {
                    AZ::Entity* clone = (*it);
                    clone->SetEntitySpawnTicketId(request.m_ticketId);
                    GameEntityContextRequestBus::Broadcast(&GameEntityContextRequestBus::Events::AddGameEntity, clone);
                }

                // Let other systems know about newly spawned entities for any post-processing after adding to the scene/game context.
                if (request.m_completionCallback)
                {
                    request.m_completionCallback(request.m_ticketId, SpawnableConstEntityContainerView(newEntitiesBegin, newEntitiesEnd));
                }

                ticket.m_currentRequestId++;
                return CommandResult::Executed;
            }
        }
        return CommandResult::Requeue;
    }

    auto SpawnableEntitiesManager::ProcessRequest(SpawnEntitiesCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (ticket.m_spawnable.IsReady() && request.m_requestId == ticket.m_currentRequestId)
        {
            if (Spawnable::EntityAliasConstVisitor aliases = ticket.m_spawnable->TryGetAliasesConst();
                aliases.IsValid() && aliases.AreAllSpawnablesReady())
            {
                AZStd::vector<AZ::Entity*>& spawnedEntities = ticket.m_spawnedEntities;
                AZStd::vector<uint32_t>& spawnedEntityIndices = ticket.m_spawnedEntityIndices;
                AZ_Assert(
                    spawnedEntities.size() == spawnedEntityIndices.size(),
                    "The indices for the spawned entities has gone out of sync with the entities.");

                // Keep track of how many entities there were in the array initially
                size_t spawnedEntitiesInitialCount = spawnedEntities.size();

                // These are 'prototype' entities we'll be cloning from
                const Spawnable::EntityList& entitiesToSpawn = ticket.m_spawnable->GetEntities();
                size_t entitiesToSpawnSize = request.m_entityIndices.size();

                if (ticket.m_entityIdReferenceMap.empty() || !request.m_referencePreviouslySpawnedEntities)
                {
                    // This map keeps track of ids from prototype (spawnable) to clone (instance) allowing patch ups of fields referring
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

                auto aliasBegin = aliases.begin();
                auto aliasEnd = aliases.end();
                if (aliasBegin == aliasEnd)
                {
                    for (uint32_t index : request.m_entityIndices)
                    {
                        if (index < entitiesToSpawn.size())
                        {
                            // If this entity has previously been spawned, give it a new id in the reference map
                            RefreshEntityIdMapping(
                                entitiesToSpawn[index].get()->GetId(), ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                            spawnedEntities.push_back(
                                CloneSingleEntity(*entitiesToSpawn[index], ticket.m_entityIdReferenceMap, *request.m_serializeContext));
                            spawnedEntityIndices.push_back(index);
                        }
                    }
                }
                else
                {
                    for (uint32_t index : request.m_entityIndices)
                    {
                        if (index < entitiesToSpawn.size())
                        {
                            // If this entity has previously been spawned, give it a new id in the reference map
                            RefreshEntityIdMapping(
                                entitiesToSpawn[index].get()->GetId(), ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

                            auto aliasIt = AZStd::lower_bound(
                                aliasBegin, aliasEnd, index,
                                [](const Spawnable::EntityAlias& lhs, uint32_t rhs)
                                {
                                    return lhs.m_sourceIndex < rhs;
                                });

                            if (aliasIt == aliasEnd || aliasIt->m_sourceIndex != index)
                            {
                                spawnedEntities.emplace_back(
                                    CloneSingleEntity(*entitiesToSpawn[index], ticket.m_entityIdReferenceMap, *request.m_serializeContext));
                                spawnedEntityIndices.push_back(index);
                            }
                            else
                            {
                                // The list of entities has already been sorted and optimized (See SpawnableEntitiesAliasList:Optimize) so
                                // can be safely executed in order without risking an invalid state.
                                AZ::Entity* previousEntity = nullptr;
                                do
                                {
                                    AZ::Entity* clone = CloneSingleAliasedEntity(
                                        *entitiesToSpawn[index], *aliasIt, ticket.m_entityIdReferenceMap, previousEntity,
                                        *request.m_serializeContext);
                                    previousEntity = clone;
                                    if (clone)
                                    {
                                        spawnedEntities.emplace_back(clone);
                                        spawnedEntityIndices.push_back(index);
                                    }

                                    ++aliasIt;
                                } while (aliasIt != aliasEnd && aliasIt->m_sourceIndex == index);
                            }
                        }
                    }
                }
                ticket.m_loadAll = false;

                // Let other systems know about newly spawned entities for any pre-processing before adding to the scene/game context.
                if (request.m_preInsertionCallback)
                {
                    request.m_preInsertionCallback(
                        request.m_ticketId,
                        SpawnableEntityContainerView(
                            ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
                }

                // Add to the game context, now the entities are active
                for (auto it = ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount; it != ticket.m_spawnedEntities.end(); ++it)
                {
                    AZ::Entity* clone = (*it);
                    clone->SetEntitySpawnTicketId(request.m_ticketId);
                    GameEntityContextRequestBus::Broadcast(&GameEntityContextRequestBus::Events::AddGameEntity, *it);
                }

                if (request.m_completionCallback)
                {
                    request.m_completionCallback(
                        request.m_ticketId,
                        SpawnableConstEntityContainerView(
                            ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
                }

                ticket.m_currentRequestId++;
                return CommandResult::Executed;
            }
        }
        return CommandResult::Requeue;
    }

    auto SpawnableEntitiesManager::ProcessRequest(DespawnAllEntitiesCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            for (AZ::Entity* entity : ticket.m_spawnedEntities)
            {
                if (entity != nullptr)
                {
                    // Setting it to 0 is needed to avoid the infinite loop between GameEntityContext and SpawnableEntitiesManager.
                    entity->SetEntitySpawnTicketId(0);
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
            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(DespawnEntityCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            AZStd::vector<AZ::Entity*>& spawnedEntities = request.m_ticket->m_spawnedEntities;
            for (auto entityIterator = spawnedEntities.begin(); entityIterator != spawnedEntities.end(); ++entityIterator)
            {
                if (*entityIterator != nullptr && (*entityIterator)->GetId() == request.m_entityId)
                {
                    // Setting it to 0 is needed to avoid the infinite loop between GameEntityContext and SpawnableEntitiesManager.
                    (*entityIterator)->SetEntitySpawnTicketId(0);
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
            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(ReloadSpawnableCommand& request) -> CommandResult
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
                    entity->SetEntitySpawnTicketId(0);
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
            // match the new set of prototype entities getting spawned.
            InitializeEntityIdMappings(entities, ticket.m_entityIdReferenceMap, ticket.m_previouslySpawned);

            if (ticket.m_loadAll)
            {
                // The new spawnable may have a different number of entities and since the intent of the user was
                // to spawn every entity, simply start over.
                ticket.m_spawnedEntityIndices.clear();
                size_t entitiesToSpawnSize = entities.size();

                for (uint32_t i = 0; i < entitiesToSpawnSize; ++i)
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

                for (uint32_t index : ticket.m_spawnedEntityIndices)
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

            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(UpdateEntityAliasTypesCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (ticket.m_spawnable.IsReady() && request.m_requestId == ticket.m_currentRequestId)
        {
            if (Spawnable::EntityAliasVisitor aliases = ticket.m_spawnable->TryGetAliases(); aliases.IsValid())
            {
                for (EntityAliasTypeChange& replacement : request.m_entityAliases)
                {
                    aliases.UpdateAliasType(replacement.m_aliasIndex, replacement.m_newAliasType);
                }
                aliases.Optimize();

                if (request.m_completionCallback)
                {
                    request.m_completionCallback(request.m_ticketId);
                }

                ticket.m_currentRequestId++;
                return CommandResult::Executed;
            }
        }
        return CommandResult::Requeue;
    }

    auto SpawnableEntitiesManager::ProcessRequest(ListEntitiesCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            request.m_listCallback(request.m_ticketId, SpawnableConstEntityContainerView(
                ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));
            ticket.m_currentRequestId++;
            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(ListIndicesEntitiesCommand& request) -> CommandResult
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
            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(ClaimEntitiesCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            request.m_listCallback(request.m_ticketId, SpawnableEntityContainerView(
                ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));

            ticket.m_spawnedEntities.clear();
            ticket.m_spawnedEntityIndices.clear();

            ticket.m_currentRequestId++;
            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(BarrierCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId);
            }

            ticket.m_currentRequestId++;
            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(LoadBarrierCommand& request) -> CommandResult
    {
        Ticket& ticket = *request.m_ticket;
        if (ticket.m_spawnable.IsReady() && request.m_requestId == ticket.m_currentRequestId)
        {
            if (request.m_checkAliasSpawnables)
            {
                if (Spawnable::EntityAliasConstVisitor visitor = ticket.m_spawnable->TryGetAliasesConst();
                    !visitor.IsValid() || !visitor.AreAllSpawnablesReady())
                {
                    return CommandResult::Requeue;
                }
            }
            
            request.m_completionCallback(request.m_ticketId);
            ticket.m_currentRequestId++;
            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(RetrieveTicketCommand& request) -> CommandResult
    {
        auto entitySpawnTicketIterator = m_entitySpawnTicketMap.find(request.m_ticketId);
        if (entitySpawnTicketIterator == m_entitySpawnTicketMap.end())
        {
            if (m_ticketsPendingRegistration > 0)
            {
                // There are still tickets pending registration, which may hold the reference, so delay this request
                // until all tickets are registered and it's known for sure if the ticket doesn't exist anymore.
                return CommandResult::Requeue;
            }
            else
            {
                // Attempting to retrieve a ticket that no longer exists will call the callback with an invalid ticket.
                request.m_callback(InternalToExternalTicket(nullptr, this));
                return CommandResult::Executed;
            }
        }

        if (entitySpawnTicketIterator->second->m_referenceCount > 0)
        {
            // About to make a copy so increase the reference count.
            entitySpawnTicketIterator->second->m_referenceCount++;
            request.m_callback(InternalToExternalTicket(entitySpawnTicketIterator->second, this));
        }
        else
        {
            // Attempting to retrieve a ticket that no longer has any references but hasn't technically
            // been deleted yet will call the callback with an invalid ticket.
            // For example, this can occur in the following situation:
            // - An entity with a ticket deactivates and destroys its reference, causing DestroyTicket to get queued,
            //   with the ticket currently having a reference count of 0.
            // - One of the entities from that ticket then deactivates and tries to queue a RetrieveTicket so that it
            //   can queue a DespawnEntity command for itself.
            // If we "reanimated" the ticket here, bumped its reference count, and called the callback, the callback could end up
            // holding onto an invalid pointer once the DestroyTicket got processed.
            request.m_callback(InternalToExternalTicket(nullptr, this));
        }

        return CommandResult::Executed;
    }

    auto SpawnableEntitiesManager::ProcessRequest(RegisterTicketCommand& request) -> CommandResult
    {
        if (request.m_requestId == request.m_ticket->m_currentRequestId)
        {
            m_entitySpawnTicketMap.insert_or_assign(request.m_ticket->m_ticketId, request.m_ticket);
            request.m_ticket->m_currentRequestId++;
            AZ_Assert(
                m_ticketsPendingRegistration > 0,
                "Attempting to decrement the number of entity spawn tickets pending registration while there are no registrations pending "
                "in the SpawnableEntitiesManager.");
            m_ticketsPendingRegistration--;
            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }

    auto SpawnableEntitiesManager::ProcessRequest(DestroyTicketCommand& request) -> CommandResult
    {
        if (request.m_requestId == request.m_ticket->m_currentRequestId)
        {
            for (AZ::Entity* entity : request.m_ticket->m_spawnedEntities)
            {
                if (entity != nullptr)
                {
                    // Setting it to 0 is needed to avoid the infinite loop between GameEntityContext and SpawnableEntitiesManager.
                    entity->SetEntitySpawnTicketId(0);
                    GameEntityContextRequestBus::Broadcast(
                        &GameEntityContextRequestBus::Events::DestroyGameEntity, entity->GetId());
                }
            }

            m_entitySpawnTicketMap.erase(request.m_ticket->m_ticketId);

            delete request.m_ticket;
            AZ_Assert(
                m_totalTickets > 0,
                "Attempting to decrement the total number of entity spawn tickets while are zero tickets in the SpawnableEntitiesManager.");
            m_totalTickets--;

            return CommandResult::Executed;
        }
        else
        {
            return CommandResult::Requeue;
        }
    }
} // namespace AzFramework
