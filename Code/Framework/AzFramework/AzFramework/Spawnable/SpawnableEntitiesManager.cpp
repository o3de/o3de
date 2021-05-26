/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            AZ::u64 value = 64;
            settingsRegistry->Get(value, "/O3DE/AzFramework/Spawnables/HighPriorityThreshold");
            m_highPriorityThreshold = aznumeric_cast<SpawnablePriority>(AZStd::clamp(value, 0llu, 255llu));
        }
    }

    void SpawnableEntitiesManager::SpawnAllEntities(
        EntitySpawnTicket& ticket, SpawnablePriority priority, EntityPreInsertionCallback preInsertionCallback,
        EntitySpawnCallback completionCallback)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to SpawnAllEntities hasn't been initialized.");

        SpawnAllEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        queueEntry.m_preInsertionCallback = AZStd::move(preInsertionCallback);
        QueueRequest(ticket, priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::SpawnEntities(
        EntitySpawnTicket& ticket, SpawnablePriority priority, AZStd::vector<size_t> entityIndices,
        EntityPreInsertionCallback preInsertionCallback, EntitySpawnCallback completionCallback)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to SpawnEntities hasn't been initialized.");

        SpawnEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_entityIndices = AZStd::move(entityIndices);
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        queueEntry.m_preInsertionCallback = AZStd::move(preInsertionCallback);
        QueueRequest(ticket, priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::DespawnAllEntities(
        EntitySpawnTicket& ticket, SpawnablePriority priority, EntityDespawnCallback completionCallback)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to DespawnAllEntities hasn't been initialized.");

        DespawnAllEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        QueueRequest(ticket, priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::ReloadSpawnable(
        EntitySpawnTicket& ticket, SpawnablePriority priority, AZ::Data::Asset<Spawnable> spawnable,
        ReloadSpawnableCallback completionCallback)
    {
        AZ_Assert(ticket.IsValid(), "Ticket provided to ReloadSpawnable hasn't been initialized.");

        ReloadSpawnableCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_spawnable = AZStd::move(spawnable);
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        QueueRequest(ticket, priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::ListEntities(EntitySpawnTicket& ticket, SpawnablePriority priority, ListEntitiesCallback listCallback)
    {
        AZ_Assert(listCallback, "ListEntities called on spawnable entities without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to ListEntities hasn't been initialized.");

        ListEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_listCallback = AZStd::move(listCallback);
        QueueRequest(ticket, priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::ListIndicesAndEntities(
        EntitySpawnTicket& ticket, SpawnablePriority priority, ListIndicesEntitiesCallback listCallback)
    {
        AZ_Assert(listCallback, "ListEntities called on spawnable entities without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to ListEntities hasn't been initialized.");

        ListIndicesEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_listCallback = AZStd::move(listCallback);
        QueueRequest(ticket, priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::ClaimEntities(EntitySpawnTicket& ticket, SpawnablePriority priority, ClaimEntitiesCallback listCallback)
    {
        AZ_Assert(listCallback, "ClaimEntities called on spawnable entities without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to ClaimEntities hasn't been initialized.");

        ClaimEntitiesCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_listCallback = AZStd::move(listCallback);
        QueueRequest(ticket, priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::Barrier(EntitySpawnTicket& ticket, SpawnablePriority priority, BarrierCallback completionCallback)
    {
        AZ_Assert(completionCallback, "Barrier on spawnable entities called without a valid callback to use.");
        AZ_Assert(ticket.IsValid(), "Ticket provided to Barrier hasn't been initialized.");

        BarrierCommand queueEntry;
        queueEntry.m_ticketId = ticket.GetId();
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        QueueRequest(ticket, priority, AZStd::move(queueEntry));
    }

    void SpawnableEntitiesManager::AddOnSpawnedHandler(AZ::Event<AZ::Data::Asset<Spawnable>>::Handler& handler)
    {
        handler.Connect(m_onSpawnedEvent);
    }

    void SpawnableEntitiesManager::AddOnDespawnedHandler(AZ::Event<AZ::Data::Asset<Spawnable>>::Handler& handler)
    {
        handler.Connect(m_onDespawnedEvent);
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
        AZStd::queue<Requests> pendingRequestQueue;
        {
            AZStd::scoped_lock queueLock(queue.m_pendingRequestMutex);
            queue.m_pendingRequest.swap(pendingRequestQueue);
        }

        if (!pendingRequestQueue.empty() || !queue.m_delayed.empty())
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to retrieve serialization context.");

            // Only process the requests that are currently in this queue, not the ones that could be re-added if they still can't complete.
            size_t delayedSize = queue.m_delayed.size();
            for (size_t i = 0; i < delayedSize; ++i)
            {
                Requests& request = queue.m_delayed.front();
                bool result = AZStd::visit([this, serializeContext](auto&& args) -> bool
                    {
                        return ProcessRequest(args, *serializeContext);
                    }, request);
                if (!result)
                {
                    queue.m_delayed.emplace_back(AZStd::move(request));
                }
                queue.m_delayed.pop_front();
            }

            do
            {
                while (!pendingRequestQueue.empty())
                {
                    Requests& request = pendingRequestQueue.front();
                    bool result = AZStd::visit([this, serializeContext](auto&& args) -> bool
                        {
                            return ProcessRequest(args, *serializeContext);
                        }, request);
                    if (!result)
                    {
                        queue.m_delayed.emplace_back(AZStd::move(request));
                    }
                    pendingRequestQueue.pop();
                }

                // Spawning entities can result in more entities being queued to spawn. Repeat spawning until the queue is
                // empty to avoid a chain of entity spawning getting dragged out over multiple frames.
                {
                    AZStd::scoped_lock queueLock(queue.m_pendingRequestMutex);
                    queue.m_pendingRequest.swap(pendingRequestQueue);
                }
            } while (!pendingRequestQueue.empty());
        }

        return queue.m_delayed.empty() ? CommandQueueStatus::NoCommandsLeft : CommandQueueStatus::HasCommandsLeft;
    }

    AZStd::pair<uint64_t, void*> SpawnableEntitiesManager::CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable)
    {
        static AZStd::atomic_uint64_t idCounter { 1 };

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

    AZ::Entity* SpawnableEntitiesManager::SpawnSingleEntity(const AZ::Entity& entityTemplate, AZ::SerializeContext& serializeContext)
    {
        AZ::Entity* clone = serializeContext.CloneObject(&entityTemplate);
        AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");
        clone->SetId(AZ::Entity::MakeId());

        GameEntityContextRequestBus::Broadcast(&GameEntityContextRequestBus::Events::AddGameEntity, clone);
        return clone;
    }

    AZ::Entity* SpawnableEntitiesManager::CloneSingleEntity(const AZ::Entity& entityTemplate,
        EntityIdMap& templateToCloneEntityIdMap, AZ::SerializeContext& serializeContext)
    {
        return AZ::IdUtils::Remapper<AZ::EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(
                &entityTemplate, templateToCloneEntityIdMap, &serializeContext);
    }

    bool SpawnableEntitiesManager::ProcessRequest(SpawnAllEntitiesCommand& request, AZ::SerializeContext& serializeContext)
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

            // Map keeps track of ids from template (spawnable) to clone (instance)
            // Allowing patch ups of fields referring to entityIds outside of a given entity
            EntityIdMap templateToCloneEntityIdMap;

            // Reserve buffers
            spawnedEntities.reserve(spawnedEntities.size() + entitiesToSpawnSize);
            spawnedEntityIndices.reserve(spawnedEntityIndices.size() + entitiesToSpawnSize);
            templateToCloneEntityIdMap.reserve(entitiesToSpawnSize);

            // Mark all indices as spawned
            for (size_t i = 0; i < entitiesToSpawnSize; ++i)
            {
                const AZ::Entity& entityTemplate = *entitiesToSpawn[i];

                AZ::Entity* clone = CloneSingleEntity(entityTemplate, templateToCloneEntityIdMap, serializeContext);

                AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");

                spawnedEntities.emplace_back(clone);
                spawnedEntityIndices.push_back(i);
            }

            // loadAll is true if every entity has been spawned only once
            if (spawnedEntities.size() == entitiesToSpawnSize)
            {
                ticket.m_loadAll = true;
            }
            else
            {
                // Case where there were already spawns from a previous request
                ticket.m_loadAll = false;
            }

            // Let other systems know about newly spawned entities for any pre-processing before adding to the scene/game context.
            if (request.m_preInsertionCallback)
            {
                request.m_preInsertionCallback(request.m_ticketId, SpawnableEntityContainerView(
                        ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
            }

            // Add to the game context, now the entities are active
            AZStd::for_each(ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end(),
                [](AZ::Entity* entity)
            {
                GameEntityContextRequestBus::Broadcast(&GameEntityContextRequestBus::Events::AddGameEntity, entity);
            });

            // Let other systems know about newly spawned entities for any post-processing after adding to the scene/game context.
            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId, SpawnableConstEntityContainerView(
                        ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
            }

            m_onSpawnedEvent.Signal(ticket.m_spawnable);

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(SpawnEntitiesCommand& request, AZ::SerializeContext& serializeContext)
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
            size_t entitiesToSpawnSize = request.m_entityIndices.size();

            spawnedEntities.reserve(spawnedEntities.size() + entitiesToSpawnSize);
            spawnedEntityIndices.reserve(spawnedEntityIndices.size() + entitiesToSpawnSize);

            for (size_t index : request.m_entityIndices)
            {
                if (index < entitiesToSpawn.size())
                {
                    const AZ::Entity& entityTemplate = *entitiesToSpawn[index];

                    AZ::Entity* clone = serializeContext.CloneObject(&entityTemplate);
                    AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");
                    clone->SetId(AZ::Entity::MakeId());

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
            AZStd::for_each(ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end(),
                [](AZ::Entity* entity)
            {
                GameEntityContextRequestBus::Broadcast(&GameEntityContextRequestBus::Events::AddGameEntity, entity);
            });

            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId, SpawnableConstEntityContainerView(
                    ticket.m_spawnedEntities.begin() + spawnedEntitiesInitialCount, ticket.m_spawnedEntities.end()));
            }

            m_onSpawnedEvent.Signal(ticket.m_spawnable);

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(DespawnAllEntitiesCommand& request,
        [[maybe_unused]] AZ::SerializeContext& serializeContext)
    {
        Ticket& ticket = *request.m_ticket;
        if (request.m_requestId == ticket.m_currentRequestId)
        {
            for (AZ::Entity* entity : ticket.m_spawnedEntities)
            {
                if (entity != nullptr)
                {
                    GameEntityContextRequestBus::Broadcast(
                        &GameEntityContextRequestBus::Events::DestroyGameEntityAndDescendants, entity->GetId());
                }
            }

            ticket.m_spawnedEntities.clear();
            ticket.m_spawnedEntityIndices.clear();

            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId);
            }

            m_onDespawnedEvent.Signal(ticket.m_spawnable);

            ticket.m_currentRequestId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ReloadSpawnableCommand& request, AZ::SerializeContext& serializeContext)
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
                    GameEntityContextRequestBus::Broadcast(
                        &GameEntityContextRequestBus::Events::DestroyGameEntityAndDescendants, entity->GetId());
                }
            }

            m_onDespawnedEvent.Signal(ticket.m_spawnable);
            
            // Rebuild the list of entities.
            ticket.m_spawnedEntities.clear();
            const Spawnable::EntityList& entities = request.m_spawnable->GetEntities();
            if (ticket.m_loadAll)
            {
                // The new spawnable may have a different number of entities and since the intent of the user was
                // to load every, simply start over.
                ticket.m_spawnedEntityIndices.clear();

                size_t entitiesToSpawnSize = entities.size();

                // Map keeps track of ids from template (spawnable) to clone (instance)
                // Allowing patch ups of fields referring to entityIds outside of a given entity
                EntityIdMap templateToCloneEntityIdMap;
                templateToCloneEntityIdMap.reserve(entitiesToSpawnSize);

                // Mark all indices as spawned
                for (size_t i = 0; i < entitiesToSpawnSize; ++i)
                {
                    const AZ::Entity& entityTemplate = *entities[i];

                    AZ::Entity* clone = CloneSingleEntity(entityTemplate, templateToCloneEntityIdMap, serializeContext);

                    AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");

                    ticket.m_spawnedEntities.emplace_back(clone);
                    ticket.m_spawnedEntityIndices.push_back(i);
                }
            }
            else
            {
                size_t entitiesSize = entities.size();
                for (size_t index : ticket.m_spawnedEntityIndices)
                {
                    ticket.m_spawnedEntities.push_back(
                        index < entitiesSize ? SpawnSingleEntity(*entities[index], serializeContext) : nullptr);
                }
            }
            ticket.m_spawnable = AZStd::move(request.m_spawnable);

            if (request.m_completionCallback)
            {
                request.m_completionCallback(request.m_ticketId, SpawnableConstEntityContainerView(
                    ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));
            }

            ticket.m_currentRequestId++;

            m_onSpawnedEvent.Signal(ticket.m_spawnable);

            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ListEntitiesCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
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

    bool SpawnableEntitiesManager::ProcessRequest(ListIndicesEntitiesCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
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

    bool SpawnableEntitiesManager::ProcessRequest(ClaimEntitiesCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
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

    bool SpawnableEntitiesManager::ProcessRequest(BarrierCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
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

    bool SpawnableEntitiesManager::ProcessRequest(DestroyTicketCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
    {
        if (request.m_requestId == request.m_ticket->m_currentRequestId)
        {
            for (AZ::Entity* entity : request.m_ticket->m_spawnedEntities)
            {
                if (entity != nullptr)
                {
                    GameEntityContextRequestBus::Broadcast(
                        &GameEntityContextRequestBus::Events::DestroyGameEntityAndDescendants, entity->GetId());
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
