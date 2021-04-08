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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableEntitiesManager.h>

namespace AzFramework
{
    void SpawnableEntitiesManager::SpawnAllEntities(EntitySpawnTicket& ticket, EntitySpawnCallback completionCallback)
    {
        SpawnAllEntitiesCommand queueEntry;
        queueEntry.m_ticket = &ticket;
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            queueEntry.m_ticketId = GetTicketPayload<Ticket>(ticket).m_nextTicketId++;
            m_pendingRequestQueue.push(AZStd::move(queueEntry));
        }
    }

    void SpawnableEntitiesManager::SpawnEntities(EntitySpawnTicket& ticket, AZStd::vector<size_t> entityIndices,
        EntitySpawnCallback completionCallback)
    {
        SpawnEntitiesCommand queueEntry;
        queueEntry.m_ticket = &ticket;
        queueEntry.m_entityIndices = AZStd::move(entityIndices);
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            queueEntry.m_ticketId = GetTicketPayload<Ticket>(ticket).m_nextTicketId++;
            m_pendingRequestQueue.push(AZStd::move(queueEntry));
        }
    }

    void SpawnableEntitiesManager::DespawnAllEntities(EntitySpawnTicket& ticket, EntityDespawnCallback completionCallback)
    {
        DespawnAllEntitiesCommand queueEntry;
        queueEntry.m_ticket = &ticket;
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            queueEntry.m_ticketId = GetTicketPayload<Ticket>(ticket).m_nextTicketId++;
            m_pendingRequestQueue.push(AZStd::move(queueEntry));
        }
    }

    void SpawnableEntitiesManager::ReloadSpawnable(EntitySpawnTicket& ticket, AZ::Data::Asset<Spawnable> spawnable,
        ReloadSpawnableCallback completionCallback)
    {
        ReloadSpawnableCommand queueEntry;
        queueEntry.m_ticket = &ticket;
        queueEntry.m_spawnable = AZStd::move(spawnable);
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            queueEntry.m_ticketId = GetTicketPayload<Ticket>(ticket).m_nextTicketId++;
            m_pendingRequestQueue.push(AZStd::move(queueEntry));
        }
    }

    void SpawnableEntitiesManager::ListEntities(EntitySpawnTicket& ticket, ListEntitiesCallback listCallback)
    {
        AZ_Assert(listCallback, "ListEntities called on spawnable entities without a valid callback to use.");

        ListEntitiesCommand queueEntry;
        queueEntry.m_ticket = &ticket;
        queueEntry.m_listCallback = AZStd::move(listCallback);
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            queueEntry.m_ticketId = GetTicketPayload<Ticket>(ticket).m_nextTicketId++;
            m_pendingRequestQueue.push(AZStd::move(queueEntry));
        }
    }

    void SpawnableEntitiesManager::ClaimEntities(EntitySpawnTicket& ticket, ClaimEntitiesCallback listCallback)
    {
        AZ_Assert(listCallback, "ClaimEntities called on spawnable entities without a valid callback to use.");

        ClaimEntitiesCommand queueEntry;
        queueEntry.m_ticket = &ticket;
        queueEntry.m_listCallback = AZStd::move(listCallback);
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            queueEntry.m_ticketId = GetTicketPayload<Ticket>(ticket).m_nextTicketId++;
            m_pendingRequestQueue.push(AZStd::move(queueEntry));
        }
    }

    void SpawnableEntitiesManager::Barrier(EntitySpawnTicket& ticket, BarrierCallback completionCallback)
    {
        AZ_Assert(completionCallback, "Barrier on spawnable entities called without a valid callback to use.");

        BarrierCommand queueEntry;
        queueEntry.m_ticket = &ticket;
        queueEntry.m_completionCallback = AZStd::move(completionCallback);
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            queueEntry.m_ticketId = GetTicketPayload<Ticket>(ticket).m_nextTicketId++;
            m_pendingRequestQueue.push(AZStd::move(queueEntry));
        }
    }

    auto SpawnableEntitiesManager::ProcessQueue() -> CommandQueueStatus
    {
        AZStd::queue<Requests> pendingRequestQueue;
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            m_pendingRequestQueue.swap(pendingRequestQueue);
        }

        if (!pendingRequestQueue.empty() || !m_delayedQueue.empty())
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to retrieve serialization context.");

            // Only process the requests that are currently in this queue, not the ones that could be re-added if they still can't complete.
            size_t delayedSize = m_delayedQueue.size();
            for (size_t i = 0; i < delayedSize; ++i)
            {
                Requests& request = m_delayedQueue.front();
                bool result = AZStd::visit([this, serializeContext](auto&& args) -> bool
                    {
                        return ProcessRequest(args, *serializeContext);
                    }, request);
                if (!result)
                {
                    m_delayedQueue.emplace_back(AZStd::move(request));
                }
                m_delayedQueue.pop_front();
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
                        m_delayedQueue.emplace_back(AZStd::move(request));
                    }
                    pendingRequestQueue.pop();
                }

                // Spawning entities can result in more entities being queued to spawn. Repeat spawning until the queue is
                // empty to avoid a chain of entity spawning getting dragged out over multiple frames.
                {
                    AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
                    m_pendingRequestQueue.swap(pendingRequestQueue);
                }
            } while (!pendingRequestQueue.empty());
        }

        return m_delayedQueue.empty() ? CommandQueueStatus::NoCommandLeft : CommandQueueStatus::HasCommandsLeft;
    }

    void* SpawnableEntitiesManager::CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable)
    {
        auto result = aznew Ticket();
        result->m_spawnable = AZStd::move(spawnable);
        return result;
    }

    void SpawnableEntitiesManager::DestroyTicket(void* ticket)
    {
        DestroyTicketCommand queueEntry;
        queueEntry.m_ticket = reinterpret_cast<Ticket*>(ticket);
        {
            AZStd::scoped_lock queueLock(m_pendingRequestQueueMutex);
            queueEntry.m_ticketId = reinterpret_cast<Ticket*>(ticket)->m_nextTicketId++;
            m_pendingRequestQueue.push(AZStd::move(queueEntry));
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

    bool SpawnableEntitiesManager::ProcessRequest(SpawnAllEntitiesCommand& request, AZ::SerializeContext& serializeContext)
    {
        Ticket& ticket = GetTicketPayload<Ticket>(*request.m_ticket);
        if (ticket.m_spawnable.IsReady() && request.m_ticketId == ticket.m_currentTicketId)
        {
            size_t spawnedEntitiesCount = ticket.m_spawnedEntities.size();

            const Spawnable::EntityList& entities = ticket.m_spawnable->GetEntities();
            size_t entitiesSize = entities.size();
            ticket.m_spawnedEntities.reserve(ticket.m_spawnedEntities.size() + entitiesSize);
            ticket.m_spawnedEntityIndices.reserve(ticket.m_spawnedEntityIndices.size() + entitiesSize);

            for(size_t i=0; i<entitiesSize; ++i)
            {
                ticket.m_spawnedEntities.push_back(SpawnSingleEntity(*entities[i], serializeContext));
                ticket.m_spawnedEntityIndices.push_back(i);
            }

            if (request.m_completionCallback)
            {
                request.m_completionCallback(*request.m_ticket, SpawnableConstEntityContainerView(
                    ticket.m_spawnedEntities.begin() + spawnedEntitiesCount, ticket.m_spawnedEntities.end()));
            }

            ticket.m_currentTicketId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(SpawnEntitiesCommand& request, AZ::SerializeContext& serializeContext)
    {
        Ticket& ticket = GetTicketPayload<Ticket>(*request.m_ticket);
        if (ticket.m_spawnable.IsReady() && request.m_ticketId == ticket.m_currentTicketId)
        {
            size_t spawnedEntitiesCount = ticket.m_spawnedEntities.size();

            const Spawnable::EntityList& entities = ticket.m_spawnable->GetEntities();
            size_t entitiesSize = entities.size();
            ticket.m_spawnedEntities.reserve(ticket.m_spawnedEntities.size() + entitiesSize);
            ticket.m_spawnedEntityIndices.reserve(ticket.m_spawnedEntityIndices.size() + entitiesSize);

            for (size_t index : request.m_entityIndices)
            {
                ticket.m_spawnedEntities.push_back(SpawnSingleEntity(*entities[index], serializeContext));
                ticket.m_spawnedEntityIndices.push_back(index);
            }
            ticket.m_loadAll = false;

            if (request.m_completionCallback)
            {
                request.m_completionCallback(*request.m_ticket, SpawnableConstEntityContainerView(
                    ticket.m_spawnedEntities.begin() + spawnedEntitiesCount, ticket.m_spawnedEntities.end()));
            }

            ticket.m_currentTicketId++;
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
        Ticket& ticket = GetTicketPayload<Ticket>(*request.m_ticket);
        if (request.m_ticketId == ticket.m_currentTicketId)
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
                request.m_completionCallback(*request.m_ticket);
            }

            ticket.m_currentTicketId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ReloadSpawnableCommand& request, AZ::SerializeContext& serializeContext)
    {
        Ticket& ticket = GetTicketPayload<Ticket>(*request.m_ticket);
        AZ_Assert(ticket.m_spawnable.GetId() == request.m_spawnable.GetId(),
            "Spawnable is being reloaded, but the provided spawnable has a different asset id. "
            "This will likely result in unexpected entities being created.");
        if (ticket.m_spawnable.IsReady() && request.m_ticketId == ticket.m_currentTicketId)
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
            
            // Rebuild the list of entities.
            ticket.m_spawnedEntities.clear();
            const Spawnable::EntityList& entities = request.m_spawnable->GetEntities();
            if (ticket.m_loadAll)
            {
                // The new spawnable may have a different number of entities and since the intent of the user was
                // to load every, simply start over.
                ticket.m_spawnedEntityIndices.clear();

                size_t entitiesSize = entities.size();
                for (size_t i = 0; i < entitiesSize; ++i)
                {
                    ticket.m_spawnedEntities.push_back(SpawnSingleEntity(*entities[i], serializeContext));
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
                request.m_completionCallback(*request.m_ticket, SpawnableConstEntityContainerView(
                    ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));
            }

            ticket.m_currentTicketId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ListEntitiesCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
    {
        Ticket& ticket = GetTicketPayload<Ticket>(*request.m_ticket);
        if (request.m_ticketId == ticket.m_currentTicketId)
        {
            request.m_listCallback(*request.m_ticket, SpawnableConstEntityContainerView(
                ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));
            ticket.m_currentTicketId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(ClaimEntitiesCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
    {
        Ticket& ticket = GetTicketPayload<Ticket>(*request.m_ticket);
        if (request.m_ticketId == ticket.m_currentTicketId)
        {
            request.m_listCallback(*request.m_ticket, SpawnableEntityContainerView(
                ticket.m_spawnedEntities.begin(), ticket.m_spawnedEntities.end()));

            ticket.m_spawnedEntities.clear();
            ticket.m_spawnedEntityIndices.clear();

            ticket.m_currentTicketId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(BarrierCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
    {
        Ticket& ticket = GetTicketPayload<Ticket>(*request.m_ticket);
        if (request.m_ticketId == ticket.m_currentTicketId)
        {
            if (request.m_completionCallback)
            {
                request.m_completionCallback(*request.m_ticket);
            }

            ticket.m_currentTicketId++;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableEntitiesManager::ProcessRequest(DestroyTicketCommand& request, [[maybe_unused]] AZ::SerializeContext& serializeContext)
    {
        if (request.m_ticketId == request.m_ticket->m_currentTicketId)
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

    bool SpawnableEntitiesManager::IsEqualTicket(const EntitySpawnTicket* lhs, const EntitySpawnTicket* rhs)
    {
        return GetTicketPayload<Ticket>(lhs) == GetTicketPayload<Ticket>(rhs);
    }

    bool SpawnableEntitiesManager::IsEqualTicket(const Ticket* lhs, const EntitySpawnTicket* rhs)
    {
        return lhs == GetTicketPayload<Ticket>(rhs);
    }

    bool SpawnableEntitiesManager::IsEqualTicket(const EntitySpawnTicket* lhs, const Ticket* rhs)
    {
        return GetTicketPayload<Ticket>(lhs) == rhs;
    }

    bool SpawnableEntitiesManager::IsEqualTicket(const Ticket* lhs, const Ticket* rhs)
    {
        return lhs = rhs;
    }
} // namespace AzFramework
